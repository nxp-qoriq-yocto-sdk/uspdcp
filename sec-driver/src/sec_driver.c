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
#include <stddef.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>


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
/* TODO: compute max sec contexts per JR based on the SEC_ASSIGNED_JOB_RINGS bitmask */
#define MAX_SEC_CONTEXTS_PER_JR   (SEC_MAX_PDCP_CONTEXTS / MAX_SEC_JOB_RINGS)

/* Maximum number of job rings supported by SEC hardware */
#define MAX_SEC_JOB_RINGS         4
/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/* Status of a SEC context */
typedef enum sec_context_usage_e
{
    SEC_CONTEXT_UNUSED = 0,
    SEC_CONTEXT_USED,
    SEC_CONTEXT_RETIRING,
}sec_context_usage_t;

/* SEC context structure. */
typedef struct sec_context_s
{
    /* The callback called for UA notifications */
    sec_out_cbk notify_packet_cbk;
    /* The status of the sec context.*/
    sec_context_usage_t usage;
    /* The handle of the JR to which this context is affined */
    sec_job_ring_handle_t * jr_handle;
    /* Number of packets in flight for this context.*/
    int packets_no;
    /* Mutex used to synchronize the access to usage & packets_no members
     * from two different threads: Producer and Consumer.
     * TODO: find a solution to remove it.*/
    pthread_mutex_t ctx_mutex;
}sec_context_t;

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
    sec_context_t sec_contexts[MAX_SEC_CONTEXTS_PER_JR];
    int sec_contexts_no;

    /* Array of jobs per job ring */
    sec_job_t jobs[SEC_JOB_RING_SIZE];
    int jobs_no;

    /* Mutex used to synchronize access to jobs array and
     * array of contexts (sec_contexts).
       TODO: Remove this mutex by replacing the arrays
       with lock-free queues (for the case with one
       producer and one consumer) */
    pthread_mutex_t mutex;
}sec_job_ring_t;
/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/

#define SEC_CIRCULAR_COUNTER(x, max)   ((x) + 1) * ((x) != (max - 1))

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
/* Job rings used for communication with SEC HW */
static sec_job_ring_t job_rings[MAX_SEC_JOB_RINGS];
static int g_job_rings_no = 0;

/* Array of job ring handles provided to UA from sec_init() function. */
static sec_job_ring_handle_t g_job_ring_handles[MAX_SEC_JOB_RINGS];

/* Last JR assigned to a context by the SEC driver using a round robin algorithm.
 * Not used if UA associates the contexts created to a certain JR.*/
static int last_jr_assigned = 0;
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

/** @brief Run the garbage collector on the retiring contexts.
 *
 * The contexts that have packets in flight are not deleted immediately when
 * sec_delete_pdcp_context() is called instead they are marked as retiring.
 *
 * When all the packets in flight were notified to UA with #SEC_STATUS_OVERDUE and
 * #SEC_STATUS_LAST_OVERDUE statuses, the contexts can be deleted. The function where
 * we can find out the first time that all packets in flight were notified is sec_poll().
 * To minimize the response time of the UA notifications, the contexts will not be deleted
 * in the sec_poll() function. Instead the retiring contexts with no more packets in flight
 * will be deleted whenever UA creates a new context or deletes another -> this is the purpose
 * of the garbage collector.
 *
 * The garbage collector will try to free retiring contexts from the pool associated to the
 * job ring of the created/deleted context.
 * TODO: handle also retiring contexts from the global pool (perhaps in a different function)
 *
 * @param [in] job_ring          The job ringInput packet read by SEC.
 */
static void run_contexts_garbage_colector(sec_job_ring_t * job_ring);

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static void reset_job_ring(sec_job_ring_t * job_ring)
{
    int j;

    job_ring->sec_contexts_no = 0;
    for (j = 0; j < MAX_SEC_CONTEXTS_PER_JR; j++)
    {
        job_ring->sec_contexts[j].usage = SEC_CONTEXT_UNUSED;

        job_ring->sec_contexts[j].packets_no = 0;
        job_ring->sec_contexts[j].notify_packet_cbk = NULL;
        job_ring->sec_contexts[j].jr_handle = NULL;
    }

    job_ring->jobs_no = 0;
    for (j = 0; j < SEC_JOB_RING_SIZE; j++)
    {
        job_ring->jobs[j].sec_context = NULL;
        job_ring->jobs[j].ua_handle = NULL;
    }
}

static void run_contexts_garbage_colector(sec_job_ring_t * job_ring)
{
    int j = 0;
    // Iterate through the pool of contexts and check if the retiring contexts
    // can be deleted (meaning that all packets in flight were notified to UA)
    // TODO: implement a list of retiring contexts and search only through that list
    for (j = 0; j < MAX_SEC_CONTEXTS_PER_JR; j++)
    {
        // if no more packets in flight for this context, free the context
        if(job_ring->sec_contexts[j].packets_no == 0 &&
           job_ring->sec_contexts[j].usage == SEC_CONTEXT_RETIRING)
        {
            job_ring->sec_contexts[j].usage = SEC_CONTEXT_UNUSED;
            job_ring->sec_contexts[j].notify_packet_cbk = NULL;
            job_ring->sec_contexts[j].jr_handle = NULL;
            job_ring->sec_contexts_no--;
            assert(job_ring->sec_contexts_no >= 0);
        }
    }
}
/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int sec_init(int job_rings_no,
             sec_job_ring_handle_t **job_ring_handles)
{
    /* Stub Implementation - START */
    int i, j;

    for (i = 0; i < MAX_SEC_JOB_RINGS; i++)
    {
        reset_job_ring(&job_rings[i]);

        for (j = 0; j < MAX_SEC_CONTEXTS_PER_JR; j++)
        {
            pthread_mutex_init(&job_rings[i].sec_contexts[j].ctx_mutex, NULL);
        }
        pthread_mutex_init(&job_rings[i].mutex, NULL);

        g_job_ring_handles[i] = (sec_job_ring_handle_t)&job_rings[i];
    }

    g_job_rings_no = job_rings_no;

    *job_ring_handles =  &g_job_ring_handles[0];

    return SEC_SUCCESS;
    /* Stub Implementation - END */
}

int sec_release()
{
    /* Stub Implementation - START */
    int i, j;

    for (i = 0; i < g_job_rings_no; i++)
    {
        reset_job_ring(&job_rings[i]);
        for (j = 0; j < MAX_SEC_CONTEXTS_PER_JR; j++)
        {
            pthread_mutex_destroy(&job_rings[i].sec_contexts[j].ctx_mutex);
        }
        pthread_mutex_destroy(&job_rings[i].mutex);
    }
    g_job_rings_no = 0;

    return SEC_SUCCESS;

    /* Stub Implementation - END */
}

int sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                             sec_pdcp_context_info_t *sec_ctx_info, 
                             sec_context_handle_t *sec_ctx_handle)
{
    /* Stub Implementation - START */
    int i = 0;
    int found = 0;
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;

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

    // synchronize access to job ring's pool of contexts
    pthread_mutex_lock(&job_ring->mutex);

    if(job_ring->sec_contexts_no >= MAX_SEC_CONTEXTS_PER_JR)
    {
        // run a contexts garbage collector on the job ring's pool of contexts first
        // The purpose is to free some retiring contexts for which all the packets in flight
        // were notified to UA
        run_contexts_garbage_colector(job_ring);
        // if still no free contexts available in the job ring's pool of contexts
        // then return error
        // TODO: implement a global pool of contexts synchronized by a mutex
        //       and try to get a context from the global pool if the JR pool is full.
        if (job_ring->sec_contexts_no >= MAX_SEC_CONTEXTS_PER_JR)
        {
            return SEC_DRIVER_NO_FREE_CONTEXTS;
        }
    }

    found = 0;
    // find an unused context
    for(i = 0; i < MAX_SEC_CONTEXTS_PER_JR; i++)
    {
        if (job_ring->sec_contexts[i].usage == SEC_CONTEXT_UNUSED)
        {
            job_ring->sec_contexts[i].notify_packet_cbk = sec_ctx_info->notify_packet;
            job_ring->sec_contexts[i].usage = SEC_CONTEXT_USED;
            job_ring->sec_contexts[i].jr_handle = (sec_job_ring_handle_t)job_ring;
            // provide to UA a sec ctx handle
            *sec_ctx_handle = (sec_context_handle_t)&job_ring->sec_contexts[i];
            // increment the number of contexts associated to a job ring
            assert(job_ring->sec_contexts_no >= 0);
            job_ring->sec_contexts_no++;
            found = 1;
            break;
        }
    }
    assert(found == 1);

    // Run a contexts garbage collector on the job ring's pool of contexts first
    // The purpose is to free some retiring contexts for which all the packets in flight
    // were notified to UA
    run_contexts_garbage_colector(job_ring);

    pthread_mutex_unlock(&job_ring->mutex);

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

int sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle)
{
    /* Stub Implementation - START */
    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;
    if (sec_context == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    sec_job_ring_t * job_ring = (sec_job_ring_t *)sec_context->jr_handle;
    if(job_ring == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    // synchronize access to job ring's pool of contexts
    pthread_mutex_lock(&job_ring->mutex);

    // Run a contexts garbage collector on the job ring's pool of contexts first
    // The contexts are deleted only from the Producer Thread such that to minimize the
    // response time for the notifications raised on the Consumer Thread
    run_contexts_garbage_colector(job_ring);

    // Now check if the current context can be deleted or must be marked as
    // retiring because there still are packets in flight.

    // synchronize access to sec_context
    pthread_mutex_lock(&sec_context->ctx_mutex);

    if (sec_context->packets_no != 0)
    {
        // if packets in flight, do not release the context yet, move it to retiring
        // the context will be deleted when all packets in flight are notified to UA
        // TODO: move context to retiring list
        sec_context->usage = SEC_CONTEXT_RETIRING;

        pthread_mutex_unlock(&sec_context->ctx_mutex);
        pthread_mutex_unlock(&job_ring->mutex);

        return SEC_PACKETS_IN_FLIGHT;
    }
    // end of critical area per context
    pthread_mutex_unlock(&sec_context->ctx_mutex);

    // if no packets in flight then we can safely release the context
    // TODO: move context to free list
    sec_context->usage = SEC_CONTEXT_UNUSED;
    sec_context->notify_packet_cbk = NULL;
    sec_context->jr_handle = NULL;
    job_ring->sec_contexts_no--;

    // end of critical area for pool of contexts
    pthread_mutex_unlock(&job_ring->mutex);

    return SEC_SUCCESS;

    /* Stub Implementation - END */

    // 1. Mark context as retiring
    // 2. If context.packet_count == 0 then move context from in-use list to free list. 
    //    Else move to retiring list
    // 3. Run context garbage collector routine
}

#if SEC_WORKING_MODE == SEC_POLLING_MODE
int sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no)
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
            ret = sec_poll_job_ring((sec_job_ring_handle_t)job_ring, jr_limit, &notified_packets_no_per_jr);
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

int sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle, int32_t limit, uint32_t *packets_no)
{
    /* Stub Implementation - START */
    sec_context_t * sec_context;
    sec_job_t *job;
    int j = 0;
    int jobs_no_to_notify = 0; // the number of done jobs to notify to UA
    sec_status_t status;
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;
    int notified_packets_no = 0;

    if(job_ring == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    if (packets_no == NULL || limit == 0)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    pthread_mutex_lock( &job_ring->mutex );

    // Compute the number of notifications that need to be raised to UA
    // If limit < 0 -> notify all done jobs
    // If limit > total number of done jobs -> notify all done jobs
    // If limit = 0 -> error
    // If limit > 0 && limit < total number of done jobs -> notify a number of done jobs equal with limit
    jobs_no_to_notify = (limit < 0 || limit > job_ring->jobs_no) ? job_ring->jobs_no : limit;
    if (jobs_no_to_notify != 0 )
    {
        for (j = 0; j < jobs_no_to_notify; j++)
        {
            job = &job_ring->jobs[j];
            sec_context = job->sec_context;
            assert (sec_context->notify_packet_cbk != NULL);

            pthread_mutex_lock(&sec_context->ctx_mutex);

            // if context is retiring, set a suggestive status for the
            // packets notified to UA
            if (sec_context->usage == SEC_CONTEXT_RETIRING)
            {
                assert(sec_context->packets_no >= 1);
                if (sec_context->packets_no > 1)
                {
                    status = SEC_STATUS_OVERDUE;
                }
                else
                {
                    status = SEC_STATUS_LAST_OVERDUE;
                }
            }
            else
            {
                status = SEC_STATUS_SUCCESS;
            }
            // call the calback
            sec_context->notify_packet_cbk(&job->in_packet,
                                           &job->out_packet,
                                           job->ua_handle,
                                           status,
                                           0);

            // decrement packet reference count in sec context
            sec_context->packets_no--;

            pthread_mutex_unlock(&sec_context->ctx_mutex);

            notified_packets_no++;

            // reset job contents
            job->in_packet.address = 0;
            job->in_packet.offset = 0;
            job->in_packet.length = 0;
            job->out_packet.address = 0;
            job->out_packet.offset = 0;
            job->out_packet.length = 0;
            job->sec_context = NULL;
            job->ua_handle = NULL;

            job_ring->jobs_no--;
        }
    }
    pthread_mutex_unlock( &job_ring->mutex );

    *packets_no = notified_packets_no;

    return SEC_SUCCESS;
    /* Stub Implementation - END */

    // 1. call sec_hw_poll_job_ring() to check directly SEC's Job Ring for ready packets.
}

#elif SEC_WORKING_MODE == SEC_INTERRUPT_MODE
int sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no)
{
    // 1. Start software poll on device files registered for all job rings owned by this user application.
    //    Return if no IRQ generated for job rings.
    // 
    // Set timeout = 0 so that it returns immediatelly if no irq's are generated for any job ring.
    // Timeout is expressed as miliseconds. Considering LTE TTI = 1 ms and the fact that the calling thread
    // may do other processing tasks, a timeout other than 0 does not seem like an option.
    //
    // NOTE: epoll is more efficient than poll for a large number of file descriptors ~ 1000 ... 100000.
    //       Considering SEC user space driver will poll for max 4 file descriptors, poll() is used instead of epoll().
    //
    // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
    //
    //2. call sec_hw_poll() to check directly SEC's Job Rings for ready packets.
#error "sec_poll() is NOT implemented for IRQ working mode"

    return SEC_SUCCESS;
}

int sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle, int32_t limit, uint32_t *packets_no)
{
    // 1. Start software poll on device file registered for this job ring.
    //    Return if no IRQ generated for job ring.
    // 2. call sec_hw_poll_job_ring() to check directly SEC's Job Ring for ready packets.
#error "sec_poll_job_ring() is NOT implemented for IRQ working mode"
    return SEC_SUCCESS;
}
#endif

int sec_process_packet(sec_context_handle_t sec_ctx_handle,
                       sec_packet_t *in_packet,
                       sec_packet_t *out_packet,
                       ua_context_handle_t ua_ctx_handle)
{

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


    pthread_mutex_lock( &job_ring->mutex );

    if(job_ring->jobs_no >= SEC_JOB_RING_SIZE)
    {
        pthread_mutex_unlock( &job_ring->mutex );
        return SEC_JR_IS_FULL;
    }

    // add new job in job ring
    job_ring->jobs[job_ring->jobs_no].in_packet.address = in_packet->address;
    job_ring->jobs[job_ring->jobs_no].in_packet.offset = in_packet->offset;
    job_ring->jobs[job_ring->jobs_no].in_packet.length = in_packet->length;

    job_ring->jobs[job_ring->jobs_no].out_packet.address = out_packet->address;
    job_ring->jobs[job_ring->jobs_no].out_packet.offset = out_packet->offset;
    job_ring->jobs[job_ring->jobs_no].out_packet.length = out_packet->length;

    job_ring->jobs[job_ring->jobs_no].sec_context = sec_context;
    job_ring->jobs[job_ring->jobs_no].ua_handle = ua_ctx_handle;

    // increment packet reference count in sec context
    sec_context->packets_no++;
    // increment the number of jobs in jr
    job_ring->jobs_no++;

    pthread_mutex_unlock( &job_ring->mutex );

    // stub function
    return SEC_SUCCESS;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
