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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>

#include "fsl_sec.h"
// for mfspr related functions
#include "compat.h"
#ifdef SEC_HW_VERSION_4_4

#warning "Ugly hack to keep compat.h unchanged."
#undef __KERNEL__

// For shared memory allocator
#include "fsl_usmmgr.h"

#endif
#include "test_sec_driver_benchmark_sg.h"

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

// Number of SEC JRs used by this test application
// @note: Currently this test application supports only 2 JRs (not less, not more)
#define JOB_RING_NUMBER              2

// Number of worker threads created by this application
// One thread is producer on JR 1 and consumer on JR 2 (PDCP UL processing).
// The other thread is producer on JR 2 and consumer on JR 1 (PDCP DL processing).
#define THREADS_NUMBER               2

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
// Max length in bytes for a confidentiality /integrity key.
#define MAX_KEY_LENGTH    32

// Read ATBL(Alternate Time Base Lower) Register
#define GET_ATBL() \
    mfspr(SPR_ATBL)

// Must be at least 8, for statistics reasons
#define TEST_OFFSET 8

/** Size in bytes of a cacheline. */
#define CACHE_LINE_SIZE  32

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

typedef enum pdcp_buffer_usage_e
{
    PDCP_BUFFER_FREE = 0,
    PDCP_BUFFER_USED
}pdcp_buffer_usage_t;

typedef struct buffer_s
{
    volatile pdcp_buffer_usage_t usage;
    uint8_t buffer[PDCP_BUFFER_SIZE];
    uint32_t offset;
    sec_packet_t pdcp_packet;
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
    // the id of the thread that handles this context
    int thread_id;

    // Pool of input and output buffers used for processing the
    // packets associated to this context. (used an array for simplicity)
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
    // the JR ID for which this thread is a producer
    int producer_job_ring_id;
    // the JR ID for which this thread is a consumer
    int consumer_job_ring_id;
    // the pool of contexts for this thread
    pdcp_context_t * pdcp_contexts;
    // no of used contexts from the pool
    int * no_of_used_pdcp_contexts;
    // a random number of pdcp contexts to create/delete at runtime
    int no_of_pdcp_contexts_to_test;
    // default to zero. set to 1 by worker thread to notify
    // main thread that it completed its work
    volatile int work_done;
    // default to zero. set to 1 when main thread instructs the worked thread to exit.
    volatile int should_exit;
    uint32_t core_cycles;
    uint32_t rx_packets_per_ctx;
    uint32_t tx_packets_per_ctx;
}thread_config_t;
/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
// configuration data for SEC driver
static sec_config_t sec_config_data;

// job ring handles provided by SEC driver
static const sec_job_ring_descriptor_t *job_ring_descriptors = NULL;

// UA pool of PDCP contexts for UL and DL.
// For simplicity, use an array of contexts and a mutex to synchronize access to it.
static pdcp_context_t pdcp_contexts[PDCP_CONTEXT_NUMBER];
static int no_of_used_pdcp_contexts = 0;

// There are 2 threads: one thread handles PDCP UL and one thread handles PDCP DL
// One thread is producer on JR1 and consumer on JR2. The other thread is producer on JR2
// and consumer on JR1.
static thread_config_t th_config[THREADS_NUMBER];
static pthread_t threads[THREADS_NUMBER];

// FSL Userspace Memory Manager structure
fsl_usmmgr_t g_usmmgr;

static users_params_t user_param = { 0, 0, SCENARIO_PDCP_INVALID, SCENARIO_DIR_INVALID};

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
 * @see the description of pdcp_ready_packet_handler() for details on what the worker
 * thread does in the callback.
 *
 * The main thread waits for both worker threads to finish their work (meaning creating
 * the configured number of contexts, sending the packets, waiting for all the responses
 * and deleting all contexts). When both worker threads finished their work, the main
 * thread instructs them to exit.
 *  */
static void* pdcp_tx_thread_routine(void*);
static void* pdcp_rx_thread_routine(void*);


/** @brief Poll a specified SEC job ring for results */
static int get_results(uint8_t job_ring, int limit, uint32_t *out_packets, uint32_t *core_cycles, uint8_t tid);

/** @brief Get statistics for a specified SEC job ring */
static void get_stats(uint8_t job_ring);

/** @brief Get a free PDCP context from the pool of UA contexts.
 *  For simplicity, the pool is implemented as an array. */
static int get_free_pdcp_context(pdcp_context_t * pdcp_contexts,
                                 int * no_of_used_pdcp_contexts,
                                 pdcp_context_t ** pdcp_context);

/** @brief Get free PDCP input and output buffers from the pool of buffers
 *  of a certain context.
 *  For simplicity, the pool is implemented as an array and is defined per context. */
static int get_free_pdcp_buffer(pdcp_context_t * pdcp_context,
                                sec_packet_t **in_packet,
                                sec_packet_t **out_packet);

/** @brief Callback called by SEC driver for each response.
 *
 * In the callback called by SEC driver for each packet processed, the worker thread
 * will do the following:
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
static int pdcp_ready_packet_handler (const sec_packet_t *in_packet,
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
static uint8_t test_cipher_algorithm;

static uint8_t test_integrity_algorithm;

static uint8_t test_user_plane;

static uint8_t test_sn_size;

static uint8_t pdcp_header_len;

static uint8_t test_crypto_key[MAX_KEY_LENGTH];
#define test_crypto_key_len     sizeof(test_crypto_key)

static uint8_t test_auth_key[MAX_KEY_LENGTH];
#define test_auth_key_len     sizeof(test_auth_key)

static uint8_t test_pdcp_hdr[MAX_PDCP_HEADER_LEN];

static uint8_t test_data_in[PDCP_BUFFER_SIZE];

static uint8_t test_bearer;

static uint32_t test_hfn;

static uint32_t test_hfn_threshold;

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
#ifdef SEC_HW_VERSION_4_4
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
   fsl_usmmgr_free(&r,g_usmmgr);
}
#endif // SEC_HW_VERSION_4_4

static void generate_test_vectors()
{
    int i = 0;

    for (i = 0; i < pdcp_header_len; i++ )
    {
        test_pdcp_hdr[i] = rand();
    }

    for(i = 0; i < user_param.payload_size; i++ )
    {
        test_data_in[i] = rand();
    }

    for( i = 0; i < MAX_KEY_LENGTH; i++ )
    {
        test_crypto_key[i] = rand();
        test_auth_key[i] = rand();
    }

    test_bearer = rand();

    test_hfn = rand();

    test_hfn_threshold = test_hfn + 1; /* Threshold reach indication will be
                                          ignored on poll */
}

static int get_free_pdcp_context(pdcp_context_t * pdcp_contexts_pool,
                                 int * used_contexts,
                                 pdcp_context_t ** pdcp_context)
{
    int i = 0;
    int found = 0;

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

            found = 1;
            break;
        }
    }
    assert(found == 1);

    // return the context chosen
    *pdcp_context = &(pdcp_contexts_pool[i]);

    return 0;
}

static int get_free_pdcp_buffer(pdcp_context_t * pdcp_context,
                                sec_packet_t **in_packet,
                                sec_packet_t **out_packet)
{
    assert(pdcp_context != NULL);
    assert(in_packet != NULL);
    assert(out_packet != NULL);


    if (pdcp_context->no_of_used_buffers >= pdcp_context->no_of_buffers_to_process)
    {
        // no free buffer available
        return 1;
    }

    assert(pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].usage == PDCP_BUFFER_FREE);
    *in_packet = &(pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].pdcp_packet);

    pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].usage = PDCP_BUFFER_USED;
    (*in_packet)->address = test_vtop(&(pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].buffer[0]));

    assert(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].usage == PDCP_BUFFER_FREE);
    *out_packet = &(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].pdcp_packet);

    pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].usage = PDCP_BUFFER_USED;
    (*out_packet)->address = test_vtop(&(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].buffer[0]));

    (*in_packet)->offset = TEST_OFFSET;

    // copy PDCP header
    //memcpy(test_ptov((*in_packet)->address) + (*in_packet)->offset, test_pdcp_hdr, sizeof(test_pdcp_hdr));
    memcpy( &(pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].buffer[0]) +
            TEST_OFFSET,
            test_pdcp_hdr,
            pdcp_header_len);

    // copy input data
    //memcpy(test_ptov((*in_packet)->address) + (*in_packet)->offset + PDCP_HEADER_LENGTH,
    //       test_data_in,
    //       sizeof(test_data_in));
    memcpy( &(pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].buffer[0]) +
            TEST_OFFSET + pdcp_header_len,
            test_data_in,
            user_param.payload_size);


    (*in_packet)->length = user_param.payload_size + pdcp_header_len;
    assert((*in_packet)->length + TEST_OFFSET <= PDCP_BUFFER_SIZE);

    (*out_packet)->length = user_param.payload_size + pdcp_header_len;
    (*out_packet)->offset = 0x00;

    if( pdcp_context->pdcp_ctx_cfg_data.user_plane == PDCP_CONTROL_PLANE )
    {
        if( pdcp_context->pdcp_ctx_cfg_data.protocol_direction == PDCP_ENCAPSULATION )
        {
            (*out_packet)->length += 4;
        }
        else
        {
            (*out_packet)->length -= 4;
        }
    }

    assert((*out_packet)->length <= PDCP_BUFFER_SIZE);

    pdcp_context->no_of_used_buffers++;

    return 0;
}

static int pdcp_ready_packet_handler (const sec_packet_t *in_packet,
                                      const sec_packet_t *out_packet,
                                      ua_context_handle_t ua_ctx_handle,
                                      uint32_t status,
                                      uint32_t error_info)
{
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
    profile_printf("thread #%d:sec_poll_job_ring cycles = %d. pkts = %d", tid, diff_cycles, *packets_out);

    if (ret_code != SEC_SUCCESS)
    {
        test_printf("sec_poll_job_ring::Error %d when polling for SEC results on Job Ring %d \n", ret_code, job_ring);
        return 1;
    }

    usleep(10);

    // validate that number of packets notified does not exceed limit when limit is > 0.
    assert(!((limit > 0) && (*packets_out > limit)));

    return 0;
}

static void get_stats(uint8_t job_ring_id)
{
    sec_statistics_t stats;
    int ret_code = 0;

    ret_code = sec_get_stats(job_ring_descriptors[job_ring_id].job_ring_handle,&stats);
    assert(ret_code == SEC_SUCCESS);

    test_printf("JR consumer index (index from where the next job will be dequeued): %d",stats.sw_consumer_index);
    test_printf("JR producer index (index where the next job will be enqueued): %d",stats.sw_producer_index);
    test_printf("CAAM JR consumer index (index from where the next job will be dequeued): %d",stats.hw_consumer_index);
    test_printf("CAAM JR producer index (index where the next job will be enqueued): %d",stats.hw_producer_index);
    test_printf("Input slots available: %d",stats.slots_available);
    test_printf("Jobs waiting to be dequeued by UA: %d",stats.jobs_waiting_dequeue);
}

static int start_sec_worker_threads(void)
{
    int ret_code = 0;

    // this scenario handles only 2 JRs and 2 worker threads
    assert (JOB_RING_NUMBER == 2);
    assert (THREADS_NUMBER == 2);

    // start PDCP UL thread
    th_config[0].tid = 0;
    // PDCP UL thread is consumer on JR ID 0 and producer on JR ID 1
    th_config[0].consumer_job_ring_id = 0;
    th_config[0].producer_job_ring_id = 1;
    th_config[0].pdcp_contexts = &pdcp_contexts[0];
    th_config[0].no_of_used_pdcp_contexts = &no_of_used_pdcp_contexts;
    th_config[0].no_of_pdcp_contexts_to_test = PDCP_CONTEXT_NUMBER;
    th_config[0].work_done = 0;
    th_config[0].should_exit = 0;
    th_config[0].core_cycles = 0;
    th_config[0].tx_packets_per_ctx = (user_param.direction == SCENARIO_DIR_UPLINK) ?
        PACKET_NUMBER_PER_CTX_UL : PACKET_NUMBER_PER_CTX_DL;
    ret_code = pthread_create(&threads[0], NULL, &pdcp_tx_thread_routine, (void*)&th_config[0]);
    assert(ret_code == 0);

    // start PDCP DL thread
    th_config[1].tid = 1;
    // PDCP DL thread is consumer on JR ID 1 and producer on JR ID 0
    th_config[1].consumer_job_ring_id = 1;
    th_config[1].producer_job_ring_id = 0;
    th_config[1].no_of_pdcp_contexts_to_test = PDCP_CONTEXT_NUMBER;
    th_config[1].work_done = 0;
    th_config[1].should_exit = 0;
    th_config[1].core_cycles = 0;
    th_config[1].rx_packets_per_ctx = (user_param.direction == SCENARIO_DIR_UPLINK) ? 
        PACKET_NUMBER_PER_CTX_UL : PACKET_NUMBER_PER_CTX_DL;
    
    ret_code = pthread_create(&threads[1], NULL, &pdcp_rx_thread_routine, (void*)&th_config[1]);
    assert(ret_code == 0);

    return ret_code;
}

static int stop_sec_worker_threads(void)
{
    int i = 0;
    int ret_code = 0;

    // this scenario handles only 2 JRs and 2 worker threads
    assert (JOB_RING_NUMBER == 2);
    assert (THREADS_NUMBER == 2);

    // wait until both worker threads finish their work
    while (!(th_config[0].work_done == 1 && th_config[1].work_done == 1))
    {
        sleep(1);
    }
    // tell the working threads that they can stop polling now and exit
    th_config[0].should_exit = 1;
    th_config[1].should_exit = 1;

    // wait for the threads to exit
    for (i = 0; i < THREADS_NUMBER; i++)
    {
        ret_code = pthread_join(threads[i], NULL);
        assert(ret_code == 0);
    }

    // double check that indeed the threads finished their work
//    assert(no_of_used_pdcp_dl_contexts == 0);
//    assert(no_of_used_pdcp_ul_contexts == 0);

    test_printf("thread main: all worker threads are stopped\n");
    printf("thread 0 core cycles = %d\n", th_config[0].core_cycles);
    printf("thread 1 core cycles = %d\n", th_config[1].core_cycles);

    return ret_code;
}

static void* pdcp_tx_thread_routine(void* config)
{
    thread_config_t *th_config_local = NULL;
    int ret_code = 0;
    int i = 0;
    pdcp_context_t *pdcp_context;
    //int total_no_of_contexts_deleted = 0;
    //int no_of_contexts_deleted = 0;
    int total_no_of_contexts_created = 0;
    unsigned int total_packets_to_send = 0;
    unsigned int total_packets_sent = 0;
    unsigned int ctx_packet_count = 0;
    struct timeval start_time;
    struct timeval end_time;
    uint32_t start_cycles = 0;
    uint32_t diff_cycles = 0;
    uint32_t pps;
    uint32_t retries = 0;

    th_config_local = (thread_config_t*)config;
    assert(th_config_local != NULL);

    test_printf("thread #%d:producer: start work, no of contexts to be created/deleted %d\n",
            th_config_local->tid, th_config_local->no_of_pdcp_contexts_to_test);

    /////////////////////////////////////////////////////////////////////
    // 1. Create a number of PDCP contexts
    /////////////////////////////////////////////////////////////////////

    // Create a number of configurable contexts and affine them to the producer JR, send a random
    // number of packets per each context
    while(total_no_of_contexts_created < th_config_local->no_of_pdcp_contexts_to_test)
    {
        ret_code = get_free_pdcp_context(th_config_local->pdcp_contexts,
                th_config_local->no_of_used_pdcp_contexts,
                &pdcp_context);
        assert(ret_code == 0);

        test_printf("thread #%d:producer: create pdcp context %d\n", th_config_local->tid, pdcp_context->id);
        pdcp_context->pdcp_ctx_cfg_data.notify_packet = &pdcp_ready_packet_handler;
        pdcp_context->pdcp_ctx_cfg_data.sn_size = test_sn_size;
        pdcp_context->pdcp_ctx_cfg_data.bearer = test_bearer;
        pdcp_context->pdcp_ctx_cfg_data.user_plane = test_user_plane;
        if( user_param.direction == SCENARIO_DIR_UPLINK )
        {
            pdcp_context->pdcp_ctx_cfg_data.packet_direction = PDCP_UPLINK;
            pdcp_context->pdcp_ctx_cfg_data.protocol_direction = PDCP_DECAPSULATION;
        }
        else
        {
            pdcp_context->pdcp_ctx_cfg_data.packet_direction = PDCP_DOWNLINK;
            pdcp_context->pdcp_ctx_cfg_data.protocol_direction = PDCP_ENCAPSULATION;
        }

        pdcp_context->pdcp_ctx_cfg_data.hfn = test_hfn;
        pdcp_context->pdcp_ctx_cfg_data.hfn_threshold = test_hfn_threshold;

        // configure confidentiality algorithm
        pdcp_context->pdcp_ctx_cfg_data.cipher_algorithm = test_cipher_algorithm;
        uint8_t* temp_crypto_key = test_crypto_key;
        if(temp_crypto_key != NULL)
        {
            memcpy(pdcp_context->pdcp_ctx_cfg_data.cipher_key,
                    temp_crypto_key,
                    test_crypto_key_len);
            pdcp_context->pdcp_ctx_cfg_data.cipher_key_len = test_crypto_key_len;
        }

        // configure integrity algorithm
        pdcp_context->pdcp_ctx_cfg_data.integrity_algorithm = test_integrity_algorithm;
        uint8_t* temp_auth_key = test_auth_key;

        if(temp_auth_key != NULL)
        {
            memcpy(pdcp_context->pdcp_ctx_cfg_data.integrity_key,
                    temp_auth_key,
                    test_auth_key_len);
            pdcp_context->pdcp_ctx_cfg_data.integrity_key_len = test_auth_key_len;
        }

        pdcp_context->thread_id = th_config_local->tid;

        // create a SEC context in SEC driver
        ret_code = sec_create_pdcp_context (job_ring_descriptors[th_config_local->producer_job_ring_id].job_ring_handle,
                &pdcp_context->pdcp_ctx_cfg_data,
                &pdcp_context->sec_ctx);
        if (ret_code != SEC_SUCCESS)
        {
            test_printf("thread #%d:producer: sec_create_pdcp_context return error %d for PDCP context no %d \n",
                    th_config_local->tid, ret_code, pdcp_context->id);
            assert(0);
        }

        total_no_of_contexts_created ++;


    }

    /////////////////////////////////////////////////////////////////////
    // 2. Send a number of packets on each of the PDCP contexts
    /////////////////////////////////////////////////////////////////////

    total_packets_to_send = PDCP_CONTEXT_NUMBER * th_config_local->tx_packets_per_ctx;

    gettimeofday(&start_time, NULL);
    // Send a burst of packets for each context, until all the packets are sent to SEC


    while(total_packets_to_send != total_packets_sent)
    {
        for(i = 0; i < PDCP_CONTEXT_NUMBER; i++)
        {
            pdcp_context = &th_config_local->pdcp_contexts[i];
            ctx_packet_count = 0;

            while(ctx_packet_count < PACKET_BURST_PER_CTX)
            {
                // for each context, send to SEC a fixed number of packets for processing
                sec_packet_t *in_frag;
                sec_packet_t *out_frag;

                sec_packet_t in_packet[16];
                sec_packet_t out_packet[16];

                int num_fragments;
                int last_len, rem_len = pdcp_header_len + user_param.payload_size;
                uint32_t last_address;
                int idx;

                // Get a free buffer, If none, then break the loop
                if(get_free_pdcp_buffer(pdcp_context, &in_frag, &out_frag) != 0)
                {
                    break;
                }

                memcpy(&in_packet[0],in_frag,sizeof(sec_packet_t));

                memcpy(&out_packet[0],out_frag,sizeof(sec_packet_t));
#ifdef RANDOM_SCATTER_GATHER_FRAGMENTS
                  num_fragments = 0 + (rand() % (user_param.max_frags - 1) );
#else
                  num_fragments = user_param.max_frags - 1;
#endif
                in_packet[0].num_fragments = num_fragments;
                in_packet[0].total_length = rem_len;

                last_address = in_packet[0].address;
                last_len = 0;

                test_printf("Num of fragments : %d",num_fragments);

                for(idx = 0; idx <= num_fragments - 1; idx++ )
                {
                    in_packet[idx].address = last_address +
                                           last_len;
#ifdef RANDOM_SCATTER_GATHER_FRAGMENTS
                    in_packet[idx].length = 1 + (rand() % (rem_len - 1));
#else
                    in_packet[idx].length = rem_len / num_fragments;
#endif
                    last_address = in_packet[idx].address;
                    last_len = in_packet[idx].length;

                    rem_len -= in_packet[idx].length;

                    test_printf("Fragment %d: len = %d, address = 0x%08x",
                                idx,
                                in_packet[idx].length,
                                in_packet[idx].address);
                }

                // Last fragment must finish the packet
                if(rem_len)
                {
                    in_packet[idx].address = last_address +
                                           last_len;
                    in_packet[idx].length = rem_len;

                    test_printf("Last fragment %d: len = %d, address = 0x%08x",
                                idx,
                                in_packet[idx].length,
                                in_packet[idx].address);
                }
                
                // get_stats(th_config_local->producer_job_ring_id);
try_again:
                start_cycles = GET_ATBL();
                ret_code = sec_process_packet(pdcp_context->sec_ctx,
                        in_packet,
                        out_packet,
                        (ua_context_handle_t)pdcp_context);

                diff_cycles = (GET_ATBL() - start_cycles);

                th_config_local->core_cycles += diff_cycles;
                
                profile_printf("thread #%d:ctx #%p:sec_process_packet cycles = %d\n",
                        th_config_local->tid, pdcp_context, diff_cycles);

                if(ret_code != SEC_SUCCESS)
                {
                    if( ret_code == SEC_JR_IS_FULL )
                    {
                        test_printf("thread #%d:producer: JR is full",th_config_local->tid);
                        retries++;
                        usleep(1);
                        goto try_again;
                    }
                    else
                    {
                        test_printf("thread #%d:producer: sec_process_packet return error %d for PDCP context no %d \n",
                                    th_config_local->tid, ret_code, pdcp_context->id);
                        assert(0);
                    }
                }
                ctx_packet_count++;
            }
            total_packets_sent += ctx_packet_count;
        }
    }

    test_printf("thread #%d: polling until all contexts are deleted\n", th_config_local->tid);

    gettimeofday(&end_time, NULL);

    printf("Sent %d packets.\nStart Time %d sec %d usec."
            "End Time %d sec %d usec.\n",
            total_packets_sent, (int)start_time.tv_sec, (int)start_time.tv_usec,
            (int)end_time.tv_sec, (int)end_time.tv_usec);

    pps = 1000000 * total_packets_sent / (((int)end_time.tv_sec - (int)start_time.tv_sec)*1000000 + 
                                           (int)end_time.tv_usec - (int)start_time.tv_usec);
    printf("Packets / second : %d\n", pps);
    printf("Bits / second : %d\n",pps*user_param.payload_size*8);
    printf("Got JR full %d times\n",retries);
            
    // signal to main thread that the work is done
    th_config_local->work_done = 1;
/*
    test_printf("thread #%d: work done, polling until exit signal is received\n", th_config_local->tid);

    // continue polling on the consumer job ring, in case the other worker thread
    // did not finish its work
    while(th_config_local->should_exit == 0)
    {
        ret_code = get_results(th_config_local->consumer_job_ring_id,
        		          JOB_RING_POLL_UNLIMITED,
        		          &packets_received,
                          &th_config_local->core_cycles);
        assert(ret_code == 0);
    }
*/
    test_printf("thread #%d: exit\n", th_config_local->tid);
    pthread_exit(NULL);
}

static void* pdcp_rx_thread_routine(void* config)
{
    thread_config_t *th_config_local = NULL;
    int ret_code = 0;
    unsigned int packets_received = 0;
    //int total_no_of_contexts_deleted = 0;
    //int no_of_contexts_deleted = 0;
    unsigned int total_packets_to_receive = 0;
    unsigned int total_packets_received = 0;
    struct timeval start_time;
    struct timeval end_time;
    uint32_t pps;

    th_config_local = (thread_config_t*)config;
    assert(th_config_local != NULL);

    test_printf("thread #%d:producer: start work, no of contexts to be created/deleted %d\n",
            th_config_local->tid, th_config_local->no_of_pdcp_contexts_to_test);

    total_packets_to_receive = PDCP_CONTEXT_NUMBER * th_config_local->rx_packets_per_ctx;

    gettimeofday(&start_time, NULL);

    // Poll the JR until no more packets are retrieved
    do
    {
        // poll the consumer JR
        ret_code = get_results(th_config_local->consumer_job_ring_id,
                               JOB_RING_POLL_UNLIMITED,
                               &packets_received,
                               &th_config_local->core_cycles,
                               th_config_local->tid);
        assert(ret_code == 0);
        total_packets_received += packets_received;
        /*printf("thread #%d: pkts to receive %d. packets received %d\n",
                th_config_local->tid,
                total_packets_to_receive, total_packets_received);
        */
        //usleep(25);
    }while(total_packets_to_receive != total_packets_received);

    test_printf("thread #%d: polling until all contexts are deleted\n", th_config_local->tid);

    gettimeofday(&end_time, NULL);

    printf("Received %d packets.\nStart Time %d sec %d usec."
            "End Time %d sec %d usec.\n",
            total_packets_received, (int)start_time.tv_sec, (int)start_time.tv_usec,
            (int)end_time.tv_sec, (int)end_time.tv_usec);

    pps = 1000000 * total_packets_received / (((int)end_time.tv_sec - (int)start_time.tv_sec)*1000000 + 
                                           (int)end_time.tv_usec - (int)start_time.tv_usec);
    printf("Packets / second : %d\n", pps);
    printf("Bits / second : %d\n",pps*user_param.payload_size*8);
            
    // signal to main thread that the work is done
    th_config_local->work_done = 1;
/*
    test_printf("thread #%d: work done, polling until exit signal is received\n", th_config_local->tid);

    // continue polling on the consumer job ring, in case the other worker thread
    // did not finish its work
    while(th_config_local->should_exit == 0)
    {
        ret_code = get_results(th_config_local->consumer_job_ring_id,
        		          JOB_RING_POLL_UNLIMITED,
        		          &packets_received,
                          &th_config_local->core_cycles);
        assert(ret_code == 0);
    }
*/
    test_printf("thread #%d: exit\n", th_config_local->tid);
    pthread_exit(NULL);
}

static int setup_sec_environment(void)
{
    int i = 0;
    int ret_code = 0;
    time_t seconds;
    int num_of_buffers;

    /* Get value from system clock and use it for seed generation  */
    time(&seconds);
    srand((unsigned int) seconds);

    memset (pdcp_contexts, 0, sizeof(pdcp_context_t) * PDCP_CONTEXT_NUMBER);

    num_of_buffers = (user_param.direction == SCENARIO_DIR_UPLINK) ? 
                        PACKET_NUMBER_PER_CTX_UL : PACKET_NUMBER_PER_CTX_DL;
    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        pdcp_contexts[i].id = i;
        // allocate input buffers from memory zone DMA-accessible to SEC engine
        pdcp_contexts[i].input_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                             sizeof(buffer_t) * num_of_buffers);
        // allocate output buffers from memory zone DMA-accessible to SEC engine
        pdcp_contexts[i].output_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                              sizeof(buffer_t) * num_of_buffers);

        pdcp_contexts[i].pdcp_ctx_cfg_data.cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                            MAX_KEY_LENGTH);
        pdcp_contexts[i].pdcp_ctx_cfg_data.integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                               MAX_KEY_LENGTH);
        pdcp_contexts[i].no_of_buffers_to_process = num_of_buffers;

        assert (pdcp_contexts[i].input_buffers != NULL);
        assert (pdcp_contexts[i].output_buffers != NULL);
        assert (pdcp_contexts[i].pdcp_ctx_cfg_data.cipher_key != NULL);
        assert (pdcp_contexts[i].pdcp_ctx_cfg_data.integrity_key != NULL);


        memset(pdcp_contexts[i].input_buffers, 0, sizeof(buffer_t) * num_of_buffers);
        memset(pdcp_contexts[i].output_buffers, 0, sizeof(buffer_t) * num_of_buffers);

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
    int ret_code = 0, i;
    int num_of_buffers = (user_param.direction == SCENARIO_DIR_UPLINK) ? 
                        PACKET_NUMBER_PER_CTX_UL : PACKET_NUMBER_PER_CTX_DL;
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
        dma_mem_free(pdcp_contexts[i].input_buffers, sizeof(buffer_t) * num_of_buffers);
        dma_mem_free(pdcp_contexts[i].output_buffers, sizeof(buffer_t) * num_of_buffers);

        dma_mem_free(pdcp_contexts[i].pdcp_ctx_cfg_data.cipher_key, MAX_KEY_LENGTH);
        dma_mem_free(pdcp_contexts[i].pdcp_ctx_cfg_data.integrity_key, MAX_KEY_LENGTH);
    }

    dma_mem_free(sec_config_data.memory_area,SEC_DMA_MEMORY_SIZE);


    return 0;
}
/*==================================================================================================
                                        GLOBAL FUNCTIONS
=================================================================================================*/
void set_test_params()
{
    switch( user_param.scenario )
    {
        case SCENARIO_PDCP_CPLANE_AES_CTR_AES_CMAC:
            test_cipher_algorithm = SEC_ALG_AES;
            test_integrity_algorithm = SEC_ALG_AES;
            test_user_plane = PDCP_CONTROL_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_5;
            pdcp_header_len = 2;
            break;

        case SCENARIO_PDCP_CPLANE_SNOW_F8_SNOW_F9:
            test_cipher_algorithm = SEC_ALG_SNOW;
            test_integrity_algorithm = SEC_ALG_SNOW;
            test_user_plane = PDCP_CONTROL_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_5;
            pdcp_header_len = 2;
            break;

        case SCENARIO_PDCP_CPLANE_AES_CTR_NULL:
            test_cipher_algorithm = SEC_ALG_AES;
            test_integrity_algorithm = SEC_ALG_NULL;
            test_user_plane = PDCP_CONTROL_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_5;
            pdcp_header_len = 2;
            break;

        case SCENARIO_PDCP_CPLANE_SNOW_F8_NULL:
            test_cipher_algorithm = SEC_ALG_SNOW;
            test_integrity_algorithm = SEC_ALG_NULL;
            test_user_plane = PDCP_CONTROL_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_5;
            pdcp_header_len = 2;
            break;

        case SCENARIO_PDCP_CPLANE_NULL_AES_CMAC:
            test_cipher_algorithm = SEC_ALG_NULL;
            test_integrity_algorithm  = SEC_ALG_AES;
            test_user_plane = PDCP_CONTROL_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_5;
            pdcp_header_len = 2;
            break;
        
        case SCENARIO_PDCP_CPLANE_NULL_SNOW_F9:
            test_cipher_algorithm = SEC_ALG_NULL;
            test_integrity_algorithm = SEC_ALG_SNOW;
            test_user_plane = PDCP_CONTROL_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_5;
            pdcp_header_len = 2;
            break;

        case SCENARIO_PDCP_CPLANE_NULL_NULL:
            test_cipher_algorithm = SEC_ALG_NULL;
            test_integrity_algorithm = SEC_ALG_NULL;
            test_user_plane = PDCP_CONTROL_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_5;
            pdcp_header_len = 2;
            break;

        case SCENARIO_PDCP_UPLANE_SHORT_SN_AES_CTR:
            test_cipher_algorithm = SEC_ALG_AES;
            test_integrity_algorithm = SEC_ALG_NULL;
            test_user_plane = PDCP_DATA_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_7;
            pdcp_header_len = 1;
            break;

        case SCENARIO_PDCP_UPLANE_LONG_SN_AES_CTR:
            test_cipher_algorithm = SEC_ALG_AES;
            test_integrity_algorithm = SEC_ALG_NULL;
            test_user_plane = PDCP_DATA_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_12;
            pdcp_header_len = 2;
            break;

        case SCENARIO_PDCP_UPLANE_SHORT_SN_SNOW_F8:
            test_cipher_algorithm = SEC_ALG_SNOW;
            test_integrity_algorithm = SEC_ALG_NULL;
            test_user_plane = PDCP_DATA_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_7;
            pdcp_header_len = 1;
            break;
        
        case SCENARIO_PDCP_UPLANE_LONG_SN_SNOW_F8:
            test_cipher_algorithm = SEC_ALG_SNOW;
            test_integrity_algorithm = SEC_ALG_NULL;
            test_user_plane = PDCP_DATA_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_12;
            pdcp_header_len = 1;
            break;

        case SCENARIO_PDCP_UPLANE_SHORT_SN_NULL:
            test_cipher_algorithm = SEC_ALG_NULL;
            test_integrity_algorithm = SEC_ALG_NULL;
            test_user_plane = PDCP_DATA_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_7;
            pdcp_header_len = 1;
            break;
            
        
        case SCENARIO_PDCP_UPLANE_LONG_SN_NULL:
            test_cipher_algorithm  = SEC_ALG_NULL;
            test_integrity_algorithm  = SEC_ALG_NULL;
            test_user_plane = PDCP_DATA_PLANE;
            test_sn_size = SEC_PDCP_SN_SIZE_12;
            pdcp_header_len = 2;
            
            break;
        case SCENARIO_PDCP_INVALID:
        default:
            /* Just to keep the compiler happy */
            break;
    }
}

int validate_params()
{
    if( user_param.scenario >= SCENARIO_PDCP_INVALID)
    {
        fprintf(stderr,"Invalid scenario %d\n",user_param.scenario);
        return -1;
    }

    if ( user_param.direction >= SCENARIO_DIR_INVALID)
    {
        fprintf(stderr,"Invalid direction: %d\n",user_param.direction);
        return -2;
    }

    if( user_param.max_frags > SEC_MAX_SG_TBL_ENTRIES )
    {
        fprintf(stderr,"Invalid number of fragments %d "
                        "(must be less than %d)\n",
                        user_param.max_frags,
                        SEC_MAX_SG_TBL_ENTRIES);
        return -3;
    }
    
    if( user_param.payload_size >= PDCP_BUFFER_SIZE)
    {
        fprintf(stderr,"Invalid payload len %d "
                       "(must be less than %d)",
                        user_param.payload_size,
                        PDCP_BUFFER_SIZE);
        return -4;
    }

    return 0;
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
    
    while ((c = getopt (argc, argv, "s:d:f:p:hl")) != -1)
    {
        switch (c)
        {
            case 's':
                user_param.scenario = atoi(optarg);
                printf("Selected scenario %d\n",user_param.scenario);
                break;
            case 'd':
                user_param.direction = atoi(optarg);
                printf("Selected direction %d\n",user_param.direction);
                break;
            case 'f':
                user_param.max_frags = atoi(optarg);
                printf("Max # of fragments %d\n",user_param.max_frags);
                break;
            case 'p':
                user_param.payload_size = atoi(optarg);
                printf("Payload size %d\n",user_param.payload_size);
                break;
            case 'h':
                printf("Usage: %s \n"
                       "-s scenario\n"
                       "-d direction\n"
                       "-f max number of scatter gather frags\n"
                       "-p <max payload size>\n",argv[0]);
                return 0;
            default:
                abort();
        }
    }
    
    if( validate_params() )
    {
        fprintf(stderr,"Error in options! Please see %s -h\n",argv[0]);
        return 0;
    }
    else
    {
        set_test_params();
    }

    // Init FSL USMMGR
    g_usmmgr = fsl_usmmgr_init();
    if(g_usmmgr == NULL){
        perror("ERROR on fsl_usmmgr_init :");
        return -1;
    }

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
    // 2. Start worker threads
    /////////////////////////////////////////////////////////////////////
    ret_code = start_sec_worker_threads();
    if (ret_code != 0)
    {
        test_printf("start_sec_worker_threads returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 3. Stop worker threads
    /////////////////////////////////////////////////////////////////////
    ret_code = stop_sec_worker_threads();
    if (ret_code != 0)
    {
        test_printf("stop_sec_worker_threads returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 4. Cleanup SEC environment
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

