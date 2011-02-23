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
#include "sec_contexts.h"

#include <stddef.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>


/**********************************************************************
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*
 * Current implementation is just for validating the API!             *
 * This is not to be taken neither as part of the final implementation*
 * nor as the final implementation itself!                            *
 **********************************************************************/
#warning "SEC user space driver current implementation is just for testing the API!"

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/
/* Max sec contexts per pool computed based on the number of Job Rings assigned.
 * There is one global pool and one pool per JR. All the available contexts will be
 * split evenly between all the pools. */
#define MAX_SEC_CONTEXTS_PER_POOL   (SEC_MAX_PDCP_CONTEXTS / (SEC_NUMBER_JOB_RINGS + 1))

/* Maximum number of job rings supported by SEC hardware */
#define MAX_SEC_JOB_RINGS         4

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/* SEC job */
typedef struct sec_job_s
{
    sec_packet_t in_packet;
    sec_packet_t out_packet;
    ua_context_handle_t ua_handle;
    sec_context_t * sec_context;
}sec_job_t;

/* SEC Job Ring */
typedef struct sec_job_ring_s
{
    /* Pool of SEC contexts */
	sec_contexts_pool_t ctx_pool;
    /* Ring of jobs. In this stub the same ring is used for
     * input jobs and output jobs. */
    sec_job_t jobs[SEC_JOB_RING_SIZE];
    // consumer index for job ring (jobs array)
    int cidx;
    // producer index for job ring (jobs array)
    int pidx;
    /* The file descriptor used for polling from user space
     * for interrupts notifications */
    int irq_fd;
}sec_job_ring_t;
/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

#define SEC_CIRCULAR_COUNTER(x, max)   ((x) + 1) * ((x) != (max - 1))

// The number of jobs in a JOB RING
#define SEC_JOB_RING_DIFF(ring_max_size, pi, ci) (((pi) < (ci)) ? ((ring_max_size) + (pi) - (ci)) : ((pi) - (ci)))

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
/* Job rings used for communication with SEC HW */
static sec_job_ring_t job_rings[MAX_SEC_JOB_RINGS];
static int g_job_rings_no = 0;

/* Array of job ring handles provided to UA from sec_init() function. */
static sec_job_ring_descriptor_t g_job_ring_handles[MAX_SEC_JOB_RINGS];

static int sec_work_mode = 0;

/* Last JR assigned to a context by the SEC driver using a round robin algorithm.
 * Not used if UA associates the contexts created to a certain JR.*/
static int last_jr_assigned = 0;

// global context pool
static sec_contexts_pool_t g_ctx_pool;
/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/** @brief Initialize the data stored in a ::sec_job_ring_t structure.
 *
 **/
static void reset_job_ring(sec_job_ring_t * job_ring);

/** @brief Enable IRQ generation for all SEC's job rings.
 *
 * @retval 0 for success
 * @retval other value for error
 */
static int enable_irq();

/** @brief Enable IRQ generation for all SEC's job rings.
 *
 * @param [in]  job_ring    The job ring to enable IRQs for.
 * @retval 0 for success
 * @retval other value for error
 */
static int enable_irq_per_job_ring(sec_job_ring_t *job_ring);

/** @brief Poll the HW for already processed jobs in the JR
 * and notify the available jobs to UA.
 *
 * @param [in]  job_ring    The job ring to poll.
 * @param [in]  limit       The maximum number of jobs to notify.
 *                          If set to -1, all available jobs are notified.
 * @param [out] packets_no  No of jobs notified to UA.
 *
 * @retval 0 for success
 * @retval other value for error
 */
static uint32_t hw_poll_job_ring(sec_job_ring_t * job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

static void reset_job_ring(sec_job_ring_t * job_ring)
{
    memset(job_ring, 0, sizeof(sec_job_ring_t));
}

static int enable_irq()
{
    // to be implemented
    return 0;
}

static int enable_irq_per_job_ring(sec_job_ring_t *job_ring)
{
    // to be implemented
    return 0;
}

static uint32_t hw_poll_job_ring(sec_job_ring_t * job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no)
{
    /* Stub Implementation - START */
    sec_context_t * sec_context;
    sec_job_t *job;
    int jobs_no_to_notify = 0; // the number of done jobs to notify to UA
    sec_status_t status = SEC_STATUS_SUCCESS;
    int notified_packets_no = 0;
    int number_of_jobs_available = 0;

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

        sec_context = job->sec_context;
        assert (sec_context->notify_packet_cbk != NULL);

        pthread_mutex_lock(&sec_context->mutex);

        status = SEC_STATUS_SUCCESS;
        // if context is retiring, set a suggestive status for the
        // packets notified to UA
        if (sec_context->usage == SEC_CONTEXT_RETIRING)
        {
            assert(sec_context->packets_no >= 1);
            status = (sec_context->packets_no > 1) ? SEC_STATUS_OVERDUE : SEC_STATUS_LAST_OVERDUE;
        }
        // call the calback
        sec_context->notify_packet_cbk(&job->in_packet,
                                       &job->out_packet,
                                       job->ua_handle,
                                       status,
                                       0);

        // decrement packet reference count in sec context
        sec_context->packets_no--;

        pthread_mutex_unlock(&sec_context->mutex);

        notified_packets_no++;

        // increment the consumer index for the current job ring
        job_ring->cidx = SEC_CIRCULAR_COUNTER(job_ring->cidx, SEC_JOB_RING_SIZE);
    }

    *packets_no = notified_packets_no;

    return SEC_SUCCESS;
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

uint32_t sec_init(sec_config_t *sec_config_data,
                  uint8_t job_rings_no,
                  sec_job_ring_descriptor_t **job_ring_descriptors)
{
    /* Stub Implementation - START */
    int i;
    int ret;

    for (i = 0; i < SEC_NUMBER_JOB_RINGS; i++)
    {
        reset_job_ring(&job_rings[i]);

        // Initialize the context pool per JR
        // No need for thread synchronizations mechanisms for this pool because
        // one of the assumptions for this API is that only one thread will
        // create/delete contexts for a certain JR (also known as the producer of the JR).
        ret = init_contexts_pool(&(job_rings[i].ctx_pool), MAX_SEC_CONTEXTS_PER_POOL, THREAD_UNSAFE_POOL);
        assert(ret == 0);

        job_rings[i].irq_fd = i + 1;

        g_job_ring_handles[i].job_ring_handle = (sec_job_ring_handle_t)&job_rings[i];
        g_job_ring_handles[i].job_ring_irq_fd = job_rings[i].irq_fd;
    }

    g_job_rings_no = job_rings_no;

    // initialize the global pool of contexts also
    // we need for thread synchronizations mechanisms for this pool
    ret = init_contexts_pool(&g_ctx_pool, MAX_SEC_CONTEXTS_PER_POOL, THREAD_SAFE_POOL);
    assert(ret == 0);

    // Remember initial work mode
    sec_work_mode = sec_config_data->work_mode;
    if (sec_work_mode == SEC_INTERRUPT_MODE )
    {
        enable_irq();
    }

    *job_ring_descriptors =  &g_job_ring_handles[0];

    return SEC_SUCCESS;
    /* Stub Implementation - END */
}

uint32_t sec_release()
{
    /* Stub Implementation - START */
    int i;

    for (i = 0; i < g_job_rings_no; i++)
    {
        // destroy the contexts pool per JR
        destroy_contexts_pool(&(job_rings[i].ctx_pool));

    	reset_job_ring(&job_rings[i]);
    }
    g_job_rings_no = 0;

    // destroy the global context pool also
    destroy_contexts_pool(&g_ctx_pool);

    return SEC_SUCCESS;

    /* Stub Implementation - END */
}

uint32_t sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                                  sec_pdcp_context_info_t *sec_ctx_info, 
                                  sec_context_handle_t *sec_ctx_handle)
{
    /* Stub Implementation - START */
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;
    sec_context_t * ctx = NULL;

    if (sec_ctx_handle == NULL || sec_ctx_info == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    if (sec_ctx_info->notify_packet == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    *sec_ctx_handle = NULL;

    if(job_ring == NULL)
    {
        /* Implement a round-robin assignment of JRs to this context */
        last_jr_assigned = SEC_CIRCULAR_COUNTER(last_jr_assigned, g_job_rings_no);
        assert(last_jr_assigned >= 0 && last_jr_assigned < g_job_rings_no);
        job_ring = &job_rings[last_jr_assigned];
    }

    // Try to get a free context from the JR's pool
    // The advantage of the JR's pool is that it needs NO synchronization mechanisms.
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
    assert(ctx != NULL);
    assert(ctx->pool != NULL);

    ctx->notify_packet_cbk = sec_ctx_info->notify_packet;
    // TODO: JR handle could be set when the pool of contexts is initialized
    ctx->jr_handle = (sec_job_ring_handle_t)job_ring;

	// provide to UA a sec ctx handle
	*sec_ctx_handle = (sec_context_handle_t)ctx;

    return SEC_SUCCESS;

    /* Stub Implementation - END */


    // 1. ret = get_free_context();

    // No free contexts available in free list. 
    // Maybe there are some retired contexts that can be recycled.
    // Run context garbage collector routine.
    //if (ret != 0)
    //{
    //    ret = collect_retired_contexts();
    //}

    //if (ret != 0 )
    //{
    //    return error, there are no free contexts
    //}

    // 2. Associate context with job ring
    // 3. Update context with PDCP info from UA, create shared descriptor
    // 4. return context handle to UA
    // 5. Run context garbage collector routine
}

uint32_t sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle)
{
    /* Stub Implementation - START */
	sec_contexts_pool_t * pool = NULL;

    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;
    if (sec_context == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    pool = sec_context->pool;
    assert (pool != NULL);

    // Now try to free the current context. If there are packets
    // in flight the context will be retired (not freed). The context
    // will be freed in the next garbage collector call.
    return free_or_retire_context(pool, sec_context);

    /* Stub Implementation - END */

    // 1. Mark context as retiring
    // 2. If context.packet_count == 0 then move context from in-use list to free list. 
    //    Else move to retiring list
    // 3. Run context garbage collector routine
}

uint32_t sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no)
{
    /* Stub Implementation - START */
    sec_job_ring_t * job_ring = NULL;
    uint32_t notified_packets_no = 0;
    uint32_t notified_packets_no_per_jr = 0;
    int i = 0;
    int ret = SEC_SUCCESS;
    int no_more_packets_on_jrs = 0;
    int jr_limit = 0; // limit computed per JR

    if (packets_no == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    /* - A limit equal with zero is considered invalid because it makes no sense to call
       sec_poll and request for no notification.
       - A weight of zero is considered invalid because the weighted round robing algorithm
       used for polling the JRs needs a weight != 0.
       To skip the round robin algorithm, UA can call sec_poll_per_jr for each JR and thus
       implement its own algorithm.
       - A limit smaller or equal than weight is considered invalid.
    */
    if (limit == 0 || weight == 0 || (limit <= weight) || (weight > SEC_JOB_RING_SIZE))
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    /* Poll job rings
       If limit < 0 && weight > 0 -> poll JRs in round robin fashion until
                                     no more notifications are available.
       If limit > 0 && weight > 0 -> poll JRs in round robin fashion until
                                     limit is reached.
     */

    assert (g_job_rings_no != 0);
    /* Exit from while if one of the following is true:
     * - the required number of notifications were raised to UA.
     * - there are no more done jobs on either of the available JRs.
     * */
    while ((notified_packets_no == limit) ||
            no_more_packets_on_jrs == 0)
    {
        no_more_packets_on_jrs = 0;
        for (i = 0; i < g_job_rings_no; i++)
        {
            job_ring = &job_rings[i];

            /* Compute the limit for this JR
               If there are less packets to notify then configured weight,
               then notify the smaller number of packets.
            */
            int packets_left_to_notify = limit - notified_packets_no;
            jr_limit = (packets_left_to_notify < weight) ? packets_left_to_notify  : weight ;

            /* Poll one JR */
            ret = hw_poll_job_ring((sec_job_ring_handle_t)job_ring, jr_limit, &notified_packets_no_per_jr);
            if (ret != SEC_SUCCESS)
            {
                return ret;
            }
            /* Update total number of packets notified to UA */
            notified_packets_no += notified_packets_no_per_jr;

            /* Update flag used to identify if there are no more notifications
             * in either of the available JRs.*/
            no_more_packets_on_jrs |= notified_packets_no_per_jr;
        }
    }
    *packets_no = notified_packets_no;

    if (limit < 0)// and no more ready packets in SEC
    {
        enable_irq();
    }
    else if (notified_packets_no < limit)// and no more ready packets in SEC
    {
        enable_irq();
    }

    return SEC_SUCCESS;

    /* Stub Implementation - END */

    // 1. call sec_hw_poll() to check directly SEC's Job Rings for ready packets.
    //
    // sec_hw_poll() will do:
    // a. SEC 3.1 specific:
    //      - poll for errors on /dev/sec_uio_x. Raise error notification to UA if this is the case.
    // b. for all owned job rings:
    //      - check and notify ready packets.
    //      - decrement packet reference count per context
    //      - other
}

uint32_t sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle, int32_t limit, uint32_t *packets_no)
{
    /* Stub Implementation - START */
    int ret = SEC_SUCCESS;
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;

    if(job_ring == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    if (packets_no == NULL || limit == 0)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    *packets_no = 0;

    // run hw poll job ring
    ret = hw_poll_job_ring(job_ring, limit, packets_no);
    if (ret != SEC_SUCCESS)
    {
        return ret;
    }

    if (limit < 0)// and no more ready packets  in SEC
    {
        enable_irq_per_job_ring(job_ring);
    }
    else if (*packets_no < limit)// and no more ready packets  in SEC
    {
        enable_irq_per_job_ring(job_ring);
    }

    return SEC_SUCCESS;
    /* Stub Implementation - END */

    // 1. call hw_poll_job_ring() to check directly SEC's Job Ring for ready packets.
}

uint32_t sec_process_packet(sec_context_handle_t sec_ctx_handle,
                            sec_packet_t *in_packet,
                            sec_packet_t *out_packet,
                            ua_context_handle_t ua_ctx_handle)
{
    /* Stub Implementation - START */

#if FSL_SEC_ENABLE_SCATTER_GATHER == ON
#error "Scatter/Gather support is not implemented!"
#endif
    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;

    if (sec_context == NULL ||
        sec_context->usage == SEC_CONTEXT_UNUSED)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    if (sec_context->usage == SEC_CONTEXT_RETIRING)
    {
        return SEC_CONTEXT_MARKED_FOR_DELETION;
    }

    sec_job_ring_t * job_ring = (sec_job_ring_t *)sec_context->jr_handle;
    if(job_ring == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    if (in_packet == NULL || out_packet == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    // check of the Job Ring is full (the difference between PI and CI is equal with the JR SIZE - 1)
    if(SEC_JOB_RING_DIFF(SEC_JOB_RING_SIZE, job_ring->pidx, job_ring->cidx) == (SEC_JOB_RING_SIZE - 1))
    {
        return SEC_JR_IS_FULL;
    }

    // add new job in job ring
    job_ring->jobs[job_ring->pidx].in_packet.address = in_packet->address;
    job_ring->jobs[job_ring->pidx].in_packet.offset = in_packet->offset;
    job_ring->jobs[job_ring->pidx].in_packet.length = in_packet->length;

    job_ring->jobs[job_ring->pidx].out_packet.address = out_packet->address;
    job_ring->jobs[job_ring->pidx].out_packet.offset = out_packet->offset;
    job_ring->jobs[job_ring->pidx].out_packet.length = out_packet->length;

    job_ring->jobs[job_ring->pidx].sec_context = sec_context;
    job_ring->jobs[job_ring->pidx].ua_handle = ua_ctx_handle;

    // increment packet reference count in sec context
    pthread_mutex_lock(&sec_context->mutex);
    sec_context->packets_no++;
    pthread_mutex_unlock(&sec_context->mutex);

    // increment the producer index for the current job ring
    job_ring->pidx = SEC_CIRCULAR_COUNTER(job_ring->pidx, SEC_JOB_RING_SIZE);

    return SEC_SUCCESS;
    /* Stub Implementation - END */
}

uint32_t sec_get_last_error(void)
{
    // stub function
    return 0;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
