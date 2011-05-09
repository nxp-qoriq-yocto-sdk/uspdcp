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
#include "sec_utils.h"
#include "sec_contexts.h"
#include "sec_config.h"
#include "sec_job_ring.h"
#include "sec_atomic.h"
#include "sec_pdcp.h"
#include "sec_hw_specific.h"

#include <stdio.h>

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
/** Max sec contexts per pool computed based on the number of Job Rings assigned.
 *  All the available contexts will be split evenly between all the per-job-ring pools. 
 *  There is one additional global pool besides the per-job-ring pools. */
#define MAX_SEC_CONTEXTS_PER_POOL   (SEC_MAX_PDCP_CONTEXTS / (g_job_rings_no))

/** Max length of a string describing a ::sec_status_t value or a ::sec_return_code_t value */
#define MAX_STRING_REPRESENTATION_LENGTH    50

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/** Lists the states possible for the SEC user space driver. */
typedef enum sec_driver_state_e
{
    SEC_DRIVER_STATE_IDLE,      /*< Driver not initialized */
    SEC_DRIVER_STATE_STARTED,   /*< Driver initialized and can be used by UA */
    SEC_DRIVER_STATE_RELEASE,   /*< Driver release is in progress */
}sec_driver_state_t;

/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
/* The current state of SEC user space driver */
volatile sec_driver_state_t g_driver_state = SEC_DRIVER_STATE_IDLE;

/* The number of job rings used by SEC user space driver */
static int g_job_rings_no = 0;

/* Array of job ring handles provided to UA from sec_init() function. */
static sec_job_ring_descriptor_t g_job_ring_handles[MAX_SEC_JOB_RINGS];

/* The work mode configured for SEC user space driver at startup,
 * when NAPI notification processing is ON */
static int g_sec_work_mode = 0;

/* Last JR assigned to a context by the SEC driver using a round robin algorithm.
 * Not used if UA associates the contexts created to a certain JR.*/
static unsigned int g_last_jr_assigned = 0;

/* Global context pool */
static sec_contexts_pool_t g_ctx_pool;

/** String representation for values from  ::sec_status_t */
static const char g_status_string[][MAX_STRING_REPRESENTATION_LENGTH] =
{
    {"Not defined"},
};

/** String representation for values from  ::sec_return_code_t */
static const char g_return_code_string[][MAX_STRING_REPRESENTATION_LENGTH] =
{
    {"Not defined"},
};
/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
/** Start address for DMA-able memory area configured by UA */
void* g_dma_mem_start = NULL;
/** Current address for unused DMA-able memory area configured by UA */
void* g_dma_mem_free = NULL;

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/


/** @brief Enable IRQ generation for all SEC's job rings.
 */
#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
static void enable_irq();
#endif

/** @brief Poll the HW for already processed jobs in the JR
 * and notify the available jobs to UA.
 *
 * @param [in]  job_ring    The job ring to poll.
 * @param [in]  limit       The maximum number of jobs to notify.
 *                          If set to negative value, all available jobs are notified.
 * @param [out] packets_no  No of jobs notified to UA.
 *
 * @retval 0 for success
 * @retval other value for error
 */
static uint32_t hw_poll_job_ring(sec_job_ring_t *job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no);

/** @brief Poll the HW for already processed jobs in the JR
 * and silently discard the available jobs.
 *
 * @param [in]  job_ring    The job ring to poll.
 */
static void hw_flush_job_ring(sec_job_ring_t *job_ring);

/** @brief Flush job rings of any processed packets.
 * The processed packets are silently dropped,
 * WITHOUT beeing notified to UA.
 */
static void flush_job_rings();
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
static void enable_irq()
{
    int i = 0;
    sec_job_ring_t * job_ring = NULL;

    for (i = 0; i < g_job_rings_no; i++)
    {
        job_ring = &g_job_rings[i];
        hw_enable_done_irq_on_job_ring(job_ring);
    }
}
#endif

static void hw_flush_job_ring(sec_job_ring_t * job_ring)
{
    sec_context_t * sec_context;
    sec_job_t *job;
    int jobs_no_to_discard = 0;
    int discarded_packets_no = 0;
    int number_of_jobs_available = 0;

    // compute the number of jobs available in the job ring based on the
    // producer and consumer index values.
    number_of_jobs_available = SEC_JOB_RING_DIFF(SEC_JOB_RING_SIZE, job_ring->pidx, job_ring->cidx);

    // Discard all jobs
    jobs_no_to_discard = number_of_jobs_available;

    while(jobs_no_to_discard > discarded_packets_no)
    {
        // get the first un-notified job from the job ring
        job = &job_ring->jobs[job_ring->cidx];
        sec_context = job->sec_context;

        // consume processed packet for this sec context
        CONTEXT_CONSUME_PACKET(sec_context);
        discarded_packets_no++;

        // increment the consumer index for the current job ring
        job_ring->cidx = SEC_CIRCULAR_COUNTER(job_ring->cidx, SEC_JOB_RING_SIZE);
    }
}

static uint32_t hw_poll_job_ring(sec_job_ring_t *job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no)
{
    sec_context_t *sec_context = NULL;
    sec_job_t *job = NULL;
    sec_job_t saved_job;

    int32_t jobs_no_to_notify = 0; // the number of done jobs to notify to UA
    sec_status_t status = SEC_STATUS_SUCCESS;
    int32_t notified_packets_no = 0;
    int32_t number_of_jobs_available = 0;
    uint32_t error_code = 0;
    int ret = 0;
    uint8_t mac_i_check_failed = FALSE;
    uint32_t original_mac_i;

    // Compute the number of notifications that need to be raised to UA
    // If limit < 0 -> notify all done jobs
    // If limit > total number of done jobs -> notify all done jobs
    // If limit = 0 -> error
    // If limit > 0 && limit < total number of done jobs -> notify a number of done jobs equal with limit

    // compute the number of jobs available in the job ring based on the
    // producer and consumer index values.
    number_of_jobs_available = SEC_JOB_RING_DIFF(SEC_JOB_RING_SIZE, job_ring->pidx, job_ring->cidx);

    jobs_no_to_notify = (limit < 0 || limit > number_of_jobs_available) ? number_of_jobs_available : limit;

    while(jobs_no_to_notify > notified_packets_no)
    {
        // get the first un-notified job from the job ring
        job = &job_ring->jobs[job_ring->cidx];

        // check if job is DONE
        if(!hw_job_is_done(job->descr))
        {
            // check if job generated error
            error_code = hw_job_ring_error(job_ring);
            if (error_code)
            {
                SEC_ERROR("Packet at cidx %d generated error 0x%x on job ring with id %d", 
                          job_ring->cidx,
                          error_code,
                          job_ring->jr_id);
                // TODO: flush jobs from JR, with error code set to callback
                return SEC_INVALID_INPUT_PARAM;
            }
            else
            {
                // Packet is not processed yet, exit
                break;
            }
        }
        
        // copy into a temporary job the fields from the job we need to raise callback
        // this is done to free the slot before the callback is called, 
        // which we cannot control in terms of how much processing it will do.
        saved_job.in_packet = job->in_packet;
        saved_job.out_packet = job->out_packet;
        saved_job.ua_handle = job->ua_handle;
        saved_job.sec_context = job->sec_context;
        saved_job.job_status = job->job_status;
        
        // Used only for #SEC_STATUS_HFN_THRESHOLD_REACHED code, which is set by the driver,
        // before the packet is sent to SEC.
        status = saved_job.job_status;

        //TODO: maybe increment consumer index here, and do all these checks on saved job.
        // Check when doing benchmark.

        // Packet processing is DONE. See if it's processed with crypto or integrity algo
        if(job->is_integrity_algo == TRUE)
        {
#if !defined(PDCP_TEST_SNOW_F9_ONLY) && !defined(PDCP_TEST_AES_CMAC_ONLY)
            if(job->sec_context->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
#endif
            {
                // SEC 3.1 generated an 8 byte long MAC-I into a per-context-memory-zone internal
                // to the driver. Copy the relevant 4 bytes of MAC-I code into output buffer.
                if(job->sec_context->pdcp_crypto_info->integrity_algorithm == SEC_ALG_SNOW)
                {
                    // SEC 3.1 generates SNOW F9 MAC-I code that is 8 bytes long, having first 4 bytes
                    // set to zero. PDCP MAC-I is in the last 4 bytes.
                    *((uint32_t*)(job->out_packet->address + job->out_packet->offset)) = 
                        *((uint32_t*)(job->sec_context->mac_i->code + 4));
                }
                else if(job->sec_context->pdcp_crypto_info->integrity_algorithm == SEC_ALG_AES)
                {

                    // SEC 3.1 generates AES CMAC MAC-I code that is 8 bytes long, 
                    // having PDCP MAC-I in the first 4 bytes. Last 4 bytes unused.
                    *((uint32_t*)(job->out_packet->address + job->out_packet->offset)) = 
                        *((uint32_t*)(job->sec_context->mac_i->code));
                }

                SEC_DEBUG("Generated MAC-I = 0x%x", *((uint32_t*)(job->out_packet->address + job->out_packet->offset)));
            }
#if !defined(PDCP_TEST_SNOW_F9_ONLY) && !defined(PDCP_TEST_AES_CMAC_ONLY)
            else
#else
            if(job->sec_context->pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION)
#endif
            {
                // Decapsulation. MAC-I validation was performed.
                if(job->sec_context->pdcp_crypto_info->integrity_algorithm == SEC_ALG_SNOW)
                {
                    // SEC 3.1 is capable of doing MAC-I validation itself for SNOW F9
                    mac_i_check_failed = hw_icv_check_failed(job->descr);
                }
                else if (job->sec_context->pdcp_crypto_info->integrity_algorithm == SEC_ALG_AES)
                {
                    // Do manually the MAC-I validation for AES CMAC.
                    // SEC 3.1 does not support ICV len smaller than 8 bytes for AES CMAC.
                    original_mac_i = (*(uint32_t*)(job->in_packet->address +
                                                   job->in_packet->length -
                                                   PDCP_MAC_I_LENGTH));

                    mac_i_check_failed = (original_mac_i != *((uint32_t*)(job->sec_context->mac_i->code)));
                }

                // Check if MAC-I validation failed
                if(mac_i_check_failed)
                {
                    SEC_ERROR("Integrity check FAILED for packet!. "
                              "in pkt addr(virt sec_packet_t) = %p. out pkt addr(virt sec_packet_t) = %p"
                              "original MAC-I = 0x%x. generated MAC-I = 0x%x\n",
                              job->in_packet, job->out_packet,
                              original_mac_i,
                              *((uint32_t*)(job->sec_context->mac_i->code)));

                    status = SEC_STATUS_ERROR;
                    // TODO: set error info to: MAC-I validation failed.
                }
            }
        }

        // increment the consumer index for the current job ring
        job_ring->cidx = SEC_CIRCULAR_COUNTER(job_ring->cidx, SEC_JOB_RING_SIZE);

        // packet is processed by SEC engine, notify it to UA
        sec_context = saved_job.sec_context;

        // if context is retiring, set a suggestive status for the packets notified to UA
        if(sec_context->state == SEC_CONTEXT_RETIRING)
        {
            // at this point, PI per context is frozen, context is retiring,
            // no more packets can be submitted for it.
            status = (CONTEXT_GET_PACKETS_NO(sec_context) > 1) ?
                     SEC_STATUS_OVERDUE : SEC_STATUS_LAST_OVERDUE;
        }

        // call the calback
        // TODO: set error_info on MAC-I validation failed, if this is the case.
        ret = sec_context->notify_packet_cbk(saved_job.in_packet,
                                             saved_job.out_packet,
                                             saved_job.ua_handle,
                                             status,
                                             0); // no error

        // consume processed packet for this sec context
        CONTEXT_CONSUME_PACKET(sec_context);
        notified_packets_no++;

        // UA requested to exit
        if (ret == SEC_RETURN_STOP)
        {
            *packets_no = notified_packets_no;
            return SEC_SUCCESS; 
        }
    }

    *packets_no = notified_packets_no;

    return SEC_SUCCESS;
}

static void flush_job_rings()
{
    sec_job_ring_t * job_ring = NULL;
    int i = 0;

    for (i = 0; i < g_job_rings_no; i++)
    {
        job_ring = &g_job_rings[i];

        // Producer index is frozen. If consumer index is not equal
        // with producer index, then we must sit and wait until all
        // packets are processed by SEC on this job ring.
        while(job_ring->pidx != job_ring->cidx)
        {
            hw_flush_job_ring(job_ring);
        }
    }
}
/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

sec_return_code_t sec_init(const sec_config_t *sec_config_data,
                           uint8_t job_rings_no,
                           const sec_job_ring_descriptor_t **job_ring_descriptors)
{
    int i = 0;
    int ret = 0;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_IDLE,
               SEC_DRIVER_ALREADY_INITALIZED,
               "Driver already initialized");

    // Validate input arguments
    SEC_ASSERT(job_rings_no <= MAX_SEC_JOB_RINGS, SEC_INVALID_INPUT_PARAM,
               "Requested number of job rings(%d) is greater than maximum hw supported(%d)",
               job_rings_no, MAX_SEC_JOB_RINGS);

    SEC_ASSERT(job_ring_descriptors != NULL, SEC_INVALID_INPUT_PARAM, "job_ring_descriptors is NULL");
    SEC_ASSERT(sec_config_data != NULL, SEC_INVALID_INPUT_PARAM, "sec_config_data is NULL");
    SEC_ASSERT(sec_config_data->memory_area != NULL, SEC_INVALID_INPUT_PARAM,
               "sec_config_data->memory_area is NULL");

    // DMA memory area must be cacheline aligned
    SEC_ASSERT ((dma_addr_t)sec_config_data->memory_area % CACHE_LINE_SIZE == 0, 
                SEC_INVALID_INPUT_PARAM,
                "Configured memory is not cacheline aligned");


    g_job_rings_no = job_rings_no;
    memset(g_job_rings, 0, sizeof(g_job_rings));
    SEC_INFO("Configuring %d number of SEC job rings", g_job_rings_no);

    // Configure DMA-capable memory area assigned to the driver by UA
    g_dma_mem_start = sec_config_data->memory_area;
    g_dma_mem_free = g_dma_mem_start;
    SEC_INFO("Using DMA memory area with start address = %p", g_dma_mem_start);
    
    // Read configuration data from DTS (Device Tree Specification).
    ret = sec_configure(g_job_rings_no, g_job_rings);
    SEC_ASSERT(ret == SEC_SUCCESS, SEC_INVALID_INPUT_PARAM, "Failed to configure SEC driver");

    // Per-job ring initialization
    for (i = 0; i < g_job_rings_no; i++)
    {
        // Initialize the context pool per JR
        // No need for thread synchronizations mechanisms for this pool because
        // one of the assumptions for this API is that only one thread will
        // create/delete contexts for a certain JR (also known as the producer of the JR).
        ret = init_contexts_pool(&(g_job_rings[i].ctx_pool),
                                 &g_dma_mem_free,
                                 MAX_SEC_CONTEXTS_PER_POOL,
                                 THREAD_UNSAFE_POOL);
        if (ret != SEC_SUCCESS)
        {
            SEC_ERROR("Failed to initialize SEC context pool "
                      "for job ring id %d. Starting sec_release()", g_job_rings[i].jr_id);
            sec_release();
            g_driver_state = SEC_DRIVER_STATE_IDLE;
            return ret;
        }

        // Configure each owned job ring with UIO data:
        // - UIO device file descriptor
        // - UIO mapping for SEC registers
        ret = sec_config_uio_job_ring(&g_job_rings[i]);
        if (ret != SEC_SUCCESS)
        {
            SEC_ERROR("Failed to configure UIO settings "
                      "for job ring id %d. Starting sec_release()",
                      g_job_rings[i].jr_id);
            sec_release();
            g_driver_state = SEC_DRIVER_STATE_IDLE;
            return ret;
        }

        // Initialize job ring
        ret = init_job_ring(&g_job_rings[i], &g_dma_mem_free, sec_config_data->work_mode);
        if (ret != SEC_SUCCESS)
        {
            SEC_ERROR("Failed to initialize job ring with id %d. "
                      "Starting sec_release()", g_job_rings[i].jr_id);
            sec_release();
            g_driver_state = SEC_DRIVER_STATE_IDLE;
            return ret;
        }

        g_job_ring_handles[i].job_ring_handle = (sec_job_ring_handle_t)&g_job_rings[i];
        g_job_ring_handles[i].job_ring_irq_fd = g_job_rings[i].uio_fd;
    }

    // Initialize the global pool of contexts also.
    // We need thread synchronizations mechanisms for this pool.
    ret = init_contexts_pool(&g_ctx_pool,
                             &g_dma_mem_free,
                             MAX_SEC_CONTEXTS_PER_POOL,
                             THREAD_SAFE_POOL);
    if (ret != SEC_SUCCESS)
    {
        SEC_ERROR("Failed to initialize global pool of SEC contexts. "
                "Starting sec_release()");
        sec_release();
        g_driver_state = SEC_DRIVER_STATE_IDLE;
        return ret;
    }

    // Remember initial work mode
    g_sec_work_mode = sec_config_data->work_mode;

    // Return handles to job rings
    *job_ring_descriptors =  &g_job_ring_handles[0];

    // Update driver state
    g_driver_state = SEC_DRIVER_STATE_STARTED;

    return SEC_SUCCESS;
}

sec_return_code_t sec_release()
{
    int i;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED ,
               "Driver release is in progress or driver not initialized");

    // Update driver state
    g_driver_state = SEC_DRIVER_STATE_RELEASE;

    // If any packets in flight for any SEC context, poll and wait
    // until all packets are received and silently discarded.
    flush_job_rings();

    for (i = 0; i < g_job_rings_no; i++)
    {
        // destroy the contexts pool per JR
        destroy_contexts_pool(&(g_job_rings[i].ctx_pool));

    	shutdown_job_ring(&g_job_rings[i]);
    }
    g_job_rings_no = 0;

    // destroy the global context pool also
    destroy_contexts_pool(&g_ctx_pool);

    memset(g_job_ring_handles, 0, sizeof(g_job_ring_handles));
    g_driver_state = SEC_DRIVER_STATE_IDLE;

    return SEC_SUCCESS;
}

sec_return_code_t sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                                           const sec_pdcp_context_info_t *sec_ctx_info,
                                           sec_context_handle_t *sec_ctx_handle)
{
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;
    sec_context_t * ctx = NULL;
    int ret = 0;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED ,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(sec_ctx_info != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_info is NULL");
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");
    SEC_ASSERT(sec_ctx_info->notify_packet != NULL, 
               SEC_INVALID_INPUT_PARAM, 
               "sec_ctx_inf has NULL notify_packet function pointer");

    // Crypto keys must come from DMA memory area and must be cacheline aligned
    SEC_ASSERT((dma_addr_t)sec_ctx_info->cipher_key % CACHE_LINE_SIZE == 0,
               SEC_INVALID_INPUT_PARAM,
               "Configured crypto key is not cacheline aligned");

    if(sec_ctx_info->integrity_key != NULL)
    {
        // Authentication keys must come from DMA memory area and must be cacheline aligned
        SEC_ASSERT((dma_addr_t)sec_ctx_info->integrity_key % CACHE_LINE_SIZE == 0,
                   SEC_INVALID_INPUT_PARAM,
                   "Configured integrity key is not cacheline aligned");
    }

    // Either UA specifies a job ring to associate with this context,
    // either the driver will choose a job ring in round robin fashion.
    if(job_ring == NULL)
    {
        /* Implement a round-robin assignment of JRs to this context */
        g_last_jr_assigned = SEC_CIRCULAR_COUNTER(g_last_jr_assigned, g_job_rings_no);
        job_ring = &g_job_rings[g_last_jr_assigned];
    }

    // Try to get a free context from the JR's pool. Run garbage collector for this JR.
    //
    // The advantage of the JR's pool is that it needs NO synchronization mechanisms,
    // because only one thread accesses it: the producer thread.
    // However, if the JR's pool is full even after running the garbage collector,
    // try to get a free context from the global pool. The disadvantage of the
    // global pool is that the access to it needs to be synchronized because it can
    // be accessed simultaneously by 2 threads (the producer thread of JR1 and the
    // producer thread for JR2).
    if((ctx = get_free_context(&job_ring->ctx_pool)) == NULL)
    {
    	// get free context from the global pool of contexts (with lock)
        if((ctx = get_free_context(&g_ctx_pool)) == NULL)
        {
			// no free contexts in the global pool
			return SEC_DRIVER_NO_FREE_CONTEXTS;
		}
    }

    ASSERT(ctx != NULL);
    ASSERT(ctx->pool != NULL);

    // set the notification callback per context
    ctx->notify_packet_cbk = sec_ctx_info->notify_packet;
    // Set the JR handle.
    ctx->jr_handle = (sec_job_ring_handle_t)job_ring;
    // Set the crypto info
    ret = sec_pdcp_context_set_crypto_info(ctx, sec_ctx_info);
    if(ret != SEC_SUCCESS)
    {
        SEC_ERROR("sec_ctx_info contains invalid data");
        free_or_retire_context(ctx->pool, ctx);
        return SEC_INVALID_INPUT_PARAM;
    }

    // provide to UA a SEC ctx handle
	*sec_ctx_handle = (sec_context_handle_t)ctx;

    return SEC_SUCCESS;
}

sec_return_code_t sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle)
{
	sec_contexts_pool_t * pool = NULL;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");

    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;

    // Validate that context handle contains valid bit patterns
    SEC_ASSERT(COND_EXPR1_EQ_AND_EXPR2_EQ(sec_context->start_pattern,
                                          CONTEXT_VALIDATION_PATTERN,
                                          sec_context->end_pattern,
                                          CONTEXT_VALIDATION_PATTERN),
               SEC_INVALID_INPUT_PARAM,
               "sec_ctx_handle is invalid");

    pool = sec_context->pool;
    ASSERT (pool != NULL);

    // Now try to free the current context. If there are packets
    // in flight the context will be retired (not freed). The context
    // will be freed in the next garbage collector call.
    return free_or_retire_context(pool, sec_context);
}

sec_return_code_t sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no)
{
    sec_job_ring_t * job_ring = NULL;
    uint32_t notified_packets_no = 0;
    uint32_t notified_packets_no_per_jr = 0;
    int32_t packets_left_to_notify = 0;
    int32_t i = 0;
    int32_t ret = SEC_SUCCESS;
    int32_t no_more_packets_on_jrs = 0;
    int32_t jr_limit = 0; // limit computed per JR

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments

    // - A limit equal with zero is considered invalid because it makes no sense to call
    // sec_poll and request for no notification.
    // - A weight of zero is considered invalid because the weighted round robing algorithm
    // used for polling the JRs needs a weight != 0.
    // To skip the round robin algorithm, UA can call sec_poll_per_jr for each JR and thus
    // implement its own algorithm.
    // - A limit smaller or equal than weight is considered invalid.
    SEC_ASSERT(!(limit == 0 || weight == 0 || (limit <= weight) || (weight > SEC_JOB_RING_HW_SIZE)),
               SEC_INVALID_INPUT_PARAM,
               "Invalid limit/weight parameter configuration");

    // Poll job rings
    // If limit < 0 -> poll JRs in round robin fashion until no more notifications are available.
    // If limit > 0 -> poll JRs in round robin fashion until limit is reached.

    // Exit from while if one of the following is true:
    //  - the required number of notifications were raised to UA.
    //  - there are no more done jobs on either of the available JRs.
    while ((notified_packets_no != limit) &&
            no_more_packets_on_jrs != 0)
    {
        no_more_packets_on_jrs = 0;
        for (i = 0; i < g_job_rings_no; i++)
        {
            job_ring = &g_job_rings[i];

            // Compute the limit for this JR
            // If there are less packets to notify than configured weight,
            // then notify the smaller number of packets.

            // how many packets do we have until reaching the budget?
            packets_left_to_notify = (limit > 0) ? (limit - notified_packets_no) : limit;
            // calculate budget per job ring
            jr_limit = (packets_left_to_notify < weight) ? packets_left_to_notify  : weight ;

            // Poll one JR
            ret = hw_poll_job_ring(job_ring, jr_limit, &notified_packets_no_per_jr);
            SEC_ASSERT(ret == SEC_SUCCESS, ret, "Error polling SEC engine job ring with id %d", job_ring->jr_id);

            // Update flag used to identify if there are no more notifications
            // in either of the available JRs.
            no_more_packets_on_jrs |= notified_packets_no_per_jr;

            // Update total number of packets notified to UA
            notified_packets_no += notified_packets_no_per_jr;
            if (notified_packets_no == limit)
            {
                break;
                // exit for loop with notified_packets_no == limit -> while loop will exit too
            }

        }
    }

    if (packets_no != NULL)
    {
        *packets_no = notified_packets_no;
    }

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_NAPI
    if (limit < 0)
    {
        enable_irq();
    }
    else if (notified_packets_no < limit)
    {
        enable_irq();
    }
#endif

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_IRQ
    // Always enable IRQ generation when in pure IRQ mode
    enable_irq();
#endif
    return SEC_SUCCESS;
}

sec_return_code_t sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle, 
                                    int32_t limit, 
                                    uint32_t *packets_no)
{
    int ret = SEC_SUCCESS;
    uint32_t notified_packets_no = 0;
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(job_ring != NULL, SEC_INVALID_INPUT_PARAM, "job_ring_handle is NULL");
    SEC_ASSERT(!((limit == 0) || (limit > SEC_JOB_RING_HW_SIZE)),
               SEC_INVALID_INPUT_PARAM,
               "Invalid limit parameter configuration");

    // Poll job ring
    // If limit < 0 -> poll JR until no more notifications are available.
    // If limit > 0 -> poll JR until limit is reached.

    // Run hw poll job ring
    ret = hw_poll_job_ring(job_ring, limit, &notified_packets_no);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Error polling SEC engine job ring with id %d", job_ring->jr_id);

    if (packets_no != NULL)
    {
        *packets_no = notified_packets_no;
    }

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_NAPI
    if (limit < 0)
    {
        hw_enable_done_irq_on_job_ring(job_ring);
    }
    else if (notified_packets_no < limit)
    {
        hw_enable_done_irq_on_job_ring(job_ring);
    }
#endif

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_IRQ
    // Always enable IRQ generation when in pure IRQ mode
    hw_enable_done_irq_on_job_ring(job_ring);

#endif
    return SEC_SUCCESS;
}

sec_return_code_t sec_process_packet(sec_context_handle_t sec_ctx_handle,
                                     const sec_packet_t *in_packet,
                                     const sec_packet_t *out_packet,
                                     uint8_t do_integrity_check,
                                     ua_context_handle_t ua_ctx_handle)
{
    int ret = SEC_SUCCESS;
    sec_job_t *job = NULL;
    sec_job_ring_t *job_ring = NULL;

#if FSL_SEC_ENABLE_SCATTER_GATHER == ON
#error "Scatter/Gather support is not implemented!"
#endif

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");
    SEC_ASSERT(in_packet != NULL, SEC_INVALID_INPUT_PARAM, "in_packet is NULL");
    SEC_ASSERT(out_packet != NULL, SEC_INVALID_INPUT_PARAM, "out_packet is NULL");

    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;

    // Validate that context handle contains valid bit patterns
    SEC_ASSERT(COND_EXPR1_EQ_AND_EXPR2_EQ(sec_context->start_pattern,
                                          CONTEXT_VALIDATION_PATTERN,
                                          sec_context->end_pattern,
                                          CONTEXT_VALIDATION_PATTERN),
               SEC_INVALID_INPUT_PARAM,
               "sec_ctx_handle is invalid");

    SEC_ASSERT(!(sec_context->state == SEC_CONTEXT_RETIRING),
               SEC_CONTEXT_MARKED_FOR_DELETION,
               "SEC context is marked for deletion. "
               "Wait until all in-fligh packets are processed.");
    ASSERT(sec_context->state != SEC_CONTEXT_UNUSED);

    // Integrity check algorithm can be run only for PDCP control-plane packets.
    SEC_ASSERT(!COND_EXPR1_EQ_AND_EXPR2_NEQ(do_integrity_check,
                                           1,
                                           sec_context->pdcp_crypto_info->user_plane,
                                           PDCP_CONTROL_PLANE),
               SEC_INVALID_INPUT_PARAM,
               "Cannot do integrity check on data plane PDCP context!");

    job_ring = (sec_job_ring_t *)sec_context->jr_handle;
    ASSERT(job_ring != NULL);

    // check if the Job Ring is full
    if(SEC_JOB_RING_IS_FULL(job_ring, SEC_JOB_RING_SIZE, SEC_JOB_RING_HW_SIZE))
    {
        return SEC_JR_IS_FULL;
    }

    // get first available job from job ring
    job = &job_ring->jobs[job_ring->pidx];

    // update job with crypto context and in/out packet data
    job->in_packet = in_packet;
    job->out_packet = out_packet;

    job->sec_context = sec_context;
    job->ua_handle = ua_ctx_handle;
    job->is_integrity_algo = do_integrity_check;

    // update descriptor with pointers to input/output data and pointers to crypto information
    ASSERT(job->descr != NULL);

    // Update SEC descriptor. If HFN reached the configured threshold, then
    // the return code will be SEC_HFN_THRESHOLD_REACHED. 
    // We MUST return it from sec_process_packet() as well!!!
    ret = sec_pdcp_context_update_descriptor(sec_context, job, job->descr, do_integrity_check);
    SEC_ASSERT(ret == SEC_SUCCESS, ret,
               "sec_pdcp_context_update_descriptor returned error code %d", ret);

    // keep count of submitted packets for this sec context 
    CONTEXT_ADD_PACKET(sec_context);

    // increment the producer index for the current job ring
    job_ring->pidx = SEC_CIRCULAR_COUNTER(job_ring->pidx, SEC_JOB_RING_SIZE);


    // Enqueue this descriptor into the Fetch FIFO of this JR
    // Need write memory barrier here so that descriptor is written before
    // packet is enqueued in SEC's job ring. 
    // GCC's builtins offers full memry barrier, use it here.
    atomic_synchronize();
    hw_enqueue_packet_on_job_ring(job_ring, job->descr_phys_addr);

    return ret;
}

uint32_t sec_get_last_error(void)
{
    // stub function
    return 0;
}

const char* sec_get_status_message(sec_status_t status)
{
    // TODO: to be implemented
    return g_status_string[0];
}

const char* sec_get_error_message(sec_return_code_t error)
{
    // TODO: to be implemented
    return g_return_code_string[0];
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
