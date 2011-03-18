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

#define SEC_CIRCULAR_COUNTER_POW_2(x, max)   (((x) + 1) & (max - 1))

/** The number of jobs in a JOB RING */
#define SEC_JOB_RING_DIFF(ring_max_size, pi, ci) (((pi) < (ci)) ? \
                    ((ring_max_size) + (pi) - (ci)) : ((pi) - (ci)))
/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
/** Job descriptor pointer entry */
struct sec_ptr {
    uint16_t len;       /*< length */
    uint8_t j_extent;   /*< jump to sg link table and/or extent */
    uint8_t eptr;       /*< extended address */
    uint32_t ptr;       /*< address */
};


/** A descriptor that instructs SEC engine how to process a packet.
 * On SEC 3.1 a descriptor consists of 8 double-words: 
 * one header dword and seven pointer dwords. */
typedef struct sec_descriptor_s
{
    uint32_t hdr;               /*< header high bits */
    uint32_t hdr_lo;            /*< header low bits */
    struct sec_ptr ptr[7];      /*< ptr/len pair array */

} __CACHELINE_ALIGNED sec_descriptor_t;

/** SEC job */
typedef struct sec_job_s
{
    sec_context_t *sec_context;         /*< SEC context this packet belongs to */
    sec_descriptor_t *descr;            /*< SEC descriptor sent to SEC engine(virtual address)*/
    dma_addr_t descr_phys_addr;         /*< SEC descriptor sent to SEC engine(physical address) */
    sec_packet_t *in_packet;             /*< Input packet */
    sec_packet_t *out_packet;            /*< Output packet */
    ua_context_handle_t ua_handle;      /*< UA handle for the context this packet belongs to */
}__CACHELINE_ALIGNED sec_job_t;


/** SEC Job Ring */
typedef struct sec_job_ring_s
{
    uint32_t cidx;                      /*< Consumer index for job ring (jobs array).
                                            @note: cidx and pidx are accessed from different threads.
                                                   Place the cidx and pidx inside the structure so that
                                                   they lay on different cachelines, to avoid false 
                                                   sharing between threads when the threads run on different cores! */
    sec_job_t jobs[SEC_JOB_RING_SIZE];  /*< Ring of jobs. The same ring is used for
                                            input jobs and output jobs because SEC engine writes 
                                            back output indication in input job.
                                            Size of array is power of 2 to allow fast update of
                                            producer/consumer indexes with bitwise operations. */
    sec_descriptor_t *descriptors;      /*< Ring of descriptors sent to SEC engine for processing */

    uint32_t pidx;                      /*< Producer index for job ring (jobs array) */
    uint32_t uio_fd;                    /*< The file descriptor used for polling from user space
                                            for interrupts notifications */
    uint32_t jr_id;                     /*< Job ring id */
    uint32_t alternate_register_range;  /*< Can be #TRUE or #FALSE. Indicates if the registers for 
                                            this job ring are mapped to an alternate 4k page.*/
    volatile uint32_t free_slots;       /*< Counts the free slots in a job ring. When it reaches value 0, 
                                            the job ring is full.*/
    volatile void *register_base_addr;  /*< Base address for SEC's register memory for this job ring.
                                            @note On SEC 3.1 all channels share the same register address space,
                                                  so this member will have the exact same value for all og them. */
	sec_contexts_pool_t ctx_pool;       /*< Pool of SEC contexts */
}__CACHELINE_ALIGNED sec_job_ring_t;
/*==============================================================================
                                 CONSTANTS
==============================================================================*/


/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/
/* Job rings used for communication with SEC HW */
extern sec_job_ring_t g_job_rings[MAX_SEC_JOB_RINGS];


/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/
/** @brief Initialize the software and hardware resources tied to a job ring.
 * @param [in,out] job_ring             The job ring
 * @param [in,out] dma_mem              DMA-capable memory area from where to
 *                                      allocate SEC descriptors.
 * @param [in]     startup_work_mode    The work mode to configure a job ring at startup.
 *                                      Used only when #SEC_NOTIFICATION_TYPE is set to
 *                                      #SEC_NOTIFICATION_TYPE_NAPI.
 * @retval  SEC_SUCCESS for success
 * @retval  other for error
 *
 */
int init_job_ring(sec_job_ring_t *job_ring, void **dma_mem, int startup_work_mode);

/** @brief Release the software and hardware resources tied to a job ring.
 * @param [in] job_ring The job ring
 *
 * @retval  SEC_SUCCESS for success
 * @retval  other for error
 */
int shutdown_job_ring(sec_job_ring_t *job_ring);

/*============================================================================*/


#endif  /* SEC_JOB_RING_H */
