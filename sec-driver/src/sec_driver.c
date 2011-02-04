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

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

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
             sec_job_ring_t **job_ring_handles)
{
    // stub function
    return SEC_SUCCESS;
}

int sec_release()
{
    // stub function
    return SEC_SUCCESS;
}

int sec_create_pdcp_context (sec_job_ring_t job_ring_handle,
                             sec_pdcp_context_info_t *sec_ctx_info, 
                             sec_context_handle_t *sec_ctx_handle)
{
    // stub function

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

    // 1. Mark context as retiring
    // 2. If context.packet_count == 0 then move context from in-use list to free list. 
    //    Else move to retiring list
    // 3. Run context garbage collector routine

    return SEC_SUCCESS;
}

#if FSL_SEC_WORKING_MODE == FSL_SEC_POLLING_MODE
int sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no)
{
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

int sec_poll_job_ring(sec_job_ring_t job_ring_handle, int32_t limit, uint32_t *packets_no)
{
    // 1. call sec_hw_poll_job_ring() to check directly SEC's Job Ring for ready packets.
    return SEC_SUCCESS;
}

#elif FSL_SEC_WORKING_MODE == FSL_SEC_INTERRUPT_MODE
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

int sec_poll_job_ring(sec_job_ring_t job_ring_handle, int32_t limit, uint32_t *packets_no)
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
    // stub function
    return SEC_SUCCESS;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
