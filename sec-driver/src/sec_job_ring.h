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

#ifndef SEC_JOB_RING_H
#define SEC_JOB_RING_H

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include "fsl_sec.h"
#include "sec_contexts.h"

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/

/** Update circular counter */
#define SEC_CIRCULAR_COUNTER(x, max)   ((x) + 1) * ((x) != (max - 1))

/** The number of jobs in a JOB RING */
#define SEC_JOB_RING_DIFF(ring_max_size, pi, ci) (((pi) < (ci)) ? \
                    ((ring_max_size) + (pi) - (ci)) : ((pi) - (ci)))
/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/

/** SEC job */
typedef struct sec_job_s
{
    sec_packet_t in_packet;             /*< Input packet */
    sec_packet_t out_packet;            /*< Output packet */
    ua_context_handle_t ua_handle;      /*< UA handle for the context this packet belongs to */
    sec_context_t * sec_context;        /*< SEC context this packet belongs to */
}sec_job_t;

/** SEC Job Ring */
typedef struct sec_job_ring_s
{
	sec_contexts_pool_t ctx_pool;       /*< Pool of SEC contexts */
    sec_job_t jobs[SEC_JOB_RING_SIZE];  /*< Ring of jobs. In this stub the same ring is used for
                                            input jobs and output jobs. */
    int cidx;                           /*< Consumer index for job ring (jobs array) */
    int pidx;                           /*< Producer index for job ring (jobs array) */
    uint32_t uio_fd;                    /*< The file descriptor used for polling from user space
                                            for interrupts notifications */
    uint32_t jr_id;                     /*< Job ring id */
}sec_job_ring_t;
/*==============================================================================
                                 CONSTANTS
==============================================================================*/


/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/


/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/*============================================================================*/


#endif  /* SEC_JOB_RING_H */
