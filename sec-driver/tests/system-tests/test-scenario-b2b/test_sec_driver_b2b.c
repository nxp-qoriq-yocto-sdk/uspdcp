/* Copyright (c) 2012 Freescale Semiconductor, Inc.
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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <signal.h>

#include "fsl_sec.h"
// for mfspr related functions
#include "compat.h"
#ifdef SEC_HW_VERSION_4_4

#warning "Ugly hack to keep compat.h unchanged."
#undef __KERNEL__

// For shared memory allocator
#include "fsl_usmmgr.h"

#endif
#include "test_sec_driver_b2b.h"

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

// Number of SEC JRs used by this test application
// @note: Currently this test application supports only 2 JRs (not less, not more)
#define JOB_RING_NUMBER              2

/* Job ring to use for DL */
#define JOB_RING_ID_FOR_DL          0

/* Job ring to use for UL */
#define JOB_RING_ID_FOR_UL          1

// Number of worker threads created by this application
// One thread is producer on JR 1 and consumer on JR 2 (PDCP UL processing).
// The other thread is producer on JR 2 and consumer on JR 1 (PDCP DL processing).
#define THREADS_NUMBER               1

// Number of User Equipments simulated
#define UE_NUMBER   32
// Number of dedicated radio bearers per UE simulated
#define DRB_PER_UE  8

// The size of a PDCP input buffer.
// Consider the size of the input and output buffers provided to SEC driver for processing identical.
#define PDCP_BUFFER_SIZE   1050

// The maximum number of packets processed on DL in a second.
#define DOWNLINK_PPS        19000

// The maximum number of packets processed on UL in a second.
#define UPLINK_PPS          10000

// The maximum number of packets processed per DL context
#define PACKET_NUMBER_PER_CTX_DL    ((DOWNLINK_PPS) / (PDCP_CONTEXT_NUMBER))

// The maximum number of packets processed per UL context
#define PACKET_NUMBER_PER_CTX_UL    ((UPLINK_PPS) / (PDCP_CONTEXT_NUMBER))

// The number of packets to send in a burst for each context
#define PACKET_BURST_PER_CTX   1

#define MAX_PDCP_HEADER_LEN     2

#ifdef SEC_HW_VERSION_4_4

#define IRQ_COALESCING_COUNT    10
#define IRQ_COALESCING_TIMER    100

#endif

#define JOB_RING_POLL_UNLIMITED -1
#define JOB_RING_POLL_LIMIT      5

// Alignment in bytes for input/output packets allocated from DMA-memory zone
#define BUFFER_ALIGNEMENT 32

/* Maximium length, in bytes, for a confidentiality /integrity key */
#define MAX_KEY_LENGTH    16

/* Value used for conversion seconds->microseconds */
#define MEGA              1000000

// Read ATBL(Alternate Time Base Lower) Register
#define GET_ATBL() \
    mfspr(SPR_ATBL)

// Must be at least 8, for statistics reasons
#define TEST_OFFSET 8

/** Size in bytes of a cacheline. */
#define CACHE_LINE_SIZE  32

/** Length of the MAC_I */
#define ICV_LEN 4

/** Parameters for parsing user options */
#define TYPE_SET      0x01
#define INT_ALG_SET   0x02
#define ENC_ALG_SET   0x04
#define DIR_SET       0x08
#define HDR_LEN_SET   0x10
#define MAX_FRAGS_SET 0x20
#define PAYLOAD_SET   0x40
#define NUM_ITER_SET  0x80

#define CPLANE_VALID_MASK       (TYPE_SET |           \
                                INT_ALG_SET |         \
                                ENC_ALG_SET |         \
                                DIR_SET |             \
                                MAX_FRAGS_SET |       \
                                PAYLOAD_SET |         \
                                NUM_ITER_SET)

#define UPLANE_VALID_MASK       (TYPE_SET |           \
                                ENC_ALG_SET |         \
                                DIR_SET |             \
                                HDR_LEN_SET |         \
                                MAX_FRAGS_SET |       \
                                PAYLOAD_SET |         \
                                NUM_ITER_SET)

// For keeping the code relatively the same between HW versions
#define dma_mem_memalign    test_memalign
#define dma_mem_free        test_free

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

typedef enum pdcp_context_usage_e
{
    PDCP_CONTEXT_FREE = 0,
    PDCP_CONTEXT_USED,
    PDCP_CONTEXT_MARKED_FOR_DELETION,
    PDCP_CONTEXT_MARKED_FOR_DELETION_LAST_IN_FLIGHT_PACKET,
    PDCP_CONTEXT_CAN_BE_DELETED
}pdcp_context_usage_t;

typedef enum packet_usage_e
{
    PACKET_FREE = 0,
    PACKET_USED
}packet_usage_t;

/* Forward declaration */
struct pdcp_context_s;

typedef struct test_packet_s
{
    int id;                     /**< Buffer index to be used when put'ing input buffers */
    packet_usage_t usage;       /**< Resource usage: free or in-use */
    struct pdcp_context_s *ctx; /**< Context to which this packet belongs */
    sec_packet_t *in_packet;    /**< Input packet array */
    sec_packet_t *out_packet;   /**< Output packet array */
}test_packet_t;

typedef struct buffer_s
{
    uint8_t buffer[PDCP_BUFFER_SIZE];
}buffer_t;

typedef struct pdcp_context_s
{
    // the status of this context: free or used
    pdcp_context_usage_t usage;
    // handle to the SEC context (from SEC driver) associated to this PDCP context
    sec_context_handle_t sec_ctx;
    // configuration data for this context
    sec_pdcp_context_info_t pdcp_ctx_cfg_data;
    // the handle to the affined job ring
    sec_job_ring_handle_t job_ring;
      // unique context id, needed just for tracking purposes in this test application
    int id;
    /* Pool of input and output packets used for processing the
     * packets associated to this context. (used an array for simplicity)
     */
    test_packet_t *test_packets;
    buffer_t *input_buffers;
    buffer_t *output_buffers;
    int no_of_used_buffers; // index incremented by Producer Thread
    int no_of_buffers_processed; // index increment by Consumer Thread
    int no_of_buffers_to_process; // configurable random number of packets to be processed per context
}pdcp_context_t;

typedef struct thread_config_s
{
    // id of the thread
    int tid;
    /* No. of used DL contexts from the pool */
    int * no_of_used_pdcp_dl_contexts;
    /* No. of used UL contexts from the pool */
    int * no_of_used_pdcp_ul_contexts;
    // default to zero. set to 1 by worker thread to notify
    // main thread that it completed its work
    volatile int work_done;
    // default to zero. set to 1 when main thread instructs the worked thread to exit.
    volatile int should_exit;
    uint32_t process_cycles;
    uint32_t poll_cycles;
}thread_config_t;
/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
// configuration data for SEC driver
static sec_config_t sec_config_data;

// job ring handles provided by SEC driver
static const sec_job_ring_descriptor_t *job_ring_descriptors = NULL;

// UA pool of PDCP contexts for UL and DL.
static pdcp_context_t pdcp_ul_contexts[PDCP_CONTEXT_NUMBER];
static int no_of_used_pdcp_ul_contexts = 0;

static pdcp_context_t pdcp_dl_contexts[PDCP_CONTEXT_NUMBER];
static int no_of_used_pdcp_dl_contexts = 0;

/* One thread manages two JR: one for DL, one for UL */
static thread_config_t th_config[THREADS_NUMBER];
static pthread_t threads[THREADS_NUMBER];

// FSL Userspace Memory Manager structure
fsl_usmmgr_t g_usmmgr;

/* Parameters given by the user and copied when parsing the cmd. line */
static users_params_t user_param;
/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/** @brief Initialize SEC driver, initialize internal data of test application. */
static int setup_sec_environment();

/** @brief Release SEC driver, clear internal data of test application. */
static int cleanup_sec_environment();

/** @brief Start the worker threads that will create/delete SEC contexts at
 * runtime, send packets for processing to SEC HW and poll for results. */
static int start_sec_worker_threads(void);

/** @brief Stop the worker threads. */
static int stop_sec_worker_threads(void);

/** @brief The worker thread function. Both worker threads execute the same function.
 * Each thread is a producer on the JR on which the other thread is a consumer.
 * Each thread does the following:
 * - for (a configured random number of SEC contexts)
 *    -> create the context and associate it with the producer JR
 *    -> send a random number of packets for processing
 *    -> if producer JR is full poll the consumer JR until an entry in
 *       the producer JR is freed.
 *    -> if context number is even, try to delete it right away
 *       (this tests the scenario when a context is deleted while having packets in flight)
 * - while (all contexts previously created are deleted)
 *    -> poll the consumer JR
 *    -> try to delete the contexts (the retiring contexts and those contexts
 *      not marked for deletion in the first step)
 * - notify main thread that work is done
 * - while (exit signal is received from UA)
 *    -> poll the consumer JR
 *
 * @see the description of done_cbk() for details on what the worker
 * thread does in the callback.
 *
 * The main thread waits for both worker threads to finish their work (meaning creating
 * the configured number of contexts, sending the packets, waiting for all the responses
 * and deleting all contexts). When both worker threads finished their work, the main
 * thread instructs them to exit.
 *  */
static void* pdcp_thread_routine(void*);


/** @brief Poll a specified SEC job ring for results */
static int get_results(uint8_t job_ring, int limit, uint32_t *out_packets, uint32_t *core_cycles, uint8_t tid);

/** @brief Get a free PDCP context from the pool of UA contexts.
 *  For simplicity, the pool is implemented as an array. */
static int get_pdcp_context(pdcp_context_t * pdcp_contexts,
                                 int * no_of_used_pdcp_contexts,
                                 pdcp_context_t ** pdcp_context);

static int put_pdcp_context(pdcp_context_t * pdcp_context,
                            int * no_of_used_pdcp_contexts);

/** @brief Get free PDCP input and output buffers from the pool of buffers
 *  of a certain context.
 *  For simplicity, the pool is implemented as an array and is defined per context. */
static int get_test_packet(pdcp_context_t * pdcp_context,
                           test_packet_t **test_packet);

static int put_test_packet(test_packet_t *test_packet);

/** @brief Callback called by SEC driver for each decapsulated packet.
 *
 * In the callback called by SEC driver for each decapsulated packet processed, 
 * the worker thread will do the following:
 * - validate the order of the notifications received per context (should be the same
 *   with the one in which the packets were submitted for processing)
 * - free the buffers (input and output)
 * - validate the status received from SEC driver
 *      -> if context is marked as retiring and the status #SEC_STATUS_LAST_OVERDUE is received
 *      for a packet, the context is marked as "can be deleted"
 *      @note: In this test application the PDCP contexts (both UA's and SEC driver's)
 *      are not deleted in the consumer thread (the one that calls sec_poll()).
 *      They are deleted in the producer thread.
 *
 * ua_ctx_handle -> is the address of the UA PDCP context associated to the response notified.
 */
static int dec_packet_handler (const sec_packet_t *in_packet,
                                      const sec_packet_t *out_packet,
                                      ua_context_handle_t ua_ctx_handle,
                                      uint32_t status,
                                      uint32_t error_info);

/** @brief Callback called by SEC driver for each encapsulated packet.
 *
 * In the callback called by SEC driver for each encapsulated packet processed, 
 * the worker thread will do the following:
 * - validate the order of the notifications received per context (should be the same
 *   with the one in which the packets were submitted for processing)
 * - free the buffers (input and output)
 * - validate the status received from SEC driver
 *      -> if context is marked as retiring and the status #SEC_STATUS_LAST_OVERDUE is received
 *      for a packet, the context is marked as "can be deleted"
 *      @note: In this test application the PDCP contexts (both UA's and SEC driver's)
 *      are not deleted in the consumer thread (the one that calls sec_poll()).
 *      They are deleted in the producer thread.
 *
 * ua_ctx_handle -> is the address of the UA PDCP context associated to the response notified.
 */
static int enc_packet_handler (const sec_packet_t *in_packet,
                                      const sec_packet_t *out_packet,
                                      ua_context_handle_t ua_ctx_handle,
                                      uint32_t status,
                                      uint32_t error_info);

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
                                        LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
static uint32_t test_cipher_algorithm = -1;

static uint32_t test_integrity_algorithm = -1;

static uint32_t test_user_plane = -1;

static uint32_t test_sn_size = -1;

static uint32_t test_direction = -1;

static uint32_t test_pdcp_hdr_len = -1;

static uint8_t test_crypto_key[MAX_KEY_LENGTH];

static uint8_t test_auth_key[MAX_KEY_LENGTH];

/* Plaintext random data to be used for DL direction. */
static uint8_t test_data_encap[PDCP_BUFFER_SIZE];

static uint8_t test_bearer;

static uint32_t test_hfn;

static uint32_t test_hfn_threshold;

static uint32_t test_payload_size_encap = -1;

static uint32_t test_payload_size_decap = -1;

static uint32_t test_num_frags = -1;

static uint32_t test_num_iter;

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static void * test_memalign(size_t align, size_t size)
{
    int ret;
    range_t r = {0,0,size};

    ret = fsl_usmmgr_memalign(&r,align,g_usmmgr);
    if(ret != 0){
        printf("FSL USMMGR memalign failed: %d",ret);
        return NULL;
    }

    return r.vaddr;
}

static void test_free(void *ptr, size_t size)
{
   range_t r;

   r.vaddr = ptr;
   // fprintf(stderr,"FREE:\n0x%08x, 0x%08x, %d\n",r.vaddr, test_vtop(r.vaddr), size);
   fsl_usmmgr_free(&r,g_usmmgr);
}

static void populate_pdcp_context(pdcp_context_t *pdcp_context,
                                  sec_out_cbk callback,
                                  uint32_t proto_dir)
{
    test_packet_t *pkt;
    sec_packet_t *in_packet;
    sec_packet_t *out_packet;
    uint32_t fragment_length;
    uint32_t address;
    uint32_t rem_len;
    int pkt_idx;

    assert(pdcp_context != NULL);
    assert(callback != NULL);
    assert(proto_dir == PDCP_ENCAPSULATION ||
           proto_dir == PDCP_DECAPSULATION);

    /* Set callback as per caller preference */
    pdcp_context->pdcp_ctx_cfg_data.notify_packet = callback;
    
    /* Set protcol direction (ENCAP or DECAP) as per caller preferences */
    pdcp_context->pdcp_ctx_cfg_data.protocol_direction = proto_dir;

    pdcp_context->pdcp_ctx_cfg_data.bearer = test_bearer;
    pdcp_context->pdcp_ctx_cfg_data.packet_direction = test_direction;

    pdcp_context->pdcp_ctx_cfg_data.hfn = test_hfn;
    pdcp_context->pdcp_ctx_cfg_data.hfn_threshold = test_hfn_threshold;

    /* Enable HFN value override */
    pdcp_context->pdcp_ctx_cfg_data.hfn_ov_en = 1;

    pdcp_context->pdcp_ctx_cfg_data.user_plane = test_user_plane;
    /* Configure confidentiality algorithm. This is valid for both Control
     * and Data Plane
     */
    pdcp_context->pdcp_ctx_cfg_data.cipher_algorithm = test_cipher_algorithm;
    pdcp_context->pdcp_ctx_cfg_data.cipher_key_len = MAX_KEY_LENGTH;
    memcpy(pdcp_context->pdcp_ctx_cfg_data.cipher_key,
           test_crypto_key,
           MAX_KEY_LENGTH);
    
    if (test_user_plane == PDCP_CONTROL_PLANE)
    {
        /* Configure integrity algorithm */
        pdcp_context->pdcp_ctx_cfg_data.integrity_algorithm = 
            test_integrity_algorithm;
        pdcp_context->pdcp_ctx_cfg_data.integrity_key_len = MAX_KEY_LENGTH;
        memcpy(pdcp_context->pdcp_ctx_cfg_data.integrity_key,
               test_auth_key,
               MAX_KEY_LENGTH);
        /* Set SN size to the only possible value */
        pdcp_context->pdcp_ctx_cfg_data.sn_size = SEC_PDCP_SN_SIZE_5;
    }
    else
    {
        /* Configure SN size */
        pdcp_context->pdcp_ctx_cfg_data.sn_size = test_sn_size;
    }
    
    /* Populate input data */
    for(pkt_idx = 0; pkt_idx < pdcp_context->no_of_buffers_to_process; pkt_idx++)
    {
        pkt = &pdcp_context->test_packets[pkt_idx];
        
        /* Set context information for packet */
        pkt->id = pkt_idx;
        pkt->usage = PACKET_FREE;
        pkt->ctx = pdcp_context;

        in_packet = pkt->in_packet;
        out_packet = pkt->out_packet;

        if (proto_dir == PDCP_ENCAPSULATION)
        {
            /* Populate the input buffer */
            memcpy(pdcp_context->input_buffers[pkt_idx].buffer,
                   test_data_encap,
                   test_payload_size_encap);

            fragment_length = test_payload_size_encap / test_num_frags +
                              (test_payload_size_encap % test_num_frags ? 1 : 0);

            /* First fragment is special */
            in_packet->address = test_vtop(pdcp_context->input_buffers[pkt_idx].buffer);
            in_packet->length = fragment_length;
            in_packet->total_length = test_payload_size_encap;
            in_packet->num_fragments = test_num_frags  - 1; /* Exclude first */

            /* Store needed values for populating the next fragments */
            address = in_packet->address + in_packet->length;
            rem_len = in_packet->total_length - in_packet->length;
            
            while(rem_len != 0)
            {
                /* Next packet */
                in_packet++;

                /* Set address to last address + last length */
                in_packet->address = address;
                in_packet->offset = 0;

                if (rem_len >= fragment_length)
                {
                    in_packet->length = fragment_length;
                }
                else
                {
                    in_packet->length = rem_len;
                }

                address += in_packet->length;
                rem_len -= in_packet->length;
            }
            
            /* Set offset for first fragment */
            /* Revert to the beginning of the array */
            in_packet = pkt->in_packet;
            in_packet->offset = TEST_OFFSET;
            in_packet->address -= TEST_OFFSET;
            
            /* Populate the output packet */
            out_packet->address = test_vtop(pdcp_context->output_buffers[pkt_idx].buffer) - TEST_OFFSET;
            out_packet->offset = TEST_OFFSET;
            out_packet->num_fragments = 0;

            out_packet->total_length = out_packet->length =
                test_payload_size_decap;
        }
        else
        {
            /* Populate the input buffer */
            memset(pdcp_context->input_buffers[pkt_idx].buffer,
                   0x00,
                   test_payload_size_decap);
            
            out_packet->total_length = test_payload_size_encap;

            fragment_length = test_payload_size_encap / test_num_frags +
                              (test_payload_size_encap % test_num_frags ? 1 : 0);
            
            /* First fragment is special */
            out_packet->address = test_vtop(pdcp_context->output_buffers[pkt_idx].buffer);
            out_packet->length = fragment_length;
            out_packet->num_fragments = test_num_frags - 1;

            /* Store needed values for populating the next fragments */
            address = out_packet->address + out_packet->length;
            rem_len = out_packet->total_length - out_packet->length;
            
            while(rem_len != 0)
            {
                /* Next packet */
                out_packet++;

                /* Set address to last address + last length */
                out_packet->address = address;
                out_packet->offset = 0;

                if (rem_len >= fragment_length)
                {
                    out_packet->length = fragment_length;
                }
                else
                {
                    out_packet->length = rem_len;
                }

                address += out_packet->length;
                rem_len -= out_packet->length;
            }
            
            /* Set offset for first fragment */
            /* Revert to the beginning of the array */
            out_packet = pkt->out_packet;
            out_packet->offset = TEST_OFFSET;
            out_packet->address -= TEST_OFFSET;
            
            /* Populate the input packet */
            in_packet->address = test_vtop(pdcp_context->input_buffers[pkt_idx].buffer) - TEST_OFFSET;
            in_packet->offset = TEST_OFFSET;
            in_packet->num_fragments = 0;
            in_packet->length = in_packet->total_length =
                test_payload_size_decap;
        }
    }
}

static void generate_test_vectors()
{
    int i = 0;

    for(i = 0; i < test_payload_size_encap; i++)
    {
        test_data_encap[i] = rand() & 0xFF;
    }

    for( i = 0; i < MAX_KEY_LENGTH; i++ )
    {
        test_crypto_key[i] = rand() & 0xFF;
        test_auth_key[i] = rand() & 0xFF;
    }

    test_bearer = rand() &0x1F;

    if(test_user_plane == PDCP_CONTROL_PLANE)
    {
        test_hfn = rand() & 0x07FFFFFF;
    }
    else
    {
        /* Data Plane */
        if(test_pdcp_hdr_len == 1)
        {
            /* Short SN */
            test_hfn = rand() & 0x01FFFFFF;
        }
        else
        {
            /* Long SN */
            test_hfn = rand() & 0x000FFFFF;
        }
    }

    test_hfn_threshold = test_hfn + 1; /* Threshold reach indication will be
                                          ignored on poll */

}

static void populate_pdcp_contexts()
{
    int i;
    
    /* First, populate DL contexts. */
    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        populate_pdcp_context(&pdcp_dl_contexts[i],
                              enc_packet_handler,
                              PDCP_ENCAPSULATION);
    }
    
    /* Second, populate UL contexts. */
    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        populate_pdcp_context(&pdcp_ul_contexts[i],
                              dec_packet_handler,
                              PDCP_DECAPSULATION);
    }
}

static int get_pdcp_context(pdcp_context_t * pdcp_contexts_pool,
                            int * used_contexts,
                            pdcp_context_t ** pdcp_context)
{
    int i = 0;

    assert(pdcp_context != NULL);
    assert(pdcp_contexts_pool != NULL);
    assert(used_contexts != NULL);

    // check if there are free contexts
    if (*used_contexts >= PDCP_CONTEXT_NUMBER)
    {
        // no free contexts available
        return 2;
    }

    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        if (pdcp_contexts_pool[i].usage == PDCP_CONTEXT_FREE)
        {
            pdcp_contexts_pool[i].usage = PDCP_CONTEXT_USED;

            (*used_contexts)++;

            // return the context chosen
            *pdcp_context = &(pdcp_contexts_pool[i]);
            
            return 0;
        }
    }
    
    /* No context available in the pool */
    return 1;
}

static int put_pdcp_context(pdcp_context_t *pdcp_context,
                            int * used_contexts)
{
    assert(pdcp_context != NULL);
    assert(used_contexts != NULL);
    
    pdcp_context->usage = PDCP_CONTEXT_FREE;
    
    (*used_contexts)--;
    
    return 0;
}

static int get_test_packet(pdcp_context_t *pdcp_context,
                           test_packet_t **test_packet)
{
    int i;
    assert(pdcp_context != NULL);
    assert(test_packet != NULL);

    if (pdcp_context->no_of_used_buffers >= pdcp_context->no_of_buffers_to_process)
    {
        /* No free packet available */
        return 1;
    }

    for(i = 0; i < pdcp_context->no_of_buffers_to_process; i++)
    {
        if(pdcp_context->test_packets[i].usage == PACKET_FREE)
        {
            *test_packet = &pdcp_context->test_packets[i];
            (*test_packet)->usage = PACKET_USED;
            
            pdcp_context->no_of_used_buffers++;

            return 0;
        }
    }

    return 1;
}

static int put_test_packet(test_packet_t *test_packet)
{
    assert(test_packet != NULL);
    assert(test_packet->ctx->test_packets[test_packet->id].usage == PACKET_USED);
    
    test_packet->ctx->test_packets[test_packet->id].usage = PACKET_FREE;

    test_packet->ctx->no_of_used_buffers--;

    return 0;
}

static int dec_packet_handler(const sec_packet_t *in_packet,
                              const sec_packet_t *out_packet,
                              ua_context_handle_t ua_ctx_handle,
                              uint32_t status,
                              uint32_t error_info)
{
    test_packet_t* test_packet = (test_packet_t*)ua_ctx_handle;
    uint8_t *tmp_buf;
    int ret;

    tmp_buf = test_ptov(out_packet->address + out_packet->offset);
    
    /* Check that the decap'ed packet matches the inserted plaintext */
    ret = memcmp(tmp_buf,
                 test_data_encap,
                 test_payload_size_encap);
    test_printf("Packets %s",ret ? "MISMATCH" : "MATCH");

    assert(ret == 0);

    assert( put_test_packet(test_packet) == 0);

    return SEC_RETURN_SUCCESS;
}

static int enc_packet_handler (const sec_packet_t *in_packet,
                               const sec_packet_t *out_packet,
                               ua_context_handle_t ua_ctx_handle,
                               uint32_t status,
                               uint32_t error_info)
{
    test_packet_t* test_packet = (test_packet_t*)ua_ctx_handle;
    test_packet_t *decap_packet;
    uint8_t *enc_buf, *dec_buf;
    pdcp_context_t *ul_ctx;
    int ret_code;
    

    /* 
     * Get the context ID. Will use the same ID for the 'mirror' UL context
     * which performs the decap.
     */
    ul_ctx = &pdcp_ul_contexts[test_packet->ctx->id];
    if(get_test_packet(ul_ctx, &decap_packet) != 0)
    {
        return SEC_RETURN_STOP;
    }
    

    enc_buf = test_ptov(out_packet->address + out_packet->offset);
    dec_buf = test_ptov(decap_packet->in_packet->address + decap_packet->in_packet->offset);
    
    memcpy(dec_buf, enc_buf, out_packet->total_length);


    ret_code = sec_process_packet_hfn_ov(ul_ctx->sec_ctx,
                                         decap_packet->in_packet,
                                         decap_packet->out_packet,
                                         test_hfn,
                                         (ua_context_handle_t)decap_packet);
    /* Free the used packet */
    assert( put_test_packet(test_packet) == 0);

    if( SEC_SUCCESS != ret_code )
    {
        test_printf("Error on inserting packet to be decrypted");
        return SEC_RETURN_STOP;
    }

    return SEC_RETURN_SUCCESS;
}

static int get_results(uint8_t job_ring, int limit, uint32_t *packets_out, uint32_t *core_cycles, uint8_t tid)
{
    uint32_t start_cycles = GET_ATBL();
    uint32_t diff_cycles = 0;
    int ret_code = 0;

    assert(limit != 0);
    assert(packets_out != NULL);

    ret_code = sec_poll_job_ring(job_ring_descriptors[job_ring].job_ring_handle, limit, packets_out);
    diff_cycles = (GET_ATBL() - start_cycles);
    *core_cycles += diff_cycles;
    profile_printf("thread #%d:sec_poll_job_ring cycles = %d. pkts = %d\n", tid, diff_cycles, *packets_out);

    if (ret_code != SEC_SUCCESS)
    {
        test_printf("sec_poll_job_ring::Error %d when polling for SEC results on Job Ring %d \n", ret_code, job_ring);
        return 1;
    }
    // validate that number of packets notified does not exceed limit when limit is > 0.
    assert(!((limit > 0) && (*packets_out > limit)));

    return 0;
}

static int start_sec_worker_threads(void)
{
    int ret_code = 0;

    // this scenario handles only 2 JRs and 2 worker threads
    assert (JOB_RING_NUMBER == 2);
    assert (THREADS_NUMBER == 1);

    /* Configure PDCP thread  for both UL and DL */
    th_config[0].tid = 0;

    th_config[0].no_of_used_pdcp_ul_contexts = &no_of_used_pdcp_ul_contexts;
    th_config[0].no_of_used_pdcp_dl_contexts = &no_of_used_pdcp_dl_contexts;
    th_config[0].work_done = 0;
    th_config[0].should_exit = 0;
    th_config[0].process_cycles = 0;
    th_config[0].poll_cycles = 0;
    ret_code = pthread_create(&threads[0], NULL, &pdcp_thread_routine, (void*)&th_config[0]);

    return ret_code;
}

static int stop_sec_worker_threads(void)
{
    int i = 0;
    int ret_code = 0;

    // this scenario handles only 2 JRs and 2 worker threads
    assert (JOB_RING_NUMBER == 2);
    assert (THREADS_NUMBER == 1);

    // wait until both worker threads finish their work
    while (!(th_config[0].work_done == 1))
    {
        sleep(1);
    }
    // tell the working threads that they can stop polling now and exit
    th_config[0].should_exit = 1;

    // wait for the threads to exit
    for (i = 0; i < THREADS_NUMBER; i++)
    {
        ret_code = pthread_join(threads[i], NULL);
        if( ret_code != 0)
            return ret_code;
    }

    test_printf("thread main: all worker threads are stopped\n");

    return ret_code;
}

static void pdcp_thread_ctx_send_packets(int tid,
                                         int jr_id,
                                         pdcp_context_t *pdcp_context,
                                         uint32_t *process_cycles,
                                         uint32_t *poll_cycles,
                                         uint32_t *packets_received,
                                         uint32_t *packets_sent)
{
    int ret_code = 0;
    unsigned int ctx_packet_count = 0;
    uint32_t start_cycles = 0;
    uint32_t diff_cycles = 0;

    while(ctx_packet_count < PACKET_BURST_PER_CTX)
    {
        test_packet_t *test_packet;

        /* Get a free packet, If none, then break the loop */
        if(get_test_packet(pdcp_context, &test_packet) != 0)
        {
            break;
        }

        /* If sec_process_packet returns JR_FULL, do some polling
         * on the consumer JR until the producer JR has free entries 
         */
        do{
            start_cycles = GET_ATBL();
            ret_code = sec_process_packet_hfn_ov(pdcp_context->sec_ctx,
                                                 test_packet->in_packet,
                                                 test_packet->out_packet,
                                                 test_hfn,
                                                 (ua_context_handle_t)test_packet);

            diff_cycles = (GET_ATBL() - start_cycles);
            *process_cycles += diff_cycles;

            profile_printf("ctx #%p:sec_process_packet cycles = %d\n",
                    pdcp_context, diff_cycles);

            if(ret_code == SEC_JR_IS_FULL)
            {
                /* Wait while the producer JR is empty, and in the mean time do some
                 * polling on the consumer JR -> retrieve all available notifications.
                 */
                ret_code = get_results(jr_id,
                                       JOB_RING_POLL_UNLIMITED,
                                       packets_received,
                                       poll_cycles,
                                       tid);
                assert(ret_code == 0);
            }
            else if(ret_code != SEC_SUCCESS)
            {
                test_printf("thread #%d:producer: sec_process_packet return error %d for PDCP context no %d \n",
                        tid, ret_code, pdcp_context->id);
                assert(0);
            }
            else
            {
                break;
            }
        }while(1);

        ctx_packet_count++;
    }
    *packets_sent = ctx_packet_count;
}

static void* pdcp_thread_routine(void* config)
{
    thread_config_t *th_config_local = NULL;
    int ret_code = 0;
    int i = 0;
    int iter;
    unsigned int packets_received = 0;
    pdcp_context_t *pdcp_context;
    unsigned int total_packets_to_send = 0;
    unsigned int total_packets_sent = 0;
    unsigned int total_packets_to_receive = 0;
    unsigned int total_packets_received = 0;

    unsigned int total_dl_packets_sent = 0;
    unsigned int total_dl_packets_to_send = 0;
    
    unsigned int total_ul_packets_sent = 0;
    unsigned int total_ul_packets_to_send = 0;

    unsigned int total_dl_packets_received = 0;
    unsigned int total_ul_packets_received = 0;
    
    unsigned int ctx_packets_received = 0;
    unsigned int ctx_packets_sent = 0;
    
    struct timeval start_time;
    struct timeval end_time;

    uint32_t send_bps_ul, receive_bps_ul, send_pps_ul, receive_pps_ul;
    uint32_t send_bps_dl, receive_bps_dl, send_pps_dl, receive_pps_dl;

    uint32_t time_us;
    
    th_config_local = (thread_config_t*)config;
    assert(th_config_local != NULL);

    test_printf("thread #%d:producer: start work, no of contexts to be created/deleted %d\n",
            th_config_local->tid, th_config_local->no_of_pdcp_contexts_to_test);

    /////////////////////////////////////////////////////////////////////
    // 1. Create a number of PDCP contexts
    /////////////////////////////////////////////////////////////////////

    /* Create a number of configurable contexts and affine them to the producer JR */
    for(i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        /* Create DL context */
        ret_code = get_pdcp_context(pdcp_dl_contexts,
                                    th_config_local->no_of_used_pdcp_dl_contexts,
                                    &pdcp_context);
        assert(ret_code == 0);

        test_printf("thread #%d:producer: create pdcp context %d\n", th_config_local->tid, pdcp_context->id);

        /* Affine the context to the proper JR */
        ret_code = sec_create_pdcp_context(job_ring_descriptors[JOB_RING_ID_FOR_DL].job_ring_handle,
                                           &pdcp_context->pdcp_ctx_cfg_data,
                                           &pdcp_context->sec_ctx);
        assert(ret_code == 0);

        /* Create UL context */
        ret_code = get_pdcp_context(pdcp_ul_contexts,
                                    th_config_local->no_of_used_pdcp_ul_contexts,
                                    &pdcp_context);
        assert(ret_code == 0);

        test_printf("thread #%d:producer: create pdcp context %d\n", th_config_local->tid, pdcp_context->id);

        /* Affine the context to the proper JR */
        ret_code = sec_create_pdcp_context(job_ring_descriptors[JOB_RING_ID_FOR_UL].job_ring_handle,
                                           &pdcp_context->pdcp_ctx_cfg_data,
                                           &pdcp_context->sec_ctx);
        if (ret_code != SEC_SUCCESS)
        {
            test_printf("thread #%d:producer: sec_create_pdcp_context return error %d for PDCP context no %d \n",
                    th_config_local->tid, ret_code, pdcp_context->id);
            assert(0);
        }
    }

    /////////////////////////////////////////////////////////////////////
    // 2. Send a number of packets on each of the PDCP contexts
    /////////////////////////////////////////////////////////////////////
    
    total_dl_packets_to_send = total_ul_packets_to_send = 
        PDCP_CONTEXT_NUMBER * PACKET_NUMBER_PER_CTX_DL;
    
    total_packets_to_send = total_dl_packets_to_send + total_ul_packets_to_send;
    total_packets_to_receive = 2* PDCP_CONTEXT_NUMBER * PACKET_NUMBER_PER_CTX_DL;

    for(iter = 0; test_num_iter == 0 ? 1 : iter < test_num_iter; iter++)
    {
        total_dl_packets_received = 0;
        total_dl_packets_sent = 0;

        total_ul_packets_received = 0;
        total_ul_packets_sent = 0;

        total_packets_sent = 0;

        th_config_local->poll_cycles = 0;
        th_config_local->process_cycles = 0;

        gettimeofday(&start_time, NULL);
        
        /* Send packets on each context, until all the packets are sent to SEC */
        while(total_packets_to_send != total_packets_sent)
        {
            for(i = 0; i < PDCP_CONTEXT_NUMBER; i++)
            {
                while(total_dl_packets_sent != total_dl_packets_to_send)
                {
                
                    ctx_packets_received = 0;
                    ctx_packets_sent = 0;
                    
                    /* Send some packets on DL context */
                    pdcp_thread_ctx_send_packets(th_config_local->tid,
                                                 JOB_RING_ID_FOR_DL,
                                                 &pdcp_dl_contexts[i],
                                                 &th_config_local->process_cycles,
                                                 &th_config_local->poll_cycles,
                                                 &ctx_packets_received,
                                                 &ctx_packets_sent);

                    total_dl_packets_received += ctx_packets_received;
                    total_ul_packets_sent += ctx_packets_sent;
                    total_dl_packets_sent += ctx_packets_sent;
                    
                    break;
                }
                
                /* Get processed packets */
                ret_code = get_results(JOB_RING_ID_FOR_UL,
                                       JOB_RING_POLL_UNLIMITED,
                                       &ctx_packets_received,
                                       &th_config_local->poll_cycles,
                                       th_config_local->tid);
                assert(ret_code == 0);
                
                total_ul_packets_received += ctx_packets_received;
                
                total_packets_received = total_dl_packets_received + 
                                         total_ul_packets_received;
                    
                total_packets_sent = total_dl_packets_sent +
                                     total_ul_packets_sent;
            }
        }
        
        test_printf("thread #%d: polling until all contexts are deleted\n", th_config_local->tid);

        /* Poll the consumer JR until no more packets are retrieved. */
        do
        {
            packets_received = 0;

            /* Poll the DL JR */
            ret_code = get_results(JOB_RING_ID_FOR_DL,
                                   JOB_RING_POLL_UNLIMITED,
                                   &packets_received,
                                   &th_config_local->poll_cycles,
                                   th_config_local->tid);
            assert(ret_code == 0);
            total_dl_packets_received += packets_received;

            packets_received = 0;

            /* Poll the UL JR */
            ret_code = get_results(JOB_RING_ID_FOR_UL,
                                   JOB_RING_POLL_UNLIMITED,
                                   &packets_received,
                                   &th_config_local->poll_cycles,
                                   th_config_local->tid);
            assert(ret_code == 0);
            total_ul_packets_received += packets_received;

            total_packets_received = total_dl_packets_received + 
                                     total_ul_packets_received;
        }while(total_packets_to_receive != total_packets_received);

        gettimeofday(&end_time, NULL);

        time_us = (end_time.tv_sec - start_time.tv_sec)*MEGA + 
                  (end_time.tv_usec - start_time.tv_usec);

        receive_pps_dl = MEGA / time_us * total_dl_packets_received ;
        send_pps_dl = MEGA / time_us * total_dl_packets_sent;
        receive_bps_dl = receive_pps_dl * (test_payload_size_decap - test_pdcp_hdr_len) * 8;
        send_bps_dl = send_pps_dl * (test_payload_size_encap - test_pdcp_hdr_len) * 8;

        receive_pps_ul = MEGA / time_us * total_ul_packets_received ;
        send_pps_ul = MEGA / time_us * total_ul_packets_sent;
        receive_bps_ul = receive_pps_ul * (test_payload_size_encap - test_pdcp_hdr_len) * 8;
        send_bps_ul = send_pps_ul * (test_payload_size_decap - test_pdcp_hdr_len) * 8;

        printf("Iteration %d:\n"
               "Sent %d DL packets. Sent %d UL packets.\n"
               "Received %d DL packets. Received %d UL packets.\n"
               "Start Time %d sec %d usec. End Time %d sec %d usec.\n"
               "Receive PPS DL: %u. Receive PPS UL: %u.\n"
               "Send PPS DL: %u. Send PPS UL: %u.\n"
               "Receive BPS DL: %u. Receive BPS UL: %u.\n"
               "Send BPS DL: %u. Send BPS UL: %u.\n",
               iter+1,
               total_dl_packets_sent,
               total_ul_packets_sent,
               total_dl_packets_received,
               total_ul_packets_received,
               (int)start_time.tv_sec,
               (int)start_time.tv_usec,
               (int)end_time.tv_sec,
               (int)end_time.tv_usec,
               receive_pps_dl, receive_pps_ul,
               send_pps_dl, send_pps_ul,
               receive_bps_dl, receive_bps_ul,
               send_bps_dl, send_bps_ul);


        /* Check if the user requested to end test */
        if(th_config_local->should_exit)
            break;

    }

    /* Cleanup */
    for(i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        put_pdcp_context(pdcp_dl_contexts,
                         th_config_local->no_of_used_pdcp_dl_contexts);
                         
        put_pdcp_context(pdcp_ul_contexts,
                         th_config_local->no_of_used_pdcp_ul_contexts);
    }

    /* Signal to main thread that the work is done */
    th_config_local->work_done = 1;

    test_printf("thread #%d: exit\n", th_config_local->tid);
    pthread_exit(NULL);
}

static int setup_sec_environment(void)
{
    int i = 0;
    int pkt_idx;
    int ret_code = 0;
    time_t seconds;

    /* Get value from system clock and use it for seed generation  */
    time(&seconds);
    srand((unsigned int) seconds);

    memset (pdcp_dl_contexts, 0, sizeof(pdcp_context_t) * PDCP_CONTEXT_NUMBER);
    memset (pdcp_ul_contexts, 0, sizeof(pdcp_context_t) * PDCP_CONTEXT_NUMBER);

    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        /* DL contexts */
        pdcp_dl_contexts[i].id = i;

        /* Allocate input buffers from memory zone DMA-accessible to SEC engine */
        pdcp_dl_contexts[i].input_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
            PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_DL);
        assert (pdcp_dl_contexts[i].input_buffers != NULL);
        memset(pdcp_dl_contexts[i].input_buffers,
               0,
               PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_DL);

        /* Allocate output buffers from DMA-accessible to SEC engine */
        pdcp_dl_contexts[i].output_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
            PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_DL);
        assert (pdcp_dl_contexts[i].output_buffers != NULL);
        memset(pdcp_dl_contexts[i].output_buffers,
               0,
               PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_DL);

        pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                            MAX_KEY_LENGTH);
        assert (pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key != NULL);

        pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                               MAX_KEY_LENGTH);
        assert (pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key != NULL);

        /* Allocate array of packet structure to be returned when getting a buffer */
        pdcp_dl_contexts[i].test_packets = malloc(sizeof(test_packet_t) * PACKET_NUMBER_PER_CTX_DL);
        assert(pdcp_dl_contexts[i].test_packets != NULL);
        memset(pdcp_dl_contexts[i].test_packets,
               0x00,
               sizeof(test_packet_t) * PACKET_NUMBER_PER_CTX_DL);

        /* Allocate input and output packet arrays */
        for(pkt_idx = 0; pkt_idx < PACKET_NUMBER_PER_CTX_DL; pkt_idx++)
        {
            /* Allocate array of input fragments */
            pdcp_dl_contexts[i].test_packets[pkt_idx].in_packet = 
                malloc(sizeof(sec_packet_t) * test_num_frags);
            assert(pdcp_dl_contexts[i].test_packets[pkt_idx].in_packet != NULL);
            memset(pdcp_dl_contexts[i].test_packets[pkt_idx].in_packet,
                   0x00,
                   sizeof(sec_packet_t) * test_num_frags);
            
            /* Allocate array of output fragments */
            pdcp_dl_contexts[i].test_packets[pkt_idx].out_packet = 
                malloc(sizeof(sec_packet_t));
            assert(pdcp_dl_contexts[i].test_packets[pkt_idx].out_packet != NULL);
            memset(pdcp_dl_contexts[i].test_packets[pkt_idx].out_packet,
                   0x00,
                   sizeof(sec_packet_t));
        }

        pdcp_dl_contexts[i].no_of_buffers_to_process = PACKET_NUMBER_PER_CTX_DL;

        /* UL contexts */
        pdcp_ul_contexts[i].id = i + PDCP_CONTEXT_NUMBER;
        
        /* Allocate input buffers from memory zone DMA-accessible to SEC engine */
        pdcp_ul_contexts[i].input_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
            PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_UL);
        assert (pdcp_ul_contexts[i].input_buffers != NULL);
        memset(pdcp_ul_contexts[i].input_buffers,
               0,
               PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_UL);

        /* Allocate output buffers from memory zone DMA-accessible to SEC engine */
        pdcp_ul_contexts[i].output_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
            PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_UL);
        assert (pdcp_ul_contexts[i].output_buffers != NULL);
        memset(pdcp_ul_contexts[i].output_buffers,
               0,
               PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_UL);
        
        pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                            MAX_KEY_LENGTH);
        assert (pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key != NULL);
        pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                               MAX_KEY_LENGTH);
        assert (pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key != NULL);

        /* Allocate array of packet structure to be returned when getting a buffer */
        pdcp_ul_contexts[i].test_packets = malloc(sizeof(test_packet_t) * PACKET_NUMBER_PER_CTX_UL);
        assert(pdcp_ul_contexts[i].test_packets != NULL);
        memset(pdcp_ul_contexts[i].test_packets,
               0x00,
               sizeof(test_packet_t) * PACKET_NUMBER_PER_CTX_UL);
        
        /* Allocate input and output packet arrays */
        for(pkt_idx = 0; pkt_idx < PACKET_NUMBER_PER_CTX_UL; pkt_idx++)
        {
            /* Allocate array of input fragments */
            pdcp_ul_contexts[i].test_packets[pkt_idx].in_packet = 
                malloc(sizeof(sec_packet_t));
            assert(pdcp_ul_contexts[i].test_packets[pkt_idx].in_packet != NULL);
            memset(pdcp_ul_contexts[i].test_packets[pkt_idx].in_packet,
                   0x00,
                   sizeof(sec_packet_t));
            
            /* Allocate array of output fragments */
            pdcp_ul_contexts[i].test_packets[pkt_idx].out_packet = 
                malloc(sizeof(sec_packet_t) * test_num_frags);
            assert(pdcp_ul_contexts[i].test_packets[pkt_idx].out_packet != NULL);
            memset(pdcp_ul_contexts[i].test_packets[pkt_idx].out_packet,
                   0x00,
                   sizeof(sec_packet_t) * test_num_frags);
        }
        pdcp_ul_contexts[i].no_of_buffers_to_process = PACKET_NUMBER_PER_CTX_UL;

        
    }

    //////////////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC user space driver requesting #JOB_RING_NUMBER Job Rings
    //////////////////////////////////////////////////////////////////////////////
    sec_config_data.memory_area = dma_mem_memalign(CACHE_LINE_SIZE,SEC_DMA_MEMORY_SIZE);
    sec_config_data.sec_drv_vtop = test_vtop;
    assert(sec_config_data.memory_area != NULL);

    // Fill SEC driver configuration data
    sec_config_data.work_mode = SEC_STARTUP_POLLING_MODE;
#ifdef SEC_HW_VERSION_4_4
#if (SEC_INT_COALESCING_ENABLE == ON)
    sec_config_data.irq_coalescing_count = IRQ_COALESCING_COUNT;
    sec_config_data.irq_coalescing_timer = IRQ_COALESCING_TIMER;
#endif // SEC_INT_COALESCING_ENABLE == ON
#endif // SEC_HW_VERSION_4_4

    ret_code = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    if (ret_code != SEC_SUCCESS)
    {
        test_printf("sec_init::Error %d\n", ret_code);
        return 1;
    }
    test_printf("thread main: initialized SEC user space driver\n");

    for (i = 0; i < JOB_RING_NUMBER; i++)
    {
        assert(job_ring_descriptors[i].job_ring_handle != NULL);
        assert(job_ring_descriptors[i].job_ring_irq_fd != 0);
    }

    return 0;
}

static int cleanup_sec_environment(void)
{
    int ret_code = 0, i, pkt_idx;

    // release SEC driver
    ret_code = sec_release();
    if (ret_code != 0)
    {
        test_printf("sec_release returned error\n");
        return 1;
    }
    test_printf("thread main: released SEC user space driver!!\n");

    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        dma_mem_free(pdcp_dl_contexts[i].input_buffers,  PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_DL);
        dma_mem_free(pdcp_dl_contexts[i].output_buffers,  PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_DL);

        dma_mem_free(pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key, MAX_KEY_LENGTH);
        dma_mem_free(pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key, MAX_KEY_LENGTH);

        for(pkt_idx = 0; pkt_idx < PACKET_NUMBER_PER_CTX_DL; pkt_idx++)
        {
            /* Allocate array of input fragments */
            free(pdcp_dl_contexts[i].test_packets[pkt_idx].in_packet);
            free(pdcp_dl_contexts[i].test_packets[pkt_idx].out_packet);
        }
        free(pdcp_dl_contexts[i].test_packets);
        
        dma_mem_free(pdcp_ul_contexts[i].input_buffers, PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_UL);
        dma_mem_free(pdcp_ul_contexts[i].output_buffers, PDCP_BUFFER_SIZE * PACKET_NUMBER_PER_CTX_UL);

        dma_mem_free(pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key, MAX_KEY_LENGTH);
        dma_mem_free(pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key, MAX_KEY_LENGTH);
        
        for(pkt_idx = 0; pkt_idx < PACKET_NUMBER_PER_CTX_UL; pkt_idx++)
        {
            /* Allocate array of input fragments */
            free(pdcp_ul_contexts[i].test_packets[pkt_idx].in_packet);
            free(pdcp_ul_contexts[i].test_packets[pkt_idx].out_packet);
        }
        free(pdcp_ul_contexts[i].test_packets);

    }
    
    /* Release memory allocated for SEC internal structures. */
    dma_mem_free(sec_config_data.memory_area,SEC_DMA_MEMORY_SIZE);

    /* Destroy FSL USMMGR object */
    ret_code = fsl_usmmgr_exit(g_usmmgr);
    if (ret_code)
    {
        perror("Error free'ing USMMGR object");
        return -1;
    }

    return 0;
}
/*==================================================================================================
                                        GLOBAL FUNCTIONS
=================================================================================================*/
int validate_params()
{
    int i;
    struct{
        char alg_name[PATH_MAX];
        sec_crypto_alg_t alg;
    }algos[] = {
        {"SNOW", SEC_ALG_SNOW },
        {"AES", SEC_ALG_AES },
        {"NULL", SEC_ALG_NULL }
    };
    struct{
        char test_type_name[PATH_MAX];
        uint32_t test_type;
    }test_types[] = {
        { "CONTROL", PDCP_CONTROL_PLANE },
        { "CPLANE", PDCP_CONTROL_PLANE },
        { "DATA", PDCP_DATA_PLANE },
        { "UPLANE", PDCP_DATA_PLANE }
    };
    struct{
        char dir_name[PATH_MAX];
        uint32_t dir;
    }test_dirs[] = {
        {"UL", PDCP_UPLINK },
        {"UPLINK", PDCP_UPLINK },
        {"DL", PDCP_DOWNLINK },
        {"DOWNLINK", PDCP_DOWNLINK }
    };
    struct{
        char hdr_len_name[PATH_MAX];
        uint32_t sn_size;
        uint8_t hdr_len;
    }test_hdrs[] = {
        {"SHORT", SEC_PDCP_SN_SIZE_7, 1},
        {"7", SEC_PDCP_SN_SIZE_7, 1},
        {"LONG", SEC_PDCP_SN_SIZE_12, 2},
        {"12", SEC_PDCP_SN_SIZE_12, 2}
    };

    for (i = 0; i < ARRAY_SIZE(algos); i++)
    {
        if(!strncasecmp(user_param.int_alg, algos[i].alg_name,PATH_MAX))
        {
            test_integrity_algorithm = algos[i].alg;
        }
        
        if(!strncasecmp(user_param.enc_alg, algos[i].alg_name,PATH_MAX))
        {
            test_cipher_algorithm = algos[i].alg;
        }
    }
    
    for (i = 0; i < ARRAY_SIZE(test_types); i++)
    {
        if(!strncasecmp(user_param.test_type, test_types[i].test_type_name, PATH_MAX))
        {
            test_user_plane = test_types[i].test_type;
            break;
        }
    }
    if (test_user_plane == -1)
    {
        fprintf(stderr,"Invalid test type: %s\n",user_param.test_type);
        return -1;
    }
    
    if (((test_user_plane == PDCP_CONTROL_PLANE) && 
        (user_param.opt_mask != CPLANE_VALID_MASK)) ||
        ((test_user_plane == PDCP_DATA_PLANE) && 
        (user_param.opt_mask != UPLANE_VALID_MASK)))
    {
        fprintf(stderr,"Invalid combination of paramters selected!\n");
        return -1;
    }

    if ((test_integrity_algorithm == -1) && 
        (test_user_plane == PDCP_CONTROL_PLANE))
    {
        fprintf(stderr,"Invalid integrity algorithm: %s\n",user_param.int_alg);
        return -1;
    }
    
    if (test_cipher_algorithm == -1)
    {
        fprintf(stderr,"Invalid encryption algorithm: %s\n",user_param.enc_alg);
        return -1;
    }

    /* Some combinations are not valid */
    if ((((test_integrity_algorithm == SEC_ALG_SNOW) &&
        (test_cipher_algorithm == SEC_ALG_AES)) ||
        ((test_integrity_algorithm == SEC_ALG_AES) &&
        (test_cipher_algorithm == SEC_ALG_SNOW))) &&
        test_user_plane == PDCP_CONTROL_PLANE)
    {
        fprintf(stderr,"Invalid algorithm combination for Control Plane: %s %s\n",
            user_param.int_alg,
            user_param.enc_alg);
            return -1;
    }

    for (i = 0; i<ARRAY_SIZE(test_dirs); i++)
    {
        if(!strncasecmp(user_param.direction, test_dirs[i].dir_name,PATH_MAX))
        {
            test_direction = test_dirs[i].dir;
            break;
        }
    }
    
    if (test_direction == -1)
    {
        fprintf(stderr, "Invalid direction: %s\n", user_param.direction);
        return -1;
    }

    for (i = 0; i<ARRAY_SIZE(test_hdrs); i++)
    {
        if(!strncasecmp(user_param.hdr_len, test_hdrs[i].hdr_len_name,PATH_MAX))
        {
            test_sn_size = test_hdrs[i].sn_size;
            test_pdcp_hdr_len = test_hdrs[i].hdr_len;
            break;
        }
    }

    if (test_user_plane == PDCP_CONTROL_PLANE)
    {
        test_pdcp_hdr_len = 1;
        test_sn_size = SEC_PDCP_SN_SIZE_5;
    }
    else
    {
        if (test_pdcp_hdr_len == -1)
        {
            fprintf(stderr, "Invalid header length: %s\n", user_param.hdr_len);
            return -1;
        }
    }
    
    if( user_param.max_frags > SEC_MAX_SG_TBL_ENTRIES )
    {
        fprintf(stderr,"Invalid number of fragments %d "
                        "(must be less than %d)\n",
                        user_param.max_frags,
                        SEC_MAX_SG_TBL_ENTRIES);
        return -1;
    }
    
    test_num_frags = user_param.max_frags;
    
    /* Make sure that the user param fits in the allocated buffers */
    if (user_param.payload_size >= PDCP_BUFFER_SIZE - test_pdcp_hdr_len - 
            (test_user_plane == PDCP_CONTROL_PLANE ? ICV_LEN : 0 ) )
    {
        fprintf(stderr,"Invalid payload len %d "
                       "(must be less than %d\n)",
                        user_param.payload_size,
                        PDCP_BUFFER_SIZE - test_pdcp_hdr_len - 
                        (test_user_plane == PDCP_CONTROL_PLANE ? ICV_LEN : 0 ));
        return -1;
    }

    test_payload_size_encap = user_param.payload_size + test_pdcp_hdr_len;
    test_payload_size_decap = user_param.payload_size + test_pdcp_hdr_len + 
                    (test_user_plane == PDCP_CONTROL_PLANE ? ICV_LEN : 0);
    
    test_num_iter = user_param.num_iter;

    return 0;
}

void abort_loop()
{
    printf("Intercepted CTRL-C, wait for running thread to end\n");
    th_config[0].should_exit = 1;
    th_config[1].should_exit = 1;
}


void print_usage(char *prg_name)
{
    printf("Usage: %s"
           " -t test_type"
           " [-l hdr_len]"
           " -d pkt_dir"
           " -e encryption_alg"
           " [-a integrity_alg]"
           " -f number_of_fragments"
           " -s payload_size"
           " -n iterations"
           "\n"
           "\n\n\t-t Selects the test type to be used. It is used"
           " for selecting PDCP Control Plane or PDCP User Plane"
           " encapsulation/decapsulation"
           "\n\t\tValid values:"
           "\n\t\t\to CONTROL or CPLANE"
           "\n\t\t\to DATA or UPLANE"
           "\n\n\t-l Selects the header length. It is valid only for"
           " PDCP Data Plane."
           "\n\t\tValid values:"
           "\n\t\t\to SHORT or 7 - Header Length is 1 byte and SN is assumed to be 7 bits"
           "\n\t\t\to LONG or 12 - Header Lengt is 2 bytes and SN is assumed to be 12 bits"
           "\n\n\t-d Selects the packet direction"
           "\n\t\tValid values:"
           "\n\t\t\to UL or UPLINK - The packets will have the direction bit set to 0"
           "\n\t\t\to DL or DOWNLINK - The packets will have the direction bit set to 1"
           "\n\n\t-e Select the encryption algorithm to be used."
           "\n\t\tValid values are:"
           "\n\t\t\to SNOW"
           "\n\t\t\to AES"
           "\n\t\t\to NULL"
           "\n\n\t-a Select the integrity algorithm to be used."
           "\n\t\tValid for Control Plane only, see -t option"
           "\n\t\tValid values are:"
           "\n\t\t\to SNOW"
           "\n\t\t\to AES"
           "\n\t\t\to NULL"
           "\n\n\t-f Select the number of Scatter-Gather"
           " fragments in which a packet is split"
           "\n\t\tNOTE: It cannot exceed 16"
           "\n\n\t-s Select the maximum payload size of the packets."
           "\n\t\tNOTE: It does NOT include the header size."
           "\n\n\t-n Number of iterations to be run."
           "\n\t\tNOTE: Setting this to 0 will result in endless looping"
           "\n\n\n",prg_name);
}

int main(int argc, char ** argv)
{
    int ret_code = 0;
    int c;
    cpu_set_t cpu_mask; /* processor 0 */

    /* bind process to processor 0 */
    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);
    if(sched_setaffinity(0, sizeof(cpu_mask), &cpu_mask) < 0)
    {
        perror("sched_setaffinity");
    }
    
    /* Make sure the user options are cleared */
    memset(&user_param, 0x00, sizeof(users_params_t));

    while ((c = getopt (argc, argv, "a:e:t:d:l:f:s:n:h")) != -1)
    {
        switch (c)
        {
            case 'a':
                strncpy(user_param.int_alg, optarg, sizeof(user_param.int_alg) - 1);
                printf("Selected integrity algorithm: %s\n", optarg);
                user_param.opt_mask |= INT_ALG_SET;
                break;
            case 'e':
                strncpy(user_param.enc_alg, optarg, sizeof(user_param.enc_alg) - 1);
                printf("Selected encryption algorithm: %s\n",optarg);
                user_param.opt_mask |= ENC_ALG_SET;
                break;
            case 't':
                strncpy(user_param.test_type, optarg, sizeof(user_param.test_type) - 1);
                printf("Selected test type: %s\n",optarg);
                user_param.opt_mask |= TYPE_SET;
                break;
            case 'd':
                strncpy(user_param.direction, optarg, sizeof(user_param.direction) - 1);
                printf("Selected direction: %s\n", optarg);
                user_param.opt_mask |= DIR_SET;
                break;
            case 'l':
                strncpy(user_param.hdr_len, optarg, sizeof(user_param.hdr_len) - 1);
                printf("Selected header length: %s\n",optarg);
                user_param.opt_mask |= HDR_LEN_SET;
                break;
            case 'f':
                user_param.max_frags = atoi(optarg);
                printf("Selected number of Scatter-Gather fragments: %d\n",user_param.max_frags);
                user_param.opt_mask |= MAX_FRAGS_SET;
                break;
            case 's':
                user_param.payload_size = atoi(optarg);
                printf("Payload size %d\n",user_param.payload_size);
                user_param.opt_mask |= PAYLOAD_SET;
                break;
            case 'n':
                user_param.num_iter = atoi(optarg);
                printf("Number of iterations: %s\n",
                    user_param.num_iter == 0 ? "infinite" : optarg);
                user_param.opt_mask |= NUM_ITER_SET;
                break;
            case '?':
                print_usage(argv[0]);
                return 1;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    if( validate_params() )
    {
        fprintf(stderr,"Error in options!\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Init FSL USMMGR */
    g_usmmgr = fsl_usmmgr_init();
    if(g_usmmgr == NULL){
        perror("ERROR on fsl_usmmgr_init :");
        return -1;
    }

    /* Install CTRL-C handler */
    signal(SIGINT, abort_loop);

    /////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC environment
    /////////////////////////////////////////////////////////////////////
    ret_code = setup_sec_environment();
    if (ret_code != 0)
    {
        test_printf("setup_sec_environment returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 1. Randomize input and keys
    /////////////////////////////////////////////////////////////////////
    generate_test_vectors();

    /////////////////////////////////////////////////////////////////////
    // 2. Populate data to be sent/received
    /////////////////////////////////////////////////////////////////////
    populate_pdcp_contexts();

    /////////////////////////////////////////////////////////////////////
    // 3. Start worker threads
    /////////////////////////////////////////////////////////////////////
    ret_code = start_sec_worker_threads();
    if (ret_code != 0)
    {
        test_printf("start_sec_worker_threads returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 4. Stop worker threads
    /////////////////////////////////////////////////////////////////////
    ret_code = stop_sec_worker_threads();
    if (ret_code != 0)
    {
        test_printf("stop_sec_worker_threads returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 5. Cleanup SEC environment
    /////////////////////////////////////////////////////////////////////
    ret_code = cleanup_sec_environment();
    if (ret_code != 0)
    {
        test_printf("cleanup_sec_environment returned error\n");
        return 1;
    }

    return 0;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
