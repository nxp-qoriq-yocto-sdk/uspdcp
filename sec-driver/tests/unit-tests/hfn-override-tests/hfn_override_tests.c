/* Copyright (c) 2011 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Create one context for each supported algorithm, uplink and downlink
 * Check that the results from the output of one algorithm match the results from the other one
 * i.e. AES_CTR(AES_CMAC(msg)) == AES_CMAC_CTR(msg)
 * Scenario 1 (both ul and dl):
 *             msg->auth(msg)->enc(auth(msg)) \
 *                                            |=> compare the two, fail if they don't match
 *                              auth_enc(msg) /
 * Scenario 2 (both ul and dl):
 *            msg->dec(msg)->auth_check(dec(auth(msg)))  \
 *                                                        |=> compare the two, fail if they don't match
 *                                    auth_check_dec(msg) /
 *
 * Checks also that all supported algorithms match the output (both encap and decap)
 */

#ifdef _cplusplus
extern "C" {
#endif

/*=================================================================================================
                                        INCLUDE FILES
==================================================================================================*/
#include "fsl_sec.h"
#include "cgreen.h"

// For shared memory allocator
#include "fsl_usmmgr.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "hfn_override_tests.h"

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
// Number of SEC JRs used by this test application
// @note: Currently this test application supports only 2 JRs (not less, not more).
//        The DTS must have configured SEC device with at least 2 user space owned job rings!
#define JOB_RING_NUMBER 2

// max number of packets used in this test
#define TEST_PACKETS_NUMBER 50

/* Number of bytes representing the offset into a packet,
 * where the PDCP header will start. Can be set to any
 * value to accomodate custom data before the PDCP header
 */
#define TEST_PACKET_OFFSET  4

// Size in bytes for the buffer occupied by a packet.
// Includes offset.
#define TEST_PACKET_LENGTH  50

// Alignment in bytes for input/output packets allocated from DMA-memory zone
#define BUFFER_ALIGNEMENT 32

// Size in bytes for the buffer used by a packet.
// A packet can use less but not more than this number of bytes.
#define BUFFER_SIZE 200

// Max length in bytes for a confidentiality /integrity key.
#define MAX_KEY_LENGTH    32

/** Size in bytes of a cacheline. */
#define CACHE_LINE_SIZE  32

// For keeping the code relatively the same between HW versions
#define dma_mem_memalign    test_memalign
#define dma_mem_free        test_free

// Number of SEC contexts in each pool. Define taken from SEC user-space driver.
#define MAX_SEC_CONTEXTS_PER_POOL   (SEC_MAX_PDCP_CONTEXTS / (JOB_RING_NUMBER))

#define DUMP_PACKET(in_pkt)                                                   \
    {                                                                         \
        int __i;                                                              \
        printf("Packet @ address : 0x%08x\n",test_ptov((in_pkt)->address));   \
        for( __i = 0; __i < (in_pkt)->offset + (in_pkt)->length; __i++)       \
        {                                                                     \
           printf("0x%02x\n",                                                 \
           *((uint8_t*)(test_ptov((in_pkt)->address)) + __i ));               \
        }                                                                     \
    }

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/** Enum for packet states */
typedef enum buffer_state_s
{
    STATE_UNUSED = 0,
    STATE_USED
}buffer_state_t;

/** Structure to define packet info */
typedef struct buffer_s
{
    uint8_t buffer[BUFFER_SIZE];
    uint32_t offset;
    sec_packet_t pdcp_packet;
    buffer_state_t state;
}buffer_t;

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
// configuration data for SEC driver
static sec_config_t sec_config_data;

// job ring handles provided by SEC driver
static const sec_job_ring_descriptor_t *job_ring_descriptors = NULL;

// array of packets
buffer_t *test_packets = NULL;

// User App specific data for every packet
uint8_t ua_data[TEST_PACKETS_NUMBER];

// cipher key, required for every PDCP context
static uint8_t *cipher_key = NULL;

// integrity key, required for every PDCP context
static uint8_t *integrity_key = NULL;

// FSL Userspace Memory Manager structure
fsl_usmmgr_t g_usmmgr;
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
// Counter of packets received with HFN > HFN threshold
static uint32_t pkts_hfn_threshold;
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
// Callback function registered for every PDCP context to handle procesed packets.
static int handle_packet_from_sec(const sec_packet_t *in_packet,
                                  const sec_packet_t *out_packet,
                                  ua_context_handle_t ua_ctx_handle,
                                  uint32_t status,
                                  uint32_t error_info);
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
// Callback function registered for PDCP contexts which have
// reached HFN threshold
static int handle_packet_from_sec_hfn_threshold(const sec_packet_t *in_packet,
                                  const sec_packet_t *out_packet,
                                  ua_context_handle_t ua_ctx_handle,
                                  uint32_t status,
                                  uint32_t error_info);
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
// Get packet from the global array with test packets.
static int get_pkt(sec_packet_t **pkt,
                   uint8_t *data, int data_len,
                   uint8_t *hdr, int hdr_len);

// Return packet in the global array with test packets.
static int put_pkt(sec_packet_t **pkt);

/* Returns the physical address corresponding to the virtual
 * address passed as a parameter. 
 */
static inline dma_addr_t test_vtop(void *v)
{
    return fsl_usmmgr_v2p(v,g_usmmgr);
}

static inline void* test_ptov(dma_addr_t p)
{
    return fsl_usmmgr_p2v(p,g_usmmgr);
}
/* Allocates an aligned memory area from the FSL USMMGR pool */
static void * test_memalign(size_t align, size_t size);

/* Frees a previously allocated FSL USMMGR memory region */
static void test_free(void *ptr, size_t size);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static int handle_packet_from_sec(const sec_packet_t *in_packet,
                                  const sec_packet_t *out_packet,
                                  ua_context_handle_t ua_ctx_handle,
                                  uint32_t status,
                                  uint32_t error_info)
{
    int ret = SEC_SUCCESS;

    return ret;
}
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
static int handle_packet_from_sec_hfn_threshold(const sec_packet_t *in_packet,
                                  const sec_packet_t *out_packet,
                                  ua_context_handle_t ua_ctx_handle,
                                  uint32_t status,
                                  uint32_t error_info)
{
    int ret = SEC_SUCCESS;

    if( status == SEC_STATUS_HFN_THRESHOLD_REACHED)
    {
        pkts_hfn_threshold++;
    }
    
    return ret;
}
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
static int get_pkt(sec_packet_t **pkt,
                   uint8_t *data, int data_len,
                   uint8_t *hdr, int hdr_len)
{
    int packet_idx;

    *pkt = NULL;

    assert( data_len != 0);
    assert( hdr_len != 0 );

    for( packet_idx = 0; packet_idx < TEST_PACKETS_NUMBER; packet_idx++ )
    {
        if( test_packets[packet_idx].state == STATE_UNUSED )
        {
            /* found it */
            test_packets[packet_idx].state = STATE_USED;

            *pkt = &test_packets[packet_idx].pdcp_packet;

            (*pkt)->address = test_vtop(&(test_packets[packet_idx].buffer[0]));
            (*pkt)->offset = TEST_PACKET_OFFSET;
            (*pkt)->total_length = 0;
            (*pkt)->num_fragments = 0;
            (*pkt)->length = data_len + hdr_len;

            if( data != NULL )
            {
                // copy PDCP header
                memcpy(test_ptov((*pkt)->address) + (*pkt)->offset, hdr, hdr_len);
                // copy input data
                memcpy(test_ptov((*pkt)->address) + (*pkt)->offset + hdr_len,
                        data,
                        data_len);
            }
            else
            {
                /* set everything to 0 */
                memset(test_ptov((*pkt)->address) + (*pkt)->offset, 0x00,hdr_len);
                memset(test_ptov((*pkt)->address) + (*pkt)->offset + hdr_len, 0x00, data_len);
            }
            return 0;
        }
    }

    return 1;
}

static int put_pkt(sec_packet_t **pkt)
{
    int pkt_idx;

    for( pkt_idx = 0; pkt_idx < TEST_PACKETS_NUMBER; pkt_idx++)
    {
        if( *pkt == &test_packets[pkt_idx].pdcp_packet )
        {
            /* found it */
            if( test_packets[pkt_idx].state == STATE_UNUSED)
                return -1;

            /* set everything to 0 */
            memset(test_ptov((*pkt)->address), 0x00, TEST_PACKET_LENGTH);

            *pkt = NULL;

            test_packets[pkt_idx].state = STATE_UNUSED;

            return 0;
        }
    }

    return 1;
}

static void * test_memalign(size_t align, size_t size)
{
    int ret;
    range_t r = {0,0,size};
    
    ret = fsl_usmmgr_memalign(&r,align,g_usmmgr);    
    assert_equal_with_message(ret, 0, "FSL USMMGR memalign failed: %d",ret);
    return r.vaddr;
}

static void test_free(void *ptr, size_t size)
{
   range_t r = {0,ptr,size};   
   fsl_usmmgr_free(&r,g_usmmgr);
}

static void test_setup(void)
{
    // Fill SEC driver configuration data
    sec_config_data.memory_area = dma_mem_memalign(CACHE_LINE_SIZE, SEC_DMA_MEMORY_SIZE);
    sec_config_data.sec_drv_vtop = test_vtop;

    sec_config_data.work_mode = SEC_STARTUP_POLLING_MODE;

    cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT, MAX_KEY_LENGTH);
    assert_not_equal_with_message(cipher_key, NULL, "ERROR allocating cipher_key with dma_mem_memalign");
    memset(cipher_key, 0, MAX_KEY_LENGTH);
    memcpy(cipher_key, test_crypto_key, sizeof(test_crypto_key));

    integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT, MAX_KEY_LENGTH);
    assert_not_equal_with_message(integrity_key, NULL, "ERROR allocating integrity_key with dma_mem_memalign");
    memset(integrity_key, 0, MAX_KEY_LENGTH);
    memcpy(integrity_key, test_auth_key, sizeof(test_auth_key));

    test_packets = dma_mem_memalign(BUFFER_ALIGNEMENT,
            sizeof(buffer_t) * TEST_PACKETS_NUMBER);
    assert_not_equal_with_message(test_packets, NULL,
            "ERROR allocating test_packets with dma_mem_memalign");
    memset(test_packets, 0, sizeof(buffer_t) * TEST_PACKETS_NUMBER);

    memset(ua_data, 0x3, TEST_PACKETS_NUMBER);
}


static void test_teardown()
{
    dma_mem_free(sec_config_data.memory_area,SEC_DMA_MEMORY_SIZE);
    
    dma_mem_free(cipher_key, MAX_KEY_LENGTH);
    dma_mem_free(integrity_key, MAX_KEY_LENGTH);
    dma_mem_free(test_packets, sizeof(buffer_t) * TEST_PACKETS_NUMBER);
}
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
static void test_hfn_threshold_reach(void)
{
    int ret = 0;
    int limit = SEC_JOB_RING_SIZE  - 1;
    int memcmp_res = 0;
    int test_idx;

    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;
    sec_packet_t *in_encap_pkt = NULL, *out_encap_pkt = NULL, *in_decap_pkt = NULL, *out_decap_pkt = NULL;
    uint8_t *hdr;
    uint8_t hdr_len;

    // configuration data for a PDCP context
    sec_pdcp_context_info_t ctx_info[] = {
        // Context for encapsulation
        {
            //.cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec_hfn_threshold,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn_threshold,
            .hfn_threshold          = test_hfn_threshold,
            .hfn_ov_en              = 0
        },
        // Context for decapsulation
        {
            //.cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec_hfn_threshold,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn_threshold,
            .hfn_threshold          = test_hfn_threshold,
            .hfn_ov_en              = 0
        }
    };

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    jr_handle_0 = job_ring_descriptors[0].job_ring_handle;

    for( test_idx = 0; test_idx < sizeof(test_params)/sizeof(test_params[0]) ; test_idx ++ )
    {
        if( (test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_5 && 
             test_params[test_idx].cipher_algorithm == SEC_ALG_NULL && 
             test_params[test_idx].integrity_algorithm == SEC_ALG_NULL ) ||
             ( (test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_7 || 
                test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_12) &&
             test_params[test_idx].cipher_algorithm == SEC_ALG_NULL) )
        {
            // It makes no sense to test for HFN threshold for SEC_ALG_NULL
            continue;
        }
        ctx_info[0].packet_direction =
        ctx_info[1].packet_direction = test_packet_direction[test_idx];

        ctx_info[0].protocol_direction = PDCP_ENCAPSULATION;
        ctx_info[1].protocol_direction = PDCP_DECAPSULATION;

        ctx_info[0].user_plane =
        ctx_info[1].user_plane = test_params[test_idx].type;

        ctx_info[0].cipher_algorithm    = test_params[test_idx].cipher_algorithm;
        ctx_info[0].integrity_algorithm = test_params[test_idx].integrity_algorithm;

        ctx_info[1].cipher_algorithm    = test_params[test_idx].cipher_algorithm;
        ctx_info[1].integrity_algorithm = test_params[test_idx].integrity_algorithm;

        ctx_info[0].sn_size =
        ctx_info[1].sn_size = test_data_sns[test_idx];

        hdr = test_hdr[test_idx];

        hdr_len = (test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_5 ? 1 :
                   test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_7 ? 1 : 2);

        ////////////////////////////////////
        ////////////////////////////////////

        // Create one context and affine it to first job ring.
        ret = sec_create_pdcp_context(jr_handle_0, &ctx_info[0], &ctx_handle_0);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);

        // Create another context and affine it to the first job ring.
        ret = sec_create_pdcp_context(jr_handle_0, &ctx_info[1], &ctx_handle_1);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);

        // Reset number of packets received with HFN threshold exceeded
        pkts_hfn_threshold = 0;

        ////////////////////////////////////
        ////////////////////////////////////

        // How many packets to send and receive to/from SEC
        packets_handled = 1;

        // Get one packet to be used as input for encapsulation
        ret = get_pkt(&in_encap_pkt,test_data_in,test_data_in_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

        // Get one packet to be used as output for encapsulation
        ret = get_pkt(&out_encap_pkt,NULL,test_data_out_len[test_idx],
                     hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

        // Submit one packet on the first context for encapsulation
        ret = sec_process_packet(ctx_handle_0, in_encap_pkt, out_encap_pkt, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                    "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                    SEC_SUCCESS, ret);
        usleep(1000);

        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);
        assert_equal_with_message(ret, SEC_SUCCESS,
                                "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                "ERROR on sec_poll: expected packets notified[%d]."
                                "actual packets notified[%d]",
                                packets_handled, packets_out);
        DUMP_PACKET(out_encap_pkt);

        // Get one packet to be used as input for decapsulation
        ret = get_pkt(&in_decap_pkt,test_data_out_hfn_threshold[test_idx],test_data_out_len[test_idx],
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

        // Get one packet to be used as output for decapsulation
        ret = get_pkt(&out_decap_pkt,NULL,test_data_in_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

        // Submit one packet on the second context for decapsulation
        ret = sec_process_packet(ctx_handle_1, in_decap_pkt, out_decap_pkt, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                                  "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                                  SEC_SUCCESS, ret);
        usleep(1000);
        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);

        assert_equal_with_message(ret, SEC_SUCCESS,
                                "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                "ERROR on sec_poll: expected packets notified[%d]."
                                "actual packets notified[%d]",
                                packets_handled, packets_out);
        DUMP_PACKET(out_decap_pkt);
        // Check that encap content is correct
        memcmp_res = memcmp(test_ptov(out_encap_pkt->address) + out_encap_pkt->offset,
                            hdr,hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: header is different!");

        memcmp_res = memcmp(test_ptov(out_encap_pkt->address) + out_encap_pkt->offset + hdr_len,
                            test_data_out_hfn_threshold[test_idx],test_data_out_len[test_idx]);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: content is different!");

        // check that decap content is correct
        memcmp_res = memcmp(test_ptov(out_decap_pkt->address) + out_decap_pkt->offset,
                            hdr,hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking decapsulation contents: header is different!");

        memcmp_res = memcmp(test_ptov(out_decap_pkt->address) + out_decap_pkt->offset + hdr_len,
                            test_data_in,test_data_in_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking decapsulation contents: content is different!");

        assert_equal_with_message(pkts_hfn_threshold, 2, "ERROR: Expected HFN threshold exceeded "
                                  "for %d packets, got instead %d",2,pkts_hfn_threshold);
        // release packets
        ret = put_pkt(&in_encap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing in encapsulation packet!");
        ret = put_pkt(&in_decap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing in decapsulation packet!");
        ret = put_pkt(&out_encap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing output encapsulation packet!");
        ret = put_pkt(&out_decap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing output decapsulation packet!");

        // delete contexts
        ret = sec_delete_pdcp_context(ctx_handle_0);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context !");

        ret = sec_delete_pdcp_context(ctx_handle_1);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context!");

    }

    // release sec driver
    ret = sec_release();
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}

static void test_hfn_increment(void)
{
    int ret = 0;
    int limit = SEC_JOB_RING_SIZE  - 1;
    int memcmp_res = 0;
    int test_idx;

    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;
    sec_packet_t *in_encap_pkt = NULL, *out_encap_pkt = NULL, *in_decap_pkt = NULL, *out_decap_pkt = NULL;
    uint8_t *hdr;
    uint8_t hdr_len;

    // configuration data for a PDCP context
    sec_pdcp_context_info_t ctx_info[] = {
        // Context for encapsulation
        {
            //.cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec_hfn_threshold,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold,
            .hfn_ov_en              = 0,
            .input_vtop             = test_vtop,
            .output_vtop            = test_vtop
        },
        // Context for decapsulation
        {
            //.cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec_hfn_threshold,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold,
            .hfn_ov_en              = 0,
            .input_vtop             = test_vtop,
            .output_vtop            = test_vtop
        }
    };

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    jr_handle_0 = job_ring_descriptors[0].job_ring_handle;

    for( test_idx = 0; test_idx < sizeof(test_params)/sizeof(test_params[0]) ; test_idx ++ )
    {
        if( (test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_5 && 
             test_params[test_idx].cipher_algorithm == SEC_ALG_NULL && 
             test_params[test_idx].integrity_algorithm == SEC_ALG_NULL ) ||
             ( (test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_7 || 
                test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_12) &&
             test_params[test_idx].cipher_algorithm == SEC_ALG_NULL) )
        {
            // It makes no sense to test for HFN threshold for SEC_ALG_NULL
            continue;
        }
        ctx_info[0].packet_direction =
        ctx_info[1].packet_direction = test_packet_direction[test_idx];

        ctx_info[0].protocol_direction = PDCP_ENCAPSULATION;
        ctx_info[1].protocol_direction = PDCP_DECAPSULATION;

        ctx_info[0].user_plane =
        ctx_info[1].user_plane = test_params[test_idx].type;

        ctx_info[0].cipher_algorithm    = test_params[test_idx].cipher_algorithm;
        ctx_info[0].integrity_algorithm = test_params[test_idx].integrity_algorithm;

        ctx_info[1].cipher_algorithm    = test_params[test_idx].cipher_algorithm;
        ctx_info[1].integrity_algorithm = test_params[test_idx].integrity_algorithm;

        ctx_info[0].sn_size =
        ctx_info[1].sn_size = test_data_sns[test_idx];

        hdr = test_hdr[test_idx];
        // hdr_sn_next = test_hdr_sn_next[test_idx];

        hdr_len = (test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_5 ? 1 :
                   test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_7 ? 1 : 2);

        ////////////////////////////////////
        ////////////////////////////////////

        // Create one context and affine it to first job ring.
        ret = sec_create_pdcp_context(jr_handle_0, &ctx_info[0], &ctx_handle_0);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);

        // Create another context and affine it to the first job ring.
        ret = sec_create_pdcp_context(jr_handle_0, &ctx_info[1], &ctx_handle_1);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);

        // Reset number of packets received with HFN threshold exceeded
        pkts_hfn_threshold = 0;

        ////////////////////////////////////
        ////////////////////////////////////

        // How many packets to send and receive to/from SEC
        packets_handled = 1;

        // Get one packet to be used as input for encapsulation
        ret = get_pkt(&in_encap_pkt,test_data_in,test_data_in_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

        // Get one packet to be used as output for encapsulation
        ret = get_pkt(&out_encap_pkt,NULL,test_data_out_len[test_idx],
                     hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

        assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);
        
        // Submit one packet on the first context for encapsulation
        ret = sec_process_packet(ctx_handle_0, in_encap_pkt, out_encap_pkt, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                    "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                    SEC_SUCCESS, ret);
        usleep(1000);

        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);
        assert_equal_with_message(ret, SEC_SUCCESS,
                                "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                "ERROR on sec_poll: expected packets notified[%d]."
                                "actual packets notified[%d]",
                                packets_handled, packets_out);
        
        // Check that encap content is correct
        memcmp_res = memcmp(test_ptov(out_encap_pkt->address) + out_encap_pkt->offset,
                            hdr,hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: header is different!");

        memcmp_res = memcmp(test_ptov(out_encap_pkt->address) + out_encap_pkt->offset + hdr_len,
                            test_data_out[test_idx],test_data_out_len[test_idx]);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: content is different!");
        DUMP_PACKET(out_encap_pkt);

        /* Clear the received packet */
        memset(test_ptov(out_encap_pkt->address),
               out_encap_pkt->offset + out_encap_pkt->length,
               0x00);
        /*
         * I will resubmit the same packet on the same context. This will trigger a
         * HFN increment. Due to HFN increment, the packet submitted will be 
         * processed with HFN = intial HFN + 1. The check will be done accordingly
         */

        // Submit one packet on the first context for encapsulation
        ret = sec_process_packet(ctx_handle_0, in_encap_pkt, out_encap_pkt, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                    "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                    SEC_SUCCESS, ret);
        usleep(1000);

        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);
        assert_equal_with_message(ret, SEC_SUCCESS,
                                "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                "ERROR on sec_poll: expected packets notified[%d]."
                                "actual packets notified[%d]",
                                packets_handled, packets_out);

        // Check that encap content is correct
        memcmp_res = memcmp(out_encap_pkt->address + out_encap_pkt->offset,
                            hdr,hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: header is different!");
        /* The content MUST be different ("wrong" HFN used) */
        memcmp_res = memcmp(out_encap_pkt->address + out_encap_pkt->offset + hdr_len,
                            test_data_out_hfn_inc[test_idx],test_data_out_len[test_idx]);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: content is different!");
        DUMP_PACKET(out_encap_pkt);
				
				
				
				
				
        // Get one packet to be used as input for decapsulation
        ret = get_pkt(&in_decap_pkt,test_data_out_hfn_threshold[test_idx],test_data_out_len[test_idx],
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

        // Get one packet to be used as output for decapsulation
        ret = get_pkt(&out_decap_pkt,NULL,test_data_in_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

        // Submit one packet on the second context for decapsulation
        ret = sec_process_packet(ctx_handle_1, in_decap_pkt, out_decap_pkt, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                                  "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                                  SEC_SUCCESS, ret);
        usleep(1000);
        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);

        assert_equal_with_message(ret, SEC_SUCCESS,
                                "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                "ERROR on sec_poll: expected packets notified[%d]."
                                "actual packets notified[%d]",
                                packets_handled, packets_out);

        // check that decap content is correct
        memcmp_res = memcmp(out_decap_pkt->address + out_decap_pkt->offset,
                            hdr,hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking decapsulation contents: header is different!");

        memcmp_res = memcmp(out_decap_pkt->address + out_decap_pkt->offset + hdr_len,
                            test_data_in,test_data_in_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking decapsulation contents: content is different!");
        DUMP_PACKET(out_decap_pkt);

        /* Clear the received packet */
        memset(out_decap_pkt->address,
               out_decap_pkt->offset + out_decap_pkt->length,
               0x00);
        /*
         * I will resubmit the same packet on the same context. This will trigger a
         * HFN increment. Due to HFN increment, the packet submitted will be 
         * processed with HFN = intial HFN + 1. The check will be done accordingly
         */

        // Submit one packet on the first context for encapsulation
        ret = sec_process_packet(ctx_handle_0, in_decap_pkt, out_decap_pkt, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                    "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                    SEC_SUCCESS, ret);
        usleep(1000);

        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);
        assert_equal_with_message(ret, SEC_SUCCESS,
                                "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                "ERROR on sec_poll: expected packets notified[%d]."
                                "actual packets notified[%d]",
                                packets_handled, packets_out);

        // Check that encap content is correct
        memcmp_res = memcmp(out_decap_pkt->address + out_decap_pkt->offset,
                            hdr,hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: header is different!");
        /* The content MUST be different ("wrong" HFN used) */
        memcmp_res = memcmp(out_decap_pkt->address + out_decap_pkt->offset + hdr_len,
                            test_data_out_hfn_inc[test_idx],test_data_out_len[test_idx]);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: content is different!");
        DUMP_PACKET(out_decap_pkt);

        // release packets
        ret = put_pkt(&in_encap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing in encapsulation packet!");
        ret = put_pkt(&in_decap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing in decapsulation packet!");
        ret = put_pkt(&out_encap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing output encapsulation packet!");
        ret = put_pkt(&out_decap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing output decapsulation packet!");

        // delete contexts
        ret = sec_delete_pdcp_context(ctx_handle_0);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context !");

        ret = sec_delete_pdcp_context(ctx_handle_1);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context!");

    }

    // release sec driver
    ret = sec_release();
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
static void test_hfn_override_single_algorithms(void)
{
    int ret = 0;
    int limit = SEC_JOB_RING_SIZE  - 1;
    int memcmp_res = 0;
    int test_idx;

    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;
    sec_packet_t *in_encap_pkt = NULL, *out_encap_pkt = NULL, *in_decap_pkt = NULL, *out_decap_pkt = NULL;
    uint8_t *hdr;
    uint8_t hdr_len;

    // configuration data for a PDCP context
    sec_pdcp_context_info_t ctx_info[] = {
        // Context for encapsulation
        {
            //.cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = 0x1234,
            .hfn_threshold          = test_hfn_threshold,
            .hfn_ov_en              = 1,
        },
        // Context for decapsulation
        {
            //.cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = 0x1234,
            .hfn_threshold          = test_hfn_threshold,
            .hfn_ov_en              = 1
        }
    };

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    jr_handle_0 = job_ring_descriptors[0].job_ring_handle;

    for( test_idx = 0; test_idx < sizeof(test_params)/sizeof(test_params[0]) ; test_idx ++ )
    {
        ctx_info[0].packet_direction =
        ctx_info[1].packet_direction = test_packet_direction[test_idx];

        ctx_info[0].protocol_direction = PDCP_ENCAPSULATION;
        ctx_info[1].protocol_direction = PDCP_DECAPSULATION;

        ctx_info[0].user_plane =
        ctx_info[1].user_plane = test_params[test_idx].type;

        ctx_info[0].cipher_algorithm    = test_params[test_idx].cipher_algorithm;
        ctx_info[0].integrity_algorithm = test_params[test_idx].integrity_algorithm;

        ctx_info[1].cipher_algorithm    = test_params[test_idx].cipher_algorithm;
        ctx_info[1].integrity_algorithm = test_params[test_idx].integrity_algorithm;

        ctx_info[0].sn_size =
        ctx_info[1].sn_size = test_data_sns[test_idx];

        hdr = test_hdr[test_idx];

        hdr_len = (test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_5 ? 1 :
                   test_data_sns[test_idx] == SEC_PDCP_SN_SIZE_7 ? 1 : 2);

        ////////////////////////////////////
        ////////////////////////////////////

        // Create one context and affine it to first job ring.
        ret = sec_create_pdcp_context(jr_handle_0, &ctx_info[0], &ctx_handle_0);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);

        // Create another context and affine it to the first job ring.
        ret = sec_create_pdcp_context(jr_handle_0, &ctx_info[1], &ctx_handle_1);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);


        ////////////////////////////////////
        ////////////////////////////////////

        // How many packets to send and receive to/from SEC
        packets_handled = 1;

        // Get one packet to be used as input for encapsulation
        ret = get_pkt(&in_encap_pkt,test_data_in,test_data_in_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

        // Get one packet to be used as output for encapsulation
        ret = get_pkt(&out_encap_pkt,NULL,test_data_out_len[test_idx],
                     hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

        // Submit one packet on the first context for encapsulation
        ret = sec_process_packet_hfn_ov(ctx_handle_0, in_encap_pkt, out_encap_pkt, test_hfn, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                    "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                    SEC_SUCCESS, ret);
        usleep(1000);

        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);
        assert_equal_with_message(ret, SEC_SUCCESS,
                                "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                "ERROR on sec_poll: expected packets notified[%d]."
                                "actual packets notified[%d]",
                                packets_handled, packets_out);

        // Get one packet to be used as input for decapsulation
        ret = get_pkt(&in_decap_pkt,test_data_out[test_idx],test_data_out_len[test_idx],
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

        // Get one packet to be used as output for decapsulation
        ret = get_pkt(&out_decap_pkt,NULL,test_data_in_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

        // Submit one packet on the second context for decapsulation
        ret = sec_process_packet_hfn_ov(ctx_handle_1, in_decap_pkt, out_decap_pkt, test_hfn, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                                  "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                                  SEC_SUCCESS, ret);
        usleep(1000);
        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);

        assert_equal_with_message(ret, SEC_SUCCESS,
                                "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                "ERROR on sec_poll: expected packets notified[%d]."
                                "actual packets notified[%d]",
                                packets_handled, packets_out);

        // Check that encap content is correct
        memcmp_res = memcmp(test_ptov(out_encap_pkt->address) + out_encap_pkt->offset,
                            hdr,hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: header is different!");

        memcmp_res = memcmp(test_ptov(out_encap_pkt->address) + out_encap_pkt->offset + hdr_len,
                            test_data_out[test_idx],test_data_out_len[test_idx]);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking encapsulation contents: content is different!");

        // check that decap content is correct
        memcmp_res = memcmp(test_ptov(out_decap_pkt->address) + out_decap_pkt->offset,
                            hdr,hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking decapsulation contents: header is different!");

        memcmp_res = memcmp(test_ptov(out_decap_pkt->address) + out_decap_pkt->offset + hdr_len,
                            test_data_in,test_data_in_len);
        assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking decapsulation contents: content is different!");

        // release packets
        ret = put_pkt(&in_encap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing in encapsulation packet!");
        ret = put_pkt(&in_decap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing in decapsulation packet!");
        ret = put_pkt(&out_encap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing output encapsulation packet!");
        ret = put_pkt(&out_decap_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing output decapsulation packet!");

        // delete contexts
        ret = sec_delete_pdcp_context(ctx_handle_0);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context !");

        ret = sec_delete_pdcp_context(ctx_handle_1);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context!");

    }

    // release sec driver
    ret = sec_release();
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

}


static void test_hfn_override_combined_algos(void)
{
    int ret = 0;
    int limit = SEC_JOB_RING_SIZE  - 1;
    int i = 0;
    int test_idx = 0;
    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_job_ring_handle_t jr_handle_1;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;
    sec_context_handle_t ctx_handle_2 = NULL;
    sec_packet_t *in_pkt = NULL,*first_step_pkt = NULL, *second_step_pkt = NULL,*one_step_pkt=NULL,*out_pkt = NULL;
    uint8_t *data_in=NULL,*data_inter = NULL, *data_out = NULL;
    uint8_t *hdr = NULL;
    uint32_t data_in_len = 0, data_inter_len = 0, data_out_len = 0;
    uint8_t hdr_len = 0;

    int memcmp_res = 0;

    // configuration data for a PDCP context
    sec_pdcp_context_info_t ctx_info[] = {
        /* cipher-only */
        {
            //.cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_NULL,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = 0x1234,
            .hfn_ov_en              = 1,
            .hfn_threshold          = test_hfn_threshold,
        },
        /* auth-only */
        {
            //.cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = 0x1234,
            .hfn_ov_en              = 1,
            .hfn_threshold          = test_hfn_threshold,
        },
        /* auth+cipher */
        {
            //.cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            //.integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            //.sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            //.user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = 0x1234,
            .hfn_ov_en              = 1,
            .hfn_threshold          = test_hfn_threshold,
        }
    };

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    jr_handle_0 = job_ring_descriptors[0].job_ring_handle;
    jr_handle_1 = job_ring_descriptors[1].job_ring_handle;

    // Set control or user plane
    ctx_info[0].user_plane          =
    ctx_info[1].user_plane          =
    ctx_info[2].user_plane          = test_params[compound_tests[test_idx].one_step].type;

    for( test_idx = 0; test_idx < sizeof(compound_tests)/sizeof(compound_tests[0]); test_idx++ )
    {
        printf("%d",test_packet_direction[compound_tests[test_idx].one_step]);

        // Set packet direction
        ctx_info[0].packet_direction =
        ctx_info[1].packet_direction =
        ctx_info[2].packet_direction = test_packet_direction[compound_tests[test_idx].one_step];

        // Set direction
        ctx_info[0].protocol_direction =
        ctx_info[1].protocol_direction =
        ctx_info[2].protocol_direction = compound_tests[test_idx].protocol_direction;

        // Set algorithms
        ctx_info[0].cipher_algorithm    = test_params[compound_tests[test_idx].first_step].cipher_algorithm;
        ctx_info[0].integrity_algorithm = test_params[compound_tests[test_idx].first_step].integrity_algorithm;

        ctx_info[1].cipher_algorithm    = test_params[compound_tests[test_idx].second_step].cipher_algorithm;
        ctx_info[1].integrity_algorithm = test_params[compound_tests[test_idx].second_step].integrity_algorithm;

        ctx_info[2].cipher_algorithm    = test_params[compound_tests[test_idx].one_step].cipher_algorithm;
        ctx_info[2].integrity_algorithm = test_params[compound_tests[test_idx].one_step].integrity_algorithm;

        ctx_info[0].sn_size =
        ctx_info[1].sn_size =
        ctx_info[2].sn_size = test_data_sns[compound_tests[test_idx].one_step];
        ////////////////////////////////////
        ////////////////////////////////////

        // Create one context and affine it to first job ring.
        ret = sec_create_pdcp_context(jr_handle_0, &ctx_info[0], &ctx_handle_0);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);

        // Create another context and affine it to the first job ring.
        ret = sec_create_pdcp_context(jr_handle_0, &ctx_info[1], &ctx_handle_1);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);

        // Create another context and affine it to the second job ring.
        ret = sec_create_pdcp_context(jr_handle_1, &ctx_info[2], &ctx_handle_2);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);

        hdr = test_hdr[compound_tests[test_idx].one_step];
        hdr_len = PDCP_CTRL_PLANE_HEADER_LENGTH;

        if( compound_tests[test_idx].protocol_direction == PDCP_ENCAPSULATION )
        {
            /* Packet to be submitted to be authenticated and then encrypted */
            data_in = test_data_in;
            data_in_len = test_data_in_len;

            data_inter = test_data_out[compound_tests[test_idx].first_step];
            data_inter_len = test_data_out_len[compound_tests[test_idx].first_step];

            /* Packet obtained after being authenticated and encrypted */
            data_out = test_data_out[compound_tests[test_idx].one_step];
            data_out_len = test_data_out_len[compound_tests[test_idx].one_step];
        }
        else
        {
            /* Packet to be submitted to be decrypted and authenticated */
            data_in =  test_data_out[compound_tests[test_idx].one_step];
            data_in_len = test_data_out_len[compound_tests[test_idx].one_step];

            data_inter = test_data_out[compound_tests[test_idx].second_step];
            data_inter_len = test_data_out_len[compound_tests[test_idx].second_step];

            /* Packet obtained after being decrypted and authenticated */
            data_out = test_data_in;
            data_out_len = test_data_in_len;
        }

        ////////////////////////////////////
        ////////////////////////////////////

        // How many packets to send and receive to/from SEC
        packets_handled = 1;

        ret = get_pkt(&in_pkt,data_in,data_in_len,
                        hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

        // Get one packet to be used for auth output
        ret = get_pkt(&first_step_pkt,NULL,data_inter_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

        /* Submit one packet on the first context */
        ret = sec_process_packet_hfn_ov(ctx_handle_0, in_pkt, first_step_pkt, test_hfn, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                        "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                        SEC_SUCCESS, ret);

        usleep(1000);
        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);

        assert_equal_with_message(ret, SEC_SUCCESS,
                                  "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                  SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                  "ERROR on sec_poll: expected packets notified[%d]."
                                  "actual packets notified[%d]",
                                  packets_handled, packets_out);

        memcmp_res = memcmp(test_ptov(first_step_pkt->address) + first_step_pkt->offset,
                            hdr,
                            hdr_len);
        assert_equal_with_message(memcmp_res, 0,
                        "ERROR on checking intermediate packet: Header is different");

        memcmp_res = memcmp(test_ptov(first_step_pkt->address) + first_step_pkt->offset + hdr_len,
                            data_inter,
                            data_inter_len);

        assert_equal_with_message(memcmp_res, 0,
                        "ERROR on checking intermediate packet: Content is different");

        ret = get_pkt(&second_step_pkt,NULL,data_out_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                        "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                        0, ret);

        // Submit one packet on the second context for ciphering
        ret = sec_process_packet_hfn_ov(ctx_handle_1, first_step_pkt, second_step_pkt, test_hfn, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                        "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                        SEC_SUCCESS, ret);

        usleep(1000);
        ret = sec_poll_job_ring(jr_handle_0, limit, &packets_out);

        assert_equal_with_message(ret, SEC_SUCCESS,
                                  "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                  SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                  "ERROR on sec_poll: expected packets notified[%d]."
                                  "actual packets notified[%d]",
                                  packets_handled, packets_out);

        ret = get_pkt(&one_step_pkt,NULL,data_out_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                        "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                        0, ret);

        // Submit one packet on the third context for ciphering and authentication in one pass
        ret = sec_process_packet_hfn_ov(ctx_handle_2, in_pkt, one_step_pkt, test_hfn, NULL);
        assert_equal_with_message(ret, SEC_SUCCESS,
                        "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                        SEC_SUCCESS, ret);

        usleep(1000);
        ret = sec_poll_job_ring(jr_handle_1, limit, &packets_out);

        assert_equal_with_message(ret, SEC_SUCCESS,
                                  "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                                  SEC_SUCCESS, ret);
        // <packets_handled> packets should be retrieved from SEC
        assert_equal_with_message(packets_out, packets_handled,
                                  "ERROR on sec_poll: expected packets notified[%d]."
                                  "actual packets notified[%d]",
                                  packets_handled, packets_out);

        // Assert that the pkts obtained through the two methods are bit-exact
        // Create pkt for output buffer
        ret = get_pkt(&out_pkt,data_out,data_out_len,
                      hdr,hdr_len);
        assert_equal_with_message(ret, 0,
                        "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                        0, ret);

        // Check that the double-pass packet matches the test-vector

        memcmp_res = memcmp(test_ptov(second_step_pkt->address) + second_step_pkt->offset,
                            test_ptov(out_pkt->address) + out_pkt->offset,
                            PDCP_CTRL_PLANE_HEADER_LENGTH);
        assert_equal_with_message(memcmp_res, 0,
                                  "ERROR on checking packet contents: header is different!");

        if ( memcmp_res != 0 )
        {
            printf("Header error for double-pass scenario:\n");
        }
        printf("Double pass header  |  Test vector header\n");
        printf("0x%02x                |                0x%02x\n",
                *(uint8_t*)(test_ptov(second_step_pkt->address) + second_step_pkt->offset),
                *(uint8_t*)(test_ptov(out_pkt->address) + out_pkt->offset) );

        memcmp_res = memcmp(test_ptov(second_step_pkt->address) + second_step_pkt->offset + PDCP_CTRL_PLANE_HEADER_LENGTH,
                            test_ptov(out_pkt->address) + out_pkt->offset + PDCP_CTRL_PLANE_HEADER_LENGTH,
                            out_pkt->length - PDCP_CTRL_PLANE_HEADER_LENGTH);

        assert_equal_with_message(memcmp_res, 0,
                                  "ERROR on checking packet contents: content is different!");

        if (memcmp_res != 0)
        {
            printf("Contents error for double-pass scenario:\n");
        }

        printf("Double pass content |  Test vector content\n");
        for( i = 0; i < out_pkt->length - PDCP_CTRL_PLANE_HEADER_LENGTH; i++)
        {
            printf("0x%02x                |                0x%02x\n",
                    *(uint8_t*)(test_ptov(second_step_pkt->address) + second_step_pkt->offset + PDCP_CTRL_PLANE_HEADER_LENGTH + i),
                    *(uint8_t*)(test_ptov(out_pkt->address) + out_pkt->offset + PDCP_CTRL_PLANE_HEADER_LENGTH + i) );
        }

        // Check that the single-pass packet matches the test-vector
        memcmp_res = memcmp(test_ptov(one_step_pkt->address) + one_step_pkt->offset,
                            test_ptov(out_pkt->address) + out_pkt->offset,
                            PDCP_CTRL_PLANE_HEADER_LENGTH);
        assert_equal_with_message(memcmp_res, 0,
                                  "ERROR on checking packet contents: header is different!");

        if ( memcmp_res != 0 )
        {
            printf("Header error for double-pass scenario:\n");
        }
        printf("Single pass header  |  Test vector header\n");
        printf("0x%02x                |                0x%02x\n",
                *(uint8_t*)(test_ptov(one_step_pkt->address) + one_step_pkt->offset),
                *(uint8_t*)(test_ptov(out_pkt->address) + out_pkt->offset) );

        memcmp_res = memcmp(test_ptov(one_step_pkt->address) + one_step_pkt->offset + PDCP_CTRL_PLANE_HEADER_LENGTH,
                            test_ptov(out_pkt->address) + out_pkt->offset + PDCP_CTRL_PLANE_HEADER_LENGTH,
                            out_pkt->length - PDCP_CTRL_PLANE_HEADER_LENGTH);

        assert_equal_with_message(memcmp_res, 0,
                                  "ERROR on checking packet contents: content is different!");

        if (memcmp_res != 0)
        {
            printf("Contents error for double-pass scenario:\n");
        }

        printf("Single Pass content |  Test vector content\n");
        for( i = 0; i < out_pkt->length - PDCP_CTRL_PLANE_HEADER_LENGTH; i++)
        {
            printf("0x%02x                |                0x%02x\n",
                    *(uint8_t*)(test_ptov(one_step_pkt->address) + one_step_pkt->offset + PDCP_CTRL_PLANE_HEADER_LENGTH + i),
                    *(uint8_t*)(test_ptov(out_pkt->address) + out_pkt->offset + PDCP_CTRL_PLANE_HEADER_LENGTH + i) );
        }

        // release packets
        ret = put_pkt(&in_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing in packet!");
        ret = put_pkt(&first_step_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing 1st packet!");
        ret = put_pkt(&second_step_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing 2nd packet!");
        ret = put_pkt(&one_step_pkt);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing 1step packet!");

        // delete contexts
        ret = sec_delete_pdcp_context(ctx_handle_0);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context !");

        ret = sec_delete_pdcp_context(ctx_handle_1);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context!");

        ret = sec_delete_pdcp_context(ctx_handle_2);
        assert_equal_with_message(ret, 0,
                                  "ERROR on releasing context!");
    }

    // release sec driver
    ret = sec_release();
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

}

static TestSuite * hfn_override_tests()
{
    /* create test suite */
    TestSuite * suite = create_test_suite();

    /* setup/teardown functions to be called before/after each unit test */
    setup(suite, test_setup);
    teardown(suite, test_teardown);

    /* start adding unit tests */
    add_test(suite, test_hfn_override_combined_algos);
    add_test(suite, test_hfn_override_single_algorithms);
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
    add_test(suite, test_hfn_threshold_reach);
    add_test(suite, test_hfn_increment);
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
    return suite;
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int main(int argc, char *argv[])
{
    /* *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** */
    /* Be aware that by using run_test_suite() instead of run_single_test(), CGreen will execute
     * each test case in a separate UNIX process, so:
     * (1) unit tests' thread safety might need to be ensured by defining critical regions
     *     (beware, CGreen error messages are not explanatory and intuitive enough)
     *
     * Although it is more difficult to maintain synchronization manually,
     * it is recommended to run_single_test() for each test case.
     */

    /* create test suite */
    TestSuite * suite = hfn_override_tests();
    TestReporter * reporter = create_text_reporter();
    
    // Init FSL USMMGR
    g_usmmgr = fsl_usmmgr_init();
    assert_not_equal_with_message(g_usmmgr, NULL, "ERROR on fsl_usmmgr_init");

    /* Run tests */
    
    run_single_test(suite, "test_hfn_override_single_algorithms", reporter);
    run_single_test(suite, "test_hfn_override_combined_algos", reporter);
#ifdef UNDER_CONSTRUCTION_HFN_OVERRIDE
    run_single_test(suite, "test_hfn_threshold_reach", reporter);
    run_single_test(suite, "test_hfn_increment", reporter);
#endif // UNDER_CONSTRUCTION_HFN_OVERRIDE

    destroy_test_suite(suite);
    (*reporter->destroy)(reporter);

    return 0;
} /* main() */

/*================================================================================================*/

#ifdef __cplusplus
}

#endif
