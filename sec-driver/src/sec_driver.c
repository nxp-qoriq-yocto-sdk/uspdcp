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
/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
/**********************************************************************
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*
 * Current implementation is just for testing the API!                *
 * This is not to be taken neither as part of the final implementation*
 * nor as the final implementation itself!                            *
 **********************************************************************/

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/
#define MAX_SEC_CONTEXTS_PER_JR   20
#define MAX_SEC_JOB_RINGS         4
/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

typedef enum sec_context_usage_e
{
	SEC_CONTEXT_UNUSED = 0,
	SEC_CONTEXT_USED,
	SEC_CONTEXT_RETIRING,
}sec_context_usage_t;

/* SEC context structure.
 * @note Current implementation is just for testing the API!
 * This is not to be taken as part of the final implementation!*/
typedef struct sec_context_s
{
	sec_out_cbk notify_packet_cbk;
	sec_context_usage_t usage;
	sec_job_ring_handle_t * jr_handle;
	int packets_no;
}sec_context_t;

typedef struct sec_job_s
{
	sec_packet_t in_packet;
	sec_packet_t out_packet;
	ua_context_handle_t ua_handle;
	sec_context_t * sec_context;
}sec_job_t;

typedef struct sec_job_ring_s
{
	sec_context_t sec_contexts[MAX_SEC_CONTEXTS_PER_JR];
	int sec_contexts_no;

	sec_job_t jobs[SEC_JOB_INPUT_RING_SIZE];
	int jobs_no;

	// mutex used to synchronize access to jobs array
	// will not be needed in the real implementation
	pthread_mutex_t mutex;
}sec_job_ring_t;
/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
static sec_job_ring_t job_rings[MAX_SEC_JOB_RINGS];
static int g_job_rings_no = 0;

static sec_job_ring_handle_t g_job_ring_handles[MAX_SEC_JOB_RINGS];

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int sec_init(int job_rings_no,
             sec_job_ring_handle_t **job_ring_handles)
{
	// stub function
	int i, j;

	for (i = 0; i < MAX_SEC_JOB_RINGS; i++)
	{
		job_rings[i].sec_contexts_no = 0;
		for (j = 0; j < MAX_SEC_CONTEXTS_PER_JR; j++)
		{
			job_rings[i].sec_contexts[j].usage = SEC_CONTEXT_UNUSED;

			job_rings[i].sec_contexts[j].packets_no = 0;
			job_rings[i].sec_contexts[j].notify_packet_cbk = NULL;
			job_rings[i].sec_contexts[j].jr_handle = NULL;
		}

		job_rings[i].jobs_no = 0;
		for (j = 0; j < SEC_JOB_INPUT_RING_SIZE; j++)
		{
			job_rings[i].jobs[j].sec_context = NULL;
			job_rings[i].jobs[j].ua_handle = NULL;
		}

		pthread_mutex_init(&job_rings[i].mutex, NULL);

		g_job_ring_handles[i] = (sec_job_ring_handle_t)&job_rings[i];
		job_ring_handles[i] = (sec_job_ring_handle_t)&job_rings[i];
	}

	g_job_rings_no = job_rings_no;

//	*job_ring_handles =  &g_job_ring_handles[0];

    return SEC_SUCCESS;
}

int sec_release()
{
	g_job_rings_no = 0;
    // stub function
    return SEC_SUCCESS;
}

int sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                             sec_pdcp_context_info_t *sec_ctx_info, 
                             sec_context_handle_t *sec_ctx_handle)
{
    // stub function
	int i;

	sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;
	if(job_ring == NULL)
	{
		return SEC_INVALID_INPUT_PARAM;
	}
	if(job_ring->sec_contexts_no >= MAX_SEC_CONTEXTS_PER_JR)
	{
		return SEC_DRIVER_NO_FREE_CONTEXTS;
	}
	if (sec_ctx_handle == NULL || sec_ctx_info == NULL)
	{
		return SEC_INVALID_INPUT_PARAM;
	}
	if (sec_ctx_info->notify_packet == NULL)
	{
		return SEC_INVALID_INPUT_PARAM;
	}


	*sec_ctx_handle = NULL;

	int found = 0;
	// find an unused context
	for(i = 0; i < MAX_SEC_CONTEXTS_PER_JR; i ++)
	{
		if (job_ring->sec_contexts[i].usage == SEC_CONTEXT_UNUSED)
		{
			job_ring->sec_contexts[i].notify_packet_cbk = sec_ctx_info->notify_packet;
			job_ring->sec_contexts[i].usage = SEC_CONTEXT_USED;
			job_ring->sec_contexts[i].jr_handle = (sec_job_ring_handle_t)job_ring;
			*sec_ctx_handle = (sec_context_handle_t)&job_ring->sec_contexts[i];
			// increment the number of contexts associated to a job ring
			job_ring->sec_contexts_no++;
			found = 1;
            break;
		}
	}
	assert(found == 1);


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

    return SEC_SUCCESS;
}

int sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle)
{
    // stub function

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

	// if no packets in flight then we can safely release the context
	if (sec_context->packets_no == 0)
	{
		sec_context->usage = SEC_CONTEXT_UNUSED;
		sec_context->notify_packet_cbk = NULL;
		sec_context->jr_handle = NULL;
		job_ring->sec_contexts_no--;
		return SEC_SUCCESS;
	}
    
    printf ("sec_delete_pdcp_context:: packets in flight %d\n", sec_context->packets_no);

	// if packets in flight, do not release the context yet, move it to retiring
	// the context will be deleted when all packets in flight were notified to UA
	sec_context->usage = SEC_CONTEXT_RETIRING;
	return SEC_PACKETS_IN_FLIGHT;

    // 1. Mark context as retiring
    // 2. If context.packet_count == 0 then move context from in-use list to free list. 
    //    Else move to retiring list
    // 3. Run context garbage collector routine
}

#if SEC_WORKING_MODE == SEC_POLLING_MODE
int sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no)
{
	sec_job_ring_t * job_ring;
	uint32_t notified_packets_no = 0;
	uint32_t notified_packets_no_per_jr = 0;
	int i;
	int ret;

	if (packets_no == NULL)
	{
		return SEC_INVALID_INPUT_PARAM;
	}

	for (i = 0; i < g_job_rings_no; i++)
	{
		job_ring = &job_rings[i];

		ret = sec_poll_job_ring((sec_job_ring_handle_t)job_ring, limit, &notified_packets_no_per_jr);
		if (ret != SEC_SUCCESS)
		{
			return ret;
		}
		notified_packets_no += notified_packets_no_per_jr;
	}
	*packets_no = notified_packets_no;
    // 1. call sec_hw_poll() to check directly SEC's Job Rings for ready packets.
    //
    // sec_hw_poll() will do:
    // a. SEC 3.1 specific: 
    //      - poll for errors on /dev/sec_uio_x. Raise error notification to UA if this is the case.
    // b. for all owned job rings:
    //      - check and notify ready packets.
    //      - decrement packet reference count per context
    //      - other 
    return SEC_SUCCESS;
}

int sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle, int32_t limit, uint32_t *packets_no)
{
	sec_context_t * sec_context;
	sec_job_t *job;
	int notified_packets_no = 0;
	int j, ready_jobs_no;
	sec_status_t status;
	sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;

	if(job_ring == NULL)
	{
		return SEC_INVALID_INPUT_PARAM;
	}

	if (packets_no == NULL)
	{
		return SEC_INVALID_INPUT_PARAM;
	}

	pthread_mutex_lock( &job_ring->mutex );

    ready_jobs_no = job_ring->jobs_no;
	if (ready_jobs_no != 0 )
	{
        printf ("sec_poll_job_ring::Start polling on Job Ring %p. Available jobs %d\n", job_ring_handle, job_ring->jobs_no);
		for (j = 0; j < ready_jobs_no; j++)
		{
			job = &job_ring->jobs[j];
			sec_context = job->sec_context;
			assert (sec_context->notify_packet_cbk != NULL);

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
			notified_packets_no++;

			// decrement packet reference count in sec context
			sec_context->packets_no--;
			// if no more packets in flight for this context, free the context
			if(sec_context->packets_no == 0 && sec_context->usage == SEC_CONTEXT_RETIRING)
			{
				sec_context->usage = SEC_CONTEXT_UNUSED;
				sec_context->notify_packet_cbk = NULL;
				sec_context->jr_handle = NULL;
				job_ring->sec_contexts_no--;
			}

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
		assert(job_ring->jobs_no == 0);
	}
	pthread_mutex_unlock( &job_ring->mutex );

	*packets_no = notified_packets_no;

    // 1. call sec_hw_poll_job_ring() to check directly SEC's Job Ring for ready packets.
    return SEC_SUCCESS;
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

    return SEC_SUCCESS;
}

int sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle, int32_t limit, uint32_t *packets_no)
{
    // 1. Start software poll on device file registered for this job ring.
    //    Return if no IRQ generated for job ring.
    // 2. call sec_hw_poll_job_ring() to check directly SEC's Job Ring for ready packets.
    return SEC_SUCCESS;
}
#endif

int sec_process_packet(sec_context_handle_t sec_ctx_handle,
                       sec_packet_t *in_packet,
                       sec_packet_t *out_packet,
                       ua_context_handle_t ua_ctx_handle)
{
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
	if (in_packet == NULL || out_packet == NULL)
	{
		return SEC_INVALID_INPUT_PARAM;
	}

	pthread_mutex_lock( &job_ring->mutex );

	if(job_ring->jobs_no >= SEC_JOB_INPUT_RING_SIZE)
	{
		return SEC_INPUT_JR_IS_FULL;
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
