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

#ifdef _cplusplus
extern "C" {
#endif

/*=================================================================================================
                                        INCLUDE FILES
==================================================================================================*/
#include "fsl_sec.h"
#include "cgreen.h"

#ifdef SEC_HW_VERSION_3_1
#include "compat.h"
#else // SEC_HW_VERSION_3_1

// For shared memory allocator
#include "fsl_usmmgr.h"

#endif // SEC_HW_VERSION_3_1

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
#define TEST_PACKET_LENGTH  200

// Alignment in bytes for input/output packets allocated from DMA-memory zone
#define BUFFER_ALIGNEMENT 32

// Size in bytes for the buffer used by a packet.
// A packet can use less but not more than this number of bytes.
#define BUFFER_SIZE 200

// Max length in bytes for a confidentiality /integrity key.
#define MAX_KEY_LENGTH    32

#ifdef SEC_HW_VERSION_4_4
/** Size in bytes of a cacheline. */
#define CACHE_LINE_SIZE  32

// For keeping the code relatively the same between HW versions
#define dma_mem_memalign  test_memalign
#define dma_mem_free      test_free

#endif

// Sizeof sec_context_t struct as defined in driver.
// @note this has to be defined to have exactly the value of sizeof(struct sec_context_t)!
#ifdef SEC_HW_VERSION_3_1
#define SIZEOF_SEC_CONTEXT_T_STRUCT 128
#else
#define SIZEOF_SEC_CONTEXT_T_STRUCT 64
#endif

// Number of SEC contexts in each pool. Define taken from SEC user-space driver.
#define MAX_SEC_CONTEXTS_PER_POOL   (SEC_MAX_PDCP_CONTEXTS / (JOB_RING_NUMBER))

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/** The declaration of a structure. with the same size as
    sec_context_t structure from SEC user-space driver. */
struct test_sec_context_t
{
    uint8_t dummy[SIZEOF_SEC_CONTEXT_T_STRUCT];
}__CACHELINE_ALIGNED;

/** Structure to define packet info */
typedef struct buffer_s
{
    uint8_t buffer[BUFFER_SIZE];
    uint32_t offset;
    sec_packet_t pdcp_packet;
}buffer_t;
/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
// configuration data for SEC driver
static sec_config_t sec_config_data;

// configuration data for a PDCP context
static sec_pdcp_context_info_t ctx_info;

// job ring handles provided by SEC driver
static const sec_job_ring_descriptor_t *job_ring_descriptors = NULL;

// Global counter incremented in handle_packet_from_sec();
static int test_packets_notified = 0;

// Global variable that indicates after how many packets
// handle_packet_from_sec() will return SEC_RETURN_STOP.
// If not desired to return SEC_RETURN_STOP at all from
// handle_packet_from_sec() then leave this variable on 0.
static int test_packets_notified_ret_stop = 0;

// array of input packets
buffer_t *test_input_packets = NULL;

// User App specific data for every packet
uint8_t ua_data[TEST_PACKETS_NUMBER];

// array of output packets
buffer_t *test_output_packets = NULL;

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
#ifdef SEC_HW_VERSION_4_4

// FSL Userspace Memory Manager structure
fsl_usmmgr_t g_usmmgr;

#endif // SEC_HW_VERSION_4_4
/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
// Callback function registered for every PDCP context to handle procesed packets.
static int handle_packet_from_sec(const sec_packet_t *in_packet,
                                  const sec_packet_t *out_packet,
                                  ua_context_handle_t ua_ctx_handle,
                                  uint32_t status,
                                  uint32_t error_info);

// Get packet at specified index from the global array with test packets.
static void get_free_packet(int packet_idx, sec_packet_t **in_packet, sec_packet_t **out_packet);

// Send a specified number of packets to SEC for processing.
static void send_packets(sec_context_handle_t ctx,
                         int packet_no,
                         int expected_ret_code);
#ifdef SEC_HW_VERSION_4_4
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
#else

#define test_ptov(x)    (x)
#define test_vtop(x)    (x)

#endif // SEC_HW_VERSION_4_4
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

    // Global counter incremented for every notified packet.
    test_packets_notified++;

    // A configurable method to make the callback return #SEC_RETURN_STOP.
    if (test_packets_notified_ret_stop != 0 &&
        test_packets_notified >= test_packets_notified_ret_stop)
    {
        ret = SEC_RETURN_STOP;
    }

    return ret;
}

static void get_free_packet(int packet_idx, sec_packet_t **in_packet, sec_packet_t **out_packet)
{
    *in_packet = &test_input_packets[packet_idx].pdcp_packet;
    *out_packet = &test_output_packets[packet_idx].pdcp_packet;

    // Initialize with valid info the input packet
    (*in_packet)->address = test_vtop(&(test_input_packets[packet_idx].buffer[0]));
    (*in_packet)->offset = TEST_PACKET_OFFSET;
#ifdef SEC_HW_VERSION_3_1
    (*in_packet)->scatter_gather = SEC_CONTIGUOUS_BUFFER;
#endif // SEC_HW_VERSION_3_1
    (*in_packet)->length = TEST_PACKET_LENGTH;
    (*in_packet)->total_length = 0;
    (*in_packet)->num_fragments = 0;

    // Initialize with valid info the output packet
    (*out_packet)->address = test_vtop(&(test_output_packets[packet_idx].buffer[0]));
    (*out_packet)->offset = TEST_PACKET_OFFSET;
#ifdef SEC_HW_VERSION_3_1
    (*out_packet)->scatter_gather = SEC_CONTIGUOUS_BUFFER;
#endif
    (*out_packet)->length = TEST_PACKET_LENGTH;
    (*out_packet)->total_length = 0;
    (*out_packet)->num_fragments = 0;

}

static void send_packets(sec_context_handle_t ctx, int packet_no, int expected_ret_code)
{
    int ret = 0;
    sec_packet_t *in_packet = NULL;
    sec_packet_t *out_packet = NULL;
    int idx = 0;

#ifdef SEC_HW_VERSION_3_1
    assert(packet_no < TEST_PACKETS_NUMBER);
#endif
    assert(ctx != NULL);

    for (idx = 0; idx < packet_no; idx++)
    {
        get_free_packet(idx
#ifdef SEC_HW_VERSION_4_4
                %TEST_PACKETS_NUMBER
#endif
                , &in_packet, &out_packet);
        // Submit one packet on the context
        ret = sec_process_packet(ctx, in_packet, out_packet, (ua_context_handle_t)&ua_data[idx]);
#ifdef SEC_HW_VERSION_4_4
        if( ret != expected_ret_code)
            break;
#else
        assert_equal_with_message(ret, expected_ret_code,
                "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                expected_ret_code, ret);
#endif
    }

#ifdef SEC_HW_VERSION_4_4
    assert_equal_with_message(ret, expected_ret_code,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                               expected_ret_code, ret);
#endif
}

#ifdef SEC_HW_VERSION_4_4
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
#endif // SEC_HW_VERSION_4_4
static void test_setup(void)
{
#ifdef SEC_HW_VERSION_3_1
    int ret = 0;

    // map the physical memory
    ret = dma_mem_setup();
    assert_equal_with_message(ret, 0, "ERROR on dma_mem_setup: ret = %d", ret);

    // Fill SEC driver configuration data
    sec_config_data.memory_area = (void*)__dma_virt;
#else
    sec_config_data.memory_area = dma_mem_memalign(CACHE_LINE_SIZE,SEC_DMA_MEMORY_SIZE);
    sec_config_data.sec_drv_vtop = test_vtop;
#endif
    sec_config_data.work_mode = SEC_STARTUP_POLLING_MODE;

    cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT, MAX_KEY_LENGTH);
    assert_not_equal_with_message(cipher_key, NULL, "ERROR allocating cipher_key with dma_mem_memalign");
    memset(cipher_key, 0, MAX_KEY_LENGTH);
    memcpy(cipher_key, test_crypto_key, sizeof(test_crypto_key));

    integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT, MAX_KEY_LENGTH);
    assert_not_equal_with_message(integrity_key, NULL, "ERROR allocating integrity_key with dma_mem_memalign");
    memset(integrity_key, 0, MAX_KEY_LENGTH);
    memcpy(integrity_key, test_auth_key, sizeof(test_auth_key));

    ctx_info.cipher_algorithm = SEC_ALG_SNOW;
    ctx_info.cipher_key = cipher_key;
    ctx_info.cipher_key_len = sizeof(test_crypto_key);
    ctx_info.integrity_algorithm = SEC_ALG_NULL;
    ctx_info.integrity_key = NULL;
    ctx_info.integrity_key_len = 0;
    ctx_info.notify_packet = &handle_packet_from_sec;
    ctx_info.sn_size = SEC_PDCP_SN_SIZE_7;
    ctx_info.bearer = 0x3;
    // Use data plane contexts as control-plane requires the packets to be
    // send two times to SEC, in case of SEC 3.1
    ctx_info.user_plane = PDCP_DATA_PLANE;
    ctx_info.packet_direction = PDCP_DOWNLINK;
    ctx_info.protocol_direction = PDCP_ENCAPSULATION;
    ctx_info.hfn = 0xFA556;
    ctx_info.hfn_threshold = 0xFF00000;

    test_input_packets = dma_mem_memalign(BUFFER_ALIGNEMENT,
            sizeof(buffer_t) * TEST_PACKETS_NUMBER);
    assert_not_equal_with_message(test_input_packets, NULL,
            "ERROR allocating test_input_packets with dma_mem_memalign");
    memset(test_input_packets, 0, sizeof(buffer_t) * TEST_PACKETS_NUMBER);

    test_output_packets = dma_mem_memalign(BUFFER_ALIGNEMENT,
            sizeof(buffer_t) * TEST_PACKETS_NUMBER);
    assert_not_equal_with_message(test_output_packets, NULL,
            "ERROR allocating test_output_packets with dma_mem_memalign");
    memset(test_output_packets, 0, sizeof(buffer_t) * TEST_PACKETS_NUMBER);

    memset(ua_data, 0x3, TEST_PACKETS_NUMBER);
}

static void test_teardown()
{
#ifdef SEC_HW_VERSION_3_1
    int ret = 0;

    // unmap the physical memory
    dma_mem_release();
#else // SEC_HW_VERSION_3_1
    // Destoy FSL USMMGR object
    //ret = fsl_usmmgr_exit(g_usmmgr);
    //assert_equal_with_message(ret,0,"Failure to destroy the FSL USMMGR: %d",ret);
    dma_mem_free(sec_config_data.memory_area,SEC_DMA_MEMORY_SIZE);
#endif // SEC_HW_VERSION_3_1

    dma_mem_free(cipher_key, MAX_KEY_LENGTH);
    dma_mem_free(integrity_key, MAX_KEY_LENGTH);
    dma_mem_free(test_input_packets, sizeof(buffer_t) * TEST_PACKETS_NUMBER);
    dma_mem_free(test_output_packets, sizeof(buffer_t) * TEST_PACKETS_NUMBER);
}

static void test_sec_init_invalid_params(void)
{
    int ret = 0;
    int i = 0;
    void* tmp = NULL;

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. Invalid sec_config_data param
    ret = sec_init(NULL, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. Invalid sec_config_data.memory_area param
    tmp = sec_config_data.memory_area;
    sec_config_data.memory_area = NULL;

    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);
    // Restore memory_area ptr
    sec_config_data.memory_area = tmp;

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. Invalid sec_config_data.memory_area param: non-cache-line aligned
    tmp = sec_config_data.memory_area;
    uint8_t cache_line_size = 32;
    if ((dma_addr_t)sec_config_data.memory_area % cache_line_size  == 0)
    {
        sec_config_data.memory_area += 1;
    }
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);
    // Restore memory_area ptr
    sec_config_data.memory_area = tmp;

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. Invalid job_rings_no param
    ret = sec_init(&sec_config_data, 5, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. Invalid job_rings_no param.
    // Initializing all 4 of SEC's job rings is likely to fail as
    // some job rings will be assigned for kernel-space usage from DTS file.

    // @note: This assert will pass ONLY if some job rings are configured
    // in DTS file to be used by kernel space SEC driver!
    ret = sec_init(&sec_config_data, MAX_SEC_JOB_RINGS, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. Invalid job_ring_descriptors param
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, NULL);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    for (i = 0; i < JOB_RING_NUMBER; i++)
    {
        assert_not_equal_with_message(job_ring_descriptors[i].job_ring_handle, NULL,
                "ERROR on sec_init: NULL job_ring_handle for jr idx %d", i);
        assert_not_equal_with_message(job_ring_descriptors[i].job_ring_irq_fd, 0,
                "ERROR on sec_init: 0 job_ring_irq_fd for jr idx %d", i);
    }

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. Expect error as driver is already initialized.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_DRIVER_ALREADY_INITIALIZED,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_DRIVER_ALREADY_INITIALIZED, ret);

    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_DRIVER_ALREADY_INITIALIZED",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // No error from SEC, ret should be 0.
    ret = sec_get_last_error();
    assert_equal_with_message(ret, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, ret);

    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}


static void test_sec_create_pdcp_context_invalid_params(void)
{
    int ret = 0;
    sec_context_handle_t ctx_handle;
    sec_context_handle_t ctx_handles[3 * MAX_SEC_CONTEXTS_PER_POOL];
    uint8_t *tmp = NULL;
    int i = 0;

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid state: driver not yet initialized.
    ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_DRIVER_NOT_INITIALIZED,
                              "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_DRIVER_NOT_INITIALIZED, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // Invalid sec_ctx_info param.
    ret = sec_create_pdcp_context(NULL, NULL, &ctx_handle);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid sec_ctx_handle param.
    ret = sec_create_pdcp_context(NULL, &ctx_info, NULL);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid cipher key = NULL.
    tmp = ctx_info.cipher_key;
    ctx_info.cipher_key = NULL;

    ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // restore cipher_key
    ctx_info.cipher_key = tmp;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid cipher key, non-cacheline-aligned.
    tmp = ctx_info.cipher_key;
    ctx_info.cipher_key += 1;

    ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // restore cipher_key
    ctx_info.cipher_key = tmp;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid integrity key, non-cacheline-aligned.
    tmp = ctx_info.integrity_key;
    ctx_info.integrity_key += 1;

    ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // restore cipher_key
    ctx_info.integrity_key = tmp;

    ////////////////////////////////////
    ////////////////////////////////////

    for (i = 0; i < MAX_SEC_CONTEXTS_PER_POOL * 3; i++)
    {
        ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handles[i]);
        assert_equal_with_message(ret, SEC_SUCCESS,
                "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
                SEC_SUCCESS, ret);
    }

    ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_DRIVER_NO_FREE_CONTEXTS,
            "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_DRIVER_NO_FREE_CONTEXTS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Now delete last created context
    ret = sec_delete_pdcp_context(ctx_handles[MAX_SEC_CONTEXTS_PER_POOL * 3 - 1]);
    assert_equal_with_message(ret, SEC_SUCCESS,
            "ERROR on sec_delete_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_SUCCESS, ret);

    // Now create another context, it should work.
    ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_SUCCESS,
            "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_SUCCESS, ret);

    // Now delete last created context
    ret = sec_delete_pdcp_context(ctx_handle);
    assert_equal_with_message(ret, SEC_SUCCESS,
            "ERROR on sec_delete_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Now delete all remaining contexts
    for (i = 0; i < MAX_SEC_CONTEXTS_PER_POOL * 3 - 1; i++)
    {
        ret = sec_delete_pdcp_context(ctx_handles[i]);
        // Cannot do cgreen-assert for all contexts because cgreen cannot
        // handle so many asserts in a single test.
        assert(ret == SEC_SUCCESS);
    }

    ////////////////////////////////////
    ////////////////////////////////////

    // No error from SEC, ret should be 0.
    ret = sec_get_last_error();
    assert_equal_with_message(ret, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, ret);
    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}

static void test_sec_delete_pdcp_context_invalid_params(void)
{
    int ret = 0;
    sec_context_handle_t ctx_handle;
    struct test_sec_context_t *ctx = NULL;

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid state: driver not yet initialized.
    ret = sec_delete_pdcp_context(&ctx_handle);
    assert_equal_with_message(ret, SEC_DRIVER_NOT_INITIALIZED,
                              "ERROR on sec_delete_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_DRIVER_NOT_INITIALIZED, ret);
    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid sec_ctx_handle: NULL
    ret = sec_delete_pdcp_context(NULL);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_delete_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Now create a context, it should work.
    ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_SUCCESS,
            "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_SUCCESS, ret);

    // Make the context handle invalid, write last 32  bytes from handle
    assert(sizeof(struct test_sec_context_t) > CACHE_LINE_SIZE);

    // Convert the opaque context handle to a local structure that
    // has the same size as sec_context_t struct from driver!
    ctx = (struct test_sec_context_t*)ctx_handle;

    // The sec_context_t struct has a start and end patterns, to validate that
    // it was not altered by User App.
    // Overwrite the last 32 bytes (size of cacheline), the end pattern will be in the last
    // 32 bytes as some padding bytes may be added to ensure cache-line alignment of sec_context_t.
    memset(ctx->dummy + sizeof(struct test_sec_context_t) - CACHE_LINE_SIZE,
           0x2, //some random value
           CACHE_LINE_SIZE);

    // Invalid sec_ctx_handle, overwritten end pattern from ctx handle.
    ret = sec_delete_pdcp_context(ctx_handle);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_delete_pdcp_context: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // No error from SEC, ret should be 0.
    ret = sec_get_last_error();
    assert_equal_with_message(ret, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, ret);
    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}

static void test_sec_process_packet_invalid_params(void)
{
    int ret = 0;
    sec_context_handle_t ctx_handle = NULL;
    struct test_sec_context_t *ctx = NULL;
    sec_packet_t *in_packet = NULL;
    sec_packet_t *out_packet = NULL;
#ifdef SEC_HW_VERSION_4_4
    dma_addr_t tmp = 0;
#else
    uint8_t* tmp = NULL;
#endif
    int packet_idx = 0;
#ifdef SEC_HW_VERSION_4_4
    uint32_t    tmp_num;
#if (SEC_ENABLE_SCATTER_GATHER == ON)
    sec_packet_t *in_packet2 = NULL;
    sec_packet_t *out_packet2 = NULL;
    sec_packet_t *in_packet3 = NULL;
    sec_packet_t *out_packet3 = NULL;
    sec_packet_t in_sg_packet[3];
    sec_packet_t out_sg_packet[3];
#endif // defined(SEC_HW_VERSION_4_4)
#endif //(SEC_ENABLE_SCATTER_GATHER == ON)

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Get packet at idx 0 from our array with test packets
    packet_idx = 0;
    get_free_packet(packet_idx, &in_packet, &out_packet);

    // Invalid state: driver not yet initialized.
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_DRIVER_NOT_INITIALIZED,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_DRIVER_NOT_INITIALIZED, ret);

    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_DRIVER_NOT_INITIALIZED",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid context handle: NULL
    ret = sec_process_packet(NULL, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Create one context
    ret = sec_create_pdcp_context(NULL, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_SUCCESS,
            "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid input packet: NULL
    ret = sec_process_packet(ctx_handle, NULL, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);
    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid input packet -> buffer address is NULL
    tmp = in_packet->address;
#ifdef SEC_HW_VERSION_4_4
    in_packet->address = 0;
#else
    in_packet->address = NULL;
#endif

    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore input packet address
    in_packet->address = tmp;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid output packet: NULL
    ret = sec_process_packet(ctx_handle, in_packet, NULL, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    tmp = out_packet->address;
#ifdef SEC_HW_VERSION_4_4
    out_packet->address = 0;
#else
    out_packet->address = NULL;
#endif

    // Invalid output packet -> buffer address is NULL
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore output packet address
    out_packet->address = tmp;
#ifdef SEC_HW_VERSION_4_4
    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid length on input packet
    tmp_num = in_packet->length;
    in_packet->length = 0;
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore input packet num of frags
    in_packet->length = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid length on input packet
    tmp_num = out_packet->length;
    out_packet->length = 0;
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore input packet num of frags
    out_packet->length = tmp_num;

#if (SEC_ENABLE_SCATTER_GATHER == ON)
    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid number of fragments on input packet
    tmp_num = in_packet->num_fragments;
    in_packet->num_fragments = 100;
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                               SEC_INVALID_INPUT_PARAM, ret);

    // Restore input packet num of frags
    in_packet->num_fragments = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid number of fragments on output packet
    tmp_num = out_packet->num_fragments;
    out_packet->num_fragments = 100;
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                             "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                             SEC_INVALID_INPUT_PARAM, ret);

    // Restore output packet num of frags
    out_packet->num_fragments = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid offset
    tmp_num = in_packet->offset;
    in_packet->offset = in_packet->length;

    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore input packet offset
    in_packet->offset = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid offset
    tmp_num = out_packet->offset;
    out_packet->offset = out_packet->length;

    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore output packet offset
    out_packet->offset = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid total length, two fragments input

    tmp_num = in_packet->total_length;
    in_packet->num_fragments = 1; // two packets
    in_packet->total_length = 2 * in_packet->length + 1; // two fragments

    in_sg_packet[0] = *in_packet;
    in_sg_packet[1] = *in_packet;

    ret = sec_process_packet(ctx_handle,in_sg_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                             "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                             SEC_INVALID_INPUT_PARAM, ret);

    // Restore
    in_packet->num_fragments = 0;
    in_packet->total_length = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid total length, two fragments output

    tmp_num = out_packet->total_length;
    out_packet->num_fragments = 1; // two packets
    out_packet->total_length = 2 * out_packet->length + 1; // two fragments

    out_sg_packet[0] = *out_packet;
    out_sg_packet[1] = *out_packet;

    ret = sec_process_packet(ctx_handle, in_packet, out_sg_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                             "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                             SEC_INVALID_INPUT_PARAM, ret);

    // Restore
    out_packet->num_fragments = 0;
    out_packet->total_length = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid fragment length, two fragments input

    tmp_num = in_packet->length;
    in_packet->length = in_packet->length + 1;
    in_packet->num_fragments = 1; // two fragments
    in_packet->total_length = 2 * tmp_num;

    in_sg_packet[0] = *in_packet;
    in_sg_packet[1] = *in_packet;

    ret = sec_process_packet(ctx_handle,in_sg_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore
    in_packet->num_fragments = 0;
    in_packet->total_length = 0;
    in_packet->length = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid fragment length, two fragments output

    tmp_num = out_packet->length;
    out_packet->length = out_packet->length + 1;
    out_packet->num_fragments = 1; // two fragments
    out_packet->total_length = 2 * tmp_num;

    out_sg_packet[0] = *out_packet;
    out_sg_packet[1] = *out_packet;

    ret = sec_process_packet(ctx_handle, in_packet, out_sg_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore
    out_packet->num_fragments = 0;
    out_packet->total_length = 0;
    out_packet->length = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid fragment pointer, two fragments input

    in_packet->num_fragments = 1; // two fragments
    in_packet->total_length = 2 * in_packet->length;

    in_sg_packet[0] = *in_packet;
    tmp = in_packet->address;
#ifdef SEC_HW_VERSION_4_4
    in_packet->address = 0;
#else
    in_packet->address = NULL;
#endif
    in_sg_packet[1] = *in_packet;
    in_packet->address = tmp;

    ret = sec_process_packet(ctx_handle,in_sg_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore
    in_packet->num_fragments = 0;
    in_packet->total_length = 0;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid fragment pointer, two fragments output

    out_packet->num_fragments = 1; // two fragments
    out_packet->total_length = 2 * in_packet->length;

    out_sg_packet[0] = *out_packet;
    tmp = out_packet->address;
#ifdef SEC_HW_VERSION_4_4
    out_packet->address = 0;
#else
    out_packet->address = NULL;
#endif
    out_sg_packet[1] = *out_packet;
    out_packet->address = tmp;

    ret = sec_process_packet(ctx_handle,in_packet, out_sg_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore
    out_packet->num_fragments = 0;
    out_packet->total_length = 0;
    out_packet->length = tmp_num;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid fragment offset, two fragments input

    in_packet->num_fragments = 1; // two fragments
    in_packet->total_length = 2 * in_packet->length;

    in_sg_packet[0] = *in_packet;
    tmp_num = in_packet->offset;
    in_packet->offset = in_packet->length;
    in_sg_packet[1] = *in_packet;
    in_packet->offset = tmp_num;

    ret = sec_process_packet(ctx_handle,in_sg_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore
    in_packet->num_fragments = 0;
    in_packet->total_length = 0;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid fragment offset, two fragments out

    out_packet->num_fragments = 1; // two fragments
    out_packet->total_length = 2 * out_packet->length;

    out_sg_packet[0] = *out_packet;
    tmp_num = out_packet->offset;
    out_packet->offset = out_packet->length;
    out_sg_packet[1] = *out_packet;
    out_packet->offset = tmp_num;

    ret = sec_process_packet(ctx_handle,in_packet, out_sg_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    // Restore
    out_packet->num_fragments = 0;
    out_packet->total_length = 0;

#endif //(SEC_ENABLE_SCATTER_GATHER == ON)
#endif // defined(SEC_HW_VERSION_4_4)

    ////////////////////////////////////
    ////////////////////////////////////

    // Submit one packet on the context
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_SUCCESS",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

#if defined(SEC_HW_VERSION_4_4) && (SEC_ENABLE_SCATTER_GATHER == ON)
    // Get one additional packets
    packet_idx = 1;
    get_free_packet(packet_idx,&in_packet2,&out_packet2);
    ////////////////////////////////////
    ////////////////////////////////////

    // Submit one packet, two fragments input, one output
    in_packet->num_fragments = 1; // two fragments
    in_packet->total_length = in_packet->length;
    in_packet->length /= 2;
    in_packet2->length /= 2;
    

    in_sg_packet[0] = *in_packet;
    in_sg_packet[1] = *in_packet2;

    ret = sec_process_packet(ctx_handle,in_sg_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // Restore
    in_packet->num_fragments = 0;
    in_packet->total_length = 0;
    
    in_packet->length *= 2;
    in_packet2->length *= 2;
    
    
    ////////////////////////////////////
    ////////////////////////////////////

    // Submit one packet, one input, two fragments output

    out_packet->num_fragments = 1; // two fragments
    out_packet->total_length = 2 * in_packet->length;

    out_sg_packet[0] = *out_packet;
    out_sg_packet[1] = *out_packet2;

    ret = sec_process_packet(ctx_handle,in_packet, out_sg_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // Restore
    out_packet->num_fragments = 0;
    out_packet->total_length = 0;

    ////////////////////////////////////
    ////////////////////////////////////

    // Submit one packet, two fragments input, two fragments output

    in_packet->num_fragments = 1; // two fragments
    in_packet->total_length = 2 * in_packet->length;

    out_packet->num_fragments = 1; // two fragments
    out_packet->total_length = 2 * out_packet->length;

    in_sg_packet[0] = *in_packet;
    in_sg_packet[1] = *in_packet2;

    out_sg_packet[0] = *out_packet;
    out_sg_packet[1] = *out_packet2;

    ret = sec_process_packet(ctx_handle,in_sg_packet, out_sg_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // Restore
    in_packet->num_fragments = 0;
    in_packet->total_length = 0;

    out_packet->num_fragments = 0;
    out_packet->total_length = 0;

    // Get one additional packet
    packet_idx = 2;
    get_free_packet(packet_idx,&in_packet3,&out_packet3);

    ////////////////////////////////////
    ////////////////////////////////////

    // Submit one packet, two fragments input, three fragments output

    in_packet->num_fragments = 1; // two fragments
    in_packet->total_length = 2 * in_packet->length;

    out_packet->num_fragments = 2; // three fragments
    out_packet->total_length = 3 * out_packet->length;

    in_sg_packet[0] = *in_packet;
    in_sg_packet[1] = *in_packet2;

    out_sg_packet[0] = *out_packet;
    out_sg_packet[1] = *out_packet2;
    out_sg_packet[2] = *out_packet3;

    ret = sec_process_packet(ctx_handle,in_sg_packet, out_sg_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // Restore
    in_packet->num_fragments = 0;
    in_packet->total_length = 0;

    out_packet->num_fragments = 0;
    out_packet->total_length = 0;

    ////////////////////////////////////
    ////////////////////////////////////

    // Submit one packet, three fragments input, two fragments output

    in_packet->num_fragments = 2; // three fragments
    in_packet->total_length = 3 * in_packet->length;

    out_packet->num_fragments = 1; // two fragments
    out_packet->total_length = 3 * out_packet->length;
    out_packet2->length *= 2;

    in_sg_packet[0] = *in_packet;
    in_sg_packet[1] = *in_packet2;
    in_sg_packet[2] = *in_packet3;

    out_sg_packet[0] = *out_packet;
    out_sg_packet[1] = *out_packet2;

    ret = sec_process_packet(ctx_handle,in_sg_packet, out_sg_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // Restore
    in_packet->num_fragments = 0;
    in_packet->total_length = 0;

    out_packet->num_fragments = 0;
    out_packet->total_length = 0;
    out_packet2->length /= 2;
    
    // Poll five packets so the test below still works
    sec_poll(5, 1, &tmp_num);
#endif // defined(SEC_HW_VERSION_4_4) && (SEC_ENABLE_SCATTER_GATHER == ON)

    // Delete context with 1 packet in flight.
    // Because we do not poll for results, the context is
    // just marked as beeing deleted in driver.
    ret = sec_delete_pdcp_context(ctx_handle);
    assert_equal_with_message(ret, SEC_LAST_PACKET_IN_FLIGHT,
            "ERROR on sec_delete_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_LAST_PACKET_IN_FLIGHT, ret);

    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_LAST_PACKET_IN_FLIGHT",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);


    // Now try and send another packet on context that was requested to be deleted.
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_CONTEXT_MARKED_FOR_DELETION,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_CONTEXT_MARKED_FOR_DELETION, ret);

    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_CONTEXT_MARKED_FOR_DELETION",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Make the context handle invalid, write last 32 bytes from handle
    assert(sizeof(struct test_sec_context_t) > CACHE_LINE_SIZE);

    // Convert the opaque context handle to a local structure that
    // has the same size as sec_context_t struct from driver!
    ctx = (struct test_sec_context_t*)ctx_handle;

    // The sec_context_t struct has a start and end patterns, to validate that
    // it was not altered by User App.
    // Overwrite the last 32 bytes (size of cacheline), the end pattern will be in the last
    // 32 bytes as some padding bytes may be added to ensure cache-line alignment of sec_context_t.
    memset(ctx->dummy + sizeof(struct test_sec_context_t) - CACHE_LINE_SIZE,
           0x2, //some random value
           CACHE_LINE_SIZE);

    // Invalid context handle
    ret = sec_process_packet(ctx_handle, in_packet, out_packet, (ua_context_handle_t)&ua_data[packet_idx]);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_process_packet: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_INVALID_INPUT_PARAM",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d\n", ret);
    ////////////////////////////////////
    ////////////////////////////////////

    // No error from SEC, ret should be 0.
    ret = sec_get_last_error();
    assert_equal_with_message(ret, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, ret);
    // Shutdown driver while packets are submitted and
    // not retrieved -> test flushing of job rings.

    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

}

static void test_sec_poll_invalid_params(void)
{
    int ret = 0;
    int32_t limit = 0;
    uint32_t weight = 0;
    uint32_t packets_out = 0;

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid state: driver not yet initialized.
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_DRIVER_NOT_INITIALIZED,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_DRIVER_NOT_INITIALIZED, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid limit param: 0.
    limit = 0;
    weight = 4;
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid weight param: 0.
    limit = 10;
    weight = 0;
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid limit and weight param: limit <  weight
    limit = 10;
    weight = 15;
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid limit and weight param: limit ==  weight
    limit = 10;
    weight = 10;
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid weight param: > size of SEC job ring size (FIFO is fixed at 24 entries on SEC 3.1)
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE + 10;
    weight = SEC_JOB_RING_HW_SIZE + 5;
#else
    limit = SEC_JOB_RING_SIZE + 10;
    weight = SEC_JOB_RING_SIZE + 5;
#endif
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Test with no invalid params. Should work
    limit = 30;
    weight = 4;

    // Assign a value to packets_out. It must be overwritten
    // by driver in sec_poll() function.
    packets_out = 1;

    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // No packets should be retrieved from SEC
    assert_equal_with_message(packets_out, 0,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              0, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Test with no invalid params and limit < 0. Should work
    limit = -1;
    weight = 4;

    // Assign a value to packets_out. It must be overwritten
    // by driver in sec_poll() function.
    packets_out = 1;

    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // No packets should be retrieved from SEC
    assert_equal_with_message(packets_out, 0,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              0, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Test with no invalid params and NULL packets_out. Should work
    limit = 30;
    weight = 4;
    ret = sec_poll(limit, weight, NULL);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // No error from SEC, ret should be 0.
    ret = sec_get_last_error();
    assert_equal_with_message(ret, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, ret);
    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}

static void test_sec_poll_job_ring_invalid_params(void)
{
    int ret = 0;
    int32_t limit = 0;
    uint32_t packets_out = 0;
    sec_job_ring_handle_t jr_handle;
    // Test only first job ring from the two JR's intialized
    int test_jr_id;

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid state: driver not yet initialized.
    ret = sec_poll_job_ring(NULL, limit, &packets_out);
    assert_equal_with_message(ret, SEC_DRIVER_NOT_INITIALIZED,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_DRIVER_NOT_INITIALIZED, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // Test only first job ring from the two JR's intialized
    test_jr_id = 0;
    jr_handle = job_ring_descriptors[test_jr_id].job_ring_handle;

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid job ring handle: NULL
    limit = 0;
    ret = sec_poll_job_ring(NULL, limit, &packets_out);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Invalid limit param: 0.
    limit = 0;
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);
    ////////////////////////////////////
    ////////////////////////////////////
#ifdef SEC_HW_VERSION_3_1
    // Invalid limit param: > size of SEC job ring size (FIFO is fixed at 24 entries on SEC 3.1)
    limit = SEC_JOB_RING_HW_SIZE + 5;
#else
    limit = SEC_JOB_RING_SIZE + 5;
#endif
    // Assign a value to packets_out. It must be overwritten
    // by driver in sec_poll_job_ring() function.
    packets_out = 1;

    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_INVALID_INPUT_PARAM,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_INVALID_INPUT_PARAM, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE;
#else
    limit = SEC_JOB_RING_SIZE;
#endif

    // Assign a value to packets_out. It must be overwritten
    // by driver in sec_poll_job_ring() function.
    packets_out = 1;

    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // No packets should be retrieved from SEC
    assert_equal_with_message(packets_out, 0,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              0, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Test with no invalid params and NULL packets_out. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE;
#else
    limit = SEC_JOB_RING_SIZE;
#endif
    ret = sec_poll_job_ring(jr_handle, limit, NULL);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // No error from SEC, ret should be 0.
    ret = sec_get_last_error();
    assert_equal_with_message(ret, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, ret);

    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}

static void test_poll_job_ring_scenarios(void)
{
    int ret = 0;
    int32_t limit = 0;
    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle;
    // Test only first job ring from the two JR's intialized
    int test_jr_id;
    sec_context_handle_t ctx_handle = NULL;

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // Test only first job ring from the two JR's intialized
    test_jr_id = 0;
    jr_handle = job_ring_descriptors[test_jr_id].job_ring_handle;

    ////////////////////////////////////
    ////////////////////////////////////

    // Create one context and affine it to one job ring.
    ret = sec_create_pdcp_context(jr_handle, &ctx_info, &ctx_handle);
    assert_equal_with_message(ret, SEC_SUCCESS,
            "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Send only one packet to SEC and retrieve it

    // How many packets to send and receive to/from SEC
    packets_handled = 1;

    // Submit one packet on the context
    send_packets(ctx_handle, packets_handled, SEC_SUCCESS);

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE;
#else
    limit = SEC_JOB_RING_SIZE  - 1;
#endif

    usleep(1000);
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Submit max number of packets = SEC_JOB_RING_HW_SIZE = 24 to SEC
    // and retrieve them.

    // How many packets to send and receive to/from SEC
#ifdef SEC_HW_VERSION_3_1
    packets_handled = SEC_JOB_RING_HW_SIZE;
#else
    packets_handled = SEC_JOB_RING_SIZE  - 1;
#endif

    // Submit packets on the context
    send_packets(ctx_handle, packets_handled, SEC_SUCCESS);

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE;
#else
    limit = SEC_JOB_RING_SIZE - 1;
#endif

    usleep(1000);
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Submit max number of packets = SEC_JOB_RING_HW_SIZE = 24 to SEC.
    // Try and submit more packets and expect job ring to be full.
    // Retrieve packets.

    // How many packets to send and receive to/from SEC
#ifdef SEC_HW_VERSION_3_1
    packets_handled = SEC_JOB_RING_HW_SIZE;
#else
    packets_handled = SEC_JOB_RING_SIZE - 1;
#endif

    // Submit packets on the context
    send_packets(ctx_handle, packets_handled, SEC_SUCCESS);

    // Submit some more packets on the context.
    // Job ring is already full, expect error.
    send_packets(ctx_handle, 5, SEC_JR_IS_FULL);

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE;
#else
    limit = SEC_JOB_RING_SIZE  - 1;
#endif

    usleep(1000);
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // How many packets to send and receive to/from SEC
    packets_handled = 5;

    // Submit some more packets on the context. Now it should work
    send_packets(ctx_handle, packets_handled, SEC_SUCCESS);

    usleep(1000);
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Test that when submited x packets, we can poll and
    // retrieve for y < x packets at a time.

    // How many packets to send and receive to/from SEC
    packets_handled = 21;

    // Submit some more packets on the context. Now it should work
    send_packets(ctx_handle, packets_handled, SEC_SUCCESS);
    usleep(1000);

    // Do not retrieve all submitted packets in the same poll step.
    limit = 10;

    // Should retrieve 10 packets
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, limit,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              limit, packets_out);

    // Should retrieve 10 packets
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, limit,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              limit, packets_out);

    // Should retrieve 1 packet
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled - 2 * limit,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled - 2 * limit, packets_out);
    // Should retrieve 0 packets
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, 0,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              0, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Reset this global counter. It is incremented each time handle_packet_from_sec() is called.
    test_packets_notified = 0;

    // Configure handle_packet_from_sec() to return SEC_RETURN_STOP after 5 packets notified.
    test_packets_notified_ret_stop = 5;

    // How many packets to send and receive to/from SEC
    packets_handled = 11;

    // Submit some more packets on the context. Now it should work
    send_packets(ctx_handle, packets_handled, SEC_SUCCESS);
    usleep(1000);

    // Do not retrieve all submitted packets in the same poll step.
    limit = 10;

    // Should retrieve 5 packets because the callback will return SEC_RETURN_STOP after 5 packets.
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, test_packets_notified_ret_stop,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              test_packets_notified_ret_stop, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Reset this global counter. It is incremented each time handle_packet_from_sec() is called.
    test_packets_notified = 0;

    // Should retrieve 5 packets because the callback will return SEC_RETURN_STOP after 5 packets.
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, test_packets_notified_ret_stop,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              test_packets_notified_ret_stop, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Reset this global counter. It is incremented each time handle_packet_from_sec() is called.
    test_packets_notified = 0;

    // Should retrieve 1 packet because only one is left.
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled - 2 * test_packets_notified_ret_stop,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled - 2 * test_packets_notified_ret_stop, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Don't return SEC_RETURN_STOP from handle_packet_from_sec() anymore.
    test_packets_notified_ret_stop = 0;

    // Should retrieve 0 packets
    ret = sec_poll_job_ring(jr_handle, limit, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll_job_ring: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, 0,
                              "ERROR on sec_poll_job_ring: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              0, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // No error from SEC, ret should be 0.
    ret = sec_get_last_error();
    assert_equal_with_message(ret, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, ret);
    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}

static void test_poll_scenarios(void)
{
    int ret = 0;
    int32_t limit = 0;
    uint32_t weight = 5;
    uint32_t packets_out = 0;
    uint32_t packets_handled = 0;
    sec_job_ring_handle_t jr_handle_0;
    sec_job_ring_handle_t jr_handle_1;
    sec_context_handle_t ctx_handle_0 = NULL;
    sec_context_handle_t ctx_handle_1 = NULL;

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

    ////////////////////////////////////
    ////////////////////////////////////

    // Create one context and affine it to first job ring.
    ret = sec_create_pdcp_context(jr_handle_0, &ctx_info, &ctx_handle_0);
    assert_equal_with_message(ret, SEC_SUCCESS,
            "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_SUCCESS, ret);

    // Create one context and affine it to second job ring.
    ret = sec_create_pdcp_context(jr_handle_1, &ctx_info, &ctx_handle_1);
    assert_equal_with_message(ret, SEC_SUCCESS,
            "ERROR on sec_create_pdcp_context: expected ret[%d]. actual ret[%d]",
            SEC_SUCCESS, ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // Send only one packet to SEC on first job ring and retrieve it

    // How many packets to send and receive to/from SEC
    packets_handled = 1;

    // Submit one packet on the context
    send_packets(ctx_handle_0, packets_handled, SEC_SUCCESS);

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE;
#else
    limit = SEC_JOB_RING_SIZE - 1;
#endif

    usleep(1000);
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Send only one packet to SEC on second job ring and retrieve it

    // How many packets to send and receive to/from SEC
    packets_handled = 1;

    // Submit one packet on the context
    send_packets(ctx_handle_1, packets_handled, SEC_SUCCESS);

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE;
#else
    limit = SEC_JOB_RING_SIZE - 1;
#endif

    usleep(1000);
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Send only one packet to SEC on first job ring and
    // one packet on second job ring. Retrieve them.

    // How many packets to send and receive to/from SEC
    packets_handled = 2;

    // Submit one packet on the context
    send_packets(ctx_handle_0, packets_handled/2, SEC_SUCCESS);

    // Submit one packet on the context
    send_packets(ctx_handle_1, packets_handled/2, SEC_SUCCESS);

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = SEC_JOB_RING_HW_SIZE;
#else
    limit = SEC_JOB_RING_SIZE - 1;
#endif

    usleep(1000);
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);
    ////////////////////////////////////
    ////////////////////////////////////

    // On both job rings, submit max number of packets = SEC_JOB_RING_HW_SIZE = 24 to
    // to SEC and retrieve them.

    // How many packets to send and receive to/from SEC
#ifdef SEC_HW_VERSION_3_1
    packets_handled = 2 * SEC_JOB_RING_HW_SIZE;
#else
    packets_handled = 2 * (SEC_JOB_RING_SIZE - 1);
#endif

    // Submit packets on first context
    send_packets(ctx_handle_0, packets_handled/2, SEC_SUCCESS);

    // Submit packets on second context
    send_packets(ctx_handle_1, packets_handled/2, SEC_SUCCESS);

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = 2 * SEC_JOB_RING_HW_SIZE;
#else
    limit = 2 * (SEC_JOB_RING_SIZE - 1);
#endif

    usleep(1000);
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Submit max number of packets = SEC_JOB_RING_HW_SIZE = 24 to SEC on each job ring.
    // Try and submit more packets and expect job ring to be full.
    // Retrieve packets.

    // How many packets to send and receive to/from SEC
#ifdef SEC_HW_VERSION_3_1
    packets_handled = 2 * SEC_JOB_RING_HW_SIZE;
#else
    packets_handled = 2 * (SEC_JOB_RING_SIZE - 1);
#endif

    // Submit packets on first context
    send_packets(ctx_handle_0, packets_handled/2, SEC_SUCCESS);

    // Submit packets on second context
    send_packets(ctx_handle_1, packets_handled/2, SEC_SUCCESS);

    // Submit some more packets on first context.
    // Job ring is already full, expect error.
    send_packets(ctx_handle_0, 5, SEC_JR_IS_FULL);

    // Submit some more packets on second context.
    // Job ring is already full, expect error.
    send_packets(ctx_handle_1, 5, SEC_JR_IS_FULL);

    // Test with no invalid params. Should work
#ifdef SEC_HW_VERSION_3_1
    limit = 2 * SEC_JOB_RING_HW_SIZE;
#else
    limit = 2 * (SEC_JOB_RING_SIZE - 1);
#endif

    usleep(1000);
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);
    ////////////////////////////////////
    ////////////////////////////////////

    // How many packets to send and receive to/from SEC
    packets_handled = 2 * 5;

    // Submit some more packets on the context. Now it should work
    send_packets(ctx_handle_0, packets_handled/2, SEC_SUCCESS);

    // Submit some more packets on the context. Now it should work
    send_packets(ctx_handle_1, packets_handled/2, SEC_SUCCESS);

    usleep(1000);
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled, packets_out);
    ////////////////////////////////////
    ////////////////////////////////////

    // Test that when submited x packets, we can poll and
    // retrieve for y < x packets at a time.

    // How many packets to send and receive to/from SEC
    packets_handled = 21 * 2;

    // Submit some more packets on the context. Now it should work
    send_packets(ctx_handle_0, packets_handled/2, SEC_SUCCESS);

    // Submit some more packets on the context. Now it should work
    send_packets(ctx_handle_1, packets_handled/2, SEC_SUCCESS);
    usleep(1000);

    // Do not retrieve all submitted packets in the same poll step.
    limit = 20;

    // Should retrieve 20 packets
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, limit,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              limit, packets_out);

    // Try a weight of 1 packet per job ring.
    weight = 1;

    // Should retrieve 20 packets
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, limit,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              limit, packets_out);

    // Should retrieve 2 packets
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled - 2 * limit,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled - 2 * limit, packets_out);
    // Should retrieve 0 packets
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, 0,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              0, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    weight = 5;

    // Reset this global counter. It is incremented each time handle_packet_from_sec() is called.
    test_packets_notified = 0;
#ifdef SEC_HW_VERSION_3_1
    // Configure handle_packet_from_sec() to return SEC_RETURN_STOP after 11 packets notified.
    test_packets_notified_ret_stop = 11;

    // How many packets to send and receive to/from SEC
    packets_handled = 2 * 12;
#else
    // Configure handle_packet_from_sec() to return SEC_RETURN_STOP after 11 packets notified.
    test_packets_notified_ret_stop = 16;

    // How many packets to send and receive to/from SEC
    packets_handled = 2 * 16;
#endif
    // Submit packets on the context.
    send_packets(ctx_handle_0, packets_handled/2, SEC_SUCCESS);

    // Submit packets on the context.
    send_packets(ctx_handle_1, packets_handled/2, SEC_SUCCESS);

    usleep(1000);

    // Do not retrieve all submitted packets in the same poll step.
    limit = 20;

    // Should retrieve 11 packets because the callback will return SEC_RETURN_STOP after 11 packets.
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, test_packets_notified_ret_stop,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              test_packets_notified_ret_stop, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Reset this global counter. It is incremented each time handle_packet_from_sec() is called.
    test_packets_notified = 0;

    // Should retrieve 11 packets because the callback will return SEC_RETURN_STOP after 11 packets.
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, test_packets_notified_ret_stop,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              test_packets_notified_ret_stop, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Reset this global counter. It is incremented each time handle_packet_from_sec() is called.
    test_packets_notified = 0;

    // Should retrieve 2 packets because only 2 are left.
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, packets_handled - 2 * test_packets_notified_ret_stop,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              packets_handled - 2 * test_packets_notified_ret_stop, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // Don't return SEC_RETURN_STOP from handle_packet_from_sec() anymore.
    test_packets_notified_ret_stop = 0;

    // Should retrieve 0 packets
    ret = sec_poll(limit, weight, &packets_out);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_poll: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // <packets_handled> packets should be retrieved from SEC
    assert_equal_with_message(packets_out, 0,
                              "ERROR on sec_poll: expected packets notified[%d]."
                              "actual packets notified[%d]",
                              0, packets_out);

    ////////////////////////////////////
    ////////////////////////////////////

    // No error from SEC, ret should be 0.
    ret = sec_get_last_error();
    assert_equal_with_message(ret, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, ret);

    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);
}

static void test_sec_get_status_message(void)
{
    sec_status_t status = SEC_STATUS_SUCCESS;

    printf("Running test %s\n", __FUNCTION__);

    assert_string_equal_with_message(sec_get_status_message(status),
            "SEC_STATUS_SUCCESS",
            "sec_get_status_message returned wrong string"
            "representation for status code %d", status);

    status = SEC_STATUS_ERROR;
    assert_string_equal_with_message(sec_get_status_message(status),
            "SEC_STATUS_ERROR",
            "sec_get_status_message returned wrong string"
            "representation for status code %d", status);

    status = SEC_STATUS_MAC_I_CHECK_FAILED;
    assert_string_equal_with_message(sec_get_status_message(status),
            "SEC_STATUS_MAC_I_CHECK_FAILED",
            "sec_get_status_message returned wrong string"
            "representation for status code %d", status);

    // Invalid value for status
    status = SEC_STATUS_MAX_VALUE;
    assert_string_equal_with_message(sec_get_status_message(status),
            "Not defined",
            "sec_get_status_message returned wrong string"
            "representation for status code %d", status);

    // Invalid value for status
    status = SEC_STATUS_MAX_VALUE + 1;
    assert_string_equal_with_message(sec_get_status_message(status),
            "Not defined",
            "sec_get_status_message returned wrong string"
            "representation for status code %d", status);
}

static void test_sec_get_error_message(void)
{
    sec_return_code_t ret = SEC_SUCCESS;

    printf("Running test %s\n", __FUNCTION__);
    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_SUCCESS",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

    ret = SEC_PACKET_PROCESSING_ERROR;
    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_PACKET_PROCESSING_ERROR",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

    ret = SEC_JOB_RING_RESET_IN_PROGRESS;
    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_JOB_RING_RESET_IN_PROGRESS",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

    ret = SEC_DRIVER_NO_FREE_CONTEXTS;
    assert_string_equal_with_message(sec_get_error_message(ret),
            "SEC_DRIVER_NO_FREE_CONTEXTS",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

    // Invalid value for return code
    ret = SEC_RETURN_CODE_MAX_VALUE;
    assert_string_equal_with_message(sec_get_error_message(ret),
            "Not defined",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);

    // Invalid value for return code
    ret = SEC_RETURN_CODE_MAX_VALUE + 1;
    assert_string_equal_with_message(sec_get_error_message(ret),
            "Not defined",
            "sec_get_error_message returned wrong string"
            "representation for ret code %d", ret);
}

static void test_sec_get_last_error(void)
{
    int ret = 0;
    int32_t last_error = 0;

    printf("Running test %s\n", __FUNCTION__);

    ////////////////////////////////////
    ////////////////////////////////////

    // SEC driver is not initalized, so errno-thread-specific variable is not initialized.
    last_error = sec_get_last_error();
    assert_equal_with_message(last_error, -1,
                             "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                             -1, last_error);

    ////////////////////////////////////
    ////////////////////////////////////

    // Init sec driver. No invalid param.
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS,
                              "ERROR on sec_init: expected ret[%d]. actual ret[%d]",
                              SEC_SUCCESS, ret);

    // No error from SEC, last_error should be 0
    last_error = sec_get_last_error();
    assert_equal_with_message(last_error, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, last_error);

    ////////////////////////////////////
    ////////////////////////////////////

    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

    ////////////////////////////////////
    ////////////////////////////////////

    // No error from SEC, last_error should be 0 even after driver shutdown.
    last_error = sec_get_last_error();
    assert_equal_with_message(last_error, 0,
                              "ERROR on sec_get_last_error: expected ret[%d]. actual ret[%d]",
                              0, last_error);
}

static TestSuite * uio_tests()
{
    /* create test suite */
    TestSuite * suite = create_test_suite();

    /* setup/teardown functions to be called before/after each unit test */
    setup(suite, test_setup);
    teardown(suite, test_teardown);

    /* start adding unit tests */
    add_test(suite, test_sec_init_invalid_params);
    add_test(suite, test_sec_create_pdcp_context_invalid_params);
    add_test(suite, test_sec_delete_pdcp_context_invalid_params);
    add_test(suite, test_sec_process_packet_invalid_params);
    add_test(suite, test_sec_poll_invalid_params);
    add_test(suite, test_sec_poll_job_ring_invalid_params);
    add_test(suite, test_poll_job_ring_scenarios);
    add_test(suite, test_poll_scenarios);
    add_test(suite, test_sec_get_status_message);
    add_test(suite, test_sec_get_error_message);
    add_test(suite, test_sec_get_last_error);

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
    TestSuite * suite = uio_tests();
    TestReporter * reporter = create_text_reporter();
#ifdef SEC_HW_VERSION_4_4
    // Init FSL USMMGR
    g_usmmgr = fsl_usmmgr_init();
    assert_not_equal_with_message(g_usmmgr, NULL, "ERROR on fsl_usmmgr_init");
#endif // SEC_HW_VERSION_4_4
    /* Run tests */

    ////////////////////////////////////
    // !!!!!  WARNING  !!!!!
    // For this test to succeed, test_sec_get_last_error() must be the very first
    // test executed as it relies on the fact that sec_init() has NEVER been called so far.
    ////////////////////////////////////
    run_single_test(suite, "test_sec_get_last_error", reporter);
    ////////////////////////////////////

    run_single_test(suite, "test_sec_init_invalid_params", reporter);
    run_single_test(suite, "test_sec_create_pdcp_context_invalid_params", reporter);
    run_single_test(suite, "test_sec_delete_pdcp_context_invalid_params", reporter);
    run_single_test(suite, "test_sec_process_packet_invalid_params", reporter);
    run_single_test(suite, "test_sec_poll_invalid_params", reporter);
    run_single_test(suite, "test_sec_poll_job_ring_invalid_params", reporter);
    run_single_test(suite, "test_poll_job_ring_scenarios", reporter);
    run_single_test(suite, "test_poll_scenarios", reporter);
    run_single_test(suite, "test_sec_get_status_message", reporter);
    run_single_test(suite, "test_sec_get_error_message", reporter);

    destroy_test_suite(suite);
    (*reporter->destroy)(reporter);

    return 0;
} /* main() */

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
