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
 */

#ifdef _cplusplus
extern "C" {
#endif

/*=================================================================================================
                                        INCLUDE FILES
==================================================================================================*/
#include "fsl_sec.h"
#include "cgreen.h"
// for dma_mem library
#include "compat.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
// Number of SEC JRs used by this test application
// @note: Currently this test application supports only 2 JRs (not less, not more).
//        The DTS must have configured SEC device with at least 2 user space owned job rings!
#define JOB_RING_NUMBER 2

// max number of packets used in this test
#define TEST_PACKETS_NUMBER 50

// Number of bytes representing the offset into a packet,
// where the PDCP header will start.
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

// Number of SEC contexts in each pool. Define taken from SEC user-space driver.
#define MAX_SEC_CONTEXTS_PER_POOL   (SEC_MAX_PDCP_CONTEXTS / (JOB_RING_NUMBER))

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

#define DUMP_PACKET(in_pkt)                             \
    {                                                   \
        int __i;                                        \
        printf("Packet @ address : 0x%08x\n",(in_pkt)->address);   \
        for( __i = 0; __i < (in_pkt)->offset + (in_pkt)->length; __i++)  \
        {                                               \
           printf("0x%02x\n",                           \
           *((uint8_t*)((in_pkt)->address) + __i ));    \
        }                                               \
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

// Crypto key
static uint8_t test_crypto_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

// Authentication key
static uint8_t test_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                  0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};

// Input test vector, used in encap packets
static uint8_t test_data_in_encap[] = { 0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                  0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8 };

// Output test vector, used in decap packets for downlink
static uint8_t test_data_in_decap_dl[] = {0xa1,0x05,0xfb,0xfe,0xa4,0x8d,0x74,0x3d,
                                  0x29,0x53,0x27,0x33,0xd9,0xba,0x91,
                                  // The MAC-I from packet
                                  0x89,0x46,0x96,0xd6};

// Output test vector, used in decap packets for downlink
static uint8_t test_data_in_decap_ul[] = {0x1f,0x75,0x2f,0x84,0xec,0x10,0x97,0xb8,
                                0x3a,0x2e,0x89,0xe8,0x6f,0x81,0x21,
                                // The MAC-I from packet
                                0x6d,0x69,0x42,0x95};

static uint8_t  test_pdcp_hdr = 0x8B;

static uint8_t test_bearer = 0x3;

static uint32_t test_hfn = 0xFA556;

static uint32_t test_hfn_threshold = 0xFF00000;

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
// Callback function registered for every PDCP context to handle procesed packets.
static int handle_packet_from_sec(const sec_packet_t *in_packet,
                                  const sec_packet_t *out_packet,
                                  ua_context_handle_t ua_ctx_handle,
                                  uint32_t status,
                                  uint32_t error_info);

// Get packet from the global array with test packets.
static int get_pkt(sec_packet_t **pkt,uint8_t *data, int pkt_len);

// Return packet in the global array with test packets.
static int put_pkt(sec_packet_t **pkt);

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

static int get_pkt(sec_packet_t **pkt,uint8_t *data, int pkt_len)
{
    int packet_idx;
    
    *pkt = NULL;
    
    if( pkt_len == 0)
        return -1;

    for( packet_idx = 0; packet_idx < TEST_PACKETS_NUMBER; packet_idx++ )
    {
        if( test_packets[packet_idx].state == STATE_UNUSED )
        {
            /* found it */
            test_packets[packet_idx].state = STATE_USED;
            
            *pkt = &test_packets[packet_idx].pdcp_packet;
            
            (*pkt)->address = &(test_packets[packet_idx].buffer[0]);
            (*pkt)->offset = TEST_PACKET_OFFSET;
            (*pkt)->total_length = 0;
            (*pkt)->num_fragments = 0;
            (*pkt)->length = pkt_len + PDCP_HEADER_LENGTH;
            
            if( data != NULL )
            {
                // copy PDCP header
                memcpy((*pkt)->address + (*pkt)->offset, &test_pdcp_hdr, sizeof(test_pdcp_hdr));
                // copy input data
                memcpy((*pkt)->address + (*pkt)->offset + PDCP_HEADER_LENGTH,
                        data,
                        pkt_len);
            }
            else
            {
                /* set everything to 0 */
                memset((*pkt)->address + (*pkt)->offset, 0x00, sizeof(test_pdcp_hdr));
                memset((*pkt)->address + (*pkt)->offset + PDCP_HEADER_LENGTH, 0x00, pkt_len);
            }
            break;
        }
    }
    
    return 0;
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
            memset((*pkt)->address + (*pkt)->offset, 0x00, sizeof(test_pdcp_hdr));
            memset((*pkt)->address + (*pkt)->offset + PDCP_HEADER_LENGTH, 0x00, TEST_PACKET_LENGTH);

            *pkt = NULL;

            test_packets[pkt_idx].state = STATE_UNUSED;

            break;
        }
    }
    
    return 0;
}


static void test_setup(void)
{
    int ret = 0;

    // map the physical memory
    ret = dma_mem_setup(SEC_DMA_MEMORY_SIZE, CACHE_LINE_SIZE);
    assert_equal_with_message(ret, 0, "ERROR on dma_mem_setup: ret = %d", ret);

    // Fill SEC driver configuration data
    sec_config_data.memory_area = (void*)__dma_virt;
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
    int ret = 0;

    // unmap the physical memory
    ret = dma_mem_release();
    assert_equal_with_message(ret, 0, "ERROR on dma_mem_release: ret = %d", ret);

    dma_mem_free(cipher_key, MAX_KEY_LENGTH);
    dma_mem_free(integrity_key, MAX_KEY_LENGTH);
    dma_mem_free(test_packets, sizeof(buffer_t) * TEST_PACKETS_NUMBER);
}

static void test_c_plane_mixed_downlink_encap(void)
{
    int ret = 0;
    int limit = SEC_JOB_RING_SIZE  - 1;
    int i = 0;
    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_job_ring_handle_t jr_handle_1;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;
    sec_context_handle_t ctx_handle_2 = NULL;
    sec_packet_t *in_pkt = NULL, *auth_ciphered_pkt_dbl_pass = NULL,*auth_ciphered_pkt=NULL, *auth_only_pkt = NULL;
    int memcmp_res = 0;
    // configuration data for a PDCP context
    sec_pdcp_context_info_t ctx_info[3] = {
        /* auth-only */
        {
            .cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
        },
        /* cipher-only */
        {
            .cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_NULL,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
        },
        /* auth+cipher */
        {
            .cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
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

    ctx_info[0].packet_direction = 
    ctx_info[1].packet_direction = 
    ctx_info[2].packet_direction = PDCP_DOWNLINK;
    
    ctx_info[0].protocol_direction = 
    ctx_info[1].protocol_direction = 
    ctx_info[2].protocol_direction = PDCP_ENCAPSULATION;

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

    ////////////////////////////////////
    ////////////////////////////////////
    
    // How many packets to send and receive to/from SEC
    packets_handled = 1;
    
    // Get one packet to be used as input for both procedures
    ret = get_pkt(&in_pkt,test_data_in_encap,sizeof(test_data_in_encap));
    assert_equal_with_message(ret, 0,
            "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
            0, ret);
            
    // Get one packet to be used for auth output
    ret = get_pkt(&auth_only_pkt,NULL,sizeof(test_data_in_encap) + 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
            "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
            0, ret);

    // Submit one packet on the first context for authentication
    ret = sec_process_packet(ctx_handle_0, in_pkt, auth_only_pkt, NULL);
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
    
    // Get one packet to be used for auth+enc output in dbl pass
    ret = get_pkt(&auth_ciphered_pkt_dbl_pass,NULL,sizeof(test_data_in_encap) + 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);
    
    // Submit one packet on the second context for ciphering
    ret = sec_process_packet(ctx_handle_1, auth_only_pkt, auth_ciphered_pkt_dbl_pass, NULL);
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

    // Get one packet to be used for auth+enc output in one pass
    ret = get_pkt(&auth_ciphered_pkt,NULL,sizeof(test_data_in_encap) + 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);
    
    // Submit one packet on the third context for ciphering and authentication in one pass
    ret = sec_process_packet(ctx_handle_2, in_pkt, auth_ciphered_pkt, NULL);
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
                              
    // assert that the pkts obtained through the two methods are bit-exact
    memcmp_res = memcmp(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset,
                        auth_ciphered_pkt->address + auth_ciphered_pkt->offset,
                        PDCP_HEADER_LENGTH);
    if ( memcmp_res != 0 )
    {
        printf("Error comparing headers:\n");
    }
    printf("Double pass header  |  Single Pass header\n");
    printf("0x%02x                |                0x%02x\n",
            *(uint8_t*)(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset),
            *(uint8_t*)(auth_ciphered_pkt->address + auth_ciphered_pkt->offset) );

    assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking packet contents: header is different!");
                        
    memcmp_res = memcmp(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset + PDCP_HEADER_LENGTH,
                        auth_ciphered_pkt->address + auth_ciphered_pkt->offset + PDCP_HEADER_LENGTH,
                        auth_ciphered_pkt->length - PDCP_HEADER_LENGTH);
    if (memcmp_res != 0)
    {
        printf("Error comparing contents:\n");
    }

    printf("Double pass content |  Single Pass content\n");
    for( i = 0; i < auth_ciphered_pkt->length - PDCP_HEADER_LENGTH; i++)
    {
        printf("0x%02x                |                0x%02x\n",
                *(uint8_t*)(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset + PDCP_HEADER_LENGTH + i),
                *(uint8_t*)(auth_ciphered_pkt->address + auth_ciphered_pkt->offset + PDCP_HEADER_LENGTH + i) );
    }

    assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking packet contents: content is different!");

    put_pkt(&in_pkt);
    put_pkt(&auth_only_pkt);
    put_pkt(&auth_ciphered_pkt_dbl_pass);
    put_pkt(&auth_ciphered_pkt);

    // release sec driver
    ret = sec_release();
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
    
}

static void test_c_plane_mixed_downlink_decap(void)
{
    int ret = 0;
    int limit = SEC_JOB_RING_SIZE  - 1;
    int i = 0;
    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_job_ring_handle_t jr_handle_1;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;
    sec_context_handle_t ctx_handle_2 = NULL;
    sec_packet_t *in_pkt = NULL, *auth_ciphered_pkt_dbl_pass = NULL,*auth_ciphered_pkt=NULL, *auth_only_pkt = NULL;
    int memcmp_res = 0;
    // configuration data for a PDCP context
    sec_pdcp_context_info_t ctx_info[3] = {
        /* cipher-only */
        {
            .cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_NULL,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
        },
        /* auth-only */
        {
            .cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
        },
        /* auth+cipher */
        {
            .cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
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

    ctx_info[0].packet_direction =
    ctx_info[1].packet_direction =
    ctx_info[2].packet_direction = PDCP_DOWNLINK;

    ctx_info[0].protocol_direction =
    ctx_info[1].protocol_direction =
    ctx_info[2].protocol_direction = PDCP_DECAPSULATION;

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

    ////////////////////////////////////
    ////////////////////////////////////

    // How many packets to send and receive to/from SEC
    packets_handled = 1;

    ret = get_pkt(&in_pkt,test_data_in_decap_dl,sizeof(test_data_in_decap_dl));
    assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

    // Get one packet to be used for auth output
    ret = get_pkt(&auth_only_pkt,NULL,sizeof(test_data_in_decap_dl));
    assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

    // Submit one packet on the first context for decryption
    ret = sec_process_packet(ctx_handle_0, in_pkt, auth_only_pkt, NULL);
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

    ret = get_pkt(&auth_ciphered_pkt_dbl_pass,NULL,sizeof(test_data_in_decap_dl) - 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

    // Submit one packet on the second context for ciphering
    ret = sec_process_packet(ctx_handle_1, auth_only_pkt, auth_ciphered_pkt_dbl_pass, NULL);
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

    ret = get_pkt(&auth_ciphered_pkt,NULL,sizeof(test_data_in_decap_dl) - 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

    // Submit one packet on the third context for ciphering and authentication in one pass
    ret = sec_process_packet(ctx_handle_2, in_pkt, auth_ciphered_pkt, NULL);
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

    // assert that the pkts obtained through the two methods are bit-exact
    memcmp_res = memcmp(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset,
                        auth_ciphered_pkt->address + auth_ciphered_pkt->offset,
                        PDCP_HEADER_LENGTH);
    if ( memcmp_res != 0 )
    {
        printf("Error comparing headers:\n");
    }
    printf("Double pass header  |  Single Pass header\n");
    printf("0x%02x                |                0x%02x\n",
            *(uint8_t*)(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset),
            *(uint8_t*)(auth_ciphered_pkt->address + auth_ciphered_pkt->offset) );

    assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking packet contents: header is different!");

    memcmp_res = memcmp(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset + PDCP_HEADER_LENGTH,
                        auth_ciphered_pkt->address + auth_ciphered_pkt->offset + PDCP_HEADER_LENGTH,
                        auth_ciphered_pkt->length - PDCP_HEADER_LENGTH);
    if (memcmp_res != 0)
    {
        printf("Error comparing contents:\n");
    }

    printf("Double pass content |  Single Pass content\n");
    for( i = 0; i < auth_ciphered_pkt->length - PDCP_HEADER_LENGTH; i++)
    {
        printf("0x%02x                |                0x%02x\n",
                *(uint8_t*)(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset + PDCP_HEADER_LENGTH + i),
                *(uint8_t*)(auth_ciphered_pkt->address + auth_ciphered_pkt->offset + PDCP_HEADER_LENGTH + i) );
    }

    assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking packet contents: content is different!");

    put_pkt(&in_pkt);
    put_pkt(&auth_only_pkt);
    put_pkt(&auth_ciphered_pkt_dbl_pass);
    put_pkt(&auth_ciphered_pkt);

    // release sec driver
    ret = sec_release();
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

}

static void test_c_plane_mixed_uplink_encap(void)
{
    int ret = 0;
    int limit = SEC_JOB_RING_SIZE  - 1;
    int i = 0;
    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_job_ring_handle_t jr_handle_1;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;
    sec_context_handle_t ctx_handle_2 = NULL;
    sec_packet_t *in_pkt = NULL,*auth_ciphered_pkt_dbl_pass = NULL,*auth_ciphered_pkt=NULL, *auth_only_pkt = NULL;
    int memcmp_res = 0;
    // configuration data for a PDCP context
    sec_pdcp_context_info_t ctx_info[3] = {
        /* auth-only */
        {
            .cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
        },
        /* cipher-only */
        {
            .cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_NULL,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
        },
        /* auth+cipher */
        {
            .cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
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

    ctx_info[0].packet_direction =
    ctx_info[1].packet_direction =
    ctx_info[2].packet_direction = PDCP_UPLINK;

    ctx_info[0].protocol_direction =
    ctx_info[1].protocol_direction =
    ctx_info[2].protocol_direction = PDCP_ENCAPSULATION;

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

    ////////////////////////////////////
    ////////////////////////////////////

    // How many packets to send and receive to/from SEC
    packets_handled = 1;

    ret = get_pkt(&in_pkt,test_data_in_encap,sizeof(test_data_in_encap));
    assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

    // Get one packet to be used for auth output
    ret = get_pkt(&auth_only_pkt,NULL,sizeof(test_data_in_encap) + 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

    // Submit one packet on the first context for authentication
    ret = sec_process_packet(ctx_handle_0, in_pkt, auth_only_pkt, NULL);
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

    ret = get_pkt(&auth_ciphered_pkt_dbl_pass,NULL,sizeof(test_data_in_encap) + 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

    // Submit one packet on the second context for ciphering
    ret = sec_process_packet(ctx_handle_1, auth_only_pkt, auth_ciphered_pkt_dbl_pass, NULL);
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

    ret = get_pkt(&auth_ciphered_pkt,NULL,sizeof(test_data_in_encap) + 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

    // Submit one packet on the third context for ciphering and authentication in one pass
    ret = sec_process_packet(ctx_handle_2, in_pkt, auth_ciphered_pkt, NULL);
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

    // assert that the pkts obtained through the two methods are bit-exact
    memcmp_res = memcmp(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset,
                        auth_ciphered_pkt->address + auth_ciphered_pkt->offset,
                        PDCP_HEADER_LENGTH);
    if ( memcmp_res != 0 )
    {
        printf("Error comparing headers:\n");
    }
    printf("Double pass header  |  Single Pass header\n");
    printf("0x%02x                |                0x%02x\n",
            *(uint8_t*)(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset),
            *(uint8_t*)(auth_ciphered_pkt->address + auth_ciphered_pkt->offset) );

    assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking packet contents: header is different!");

    memcmp_res = memcmp(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset + PDCP_HEADER_LENGTH,
                        auth_ciphered_pkt->address + auth_ciphered_pkt->offset + PDCP_HEADER_LENGTH,
                        auth_ciphered_pkt->length - PDCP_HEADER_LENGTH);
    if (memcmp_res != 0)
    {
        printf("Error comparing contents:\n");
    }

    printf("Double pass content |  Single Pass content\n");
    for( i = 0; i < auth_ciphered_pkt->length - PDCP_HEADER_LENGTH; i++)
    {
        printf("0x%02x                |                0x%02x\n",
                *(uint8_t*)(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset + PDCP_HEADER_LENGTH + i),
                *(uint8_t*)(auth_ciphered_pkt->address + auth_ciphered_pkt->offset + PDCP_HEADER_LENGTH + i) );
    }

    assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking packet contents: content is different!");

    put_pkt(&in_pkt);
    put_pkt(&auth_only_pkt);
    put_pkt(&auth_ciphered_pkt_dbl_pass);
    put_pkt(&auth_ciphered_pkt);

    // release sec driver
    ret = sec_release();
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

}
static void test_c_plane_mixed_uplink_decap(void)
{
    int ret = 0;
    int limit = SEC_JOB_RING_SIZE  - 1;
    int i = 0;
    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_job_ring_handle_t jr_handle_1;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;
    sec_context_handle_t ctx_handle_2 = NULL;
    sec_packet_t *in_pkt = NULL, *auth_ciphered_pkt_dbl_pass = NULL,*auth_ciphered_pkt=NULL, *auth_only_pkt = NULL;
    int memcmp_res = 0;
    // configuration data for a PDCP context
    sec_pdcp_context_info_t ctx_info[3] = {
        /* cipher-only */
        {
            .cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_NULL,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
        },
        /* auth-only */
        {
            .cipher_algorithm       = SEC_ALG_NULL,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
        },
        /* auth+cipher */
        {
            .cipher_algorithm       = SEC_ALG_AES,
            .cipher_key             = cipher_key,
            .cipher_key_len         = sizeof(test_crypto_key),
            .integrity_algorithm    = SEC_ALG_AES,
            .integrity_key          = integrity_key,
            .integrity_key_len      = sizeof(test_auth_key),
            .notify_packet          = &handle_packet_from_sec,
            .sn_size                = SEC_PDCP_SN_SIZE_5,
            .bearer                 = test_bearer,
            .user_plane             = PDCP_CONTROL_PLANE,
            //.packet_direction       = test_pkt_dir;
            //.protocol_direction     = test_proto_dir;
            .hfn                    = test_hfn,
            .hfn_threshold          = test_hfn_threshold
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

    ctx_info[0].packet_direction =
    ctx_info[1].packet_direction =
    ctx_info[2].packet_direction = PDCP_UPLINK;

    ctx_info[0].protocol_direction =
    ctx_info[1].protocol_direction =
    ctx_info[2].protocol_direction = PDCP_DECAPSULATION;

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

    ////////////////////////////////////
    ////////////////////////////////////

    // How many packets to send and receive to/from SEC
    packets_handled = 1;

    ret = get_pkt(&in_pkt,test_data_in_decap_ul,sizeof(test_data_in_decap_ul));
    assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

    // Get one packet to be used for auth output
    ret = get_pkt(&auth_only_pkt,NULL,sizeof(test_data_in_decap_ul));
    assert_equal_with_message(ret, 0,
                "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                0, ret);

    // Submit one packet on the first context for decryption
    ret = sec_process_packet(ctx_handle_0, in_pkt, auth_only_pkt, NULL);
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

    ret = get_pkt(&auth_ciphered_pkt_dbl_pass,NULL,sizeof(test_data_in_decap_ul) - 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

    // Submit one packet on the second context for ciphering
    ret = sec_process_packet(ctx_handle_1, auth_only_pkt, auth_ciphered_pkt_dbl_pass, NULL);
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

    ret = get_pkt(&auth_ciphered_pkt,NULL,sizeof(test_data_in_decap_ul) - 4 /* ICV size */);
    assert_equal_with_message(ret, 0,
                    "ERROR on get_pkt: expected ret[%d]. actual ret[%d]",
                    0, ret);

    // Submit one packet on the third context for ciphering and authentication in one pass
    ret = sec_process_packet(ctx_handle_2, in_pkt, auth_ciphered_pkt, NULL);
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

    // assert that the pkts obtained through the two methods are bit-exact
    memcmp_res = memcmp(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset,
                        auth_ciphered_pkt->address + auth_ciphered_pkt->offset,
                        PDCP_HEADER_LENGTH);
    if ( memcmp_res != 0 )
    {
        printf("Error comparing headers:\n");
    }
    printf("Double pass header  |  Single Pass header\n");
    printf("0x%02x                |                0x%02x\n",
            *(uint8_t*)(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset),
            *(uint8_t*)(auth_ciphered_pkt->address + auth_ciphered_pkt->offset) );

    assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking packet contents: header is different!");

    memcmp_res = memcmp(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset + PDCP_HEADER_LENGTH,
                        auth_ciphered_pkt->address + auth_ciphered_pkt->offset + PDCP_HEADER_LENGTH,
                        auth_ciphered_pkt->length - PDCP_HEADER_LENGTH);
    if (memcmp_res != 0)
    {
        printf("Error comparing contents:\n");
    }

    printf("Double pass content |  Single Pass content\n");
    for( i = 0; i < auth_ciphered_pkt->length - PDCP_HEADER_LENGTH; i++)
    {
        printf("0x%02x                |                0x%02x\n",
                *(uint8_t*)(auth_ciphered_pkt_dbl_pass->address + auth_ciphered_pkt_dbl_pass->offset + PDCP_HEADER_LENGTH + i),
                *(uint8_t*)(auth_ciphered_pkt->address + auth_ciphered_pkt->offset + PDCP_HEADER_LENGTH + i) );
    }

    assert_equal_with_message(memcmp_res, 0,
                              "ERROR on checking packet contents: content is different!");

    put_pkt(&in_pkt);
    put_pkt(&auth_only_pkt);
    put_pkt(&auth_ciphered_pkt_dbl_pass);
    put_pkt(&auth_ciphered_pkt);

    // release sec driver
    ret = sec_release();
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

}

static TestSuite * mixed_descs_tests()
{
    /* create test suite */
    TestSuite * suite = create_test_suite();

    /* setup/teardown functions to be called before/after each unit test */
    setup(suite, test_setup);
    teardown(suite, test_teardown);

    /* start adding unit tests */
    add_test(suite, test_c_plane_mixed_downlink_encap);
    add_test(suite, test_c_plane_mixed_uplink_encap);
    add_test(suite, test_c_plane_mixed_downlink_decap);
    add_test(suite, test_c_plane_mixed_uplink_decap);

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
    TestSuite * suite = mixed_descs_tests();
    TestReporter * reporter = create_text_reporter();

    /* Run tests */

    run_single_test(suite, "test_c_plane_mixed_downlink_encap", reporter);
    run_single_test(suite, "test_c_plane_mixed_uplink_encap", reporter);
    run_single_test(suite, "test_c_plane_mixed_downlink_decap", reporter);
    run_single_test(suite, "test_c_plane_mixed_uplink_decap", reporter);

    destroy_test_suite(suite);
    (*reporter->destroy)(reporter);

    return 0;
} /* main() */

/*================================================================================================*/

#ifdef __cplusplus
}

#endif
