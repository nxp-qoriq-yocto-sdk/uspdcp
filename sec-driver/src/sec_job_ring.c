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
#include "sec_job_ring.h"
#include "sec_utils.h"
#include "sec_hw_specific.h"

// For definition of sec_vtop and sec_ptov macros
#include "alu_mem_management.h"

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
/* Job rings used for communication with SEC HW */
sec_job_ring_t g_job_rings[MAX_SEC_JOB_RINGS];

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/
int init_job_ring(sec_job_ring_t * job_ring, void **dma_mem, int startup_work_mode)
{
    int ret = 0;
    int i = 0;

    ASSERT(job_ring != NULL);
    ASSERT(dma_mem != NULL);

    SEC_INFO("Job ring %d UIO fd = %d", job_ring->jr_id, job_ring->uio_fd);

    // Reset job ring in SEC hw and configure job ring registers
    ret = hw_reset_job_ring(job_ring);
    SEC_ASSERT(ret == 0, ret, "Failed to reset hardware job ring with id %d", job_ring->jr_id);


#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_NAPI
    // When SEC US driver works in NAPI mode, the UA can select
    // if the driver starts with IRQs on or off.
    if(startup_work_mode == SEC_STARTUP_INTERRUPT_MODE)
    {
        SEC_INFO("Enabling DONE IRQ generation on job ring with id %d", job_ring->jr_id);
        hw_enable_done_irq_on_job_ring(job_ring);
    }
#endif

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_IRQ
    // When SEC US driver works in pure interrupt mode, IRQ's are always enabled.
    SEC_INFO("Enabling DONE IRQ generation on job ring with id %d", job_ring->jr_id);
    hw_enable_done_irq_on_job_ring(job_ring);
#endif

    // Memory area must start from cacheline-aligned boundary.
    // Each job entry is itself aligned to cacheline.
    SEC_ASSERT ((dma_addr_t)*dma_mem % CACHE_LINE_SIZE == 0, 
            SEC_INVALID_INPUT_PARAM,
            "Current memory position is not cacheline aligned."
            "Job ring id = %d", job_ring->jr_id);

    // Allocate job items from the DMA-capable memory area provided by UA
    ASSERT(job_ring->descriptors == NULL);

    job_ring->descriptors = *dma_mem;
    memset(job_ring->descriptors, 0, SEC_JOB_RING_SIZE * sizeof(struct sec_descriptor_t));
    *dma_mem += SEC_JOB_RING_SIZE * sizeof(struct sec_descriptor_t);
    // TODO: check that we do not use more DMA mem than actually allocated/reserved for us by User App.
    // Options: 
    // - check if used more than #SEC_DMA_MEMORY_SIZE and/or
    // - implement wrapper functions that access dma_mem and maintain/increment size of dma mem used -> central point of
    // accessing dma mem -> central point of check for boundary issues!

    for(i = 0; i < SEC_JOB_RING_SIZE; i++)
    {
        // Remember virtual address for a job descriptor
        job_ring->jobs[i].descr = &job_ring->descriptors[i];
        
        // Obtain and store the physical address for a job descriptor        
        job_ring->jobs[i].descr_phys_addr = sec_vtop(&job_ring->descriptors[i]);

    }

    job_ring->jr_state = SEC_JOB_RING_STATE_STARTED;

    return SEC_SUCCESS;
}

int shutdown_job_ring(sec_job_ring_t * job_ring)
{
    int ret = 0;

    ASSERT(job_ring != NULL);
    if(job_ring->uio_fd != 0)
    {
        close(job_ring->uio_fd);
    }

    ret = hw_shutdown_job_ring(job_ring);
    SEC_ASSERT(ret == 0, ret, "Failed to shutdown hardware job ring with id %d", job_ring->jr_id);

    memset(job_ring, 0, sizeof(sec_job_ring_t));

    return SEC_SUCCESS;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
