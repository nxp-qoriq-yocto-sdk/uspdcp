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

#include "fsl_sec.h"
// for dma_mem library
#include "compat.h"

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

// The number of PDCP contexts used
#define MAX_PDCP_CONTEXT_NUMBER         15
#define MIN_PDCP_CONTEXT_NUMBER         5

// The size of a PDCP input buffer.
// Consider the size of the input and output buffers provided to SEC driver for processing identical.
#define PDCP_BUFFER_SIZE   50

// The maximum number of packets processed per context
// This test application will process a random number of packets per context ranging
// from a minimum to a maximum value.
#define MAX_PACKET_NUMBER_PER_CTX   10
#define MIN_PACKET_NUMBER_PER_CTX   1

#ifdef SEC_HW_VERSION_4_4

#define IRQ_COALESCING_COUNT    10
#define IRQ_COALESCING_TIMER    100

#endif

#define JOB_RING_POLL_UNLIMITED -1
#define JOB_RING_POLL_LIMIT      5

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

// TODO: allocate buffers from DMA-mem pool
//    buffer_t input_buffers[MAX_PACKET_NUMBER_PER_CTX];
//    buffer_t output_buffers[MAX_PACKET_NUMBER_PER_CTX];
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
}thread_config_t;

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
static void* pdcp_thread_routine(void*);


/** @brief Poll a specified SEC job ring for results */
static int get_results(uint8_t job_ring, int limit, uint32_t *out_packets);

/** @brief Get a free PDCP context from the pool of UA contexts.
 *  For simplicity, the pool is implemented as an array. */
static int get_free_pdcp_context(pdcp_context_t * pdcp_contexts,
                                 int * no_of_used_pdcp_contexts,
                                 pdcp_context_t ** pdcp_context);

/** @brief Release a PDCP context in the pool of UA contexts.
 *  For simplicity, the pool is implemented as an array.*/
static void release_pdcp_context(int * no_of_used_pdcp_contexts, pdcp_context_t * pdcp_context);

/** @brief Release the PDCP input and output buffers in the pool of buffers
 *  of a certain context.
 *  For simplicity, the pool is implemented as an array and is defined per context.
 *
 *  If status is #SEC_STATUS_LAST_OVERDUE mark the context "can be deleted" */
static int release_pdcp_buffers(pdcp_context_t * pdcp_context,
                                sec_packet_t *in_packet,
                                sec_packet_t *out_packet,
                                sec_status_t status);

/** @brief Get free PDCP input and output buffers from the pool of buffers
 *  of a certain context.
 *  For simplicity, the pool is implemented as an array and is defined per context. */
static int get_free_pdcp_buffer(pdcp_context_t * pdcp_context,
                                sec_packet_t *in_packet,
                                sec_packet_t *out_packet);

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
static int pdcp_ready_packet_handler (sec_packet_t *in_packet,
                                      sec_packet_t *out_packet,
                                      ua_context_handle_t ua_ctx_handle,
                                      uint32_t status,
                                      uint32_t error_info);

/** @brief Try to delete a context.
 *  - first try to delete the SEC context in SEC driver
 *  - if success is returned, free also the PDCP context of UA
 *  - if packets in flight is returned, mark the PDCP context of UA to be deleted later
 *    when all the responses were received.
 */
static int delete_context(pdcp_context_t * pdcp_context, int *no_of_used_pdcp_contexts);

/** @brief Try to delete all contexts not deleted yet.
 *  This function calls delete_context() for each context from the pool of contexts
 *  allocated to a worker thread.
 */
static int delete_contexts(pdcp_context_t * pdcp_contexts,
                           int *no_of_used_pdcp_contexts,
                           int* contexts_deleted);
/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
// configuration data for SEC driver
static sec_config_t sec_config_data;

// job ring handles provided by SEC driver
static sec_job_ring_descriptor_t *job_ring_descriptors = NULL;

// UA pool of PDCP contexts for UL and DL.
// For simplicity, use an array of contexts and a mutex to synchronize access to it.
static pdcp_context_t pdcp_ul_contexts[MAX_PDCP_CONTEXT_NUMBER];
static int no_of_used_pdcp_ul_contexts = 0;

static pdcp_context_t pdcp_dl_contexts[MAX_PDCP_CONTEXT_NUMBER];
static int no_of_used_pdcp_dl_contexts = 0;

// There are 2 threads: one thread handles PDCP UL and one thread handles PDCP DL
// One thread is producer on JR1 and consumer on JR2. The other thread is producer on JR2
// and consumer on JR1.
static thread_config_t th_config[THREADS_NUMBER];
static pthread_t threads[THREADS_NUMBER];

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static int get_free_pdcp_context(pdcp_context_t * pdcp_contexts,
                                 int * no_of_used_pdcp_contexts,
                                 pdcp_context_t ** pdcp_context)
{
    int i = 0;
    int found = 0;

    assert(pdcp_context != NULL);
    assert(pdcp_contexts != NULL);
    assert(no_of_used_pdcp_contexts != NULL);

    // check if there are free contexts
    if (*no_of_used_pdcp_contexts >= MAX_PDCP_CONTEXT_NUMBER)
    {
        // no free contexts available
        return 2;
    }

    for (i = 0; i < MAX_PDCP_CONTEXT_NUMBER; i++)
    {
        if (pdcp_contexts[i].usage == PDCP_CONTEXT_FREE)
        {
            pdcp_contexts[i].usage = PDCP_CONTEXT_USED;

            (*no_of_used_pdcp_contexts)++;

            found = 1;
            break;
        }
    }
    assert(found == 1);

    // Configure this PDCP context with a random number of buffers to process for test
    // The random number will range between MIN_PACKET_NUMBER_PER_CTX and MAX_PACKET_NUMBER_PER_CTX
    pdcp_contexts[i].no_of_buffers_to_process =
            MIN_PACKET_NUMBER_PER_CTX + rand() % (MAX_PACKET_NUMBER_PER_CTX - MIN_PACKET_NUMBER_PER_CTX + 1);
    assert(pdcp_contexts[i].no_of_buffers_to_process >= MIN_PACKET_NUMBER_PER_CTX &&
           pdcp_contexts[i].no_of_buffers_to_process <= MAX_PACKET_NUMBER_PER_CTX);

    // return the context chosen
    *pdcp_context = &(pdcp_contexts[i]);

    return 0;
}

static void release_pdcp_context(int * no_of_used_pdcp_contexts, pdcp_context_t * pdcp_context)
{
    assert(pdcp_context != NULL);
    assert(no_of_used_pdcp_contexts != NULL);

    printf("thread #%d:producer: release pdcp context id = %d\n", pdcp_context->thread_id, pdcp_context->id);

    // there should be at least one pdcp context in the pool of contexts
    assert(*no_of_used_pdcp_contexts > 0);
    // decrement the number of used PDCP contexts
    (*no_of_used_pdcp_contexts)--;

    // the PDCP context is released only after all the buffers were received
    // back from sec_driver
    assert(pdcp_context->usage == PDCP_CONTEXT_USED || pdcp_context->usage == PDCP_CONTEXT_CAN_BE_DELETED);
    assert(pdcp_context->no_of_used_buffers == pdcp_context->no_of_buffers_to_process);
    assert(pdcp_context->no_of_buffers_processed == pdcp_context->no_of_buffers_to_process);

    pdcp_context->usage = PDCP_CONTEXT_FREE;
    pdcp_context->job_ring = NULL;
    pdcp_context->sec_ctx = NULL;

    memset(pdcp_context->input_buffers, 0, sizeof(pdcp_context->input_buffers));
    memset(pdcp_context->output_buffers, 0, sizeof(pdcp_context->output_buffers));
    pdcp_context->no_of_buffers_to_process = 0;
    pdcp_context->no_of_buffers_processed = 0;
    pdcp_context->no_of_used_buffers = 0;
}

static int release_pdcp_buffers(pdcp_context_t * pdcp_context,
                                sec_packet_t *in_packet,
                                sec_packet_t *out_packet,
                                sec_status_t status)
{
    assert(pdcp_context != NULL);
    assert(in_packet != NULL);
    assert(out_packet != NULL);

    // Validate the order of the buffers release
    // The order in which the buffers are relased must be the same with the order
    // in which the buffers were submitted to SEC driver for processing.
//    assert((dma_addr_t)&pdcp_context->input_buffers[pdcp_context->no_of_buffers_processed].buffer[0] ==
//            in_packet->address);
//    assert(pdcp_context->input_buffers[pdcp_context->no_of_buffers_processed].offset ==
//                in_packet->offset);
    assert(PDCP_BUFFER_SIZE == in_packet->length);

//    assert((dma_addr_t)&pdcp_context->output_buffers[pdcp_context->no_of_buffers_processed].buffer[0] ==
//            out_packet->address);
//    assert(pdcp_context->output_buffers[pdcp_context->no_of_buffers_processed].offset ==
//                    out_packet->offset);
    assert(PDCP_BUFFER_SIZE == out_packet->length);

    // mark the input buffer free
    assert(pdcp_context->input_buffers[pdcp_context->no_of_buffers_processed].usage == PDCP_BUFFER_USED);
    pdcp_context->input_buffers[pdcp_context->no_of_buffers_processed].usage = PDCP_BUFFER_FREE;
    pdcp_context->input_buffers[pdcp_context->no_of_buffers_processed].offset = 0;
    memset(pdcp_context->input_buffers[pdcp_context->no_of_buffers_processed].buffer, 0, PDCP_BUFFER_SIZE);

    // mark the output buffer free
    assert(pdcp_context->output_buffers[pdcp_context->no_of_buffers_processed].usage == PDCP_BUFFER_USED);
    pdcp_context->output_buffers[pdcp_context->no_of_buffers_processed].usage = PDCP_BUFFER_FREE;
    pdcp_context->output_buffers[pdcp_context->no_of_buffers_processed].offset = 0;
    memset(pdcp_context->output_buffers[pdcp_context->no_of_buffers_processed].buffer, 0, PDCP_BUFFER_SIZE);

    //increment the number of buffers processed counter (which acts as a consumer index for the array of buffers)
    pdcp_context->no_of_buffers_processed++;

    // Mark the context to be deleted if the last overdue packet
    // was received.
    if (status == SEC_STATUS_LAST_OVERDUE)
    {
        assert(pdcp_context->usage == PDCP_CONTEXT_MARKED_FOR_DELETION ||
        pdcp_context->usage == PDCP_CONTEXT_MARKED_FOR_DELETION_LAST_IN_FLIGHT_PACKET);

        pdcp_context->usage = PDCP_CONTEXT_CAN_BE_DELETED;
    }
    else if (status == SEC_STATUS_OVERDUE)
    {
        // validate the UA's context's status
        assert(pdcp_context->usage == PDCP_CONTEXT_MARKED_FOR_DELETION);
    }
    else
    {
        // the stub implementation does not return an error status
        assert(status == SEC_STATUS_SUCCESS);
        if (pdcp_context->usage == PDCP_CONTEXT_MARKED_FOR_DELETION &&
            pdcp_context->no_of_buffers_processed == pdcp_context->no_of_buffers_to_process)
        {
            pdcp_context->usage = PDCP_CONTEXT_CAN_BE_DELETED;
        }
    }

    return 0;

}

static int get_free_pdcp_buffer(pdcp_context_t * pdcp_context,
                                sec_packet_t *in_packet,
                                sec_packet_t *out_packet)
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
    pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].usage = PDCP_BUFFER_USED;
    in_packet->address = (packet_addr_t)&pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].buffer[0];
//    in_packet->offset = pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].offset;
    in_packet->offset = 15;
    in_packet->length = PDCP_BUFFER_SIZE;
    in_packet->scatter_gather = SEC_CONTIGUOUS_BUFFER;

    assert(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].usage == PDCP_BUFFER_FREE);
    pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].usage = PDCP_BUFFER_USED;
    out_packet->address = (packet_addr_t)&pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].buffer[0];
    out_packet->offset = pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].offset;
    out_packet->length = PDCP_BUFFER_SIZE;
    out_packet->scatter_gather = SEC_CONTIGUOUS_BUFFER;

    pdcp_context->no_of_used_buffers++;

    return 0;
}

static int pdcp_ready_packet_handler (sec_packet_t *in_packet,
                                      sec_packet_t *out_packet,
                                      ua_context_handle_t ua_ctx_handle,
                                      uint32_t status,
                                      uint32_t error_info)
{
    int ret;
    pdcp_context_t *pdcp_context = NULL;

    // validate input params
    assert(ua_ctx_handle != NULL);
    assert(in_packet != NULL);
    assert(out_packet != NULL);

    // the stub implementation for SEC driver never returns a status error
    assert(status != SEC_STATUS_ERROR);

    pdcp_context = (pdcp_context_t *)ua_ctx_handle;

    printf("thread #%d:consumer: sec_callback called for context_id = %d, "
//            "context usage = %d, no of buffers processed = %d, no of buffers to process = %d, "
            "status = %d || in buf.offset = %d\n",
            (pdcp_context->thread_id + 1)%2,
            pdcp_context->id,
  //          pdcp_context->usage,
  //          pdcp_context->no_of_buffers_processed + 1,
  //          pdcp_context->no_of_buffers_to_process,
            status,
            in_packet->offset);

    // Buffers processing.
    // In this test application we will release the input and output buffers
    // Check also if all the buffers were received for a retiring context
    // and if so mark it to be deleted
    ret = release_pdcp_buffers(pdcp_context, in_packet, out_packet, status);
    assert(ret == 0);

    return SEC_RETURN_SUCCESS;
}

static int get_results(uint8_t job_ring, int limit, uint32_t *packets_out)
{
    int ret = 0;

    assert(limit != 0);
    assert(packets_out != NULL);

    ret = sec_poll_job_ring(job_ring_descriptors[job_ring].job_ring_handle, limit, packets_out);
    if (ret != SEC_SUCCESS)
    {
        printf("sec_poll_job_ring::Error %d when polling for SEC results on Job Ring %d \n", ret, job_ring);
        return 1;
    }
    // validate that number of packets notified does not exceed limit when limit is > 0.
    assert(!((limit > 0) && (*packets_out > limit)));

    return 0;
}

static int delete_context(pdcp_context_t * pdcp_context, int *no_of_used_pdcp_contexts)
{
    int ret;

    assert(pdcp_context != NULL);
    assert(no_of_used_pdcp_contexts != NULL);

    // Try to delete the context from SEC driver and also
    // release the UA's context

    // context is in use and it was not yet removed from SEC driver
    if (pdcp_context->usage == PDCP_CONTEXT_USED)
    {
        // mark the context for deletion
        pdcp_context->usage = PDCP_CONTEXT_MARKED_FOR_DELETION;

        // try to delete the context from SEC driver
        ret = sec_delete_pdcp_context(pdcp_context->sec_ctx);
        if (ret == SEC_PACKETS_IN_FLIGHT)
        {
            printf("thread #%d:producer: delete PDCP context no %d -> packets in flight \n",
                    pdcp_context->thread_id, pdcp_context->id);
        }
        else if (ret == SEC_LAST_PACKET_IN_FLIGHT)
        {
            printf("thread #%d:producer: delete PDCP context no %d -> last packet in flight \n",
                    pdcp_context->thread_id, pdcp_context->id);
        }
        else if (ret == SEC_SUCCESS)
        {
            // context was successfully removed from SEC driver
            // do the same with the UA's context
            pdcp_context->usage = PDCP_CONTEXT_CAN_BE_DELETED;
            release_pdcp_context(no_of_used_pdcp_contexts, pdcp_context);
            // return 1 if context was deleted both in SEC driver and UA
            return 1;
        }
        else
        {
            printf("thread #%d:producer: sec_delete_pdcp_context return error %d for PDCP context no %d \n",
                    pdcp_context->thread_id, ret, pdcp_context->id);
            // the stub implementation of SEC driver should not return an error
            assert(0);
        }
    }
    // if context was marked for deletion and the last overdue packet was
    // received, we can release the context and reuse it
    else if (pdcp_context->usage == PDCP_CONTEXT_CAN_BE_DELETED)
    {
        // all overdue packets were received and freed
        // so we can reuse the context -> will be marked as free
        release_pdcp_context(no_of_used_pdcp_contexts, pdcp_context);
        // return 1 if context was deleted both in SEC driver and UA
        return 1;
    }
    // if context was marked for deletion, wait for the last overdue
    // packet to be received before deleting it.
    else if (pdcp_context->usage == PDCP_CONTEXT_MARKED_FOR_DELETION)
    {
        // nothing to do here
        // wait for the last overdue packet to be received
    }
    else if (pdcp_context->usage == PDCP_CONTEXT_MARKED_FOR_DELETION_LAST_IN_FLIGHT_PACKET)
    {
        // nothing to do here
        // wait for the last packet to be received with status SUCCESS
    }
    else
    {
        // nothing to do here
        // double check that the only possible value of usage on the else
        // branch is PDCP_CONTEXT_FREE
        assert(pdcp_context->usage == PDCP_CONTEXT_FREE);
    }
    // context was not deleted, it was marked for deletion
    return 0;
}

static int delete_contexts(pdcp_context_t * pdcp_contexts, int *no_of_used_pdcp_contexts, int* contexts_deleted)
{
    int i;

    assert(pdcp_contexts != NULL);
    assert(no_of_used_pdcp_contexts != NULL);
    assert(contexts_deleted != NULL);

    *contexts_deleted = 0;

    // check if there are used contexts
    if (*no_of_used_pdcp_contexts == 0)
    {
        // all contexts are free, nothing to do here
        return 0;
    }

    for (i = 0; i < MAX_PDCP_CONTEXT_NUMBER; i++)
    {
        // Try an delete the contexts and count the total number of contexts deleted.
        // delete_context returns 0 if the context was not deleted, and 1 if it was deleted.
        *contexts_deleted += delete_context(&pdcp_contexts[i], no_of_used_pdcp_contexts);
    }
    return 0;
}

static int start_sec_worker_threads(void)
{
    int ret = 0;

    // this scenario handles only 2 JRs and 2 worker threads
    assert (JOB_RING_NUMBER == 2);
    assert (THREADS_NUMBER == 2);

    // start PDCP UL thread
    th_config[0].tid = 0;
    // PDCP UL thread is consumer on JR ID 0 and producer on JR ID 1
    th_config[0].consumer_job_ring_id = 0;
    th_config[0].producer_job_ring_id = 1;
    th_config[0].pdcp_contexts = &pdcp_ul_contexts[0];
    th_config[0].no_of_used_pdcp_contexts = &no_of_used_pdcp_ul_contexts;
    th_config[0].no_of_pdcp_contexts_to_test =
            MIN_PDCP_CONTEXT_NUMBER + rand()% (MAX_PDCP_CONTEXT_NUMBER - MIN_PDCP_CONTEXT_NUMBER + 1);
    assert(th_config[0].no_of_pdcp_contexts_to_test >= MIN_PDCP_CONTEXT_NUMBER &&
            th_config[0].no_of_pdcp_contexts_to_test <= MAX_PDCP_CONTEXT_NUMBER);
    th_config[0].work_done = 0;
    th_config[0].should_exit = 0;
    ret = pthread_create(&threads[0], NULL, &pdcp_thread_routine, (void*)&th_config[0]);
    assert(ret == 0);

    // start PDCP DL thread
    th_config[1].tid = 1;
    // PDCP DL thread is consumer on JR ID 1 and producer on JR ID 0
    th_config[1].consumer_job_ring_id = 1;
    th_config[1].producer_job_ring_id = 0;
    th_config[1].pdcp_contexts = &pdcp_dl_contexts[0];
    th_config[1].no_of_used_pdcp_contexts = &no_of_used_pdcp_dl_contexts;
    th_config[1].no_of_pdcp_contexts_to_test =
            MIN_PDCP_CONTEXT_NUMBER + rand()% (MAX_PDCP_CONTEXT_NUMBER - MIN_PDCP_CONTEXT_NUMBER + 1);
    assert(th_config[1].no_of_pdcp_contexts_to_test >= MIN_PDCP_CONTEXT_NUMBER &&
            th_config[1].no_of_pdcp_contexts_to_test <= MAX_PDCP_CONTEXT_NUMBER);
    th_config[1].work_done = 0;
    th_config[1].should_exit = 0;
    ret = pthread_create(&threads[1], NULL, &pdcp_thread_routine, (void*)&th_config[1]);
    assert(ret == 0);

    return 0;
}

static int stop_sec_worker_threads(void)
{
    int i = 0;
    int ret = 0;

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
    for (i = 0; i < JOB_RING_NUMBER; i++)
    {
        ret = pthread_join(threads[i], NULL);
        assert(ret == 0);
    }

    // double check that indeed the threads finished their work
    assert(no_of_used_pdcp_dl_contexts == 0);
    assert(no_of_used_pdcp_ul_contexts == 0);

    printf("thread main: all worker threads are stopped\n");
    return 0;
}

static void* pdcp_thread_routine(void* config)
{
    thread_config_t *th_config_local = NULL;
    int ret = 0;
    unsigned int packets_received = 0;
    pdcp_context_t *pdcp_context;

    int total_no_of_contexts_deleted = 0;
    int no_of_contexts_deleted = 0;
    int total_no_of_contexts_created = 0;

    th_config_local = (thread_config_t*)config;
    assert(th_config_local != NULL);

    printf("thread #%d:producer: start work, no of contexts to be created/deleted %d\n",
            th_config_local->tid, th_config_local->no_of_pdcp_contexts_to_test);

    // Create a number of configurable contexts and affine them to the producer JR, send a random
    // number of packets per each context
    while(total_no_of_contexts_created < th_config_local->no_of_pdcp_contexts_to_test)
    {
        ret = get_free_pdcp_context(th_config_local->pdcp_contexts,
                                    th_config_local->no_of_used_pdcp_contexts,
                                    &pdcp_context);
        assert(ret == 0);

        printf("thread #%d:producer: create pdcp context %d\n", th_config_local->tid, pdcp_context->id);
        pdcp_context->pdcp_ctx_cfg_data.notify_packet = &pdcp_ready_packet_handler;
        pdcp_context->thread_id = th_config_local->tid;

        // create a SEC context in SEC driver
        ret = sec_create_pdcp_context (job_ring_descriptors[th_config_local->producer_job_ring_id].job_ring_handle,
                                       &pdcp_context->pdcp_ctx_cfg_data,
                                       &pdcp_context->sec_ctx);
        if (ret != SEC_SUCCESS)
        {
            printf("thread #%d:producer: sec_create_pdcp_context return error %d for PDCP context no %d \n",
                    th_config_local->tid, ret, pdcp_context->id);
            assert(0);
        }

        total_no_of_contexts_created ++;

        // for the newly created context, send to SEC a random number of packets for processing
        sec_packet_t in_packet;
        sec_packet_t out_packet;


        while (get_free_pdcp_buffer(pdcp_context, &in_packet, &out_packet) == 0)
        {
//            in_packet.offset = total_no_of_contexts_created;

            // if SEC process packet returns that the producer JR is full, do some polling
            // on the consumer JR until the producer JR has free entries.
            while (sec_process_packet(pdcp_context->sec_ctx,
                                     &in_packet,
                                     &out_packet,
                                     (ua_context_handle_t)pdcp_context) == SEC_JR_IS_FULL)
            {
            	// wait while the producer JR is empty, and in the mean time do some
                // polling on the consumer JR -> retrieve only 5 notifications if available
                ret = get_results(th_config_local->consumer_job_ring_id,
                		          JOB_RING_POLL_LIMIT,
                		          &packets_received);
                assert(ret == 0);
            };
            if (ret != SEC_SUCCESS)
            {
                printf("thread #%d:producer: sec_process_packet return error %d for PDCP context no %d \n",
                        th_config_local->tid, ret, pdcp_context->id);
                assert(0);
            }
        }


        // Now remove only the contexts with an even id.
        // This way two scenarios are tested:
        // - remove a context with packets in flight
        // - remove a context with no packets in flight
        if (total_no_of_contexts_created % 2 == 0)
        {
            // Remove the context now, if packets in flight the context will not be removed yet
            // Also count the total number of contexts deleted (delete_context returns 0 if the context
            // was not deleted, and 1 if it was deleted.)
            ret = delete_context(pdcp_context, th_config_local->no_of_used_pdcp_contexts);
            total_no_of_contexts_deleted += ret;
        }
    }

    printf("thread #%d: polling until all contexts are deleted\n", th_config_local->tid);

    // Poll the consumer's JR for already processed packets and try to delete
    // the contexts marked as deleted (the contexts will be deleted if all
    // the packets in flight were notified to UA with the OVERDUE && LAST_OVERDUE status)
    do
    {
        // poll the consumer JR
        ret = get_results(th_config_local->consumer_job_ring_id,
        		          JOB_RING_POLL_UNLIMITED,
        		          &packets_received);
        assert(ret == 0);

        // try to delete the contexts with packets in flight
        ret = delete_contexts(th_config_local->pdcp_contexts,
                              th_config_local->no_of_used_pdcp_contexts,
                              &no_of_contexts_deleted);
        assert(ret == 0);
        total_no_of_contexts_deleted += no_of_contexts_deleted;
    }while(total_no_of_contexts_deleted < total_no_of_contexts_created);


    // signal to main thread that the work is done
    th_config_local->work_done = 1;
    printf("thread #%d: work done, polling until exit signal is received\n", th_config_local->tid);

    // continue polling on the consumer job ring, in case the other worker thread
    // did not finish its work
    while(th_config_local->should_exit == 0)
    {
        ret = get_results(th_config_local->consumer_job_ring_id,
        		          JOB_RING_POLL_UNLIMITED,
        		          &packets_received);
        assert(ret == 0);
    }

    printf("thread #%d: exit\n", th_config_local->tid);
    pthread_exit(NULL);
}

static int setup_sec_environment(void)
{
    int i = 0;
    int ret = 0;
    time_t seconds;

    /* Get value from system clock and use it for seed generation  */
    time(&seconds);
    srand((unsigned int) seconds);

    memset (pdcp_dl_contexts, 0, sizeof(pdcp_context_t) * MAX_PDCP_CONTEXT_NUMBER);
    memset (pdcp_ul_contexts, 0, sizeof(pdcp_context_t) * MAX_PDCP_CONTEXT_NUMBER);
    for (i = 0; i < MAX_PDCP_CONTEXT_NUMBER; i++)
    {
        pdcp_dl_contexts[i].id = i;
        pdcp_ul_contexts[i].id = i + MAX_PDCP_CONTEXT_NUMBER;
    }

    //////////////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC user space driver requesting #JOB_RING_NUMBER Job Rings
    //////////////////////////////////////////////////////////////////////////////

    // map the physical memory
    ret = dma_mem_setup();
    if (ret != 0)
    {
        printf("dma_mem_setup failed with ret = %d\n", ret);
        return 1;
    }
	printf("dma_mem_setup: mapped virtual mem 0x%x to physical mem 0x%x\n", __dma_virt, DMA_MEM_PHYS);


    sec_config_data.memory_area = (void*)__dma_virt;
    assert(sec_config_data.memory_area != NULL);

    // Fill SEC driver configuration data
    sec_config_data.ptov = &dma_mem_ptov;
    sec_config_data.vtop = &dma_mem_vtop;
    sec_config_data.work_mode = SEC_STARTUP_POLLING_MODE;
#ifdef SEC_HW_VERSION_4_4
    sec_config_data.irq_coalescing_count = IRQ_COALESCING_COUNT;
    sec_config_data.irq_coalescing_timer = IRQ_COALESCING_TIMER;
#endif

    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    if (ret != SEC_SUCCESS)
    {
        printf("sec_init::Error %d\n", ret);
        return 1;
    }
    printf("thread main: initialized SEC user space driver\n");

    for (i = 0; i < JOB_RING_NUMBER; i++)
    {
        assert(job_ring_descriptors[i].job_ring_handle != NULL);
        assert(job_ring_descriptors[i].job_ring_irq_fd != 0);
    }

    return 0;
}

static int cleanup_sec_environment(void)
{
    int ret = 0;

    // release SEC driver
    ret = sec_release();
    if (ret != 0)
    {
        printf("sec_release returned error\n");
        return 1;
    }
    printf("thread main: released SEC user space driver\n");

	// unmap the physical memory
	ret = dma_mem_release();
	if (ret != 0)
	{
		return 1;
	}

    return 0;
}
/*==================================================================================================
                                        GLOBAL FUNCTIONS
=================================================================================================*/

int main(void)
{
    int ret = 0;

    /////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC environment
    /////////////////////////////////////////////////////////////////////
    ret = setup_sec_environment();
    if (ret != 0)
    {
        printf("setup_sec_environment returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 2. Start worker threads
    /////////////////////////////////////////////////////////////////////
    ret = start_sec_worker_threads();
    if (ret != 0)
    {
        printf("start_sec_worker_threads returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 3. Stop worker threads
    /////////////////////////////////////////////////////////////////////
    ret = stop_sec_worker_threads();
    if (ret != 0)
    {
        printf("stop_sec_worker_threads returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 4. Cleanup SEC environment
    /////////////////////////////////////////////////////////////////////
    ret = cleanup_sec_environment();
    if (ret != 0)
    {
        printf("cleanup_sec_environment returned error\n");
        return 1;
    }

    return 0;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif

