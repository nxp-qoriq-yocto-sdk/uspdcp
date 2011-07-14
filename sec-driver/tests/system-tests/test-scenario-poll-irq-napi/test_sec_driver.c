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
#include <sys/select.h>

#include "fsl_sec.h"
// for dma_mem library
#include "compat.h"
#include "test_sec_driver.h"
#include "test_sec_driver_test_vectors.h"

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

// The size of a PDCP input buffer.
// Consider the size of the input and output buffers provided to SEC driver for processing identical.
#define PDCP_BUFFER_SIZE   100

#ifdef SEC_HW_VERSION_4_4
#define IRQ_COALESCING_COUNT    10
#define IRQ_COALESCING_TIMER    100
#endif

#define JOB_RING_POLL_UNLIMITED -1
#define JOB_RING_POLL_LIMIT      10

// Alignment for input/output packets allocated from DMA-memory zone
#define BUFFER_ALIGNEMENT 32
#define BUFFER_SIZE       128
// Max length in bytes for a confidentiality /integrity key.
#define MAX_KEY_LENGTH    32


// Size in bytes required to be available for driver's use in input packet,
// BEFORE PDCP header start, when using PDCP control-plane with
// AES CMAC integrity check algorithm.
// I.e: packet offset should be at least 8. SEC driver will use the last 8 bytes
// before PDCP header starts, to calculate initialization data required for AES CMAC algorithm.
#define AES_CMAC_SCTRATCHPAD_PACKET_AREA_LENGHT 8

// Offset in input and output packet, where PDCP header starts
#define PACKET_OFFSET   3

// Length in bytes requried for MAC-I code generation,
// in case of PDCP control-plane.
#define MAC_I_LENGTH    4

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
    volatile pdcp_context_usage_t usage;
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

#ifdef VALIDATE_CONFORMITY
/** @brief Validate a processed packet against validation criteria.
 * Returns 1 if packet meets validation criteria.
 * Returns 0 if packet is incorrect.
 */
static int is_packet_valid(pdcp_context_t *pdcp_context,
                           const sec_packet_t *in_packet,
                           const sec_packet_t *out_packet,
                           uint32_t status);
#endif
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
static const sec_job_ring_descriptor_t *job_ring_descriptors = NULL;

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
static pthread_barrier_t th_barrier;

#ifndef PDCP_TEST_SCENARIO
#error "PDCP_TEST_SCENARIO is undefined!!!"
#endif


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

    test_printf("thread #%d:producer: release pdcp context id = %d\n", pdcp_context->thread_id, pdcp_context->id);

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

    memset(pdcp_context->input_buffers, 0, sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);
    memset(pdcp_context->output_buffers, 0, sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);
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
//    assert(&pdcp_context->input_buffers[pdcp_context->no_of_buffers_processed].buffer[0] ==
//            in_packet->address);
//    assert(pdcp_context->input_buffers[pdcp_context->no_of_buffers_processed].offset ==
//                in_packet->offset);
//    assert(PDCP_BUFFER_SIZE == in_packet->length);

//    assert(&pdcp_context->output_buffers[pdcp_context->no_of_buffers_processed].buffer[0] ==
//            out_packet->address);
//    assert(pdcp_context->output_buffers[pdcp_context->no_of_buffers_processed].offset ==
//                    out_packet->offset);
//    assert(PDCP_BUFFER_SIZE == out_packet->length);

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
        assert(status == SEC_STATUS_SUCCESS ||
               status == SEC_STATUS_HFN_THRESHOLD_REACHED ||
               status == SEC_STATUS_ERROR ||
               status == SEC_STATUS_MAC_I_CHECK_FAILED);

        if (pdcp_context->usage == PDCP_CONTEXT_MARKED_FOR_DELETION &&
            pdcp_context->no_of_buffers_processed == pdcp_context->no_of_buffers_to_process)
        {
            pdcp_context->usage = PDCP_CONTEXT_CAN_BE_DELETED;
        }
    }

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
    (*in_packet)->address = &(pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].buffer[0]);
    //in_packet->offset = pdcp_context->input_buffers[pdcp_context->no_of_used_buffers].offset;

    // Needed 8 bytes before actual start of PDCP packet, for PDCP control-plane + AES algo testing.
    (*in_packet)->offset = test_packet_offset;
    (*in_packet)->scatter_gather = SEC_CONTIGUOUS_BUFFER;

    assert(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].usage == PDCP_BUFFER_FREE);
    *out_packet = &(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].pdcp_packet);

    pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].usage = PDCP_BUFFER_USED;
    (*out_packet)->address = &(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].buffer[0]);

    //out_packet->offset = pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].offset;

    // Needed 8 bytes before actual start of PDCP packet, for PDCP control-plane + AES algo testing.
    (*out_packet)->offset = test_packet_offset;
    (*out_packet)->scatter_gather = SEC_CONTIGUOUS_BUFFER;

    // copy PDCP header
    memcpy((*in_packet)->address + (*in_packet)->offset, test_pdcp_hdr, sizeof(test_pdcp_hdr));
    // copy input data
    memcpy((*in_packet)->address + (*in_packet)->offset + PDCP_HEADER_LENGTH,
           test_data_in,
           sizeof(test_data_in));

    (*in_packet)->length = sizeof(test_data_in) + PDCP_HEADER_LENGTH + (*in_packet)->offset;
#ifdef TEST_PDCP_CONTROL_PLANE_DOUBLE_PASS_ENC
    // Need extra 4 bytes at end of input/output packet for MAC-I code, in case of PDCP control-plane packets
    (*in_packet)->length += MAC_I_LENGTH;
#endif
    // Need extra 4 bytes at end of input/output packet for MAC-I code, in case of PDCP control-plane packets
    // Need  extra 8 bytes at start of input packet  for Initialization Vector (IV) when testing
    // PDCP control-plane with AES CMAC algorithm, which is captured in 'offset' field.
    assert((*in_packet)->length <= PDCP_BUFFER_SIZE);

    (*out_packet)->length = sizeof(test_data_in) + PDCP_HEADER_LENGTH + (*out_packet)->offset;
#ifdef TEST_PDCP_CONTROL_PLANE_DOUBLE_PASS_ENC
    // Need extra 4 bytes at end of input/output packet for MAC-I code, in case of PDCP control-plane packets
    (*out_packet)->length += MAC_I_LENGTH;
#endif
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
    int ret;
    pdcp_context_t *pdcp_context = NULL;
    int i;

    // validate input params
    assert(ua_ctx_handle != NULL);
    assert(in_packet != NULL);
    assert(out_packet != NULL);

    if(status == SEC_STATUS_HFN_THRESHOLD_REACHED)
    {
        test_printf("HFN threshold reached for packet\n");
    }

    pdcp_context = (pdcp_context_t *)ua_ctx_handle;

    test_printf("\nthread #%d:consumer: sec_callback called for context_id = %d, "
            "no of buffers processed = %d, no of buffers to process = %d, "
            "status = %d, SEC error = 0x%x\n",
            (pdcp_context->thread_id + 1)%2,
            pdcp_context->id,
            pdcp_context->no_of_buffers_processed + 1,
            pdcp_context->no_of_buffers_to_process,
            status,
            error_info);

    if(error_info != 0)
    {
        // Buffers processing.
        // In this test application we will release the input and output buffers
        // Check also if all the buffers were received for a retiring context
        // and if so mark it to be deleted
        ret = release_pdcp_buffers(pdcp_context,
                (sec_packet_t*)in_packet,
                (sec_packet_t*)out_packet,
                status);
        assert(ret == 0);
        return SEC_RETURN_SUCCESS;

    }

#ifdef VALIDATE_CONFORMITY
    int test_failed = 0;
    // Check if packet is valid
    test_failed = !is_packet_valid(pdcp_context, in_packet, out_packet, status);

    if(test_failed)
    {
        test_printf("\nthread #%d:consumer: out packet INCORRECT!."
                " out pkt= ",
                (pdcp_context->thread_id + 1)%2);
        for(i = 0; i <  out_packet->length; i++)
        {
            test_printf("%02x ", out_packet->address[i]);
        }
        test_printf("\n");
        assert(0);
    }
    else
    {
        test_printf("\nthread #%d:consumer: packet CORRECT! out pkt = . ", (pdcp_context->thread_id + 1)%2);
        for(i = 0; i <  out_packet->length; i++)
        {
            test_printf("%02x ", out_packet->address[i]);
        }
        test_printf("\n");
    }
#else // #ifdef VALIDATE_CONFORMITY

    // Validation against test vector not done. Just print output packet
    test_printf("\nthread #%d:consumer: packet NOT validated! out pkt = . ", (pdcp_context->thread_id + 1)%2);
    for(i = 0; i <  out_packet->length; i++)
    {
        test_printf("%02x ", out_packet->address[i]);
    }
    test_printf("\n");
#endif

    // Buffers processing.
    // In this test application we will release the input and output buffers
    // Check also if all the buffers were received for a retiring context
    // and if so mark it to be deleted
    ret = release_pdcp_buffers(pdcp_context,
                               (sec_packet_t*)in_packet,
                               (sec_packet_t*)out_packet,
                               status);
    assert(ret == 0);

    return SEC_RETURN_SUCCESS;
}

static int get_results(uint8_t job_ring, int limit, uint32_t *packets_out)
{
    int ret = 0;
    uint32_t packets_retrieved = 0;
#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
    fd_set readfds;
    struct timeval tv;
    int irq_count;
#endif

    assert(limit != 0);
    assert(packets_out != NULL);
    *packets_out = 0;

#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
    FD_ZERO(&readfds);
    FD_SET(job_ring_descriptors[job_ring].job_ring_irq_fd, &readfds);

    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    select(job_ring_descriptors[job_ring].job_ring_irq_fd + 1, &readfds, NULL, NULL, &tv);
    if(FD_ISSET(job_ring_descriptors[job_ring].job_ring_irq_fd, &readfds))
    {

        // Now read the IRQ counter
        ret = read(job_ring_descriptors[job_ring].job_ring_irq_fd,
                   &irq_count,
                   4); // size of irq counter = sizeof(int)
        test_printf("Job ring %d. SEC IRQ received, doing hw polling. irq_count[%d]",
                    job_ring, irq_count);
    }
    else
    {
        test_printf("Job ring %d. Select timed out, no IRQ.\n", job_ring);
        return 0;
    }

    do
    {
#endif
        packets_retrieved = 0;
        ret = sec_poll_job_ring(job_ring_descriptors[job_ring].job_ring_handle, limit, &packets_retrieved);
        if(ret == SEC_PROCESSING_ERROR)
        {
            test_printf("sec_poll_job_ring:: Returned SEC_PROCESSING_ERROR on Job Ring %d. "
                    "Need to call sec_release() ! \n", job_ring);
            assert(ret != SEC_PROCESSING_ERROR);
        }
        if (ret != SEC_SUCCESS)
        {
            if(ret == SEC_PACKET_PROCESSING_ERROR)
            {
                test_printf("sec_poll_job_ring::Error %d when polling for SEC results on Job Ring %d.\n"
                        "Job ring restarted and can be used normally again.\n", ret, job_ring);
            }
            else
            {
                test_printf("sec_poll_job_ring::Error %d when polling for SEC results on Job Ring %d \n", ret, job_ring);
                return 1;
            }
        }
        test_printf("Job ring %d. Retrieved %d packets\n", job_ring, packets_retrieved);
        *packets_out += packets_retrieved;

        // validate that number of packets notified does not exceed limit when limit is > 0.
        assert(!((limit > 0) && (packets_retrieved > limit)));

#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
    }while((limit > 0) && (packets_retrieved == limit));
#endif
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
            test_printf("thread #%d:producer: delete PDCP context no %d -> packets in flight \n",
                    pdcp_context->thread_id, pdcp_context->id);
        }
        else if (ret == SEC_LAST_PACKET_IN_FLIGHT)
        {
            test_printf("thread #%d:producer: delete PDCP context no %d -> last packet in flight \n",
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
            test_printf("thread #%d:producer: sec_delete_pdcp_context return error %d for PDCP context no %d \n",
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

#ifdef VALIDATE_CONFORMITY
static int is_packet_valid(pdcp_context_t *pdcp_context,
                           const sec_packet_t *in_packet,
                           const sec_packet_t *out_packet,
                           uint32_t status)
{
    int test_pass = 1;
    int user_plane = test_user_plane;

    if(user_plane == PDCP_DATA_PLANE)
    {
        assert(in_packet->length == sizeof(test_data_in) + PDCP_HEADER_LENGTH + in_packet->offset);
        assert(out_packet->length == sizeof(test_data_out) + PDCP_HEADER_LENGTH + out_packet->offset);

        test_pass = (0 == memcmp(out_packet->address + out_packet->offset,
                                 test_pdcp_hdr,
                                 PDCP_HEADER_LENGTH) &&
                     0 == memcmp(out_packet->address + out_packet->offset + PDCP_HEADER_LENGTH,
                                 test_data_out,
                                 sizeof(test_data_out)));
    }
    else
    {
        // control plane context -> check if packet belong to last context which is data plane
        // -> then DO NOT validate packets!
        if(pdcp_context->pdcp_ctx_cfg_data.user_plane == PDCP_DATA_PLANE)
        {
            test_printf("Testing c-plane.Last PDCP context id %d is d-plane.Data plane packet NOT validated!",
                         pdcp_context->id);
            return 1;
        }

        // Status must be SUCCESS. When MAC-I validation failed,
        // status is set to #SEC_STATUS_MAC_I_CHECK_FAILED.
        test_pass = (status != SEC_STATUS_MAC_I_CHECK_FAILED && status != SEC_STATUS_ERROR);

#if (PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_ENC) || \
    (PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_ENC)

        // For SNOW F9 and AES CMAC, output data consists only of MAC-I code = 4 bytes
        test_pass = test_pass && (0 == memcmp(out_packet->address + out_packet->offset,
                                              test_data_out,
                                              sizeof(test_data_out)));
#elif (PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_DEC) || \
        (PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_DEC)
        // Status must be SUCCESS. When MAC-I validation failed,
        // status is set to #SEC_STATUS_MAC_I_CHECK_FAILED.
#else

        test_pass = test_pass && (0 == memcmp(out_packet->address + out_packet->offset,
                                              test_pdcp_hdr,
                                              PDCP_HEADER_LENGTH) &&
                                  0 == memcmp(out_packet->address + out_packet->offset + PDCP_HEADER_LENGTH,
                                              test_data_out,
                                              sizeof(test_data_out)));
#endif
    }
    return test_pass;
}
#endif //#ifdef VALIDATE_CONFORMITY

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


    // Initialize thread barrier.
    // The barrier is required in case of testing PDCP control-plane.
    // Because PDCP control-plane packets need to be double-passed through SEC engine,
    // New packets must be sent to trigger second processing stage for c-plane packets.
    // As such, last packets sent by this test will be data plane packets.
    // Threads must meet at the barrier before sending data-plane packets.
    ret = pthread_barrier_init(&th_barrier, NULL, THREADS_NUMBER);
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
    for (i = 0; i < THREADS_NUMBER; i++)
    {
        ret = pthread_join(threads[i], NULL);
        assert(ret == 0);
    }

    // double check that indeed the threads finished their work
    assert(no_of_used_pdcp_dl_contexts == 0);
    assert(no_of_used_pdcp_ul_contexts == 0);

    test_printf("thread main: all worker threads are stopped\n");

    ret = pthread_barrier_destroy(&th_barrier);
    assert(ret == 0);

    return 0;
}

static void* pdcp_thread_routine(void* config)
{
    thread_config_t *th_config_local = NULL;
    int ret = 0;
    unsigned int packets_received = 0;
    unsigned int total_packets_received = 0;
    unsigned int total_packets_sent = 0;
    pdcp_context_t *pdcp_context;
    int total_no_of_contexts_deleted = 0;
    int no_of_contexts_deleted = 0;
    int total_no_of_contexts_created = 0;
    int is_last_context = 0;

    th_config_local = (thread_config_t*)config;
    assert(th_config_local != NULL);

    test_printf("thread #%d:producer: start work, no of contexts to be created/deleted %d\n",
            th_config_local->tid, th_config_local->no_of_pdcp_contexts_to_test);

    // Create a number of configurable contexts and affine them to the producer JR, send a random
    // number of packets per each context
    while(total_no_of_contexts_created < th_config_local->no_of_pdcp_contexts_to_test)
    {
        if(total_no_of_contexts_created + 1 == th_config_local->no_of_pdcp_contexts_to_test)
        {
            is_last_context = 1;
        }
        ret = get_free_pdcp_context(th_config_local->pdcp_contexts,
                                    th_config_local->no_of_used_pdcp_contexts,
                                    &pdcp_context);
        assert(ret == 0);

        test_printf("thread #%d:producer: create pdcp context %d."
                    "nr of ctx created %d. nr of ctx to create %d.\n",
                    th_config_local->tid, pdcp_context->id,
                    total_no_of_contexts_created,
                    th_config_local->no_of_pdcp_contexts_to_test);

        pdcp_context->pdcp_ctx_cfg_data.notify_packet = &pdcp_ready_packet_handler;
        pdcp_context->pdcp_ctx_cfg_data.sn_size = test_sn_size;
        pdcp_context->pdcp_ctx_cfg_data.bearer = test_bearer;
        pdcp_context->pdcp_ctx_cfg_data.user_plane = test_user_plane;
        pdcp_context->pdcp_ctx_cfg_data.packet_direction = test_packet_direction;
        pdcp_context->pdcp_ctx_cfg_data.protocol_direction = test_protocol_direction;
        pdcp_context->pdcp_ctx_cfg_data.hfn = test_hfn;
        pdcp_context->pdcp_ctx_cfg_data.hfn_threshold = test_hfn_threshold;
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

        if(is_last_context)
        {
            if(pdcp_context->pdcp_ctx_cfg_data.user_plane == PDCP_CONTROL_PLANE)
            {
                pdcp_context->pdcp_ctx_cfg_data.user_plane = PDCP_DATA_PLANE;
                pdcp_context->pdcp_ctx_cfg_data.sn_size = SEC_PDCP_SN_SIZE_12;
                pdcp_context->pdcp_ctx_cfg_data.integrity_algorithm = SEC_ALG_NULL;
                pdcp_context->pdcp_ctx_cfg_data.integrity_key = NULL;
                pdcp_context->pdcp_ctx_cfg_data.integrity_key_len = 0;
            }
        }


        pdcp_context->thread_id = th_config_local->tid;

        // create a SEC context in SEC driver
        ret = sec_create_pdcp_context (job_ring_descriptors[th_config_local->producer_job_ring_id].job_ring_handle,
                                       &pdcp_context->pdcp_ctx_cfg_data,
                                       &pdcp_context->sec_ctx);
        if (ret != SEC_SUCCESS)
        {
            test_printf("thread #%d:producer: sec_create_pdcp_context return error %d for PDCP context no %d \n",
                    th_config_local->tid, ret, pdcp_context->id);
            assert(0);
        }

        total_no_of_contexts_created ++;

        // for the newly created context, send to SEC a random number of packets for processing
        sec_packet_t *in_packet;
        sec_packet_t *out_packet;

        if(is_last_context)
        {
            int counter;
            for(counter = 0; counter < 10; counter++)
            {
                do
                {
                    packets_received = 0;
                    // poll for responses
                    ret = get_results(th_config_local->consumer_job_ring_id,
                            JOB_RING_POLL_LIMIT,
                            &packets_received);
                    total_packets_received += packets_received;
                    usleep(10);
                }while(packets_received != 0);
            }

            test_printf("thread #%d:Now waiting on barrier\n", th_config_local->tid);
            ret = pthread_barrier_wait(&th_barrier);
            if(ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
            {
                printf("thread #%d:Error waiting on barrier\n", th_config_local->tid);
                pthread_exit(NULL);
            }
        }


        while (get_free_pdcp_buffer(pdcp_context, &in_packet, &out_packet) == 0)
        {
            // When testing PDCP c-plane, the last context tested MUST be
            // PDCP data plane. This will guarantee that the last c-plane packets stored
            // into driver's internal FIFO will be sent to SEC, without buffering any other
            // c-plane packets..because now we are sending data-plane packets.
#ifdef TEST_PDCP_CONTROL_PLANE_DOUBLE_PASS_ENC
            if(is_last_context)
            {
                in_packet->length -= MAC_I_LENGTH;
                out_packet->length -= MAC_I_LENGTH;
            }
#endif
            // if SEC process packet returns that the producer JR is full, do some polling
            // on the consumer JR until the producer JR has free entries.
            do
            {
                ret = sec_process_packet(pdcp_context->sec_ctx,
                        in_packet,
                        out_packet,
                        (ua_context_handle_t)pdcp_context);

                if(ret == SEC_JR_IS_FULL || ret == SEC_JOB_RING_RESET_IN_PROGRESS)
                {
                    packets_received = 0;
                    // wait while the producer JR is empty, and in the mean time do some
                    // polling on the consumer JR -> retrieve only #JOB_RING_POLL_LIMIT
                    // notifications if available.
                    ret = get_results(th_config_local->consumer_job_ring_id,
                            JOB_RING_POLL_LIMIT,
                            &packets_received);
                    total_packets_received += packets_received;
                    //assert(ret == 0);

                    usleep(10);
                }else if (ret != SEC_SUCCESS)
                {
                    test_printf("thread #%d:producer: sec_process_packet return error %d for PDCP context no %d \n",
                            th_config_local->tid, ret, pdcp_context->id);
                    assert(0);
                }
                else
                {
                    break;
                }
            }while(1);
            total_packets_sent++;
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

    test_printf("thread #%d: polling until all contexts are deleted\n", th_config_local->tid);

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
        total_packets_received += packets_received;

        // try to delete the contexts with packets in flight
        ret = delete_contexts(th_config_local->pdcp_contexts,
                              th_config_local->no_of_used_pdcp_contexts,
                              &no_of_contexts_deleted);
        assert(ret == 0);
        total_no_of_contexts_deleted += no_of_contexts_deleted;
        usleep(10);

    }while(total_no_of_contexts_deleted < total_no_of_contexts_created);


    // signal to main thread that the work is done
    th_config_local->work_done = 1;
    test_printf("thread #%d: work done, polling until exit signal is received\n", th_config_local->tid);

    // continue polling on the consumer job ring, in case the other worker thread
    // did not finish its work
    while(th_config_local->should_exit == 0)
    {
        ret = get_results(th_config_local->consumer_job_ring_id,
        		          JOB_RING_POLL_UNLIMITED,
        		          &packets_received);
        assert(ret == 0);
        total_packets_received += packets_received;
    }

    printf("thread #%d: Received %d packets. Sent %d packets. Exit\n",
           th_config_local->tid, total_packets_received, total_packets_sent);
    pthread_exit(NULL);
}

static int setup_sec_environment(void)
{
    int i = 0;
    int ret = 0;
    time_t seconds;

    printf("\nTesting scenario: %s\n", PDCP_TEST_SCENARIO_NAME);

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_POLL
    printf("SEC user-space driver working mode: SEC_NOTIFICATION_TYPE_POLL\n\n");
#elif SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_IRQ
    printf("SEC user-space driver working mode: SEC_NOTIFICATION_TYPE_IRQ\n\n");
#elif SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_NAPI
    printf("SEC user-space driver working mode: SEC_NOTIFICATION_TYPE_NAPI\n\n");
#endif

    /* Get value from system clock and use it for seed generation  */
    time(&seconds);
    srand((unsigned int) seconds);

    memset (pdcp_dl_contexts, 0, sizeof(pdcp_context_t) * MAX_PDCP_CONTEXT_NUMBER);
    memset (pdcp_ul_contexts, 0, sizeof(pdcp_context_t) * MAX_PDCP_CONTEXT_NUMBER);

    // map the physical memory
    ret = dma_mem_setup();
    if (ret != 0)
    {
        test_printf("dma_mem_setup failed with ret = %d\n", ret);
        return 1;
    }
	test_printf("dma_mem_setup: mapped virtual mem 0x%x to physical mem 0x%x\n", __dma_virt, DMA_MEM_PHYS);

    for (i = 0; i < MAX_PDCP_CONTEXT_NUMBER; i++)
    {
        pdcp_dl_contexts[i].id = i;
        // allocate input buffers from memory zone DMA-accessible to SEC engine
        pdcp_dl_contexts[i].input_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                             sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);
        // allocate output buffers from memory zone DMA-accessible to SEC engine
        pdcp_dl_contexts[i].output_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                              sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);

        pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                            MAX_KEY_LENGTH);
        pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                               MAX_KEY_LENGTH);
        // validate that the address of the freshly allocated buffer falls in the second memory are.
        assert (pdcp_dl_contexts[i].input_buffers != NULL);
        assert (pdcp_dl_contexts[i].output_buffers != NULL);
        assert (pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key != NULL);
        assert (pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key != NULL);

        assert((dma_addr_t)pdcp_dl_contexts[i].input_buffers >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_dl_contexts[i].output_buffers >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key >= DMA_MEM_SEC_DRIVER);

        memset(pdcp_dl_contexts[i].input_buffers, 0, sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);
        memset(pdcp_dl_contexts[i].output_buffers, 0, sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);

        pdcp_ul_contexts[i].id = i + MAX_PDCP_CONTEXT_NUMBER;
        // allocate input buffers from memory zone DMA-accessible to SEC engine
        pdcp_ul_contexts[i].input_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                             sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);
        // validate that the address of the freshly allocated buffer falls in the second memory are.
        pdcp_ul_contexts[i].output_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT, 
                                                              sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);

        pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                            MAX_KEY_LENGTH);
        pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                               MAX_KEY_LENGTH);
        // validate that the address of the freshly allocated buffer falls in the second memory are.
        assert (pdcp_ul_contexts[i].input_buffers != NULL);
        assert (pdcp_ul_contexts[i].output_buffers != NULL);
        assert (pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key != NULL);
        assert (pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key != NULL);

        assert((dma_addr_t)pdcp_ul_contexts[i].input_buffers >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_ul_contexts[i].output_buffers >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key >= DMA_MEM_SEC_DRIVER);


        memset(pdcp_ul_contexts[i].input_buffers, 0, sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);
        memset(pdcp_ul_contexts[i].output_buffers, 0, sizeof(buffer_t) * MAX_PACKET_NUMBER_PER_CTX);
    }

    //////////////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC user space driver requesting #JOB_RING_NUMBER Job Rings
    //////////////////////////////////////////////////////////////////////////////

    sec_config_data.memory_area = (void*)__dma_virt;
    assert(sec_config_data.memory_area != NULL);

    // Fill SEC driver configuration data
//    sec_config_data.work_mode = SEC_STARTUP_POLLING_MODE;
    sec_config_data.work_mode = SEC_STARTUP_INTERRUPT_MODE;
#ifdef SEC_HW_VERSION_4_4
    sec_config_data.irq_coalescing_count = IRQ_COALESCING_COUNT;
    sec_config_data.irq_coalescing_timer = IRQ_COALESCING_TIMER;
#endif

    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    if (ret != SEC_SUCCESS)
    {
        test_printf("sec_init::Error %d\n", ret);
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
    int ret = 0, i;

    // release SEC driver
    ret = sec_release();
    if (ret != 0)
    {
        test_printf("sec_release returned error\n");
        return 1;
    }
    test_printf("thread main: released SEC user space driver!!\n");

    for (i = 0; i < MAX_PDCP_CONTEXT_NUMBER; i++)
    {
        dma_mem_free(pdcp_dl_contexts[i].input_buffers,  BUFFER_SIZE * MAX_PACKET_NUMBER_PER_CTX);
        dma_mem_free(pdcp_dl_contexts[i].output_buffers,  BUFFER_SIZE * MAX_PACKET_NUMBER_PER_CTX);

        dma_mem_free(pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key, MAX_KEY_LENGTH);
        dma_mem_free(pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key, MAX_KEY_LENGTH);

        dma_mem_free(pdcp_ul_contexts[i].input_buffers,  BUFFER_SIZE * MAX_PACKET_NUMBER_PER_CTX);
        dma_mem_free(pdcp_ul_contexts[i].output_buffers,  BUFFER_SIZE * MAX_PACKET_NUMBER_PER_CTX);

        dma_mem_free(pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key, MAX_KEY_LENGTH);
        dma_mem_free(pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key, MAX_KEY_LENGTH);

    }
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
        test_printf("setup_sec_environment returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 2. Start worker threads
    /////////////////////////////////////////////////////////////////////
    ret = start_sec_worker_threads();
    if (ret != 0)
    {
        test_printf("start_sec_worker_threads returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 3. Stop worker threads
    /////////////////////////////////////////////////////////////////////
    ret = stop_sec_worker_threads();
    if (ret != 0)
    {
        test_printf("stop_sec_worker_threads returned error\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 4. Cleanup SEC environment
    /////////////////////////////////////////////////////////////////////
    ret = cleanup_sec_environment();
    if (ret != 0)
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

