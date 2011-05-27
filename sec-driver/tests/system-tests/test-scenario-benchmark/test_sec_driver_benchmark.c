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
// for dma_mem library
#include "compat.h"

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

#define PDCP_TEST_SNOW_F8_ENC   0
#define PDCP_TEST_SNOW_F8_DEC   1
#define PDCP_TEST_SNOW_F9_ENC   2
#define PDCP_TEST_SNOW_F9_DEC   3
#define PDCP_TEST_AES_CTR_ENC   4
#define PDCP_TEST_AES_CTR_DEC   5
#define PDCP_TEST_AES_CMAC_ENC  6
#define PDCP_TEST_AES_CMAC_DEC  7

//////////////////////////////////////////////////////////////////////////
// !!!!!!!!!!!!!!!!!       IMPORTANT !!!!!!!!!!!!!!!!
//Select one and only one algorithm at a time, from below
//////////////////////////////////////////////////////////////////////////

// Ciphering
#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F8_ENC
// Deciphering
//#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F8_DEC

// Authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F9_ENC
// Authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F9_DEC

// Ciphering
//#define PDCP_TEST_SCENARIO  PDCP_TEST_AES_CTR_ENC
// Deciphering
//#define PDCP_TEST_SCENARIO  PDCP_TEST_AES_CTR_DEC

// Authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_AES_CMAC_ENC
// Authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_AES_CMAC_DEC




#define test_printf(format, ...)
#define profile_printf(format, ...)
//#define test_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__) 
//#define profile_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__) 



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

// The number of PDCP contexts used. Each DRB will have 2 PDCP contexts, one for each direction.
#define PDCP_CONTEXT_NUMBER         (UE_NUMBER * DRB_PER_UE * 2)
//#define PDCP_CONTEXT_NUMBER         1

// The size of a PDCP input buffer.
// Consider the size of the input and output buffers provided to SEC driver for processing identical.
#define PDCP_BUFFER_SIZE   1050

// The maximum number of packets processed per DL context
//#define PACKET_NUMBER_PER_CTX_DL   19000
#define PACKET_NUMBER_PER_CTX_DL   38

// The maximum number of packets processed per UL context
//#define PACKET_NUMBER_PER_CTX_UL   10000
#define PACKET_NUMBER_PER_CTX_UL   20

// The number of packets to send in a burst for each context
#define PACKET_BURST_PER_CTX   1

#ifdef SEC_HW_VERSION_4_4

#define IRQ_COALESCING_COUNT    10
#define IRQ_COALESCING_TIMER    100

#endif

#define JOB_RING_POLL_UNLIMITED -1
//#define JOB_RING_POLL_LIMIT      5
#define JOB_RING_POLL_LIMIT      JOB_RING_POLL_UNLIMITED

// Alignment for input/output packets allocated from DMA-memory zone
#define BUFFER_ALIGNEMENT 32
#define BUFFER_SIZE       128
// Max length in bytes for a confidentiality /integrity key.
#define MAX_KEY_LENGTH    32

// Read ATBL(Alternate Time Base Lower) Register
#define GET_ATBL() \
    mfspr(SPR_ATBL)
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
static int get_results(uint8_t job_ring, int limit, uint32_t *out_packets, uint32_t *core_cycles, uint8_t tid);

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
static pdcp_context_t pdcp_ul_contexts[PDCP_CONTEXT_NUMBER];
static int no_of_used_pdcp_ul_contexts = 0;

static pdcp_context_t pdcp_dl_contexts[PDCP_CONTEXT_NUMBER];
static int no_of_used_pdcp_dl_contexts = 0;

// There are 2 threads: one thread handles PDCP UL and one thread handles PDCP DL
// One thread is producer on JR1 and consumer on JR2. The other thread is producer on JR2
// and consumer on JR1.
static thread_config_t th_config[THREADS_NUMBER];
static pthread_t threads[THREADS_NUMBER];



#ifndef PDCP_TEST_SCENARIO
#error "PDCP_TEST_SCENARIO is undefined!!!"
#endif

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F8_ENC
//////////////////////////////////////////////////////////////////////////////
#if PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F8_ENC

// Extracted from:
// Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2
// Document 3: Implementors. Test Data
// Version: 1.0
// Date: 10th January 2006
// Section 4. CONFIDENTIALITY ALGORITHM UEA2, Test Set 3

// Length of PDCP header
#define PDCP_HEADER_LENGTH 2

static uint8_t snow_f8_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// PDCP header
static uint8_t snow_f8_enc_pdcp_hdr[] = {0x8B, 0x26};

// PDCP payload not encrypted
static uint8_t snow_f8_enc_data_in[] = {
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
   

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,

    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,0x57,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,0xC4,0xA4,
    0x9D,0x42,0x14,0x07,0xE8,0x89,0x0B,0x38,
    };

// PDCP payload encrypted
/*static uint8_t snow_f8_enc_data_out[] = {0xBA,0x0F,0x31,0x30,0x03,0x34,0xC5,0x6B, // PDCP payload encrypted
                                         0x52,0xA7,0x49,0x7C,0xBA,0xC0,0x46};
                                         */

// Radio bearer id
static uint8_t snow_f8_enc_bearer = 0x3;

// Start HFN
static uint32_t snow_f8_enc_hfn = 0xFA556;

// HFN threshold
static uint32_t snow_f8_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F8_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F8_DEC

// Extracted from:
// Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2
// Document 3: Implementors. Test Data
// Version: 1.0
// Date: 10th January 2006
// Section 4. CONFIDENTIALITY ALGORITHM UEA2, Test Set 3

// Length of PDCP header
#define PDCP_HEADER_LENGTH 2


static uint8_t snow_f8_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// PDCP header
static uint8_t snow_f8_dec_pdcp_hdr[] = {0x8B, 0x26};

// PDCP payload not encrypted
static uint8_t snow_f8_dec_data_out[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                         0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

// PDCP payload encrypted
static uint8_t snow_f8_dec_data_in[] = { 0xBA,0x0F,0x31,0x30,0x03,0x34,0xC5,0x6B, // PDCP payload encrypted
                                         0x52,0xA7,0x49,0x7C,0xBA,0xC0,0x46};
// Radio bearer id
static uint8_t snow_f8_dec_bearer = 0x3;

// Start HFN
static uint32_t snow_f8_dec_hfn = 0xFA556;

// HFN threshold
static uint32_t snow_f8_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CTR_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CTR_ENC

// Extracted from:
// document 3GPP TS 33.401 V9.6.0 (2010-12)
// Annex C, 128-EEA2, Test Set 1

// Length of PDCP header
#define PDCP_HEADER_LENGTH 2


static uint8_t aes_ctr_enc_key[] = {0xd3, 0xc5, 0xd5, 0x92, 0x32, 0x7f, 0xb1, 0x1c,
                                    0x40, 0x35, 0xc6, 0x68, 0x0a, 0xf8, 0xc6, 0xd1};

// PDCP header
static uint8_t aes_ctr_enc_pdcp_hdr[] = {0x89, 0xB4};

// PDCP payload not encrypted
static uint8_t aes_ctr_enc_data_in[] = {0x98, 0x1b, 0xa6, 0x82, 0x4c, 0x1b, 0xfb, 0x1a,
                                        0xb4, 0x85, 0x47, 0x20, 0x29, 0xb7, 0x1d, 0x80,
                                        0x8c, 0xe3, 0x3e, 0x2c, 0xc3, 0xc0, 0xb5, 0xfc,
                                        0x1f, 0x3d, 0xe8, 0xa6, 0xdc, 0x66, 0xb1, 0xf0};


// PDCP payload encrypted
static uint8_t aes_ctr_enc_data_out[] = {0xe9, 0xfe, 0xd8, 0xa6, 0x3d, 0x15, 0x53, 0x04,
                                         0xd7, 0x1d, 0xf2, 0x0b, 0xf3, 0xe8, 0x22, 0x14,
                                         0xb2, 0x0e, 0xd7, 0xda, 0xd2, 0xf2, 0x33, 0xdc,
                                         0x3c, 0x22, 0xd7, 0xbd, 0xee, 0xed, 0x8e, 0x78};
// Radio bearer id
static uint8_t aes_ctr_enc_bearer = 0x15;
// Start HFN
static uint32_t aes_ctr_enc_hfn = 0x398A5;

// HFN threshold
static uint32_t aes_ctr_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CTR_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CTR_DEC

// Extracted from:
// document 3GPP TS 33.401 V9.6.0 (2010-12)
// Annex C, 128-EEA2, Test Set 1

// Length of PDCP header
#define PDCP_HEADER_LENGTH 2


static uint8_t aes_ctr_dec_key[] = {0xd3, 0xc5, 0xd5, 0x92, 0x32, 0x7f, 0xb1, 0x1c,
                                    0x40, 0x35, 0xc6, 0x68, 0x0a, 0xf8, 0xc6, 0xd1};

// PDCP header
static uint8_t aes_ctr_dec_pdcp_hdr[] = {0x89, 0xB4};

// PDCP payload encrypted
static uint8_t aes_ctr_dec_data_in[] = {0xe9, 0xfe, 0xd8, 0xa6, 0x3d, 0x15, 0x53, 0x04,
                                        0xd7, 0x1d, 0xf2, 0x0b, 0xf3, 0xe8, 0x22, 0x14,
                                        0xb2, 0x0e, 0xd7, 0xda, 0xd2, 0xf2, 0x33, 0xdc,
                                        0x3c, 0x22, 0xd7, 0xbd, 0xee, 0xed, 0x8e, 0x78};


// PDCP payload not encrypted
static uint8_t aes_ctr_dec_data_out[] = {0x98, 0x1b, 0xa6, 0x82, 0x4c, 0x1b, 0xfb, 0x1a,
                                         0xb4, 0x85, 0x47, 0x20, 0x29, 0xb7, 0x1d, 0x80,
                                         0x8c, 0xe3, 0x3e, 0x2c, 0xc3, 0xc0, 0xb5, 0xfc,
                                         0x1f, 0x3d, 0xe8, 0xa6, 0xdc, 0x66, 0xb1, 0xf0};
// Radio bearer id
static uint8_t aes_ctr_dec_bearer = 0x15;
// Start HFN
static uint32_t aes_ctr_dec_hfn = 0x398A5;

// HFN threshold
static uint32_t aes_ctr_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F9_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_ENC

// Extracted from:
// Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2
// Document 3: Implementors. Test Data
// Version: 1.0
// Date: 10th January 2006
// Section 5. INTEGRITY ALGORITHM UIA2, Test Set 4

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

static uint8_t snow_f9_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

//static uint8_t snow_f9_auth_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
//                                         0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

static uint8_t snow_f9_auth_enc_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                         0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
//static uint8_t snow_f9_enc_pdcp_hdr[] = {0x8B, 0x26};
static uint8_t snow_f9_enc_pdcp_hdr[] = {0xD0};

// PDCP payload not encrypted
//static uint8_t snow_f9_enc_data_in[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
//                                        0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

static uint8_t snow_f9_enc_data_in[] = {0xA7 ,0xD4,0x63,0xDF,0x9F,0xB2,0xB2,
                                        0x78,0x83,0x3F,0xA0,0x2E,0x23,0x5A,0xA1,
                                        0x72,0xBD,0x97,0x0C,0x14,0x73,0xE1,0x29,
                                        0x07,0xFB,0x64,0x8B,0x65,0x99,0xAA,0xA0,
                                        0xB2,0x4A,0x03,0x86,0x65,0x42,0x2B,0x20,
                                        0xA4,0x99,0x27,0x6A,0x50,0x42,0x70,0x09};

// PDCP payload encrypted
//static uint8_t snow_f9_enc_data_out[] = {0xBA,0x0F,0x31,0x30,0x03,0x34,0xC5,0x6B, // PDCP payload encrypted
//                                         0x52,0xA7,0x49,0x7C,0xBA,0xC0,0x46};
static uint8_t snow_f9_enc_data_out[] = {0x38,0xB5,0x54,0xC0};

// Radio bearer id
static uint8_t snow_f9_enc_bearer = 0x0;

// Start HFN
static uint32_t snow_f9_enc_hfn = 0xA3C9F2;

// HFN threshold
static uint32_t snow_f9_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F9_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_DEC

// Extracted from:
// Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2
// Document 3: Implementors. Test Data
// Version: 1.0
// Date: 10th January 2006
// Section 5. INTEGRITY ALGORITHM UIA2, Test Set 4

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

static uint8_t snow_f9_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

static uint8_t snow_f9_auth_dec_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                         0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t snow_f9_dec_pdcp_hdr[] = { 0xD0};

// PDCP payload not encrypted
//static uint8_t snow_f9_enc_data_in[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
//                                        0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

static uint8_t snow_f9_dec_data_in[] = {0xA7 ,0xD4,0x63,0xDF,0x9F,0xB2,0xB2,
                                        0x78,0x83,0x3F,0xA0,0x2E,0x23,0x5A,0xA1,
                                        0x72,0xBD,0x97,0x0C,0x14,0x73,0xE1,0x29,
                                        0x07,0xFB,0x64,0x8B,0x65,0x99,0xAA,0xA0,
                                        0xB2,0x4A,0x03,0x86,0x65,0x42,0x2B,0x20,
                                        0xA4,0x99,0x27,0x6A,0x50,0x42,0x70,0x09,
                                        // The MAC-I from packet
                                        0x38,0xB5,0x54,0xC0};

// PDCP payload encrypted
//static uint8_t snow_f9_enc_data_out[] = {0xBA,0x0F,0x31,0x30,0x03,0x34,0xC5,0x6B, // PDCP payload encrypted
//                                         0x52,0xA7,0x49,0x7C,0xBA,0xC0,0x46};
static uint8_t snow_f9_dec_data_out[] = { 0x38,0xB5,0x54,0xC0};

// Radio bearer id
static uint8_t snow_f9_dec_bearer = 0x0;

// Start HFN
static uint32_t snow_f9_dec_hfn = 0xA3C9F2;

// HFN threshold
static uint32_t snow_f9_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CMAC_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_ENC

// Test Set 2


// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

static uint8_t aes_cmac_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

static uint8_t aes_cmac_auth_enc_key[] = {0xd3,0xc5,0xd5,0x92,0x32,0x7f,0xb1,0x1c,
                                          0x40,0x35,0xc6,0x68,0x0a,0xf8,0xc6,0xd1};

// PDCP header
static uint8_t aes_cmac_enc_pdcp_hdr[] = { 0x48};

// PDCP payload
static uint8_t aes_cmac_enc_data_in[] = {0x45,0x83,0xd5,0xaf,0xe0,0x82,0xae};

// PDCP payload encrypted
static uint8_t aes_cmac_enc_data_out[] = {0xb9,0x37,0x87,0xe6};

// Radio bearer id
static uint8_t aes_cmac_enc_bearer = 0x1a;

// Start HFN
static uint32_t aes_cmac_enc_hfn = 0x1CC52CD;

// HFN threshold
static uint32_t aes_cmac_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CMAC_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_DEC

// Test Set 2


// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

static uint8_t aes_cmac_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

static uint8_t aes_cmac_auth_dec_key[] = {0xd3,0xc5,0xd5,0x92,0x32,0x7f,0xb1,0x1c,
                                          0x40,0x35,0xc6,0x68,0x0a,0xf8,0xc6,0xd1};
    
// PDCP header
static uint8_t aes_cmac_dec_pdcp_hdr[] = { 0x48};

// PDCP payload not encrypted
static uint8_t aes_cmac_dec_data_in[] = {0x45,0x83,0xd5,0xaf,0xe0,0x82,0xae,
                                         // The MAC-I from packet
                                         0xb9,0x37,0x87,0xe6};

// PDCP payload encrypted
static uint8_t aes_cmac_dec_data_out[] = {0xb9,0x37,0x87,0xe6};

// Radio bearer id
static uint8_t aes_cmac_dec_bearer = 0x1a;

// Start HFN
static uint32_t aes_cmac_dec_hfn = 0x1CC52CD;

// HFN threshold
static uint32_t aes_cmac_dec_hfn_threshold = 0xFF00000;
#else
#error "Unsuported test scenario!"
#endif


//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F8_ENC
//////////////////////////////////////////////////////////////////////////////
#if PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F8_ENC


#define test_crypto_key         snow_f8_enc_key
#define test_crypto_key_len     sizeof(snow_f8_enc_key)

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            snow_f8_enc_data_in
#define test_data_out           snow_f8_enc_data_out

#define test_pdcp_hdr           snow_f8_enc_pdcp_hdr
#define test_bearer             snow_f8_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_12
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                snow_f8_enc_hfn
#define test_hfn_threshold      snow_f8_enc_hfn_threshold

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F8_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F8_DEC

#define test_crypto_key         snow_f8_dec_key
#define test_crypto_key_len     sizeof(snow_f8_dec_key)

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            snow_f8_dec_data_in
#define test_data_out           snow_f8_dec_data_out

#define test_pdcp_hdr           snow_f8_dec_pdcp_hdr
#define test_bearer             snow_f8_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_12
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm    SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                snow_f8_dec_hfn
#define test_hfn_threshold      snow_f8_dec_hfn_threshold

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CTR_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CTR_ENC

#define test_crypto_key         aes_ctr_enc_key
#define test_crypto_key_len     sizeof(aes_ctr_enc_key)

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            aes_ctr_enc_data_in
#define test_data_out           aes_ctr_enc_data_out

#define test_pdcp_hdr           aes_ctr_enc_pdcp_hdr
#define test_bearer             aes_ctr_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_12
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm    SEC_ALG_AES
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                aes_ctr_enc_hfn
#define test_hfn_threshold      aes_ctr_enc_hfn_threshold

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CTR_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CTR_DEC

#define test_crypto_key         aes_ctr_dec_key
#define test_crypto_key_len     sizeof(aes_ctr_dec_key)

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            aes_ctr_dec_data_in
#define test_data_out           aes_ctr_dec_data_out

#define test_pdcp_hdr           aes_ctr_dec_pdcp_hdr
#define test_bearer             aes_ctr_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_12
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm    SEC_ALG_AES
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                aes_ctr_dec_hfn
#define test_hfn_threshold      aes_ctr_dec_hfn_threshold

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F9_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_ENC

#define test_crypto_key         snow_f9_enc_key
#define test_crypto_key_len     sizeof(snow_f9_enc_key)

#define test_auth_key           snow_f9_auth_enc_key
#define test_auth_key_len       sizeof(snow_f9_auth_enc_key)

#define test_data_in            snow_f9_enc_data_in
#define test_data_out           snow_f9_enc_data_out

#define test_pdcp_hdr           snow_f9_enc_pdcp_hdr
#define test_bearer             snow_f9_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
//#define test_cipher_algorithm   SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_SNOW
#define test_hfn                snow_f9_enc_hfn
#define test_hfn_threshold      snow_f9_enc_hfn_threshold

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F9_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_DEC

#define test_crypto_key         snow_f9_dec_key
#define test_crypto_key_len     sizeof(snow_f9_dec_key)

#define test_auth_key           snow_f9_auth_dec_key
#define test_auth_key_len       sizeof(snow_f9_auth_dec_key)

#define test_data_in            snow_f9_dec_data_in
#define test_data_out           snow_f9_dec_data_out

#define test_pdcp_hdr           snow_f9_dec_pdcp_hdr
#define test_bearer             snow_f9_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
#define test_integrity_algorithm SEC_ALG_SNOW
#define test_hfn                snow_f9_dec_hfn
#define test_hfn_threshold      snow_f9_dec_hfn_threshold

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CMAC_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_ENC

#define test_crypto_key         aes_cmac_enc_key
#define test_crypto_key_len     sizeof(aes_cmac_enc_key)

#define test_auth_key           aes_cmac_auth_enc_key
#define test_auth_key_len       sizeof(aes_cmac_auth_enc_key)

#define test_data_in            aes_cmac_enc_data_in
#define test_data_out           aes_cmac_enc_data_out

#define test_pdcp_hdr           aes_cmac_enc_pdcp_hdr
#define test_bearer             aes_cmac_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
#define test_integrity_algorithm SEC_ALG_AES
#define test_hfn                aes_cmac_enc_hfn
#define test_hfn_threshold      aes_cmac_enc_hfn_threshold

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CMAC_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_DEC

#define test_crypto_key         aes_cmac_dec_key
#define test_crypto_key_len     sizeof(aes_cmac_dec_key)

#define test_auth_key           aes_cmac_auth_dec_key
#define test_auth_key_len       sizeof(aes_cmac_auth_dec_key)

#define test_data_in            aes_cmac_dec_data_in
#define test_data_out           aes_cmac_dec_data_out

#define test_pdcp_hdr           aes_cmac_dec_pdcp_hdr
#define test_bearer             aes_cmac_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
#define test_integrity_algorithm SEC_ALG_AES
#define test_hfn                aes_cmac_dec_hfn
#define test_hfn_threshold      aes_cmac_dec_hfn_threshold
#else
#error "Unsuported test scenario!"
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
    if (*no_of_used_pdcp_contexts >= PDCP_CONTEXT_NUMBER)
    {
        // no free contexts available
        return 2;
    }

    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
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

//    memset(pdcp_context->input_buffers, 0, sizeof(buffer_t) * PACKET_NUMBER_PER_CTX);
//    memset(pdcp_context->output_buffers, 0, sizeof(buffer_t) * PACKET_NUMBER_PER_CTX);
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
        // the stub implementation does not return an error status
        assert(status == SEC_STATUS_SUCCESS || status == SEC_STATUS_HFN_THRESHOLD_REACHED);
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
    (*in_packet)->offset = 8;
    (*in_packet)->scatter_gather = SEC_CONTIGUOUS_BUFFER;

    assert(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].usage == PDCP_BUFFER_FREE);
    *out_packet = &(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].pdcp_packet);

    pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].usage = PDCP_BUFFER_USED;
    (*out_packet)->address = &(pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].buffer[0]);

    //out_packet->offset = pdcp_context->output_buffers[pdcp_context->no_of_used_buffers].offset;

    // Needed 8 bytes before actual start of PDCP packet, for PDCP control-plane + AES algo testing.
    (*out_packet)->offset = 8;
    (*out_packet)->scatter_gather = SEC_CONTIGUOUS_BUFFER;

    // copy PDCP header
    memcpy((*in_packet)->address + (*in_packet)->offset, test_pdcp_hdr, sizeof(test_pdcp_hdr));
    // copy input data
    memcpy((*in_packet)->address + (*in_packet)->offset + PDCP_HEADER_LENGTH,
           test_data_in,
           sizeof(test_data_in));

    (*in_packet)->length = sizeof(test_data_in) + PDCP_HEADER_LENGTH + (*in_packet)->offset;
    // TODO: finalize this...
    // Need extra 4 bytes at end of input/output packet for MAC-I code, in case of PDCP control-plane packets
    // Need  extra 8 bytes at start of input packet  for Initialization Vector (IV) when testing
    // PDCP control-plane with AES CMAC algorithm.
    assert((*in_packet)->length + 4 <= PDCP_BUFFER_SIZE);

    (*out_packet)->length = sizeof(test_data_in) + PDCP_HEADER_LENGTH + (*out_packet)->offset;
    assert((*out_packet)->length + 4 <= PDCP_BUFFER_SIZE);

    pdcp_context->no_of_used_buffers++;

    return 0;
}

static int pdcp_ready_packet_handler (const sec_packet_t *in_packet,
                                      const sec_packet_t *out_packet,
                                      ua_context_handle_t ua_ctx_handle,
                                      uint32_t status,
                                      uint32_t error_info)
{

#if 0
    uint32_t diff_cycles = 0;
    int ret_code;
    pdcp_context_t *pdcp_context = NULL;
    int test_failed = 0;
    uint32_t start_cycles = GET_ATBL();

    // validate input params
    assert(ua_ctx_handle != NULL);
    assert(in_packet != NULL);
    assert(out_packet != NULL);

    assert(status != SEC_STATUS_ERROR);
    if(status == SEC_STATUS_HFN_THRESHOLD_REACHED)
    {
        test_printf("HFN threshold reached for packet\n");
    }

    pdcp_context = (pdcp_context_t *)ua_ctx_handle;

    test_printf("\nthread #%d:consumer: sec_callback called for context_id = %d, "
            "no of buffers processed = %d, no of buffers to process = %d, "
            "status = %d\n",
            (pdcp_context->thread_id + 1)%2,
            pdcp_context->id,
            pdcp_context->no_of_buffers_processed + 1,
            pdcp_context->no_of_buffers_to_process,
            status);

    assert(in_packet->length == sizeof(test_data_in) + PDCP_HEADER_LENGTH + in_packet->offset);
#if (PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_ENC) || \
    (PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_ENC)

    // For SNOW F9 and AES CMAC, output data consists only of MAC-I code = 4 bytes
    test_failed = (0 != memcmp(out_packet->address + out_packet->offset,
                               test_data_out,
                               sizeof(test_data_out)));

#elif (PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_DEC) || \
      (PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_DEC)
      // Status must be SUCCESS. When MAC-I validation failed,
      // status is set to #SEC_STATUS_HFN_THRESHOLD_REACHED. 
      test_failed = (status == SEC_STATUS_HFN_THRESHOLD_REACHED || status == SEC_STATUS_ERROR);
#else

    assert(out_packet->length == sizeof(test_data_out) + PDCP_HEADER_LENGTH + out_packet->offset);
    test_failed = (0 != memcmp(out_packet->address + out_packet->offset,
                               test_pdcp_hdr,
                               PDCP_HEADER_LENGTH) ||
                   0 != memcmp(out_packet->address + out_packet->offset + PDCP_HEADER_LENGTH,
                               test_data_out,
                               sizeof(test_data_out)));
#endif

    if(test_failed)
    {
        test_printf("\nthread #%d:consumer: out packet INCORRECT!!!."
               " out pkt= ",
               (pdcp_context->thread_id + 1)%2);
        int i;
        for(i = 0; i <  out_packet->length; i++)
        {
            test_printf("%02x ", out_packet->address[i]);
        }
        test_printf("\n");
        /*
           test_printf("\nreference data: ");

           for(i = 0; i <  job->out_packet->length; i++)
           {
           test_printf("%02x ", snow_f8_enc_data_out[i]);
           }
           test_printf("\n");
           */
        assert(0);

    }
    else
    {
        int i;
        test_printf("\nthread #%d:consumer: packet CORRECT!!! out pkt = . ", (pdcp_context->thread_id + 1)%2);
        for(i = 0; i <  out_packet->length; i++)
        {
//            test_printf("%02x ", out_packet->address[i]);
        }
        test_printf("\n");
    }

    // Buffers processing.
    // In this test application we will release the input and output buffers
    // Check also if all the buffers were received for a retiring context
    // and if so mark it to be deleted
    ret_code = release_pdcp_buffers(pdcp_context,
                               (sec_packet_t*)in_packet,
                               (sec_packet_t*)out_packet,
                               status);
    assert(ret_code == 0);

    diff_cycles = (GET_ATBL() - start_cycles);
    profile_printf("thread #%d:UA calback cycles = %d\n", th_config_local->tid, diff_cycles);

#endif
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

static int delete_context(pdcp_context_t * pdcp_context, int *no_of_used_pdcp_contexts)
{
    int ret_code;

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
        ret_code = sec_delete_pdcp_context(pdcp_context->sec_ctx);
        if (ret_code == SEC_PACKETS_IN_FLIGHT)
        {
            test_printf("thread #%d:producer: delete PDCP context no %d -> packets in flight \n",
                    pdcp_context->thread_id, pdcp_context->id);
        }
        else if (ret_code == SEC_LAST_PACKET_IN_FLIGHT)
        {
            test_printf("thread #%d:producer: delete PDCP context no %d -> last packet in flight \n",
                    pdcp_context->thread_id, pdcp_context->id);
        }
        else if (ret_code == SEC_SUCCESS)
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
                    pdcp_context->thread_id, ret_code, pdcp_context->id);
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

    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        // Try an delete the contexts and count the total number of contexts deleted.
        // delete_context returns 0 if the context was not deleted, and 1 if it was deleted.
        *contexts_deleted += delete_context(&pdcp_contexts[i], no_of_used_pdcp_contexts);
    }
    return 0;
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
    th_config[0].pdcp_contexts = &pdcp_ul_contexts[0];
    th_config[0].no_of_used_pdcp_contexts = &no_of_used_pdcp_ul_contexts;
    th_config[0].no_of_pdcp_contexts_to_test = PDCP_CONTEXT_NUMBER;
    th_config[0].work_done = 0;
    th_config[0].should_exit = 0;
    th_config[0].core_cycles = 0;
    th_config[0].rx_packets_per_ctx = PACKET_NUMBER_PER_CTX_DL;
    th_config[0].tx_packets_per_ctx = PACKET_NUMBER_PER_CTX_UL;
    ret_code = pthread_create(&threads[0], NULL, &pdcp_thread_routine, (void*)&th_config[0]);
    assert(ret_code == 0);

    // start PDCP DL thread
    th_config[1].tid = 1;
    // PDCP DL thread is consumer on JR ID 1 and producer on JR ID 0
    th_config[1].consumer_job_ring_id = 1;
    th_config[1].producer_job_ring_id = 0;
    th_config[1].pdcp_contexts = &pdcp_dl_contexts[0];
    th_config[1].no_of_used_pdcp_contexts = &no_of_used_pdcp_dl_contexts;
    th_config[1].no_of_pdcp_contexts_to_test = PDCP_CONTEXT_NUMBER;
    th_config[1].work_done = 0;
    th_config[1].should_exit = 0;
    th_config[1].core_cycles = 0;
    th_config[1].rx_packets_per_ctx = PACKET_NUMBER_PER_CTX_UL;
    th_config[1].tx_packets_per_ctx = PACKET_NUMBER_PER_CTX_DL;
    ret_code = pthread_create(&threads[1], NULL, &pdcp_thread_routine, (void*)&th_config[1]);
    assert(ret_code == 0);

    return 0;
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
    profile_printf("thread 0 core cycles = %d\n", th_config[0].core_cycles);
    profile_printf("thread 1 core cycles = %d\n", th_config[1].core_cycles);
    return 0;
}

static void* pdcp_thread_routine(void* config)
{
    thread_config_t *th_config_local = NULL;
    int ret_code = 0;
    int i = 0;
    unsigned int packets_received = 0;
    pdcp_context_t *pdcp_context;
    //int total_no_of_contexts_deleted = 0;
    //int no_of_contexts_deleted = 0;
    int total_no_of_contexts_created = 0;
    unsigned int total_packets_to_send = 0;
    unsigned int total_packets_sent = 0;
    unsigned int total_packets_to_receive = 0;
    unsigned int total_packets_received = 0;
    unsigned int ctx_packet_count = 0;
    struct timeval start_time;
    struct timeval end_time;
    uint32_t start_cycles = 0;
    uint32_t diff_cycles = 0;

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
        pdcp_context->pdcp_ctx_cfg_data.packet_direction = test_packet_direction;
        pdcp_context->pdcp_ctx_cfg_data.protocol_direction = test_protocol_direction;
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
    total_packets_to_receive = PDCP_CONTEXT_NUMBER * th_config_local->rx_packets_per_ctx;

    gettimeofday(&start_time, NULL);
    // Send a burst of packets for each context, until all the packets are sent to SEC
//    while(1)
//    {
    while(total_packets_to_send != total_packets_sent)
    {
        for(i = 0; i < PDCP_CONTEXT_NUMBER; i++)
        {
            pdcp_context = &th_config_local->pdcp_contexts[i];
            ctx_packet_count = 0;

            while(ctx_packet_count < PACKET_BURST_PER_CTX)
            {
                // for each context, send to SEC a fixed number of packets for processing
                sec_packet_t *in_packet;
                sec_packet_t *out_packet;

                // Get a free buffer, If none, then break the loop
                if(get_free_pdcp_buffer(pdcp_context, &in_packet, &out_packet) != 0)
                {
                    break;
                }

                // if SEC process packet returns that the producer JR is full, do some polling
                // on the consumer JR until the producer JR has free entries.
                do{
                    start_cycles = GET_ATBL();
                    ret_code = sec_process_packet(pdcp_context->sec_ctx,
                            in_packet,
                            out_packet,
                            (ua_context_handle_t)pdcp_context);

                    diff_cycles = (GET_ATBL() - start_cycles);
                    th_config_local->core_cycles += diff_cycles;

                    profile_printf("thread #%d:ctx #%p:sec_process_packet cycles = %d\n",
                            th_config_local->tid, pdcp_context, diff_cycles);

                    if(ret_code == SEC_JR_IS_FULL)
                    {
                        //printf("thread #%d: jr full\n", th_config_local->tid);
                        // wait while the producer JR is empty, and in the mean time do some
                        // polling on the consumer JR -> retrieve only 5 notifications if available
                        ret_code = get_results(th_config_local->consumer_job_ring_id,
                                JOB_RING_POLL_LIMIT,
                                &packets_received,
                                &th_config_local->core_cycles,
                                th_config_local->tid);
                        assert(ret_code == 0);
                        total_packets_received += packets_received;
                        usleep(10);
                    }
                    else if(ret_code != SEC_SUCCESS)
                    {
                        test_printf("thread #%d:producer: sec_process_packet return error %d for PDCP context no %d \n",
                                th_config_local->tid, ret_code, pdcp_context->id);
                        assert(0);
                    }
                    else
                    {
                        break;
                    }
                }while(1);

                ctx_packet_count++;
            }
            total_packets_sent += ctx_packet_count;
            //usleep(5);
        }
    }
    //        usleep(1000);
//    }

    test_printf("thread #%d: polling until all contexts are deleted\n", th_config_local->tid);

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
        usleep(100);

    }while(total_packets_to_receive != total_packets_received);
    /*printf("thread #%d: pkts to receive %d. packets received %d\n",
            th_config_local->tid,
           total_packets_to_receive, total_packets_received);
    */

    gettimeofday(&end_time, NULL);

    printf("Sent %d packets. Received %d packets.\nStart Time %d sec %d usec."
            "End Time %d sec %d usec.\n",
            total_packets_sent, total_packets_received, (int)start_time.tv_sec, (int)start_time.tv_usec, 
            (int)end_time.tv_sec, (int)end_time.tv_usec);

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

    /* Get value from system clock and use it for seed generation  */
    time(&seconds);
    srand((unsigned int) seconds);

    memset (pdcp_dl_contexts, 0, sizeof(pdcp_context_t) * PDCP_CONTEXT_NUMBER);
    memset (pdcp_ul_contexts, 0, sizeof(pdcp_context_t) * PDCP_CONTEXT_NUMBER);

    // map the physical memory
    ret_code = dma_mem_setup();
    if (ret_code != 0)
    {
        test_printf("dma_mem_setup failed with ret_code = %d\n", ret_code);
        return 1;
    }
	test_printf("dma_mem_setup: mapped virtual mem 0x%x to physical mem 0x%x\n", __dma_virt, DMA_MEM_PHYS);

    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        pdcp_dl_contexts[i].id = i;
        // allocate input buffers from memory zone DMA-accessible to SEC engine
        pdcp_dl_contexts[i].input_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                             sizeof(buffer_t) * PACKET_NUMBER_PER_CTX_DL);
        // allocate output buffers from memory zone DMA-accessible to SEC engine
        pdcp_dl_contexts[i].output_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                              sizeof(buffer_t) * PACKET_NUMBER_PER_CTX_DL);

        pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                            MAX_KEY_LENGTH);
        pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                               MAX_KEY_LENGTH);

        pdcp_dl_contexts[i].no_of_buffers_to_process = PACKET_NUMBER_PER_CTX_DL;

        // validate that the address of the freshly allocated buffer falls in the second memory are.
        assert (pdcp_dl_contexts[i].input_buffers != NULL);
        assert (pdcp_dl_contexts[i].output_buffers != NULL);
        assert (pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key != NULL);
        assert (pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key != NULL);

        assert((dma_addr_t)pdcp_dl_contexts[i].input_buffers >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_dl_contexts[i].output_buffers >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key >= DMA_MEM_SEC_DRIVER);

        memset(pdcp_dl_contexts[i].input_buffers, 0, sizeof(buffer_t) * PACKET_NUMBER_PER_CTX_DL);
        memset(pdcp_dl_contexts[i].output_buffers, 0, sizeof(buffer_t) * PACKET_NUMBER_PER_CTX_DL);

        pdcp_ul_contexts[i].id = i + PDCP_CONTEXT_NUMBER;
        // allocate input buffers from memory zone DMA-accessible to SEC engine
        pdcp_ul_contexts[i].input_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                             sizeof(buffer_t) * PACKET_NUMBER_PER_CTX_UL);
        // validate that the address of the freshly allocated buffer falls in the second memory are.
        pdcp_ul_contexts[i].output_buffers = dma_mem_memalign(BUFFER_ALIGNEMENT, 
                                                              sizeof(buffer_t) * PACKET_NUMBER_PER_CTX_UL);

        pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                            MAX_KEY_LENGTH);
        pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key = dma_mem_memalign(BUFFER_ALIGNEMENT,
                                                                               MAX_KEY_LENGTH);
        pdcp_ul_contexts[i].no_of_buffers_to_process = PACKET_NUMBER_PER_CTX_UL;

        // validate that the address of the freshly allocated buffer falls in the second memory are.
        assert (pdcp_ul_contexts[i].input_buffers != NULL);
        assert (pdcp_ul_contexts[i].output_buffers != NULL);
        assert (pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key != NULL);
        assert (pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key != NULL);

        assert((dma_addr_t)pdcp_ul_contexts[i].input_buffers >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_ul_contexts[i].output_buffers >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key >= DMA_MEM_SEC_DRIVER);
        assert((dma_addr_t)pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key >= DMA_MEM_SEC_DRIVER);


        memset(pdcp_ul_contexts[i].input_buffers, 0, sizeof(buffer_t) * PACKET_NUMBER_PER_CTX_UL);
        memset(pdcp_ul_contexts[i].output_buffers, 0, sizeof(buffer_t) * PACKET_NUMBER_PER_CTX_UL);
    }

    //////////////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC user space driver requesting #JOB_RING_NUMBER Job Rings
    //////////////////////////////////////////////////////////////////////////////

    sec_config_data.memory_area = (void*)__dma_virt;
    assert(sec_config_data.memory_area != NULL);

    // Fill SEC driver configuration data
    sec_config_data.work_mode = SEC_STARTUP_POLLING_MODE;
#ifdef SEC_HW_VERSION_4_4
    sec_config_data.irq_coalescing_count = IRQ_COALESCING_COUNT;
    sec_config_data.irq_coalescing_timer = IRQ_COALESCING_TIMER;
#endif

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
        dma_mem_free(pdcp_dl_contexts[i].input_buffers,  BUFFER_SIZE * PACKET_NUMBER_PER_CTX_DL);
        dma_mem_free(pdcp_dl_contexts[i].output_buffers,  BUFFER_SIZE * PACKET_NUMBER_PER_CTX_DL);

        dma_mem_free(pdcp_dl_contexts[i].pdcp_ctx_cfg_data.cipher_key, MAX_KEY_LENGTH);
        dma_mem_free(pdcp_dl_contexts[i].pdcp_ctx_cfg_data.integrity_key, MAX_KEY_LENGTH);

        dma_mem_free(pdcp_ul_contexts[i].input_buffers,  BUFFER_SIZE * PACKET_NUMBER_PER_CTX_UL);
        dma_mem_free(pdcp_ul_contexts[i].output_buffers,  BUFFER_SIZE * PACKET_NUMBER_PER_CTX_UL);

        dma_mem_free(pdcp_ul_contexts[i].pdcp_ctx_cfg_data.cipher_key, MAX_KEY_LENGTH);
        dma_mem_free(pdcp_ul_contexts[i].pdcp_ctx_cfg_data.integrity_key, MAX_KEY_LENGTH);

    }
	// unmap the physical memory
	ret_code = dma_mem_release();
	if (ret_code != 0)
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
    int ret_code = 0;
    cpu_set_t cpu_mask; /* processor 0 */

    /* bind process to processor 0 */
    CPU_ZERO(&cpu_mask);
    CPU_SET(1, &cpu_mask);
    if(sched_setaffinity(0, sizeof(cpu_mask), &cpu_mask) < 0)
    {
        perror("sched_setaffinity");
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

