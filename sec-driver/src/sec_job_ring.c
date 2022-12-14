/* Copyright (c) 2011 - 2012 Freescale Semiconductor, Inc.
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
#include <sys/mman.h>
#include "sec_job_ring.h"
#include "sec_utils.h"
#include "sec_hw_specific.h"
#include "sec_config.h"

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
extern sec_vtop g_sec_vtop;

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/
int init_job_ring(sec_job_ring_t * job_ring, void **dma_mem, int startup_work_mode
        , uint16_t irq_coalescing_timer, uint8_t irq_coalescing_count
        )
{
    int ret = 0;
    int i = 0;

    ASSERT(job_ring != NULL);
    ASSERT(dma_mem != NULL);

    SEC_INFO("Job ring %d UIO fd = %d", job_ring->jr_id, job_ring->uio_fd);

    // Memory area must start from cacheline-aligned boundary.
    // Each job entry is itself aligned to cacheline.
    SEC_ASSERT ((uintptr_t)*dma_mem % L1_CACHE_BYTES == 0,
            SEC_INVALID_INPUT_PARAM,
            "Current memory position is not cacheline aligned."
            "Job ring id = %d", job_ring->jr_id);

    // Allocate mem for input and output ring
    ASSERT(job_ring->input_ring == NULL);

    // Allocate memory for input ring
    job_ring->input_ring = *dma_mem;
    memset(job_ring->input_ring, 0, SEC_DMA_MEM_INPUT_RING_SIZE);
    *dma_mem += SEC_DMA_MEM_INPUT_RING_SIZE;

    // Allocate memory for output ring
    ASSERT(job_ring->output_ring == NULL);

    SEC_ASSERT ((uintptr_t)*dma_mem % L1_CACHE_BYTES == 0,
            SEC_INVALID_INPUT_PARAM,
            "Current memory position is not cacheline aligned."
            "Job ring id = %d", job_ring->jr_id);
    SEC_ASSERT ((uintptr_t)*dma_mem % L1_CACHE_BYTES == 0,
                                SEC_INVALID_INPUT_PARAM,
                                "Current memory position is not cacheline aligned."
                                "Job ring id = %d", job_ring->jr_id);
    job_ring->output_ring = *dma_mem;
    memset(job_ring->output_ring, 0, SEC_DMA_MEM_OUTPUT_RING_SIZE);
    *dma_mem += SEC_DMA_MEM_OUTPUT_RING_SIZE;

    // Reset job ring in SEC hw and configure job ring registers
    ret = hw_reset_job_ring(job_ring);
    SEC_ASSERT(ret == 0, ret, "Failed to reset hardware job ring with id %d", job_ring->jr_id);


#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_NAPI
    // When SEC US driver works in NAPI mode, the UA can select
    // if the driver starts with IRQs on or off.
    if(startup_work_mode == SEC_STARTUP_INTERRUPT_MODE)
    {
        SEC_INFO("Enabling DONE IRQ generation on job ring with id %d", job_ring->jr_id);
        uio_job_ring_enable_irqs(job_ring);
    }
#endif

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_IRQ
    // When SEC US driver works in pure interrupt mode, IRQ's are always enabled.
    SEC_INFO("Enabling DONE IRQ generation on job ring with id %d", job_ring->jr_id);
    uio_job_ring_enable_irqs(job_ring);
#endif

    // Allocate job items from the DMA-capable memory area provided by UA
    ASSERT(job_ring->descriptors == NULL);

    job_ring->descriptors = *dma_mem;

    /* Store base address here. It will be used for 'lookups' in sec_poll() */
    job_ring->descriptors_base_addr = g_sec_vtop(job_ring->descriptors);
    
    memset(job_ring->descriptors, 0, SEC_DMA_MEM_DESCRIPTORS);
    *dma_mem += SEC_DMA_MEM_DESCRIPTORS;

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
        job_ring->jobs[i].descr_phys_addr = g_sec_vtop(&job_ring->descriptors[i]);

#if (SEC_ENABLE_SCATTER_GATHER == ON)
        job_ring->jobs[i].sg_ctx = &job_ring->sg_ctxs[i];

        // Need two SG tables, one for input packet, the other for the output
        SEC_ASSERT ((uintptr_t)*dma_mem % L1_CACHE_BYTES == 0,
                        SEC_INVALID_INPUT_PARAM,
                        "Current jobs[i]->sg_ctx [i=%d] input SG table is not cacheline aligned.", i);
        job_ring->sg_ctxs[i].in_sg_tbl = (struct sec_sg_tbl_entry*)*dma_mem;
        memset(job_ring->sg_ctxs[i].in_sg_tbl, 0, sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES);
        *dma_mem += sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES;

        SEC_ASSERT ((uintptr_t)*dma_mem % L1_CACHE_BYTES == 0,
                                SEC_INVALID_INPUT_PARAM,
                                "Current jobs[i]->sg_ctx [i=%d] output SG table is not cacheline aligned.", i);
        job_ring->sg_ctxs[i].out_sg_tbl = (struct sec_sg_tbl_entry*)*dma_mem;
        memset(job_ring->sg_ctxs[i].out_sg_tbl, 0, sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES);
        *dma_mem += sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES;

        job_ring->sg_ctxs[i].in_sg_tbl_phy = g_sec_vtop(job_ring->sg_ctxs[i].in_sg_tbl);
        job_ring->sg_ctxs[i].out_sg_tbl_phy = g_sec_vtop(job_ring->sg_ctxs[i].out_sg_tbl);

#endif // (SEC_ENABLE_SCATTER_GATHER == ON)
    }

#if (SEC_INT_COALESCING_ENABLE == ON)
    hw_job_ring_set_coalescing_param(job_ring,
                                     irq_coalescing_timer,
                                     irq_coalescing_count);

    hw_job_ring_enable_coalescing(job_ring);
#endif // SEC_INT_COALESCING_ENABLE == ON

    job_ring->jr_state = SEC_JOB_RING_STATE_STARTED;

    return SEC_SUCCESS;
}

int shutdown_job_ring(sec_job_ring_t * job_ring)
{
    int ret = 0;

    ASSERT(job_ring != NULL);

    ret = hw_shutdown_job_ring(job_ring);
    SEC_ASSERT(ret == 0, ret, "Failed to shutdown hardware job ring with id %d", job_ring->jr_id);

#if (SEC_INT_COALESCING_ENABLE == ON)
    hw_job_ring_disable_coalescing(job_ring);
#endif // SEC_INT_COALESCING_ENABLE == ON

#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
    uio_job_ring_disable_irqs(job_ring);
#endif

    /*
     * munmap SEC's register memory
     */
    munmap(job_ring->register_base_addr, job_ring->map_size);

    /* I need to close the fd after shutdown UIO commands need to be 
     * sent using the fd 
     */

    if(job_ring->uio_fd != 0)
    {
        SEC_INFO("Closed device file for job ring %d , fd = %d", job_ring->jr_id, job_ring->uio_fd);
        close(job_ring->uio_fd);
    }

    memset(job_ring, 0, sizeof(sec_job_ring_t));

    return SEC_SUCCESS;
}

void uio_reset_sec_engine(sec_job_ring_t *job_ring)
{
    int ret;

    // Use UIO file descriptor we have for this job ring.
    // Writing a command code to this file descriptor will make the
    // SEC kernel driver reset SEC engine.
    ret = sec_uio_send_command(job_ring, SEC_UIO_RESET_SEC_ENGINE_CMD);
    SEC_ASSERT_RET_VOID(ret == sizeof(int),
                        "Failed to request SEC engine restart through UIO control."
                        "Reset SEC driver!");
}

void uio_job_ring_enable_irqs(sec_job_ring_t *job_ring)
{
    int ret;

    // Use UIO file descriptor we have for this job ring.
    // Writing a command code to this file descriptor will make the
    // SEC kernel driver enable DONE and Error IRQs for this job ring,
    // at Controller level.
    ret = sec_uio_send_command(job_ring, SEC_UIO_ENABLE_IRQ_CMD);
    SEC_ASSERT_RET_VOID(ret == sizeof(int),
                        "Failed to request SEC engine to enable job done and "
                        "error IRQs through UIO control. Job ring id %d. Reset SEC driver!",
                        job_ring->jr_id);
    SEC_DEBUG("Jr[%p]. Enabled IRQs on jr id %d", job_ring, job_ring->jr_id);
}


void uio_job_ring_disable_irqs(sec_job_ring_t *job_ring)
{
    int ret;

    // Use UIO file descriptor we have for this job ring.
    // Writing a command code to this file descriptor will make the
    // SEC kernel driver disable IRQs for this job ring,
    // at Controller level.
    ret = sec_uio_send_command(job_ring, SEC_UIO_DISABLE_IRQ_CMD);
    SEC_ASSERT_RET_VOID(ret == sizeof(int),
                        "Failed to request SEC engine to disable job done and "
                        "IRQs through UIO control. Job ring id %d. Reset SEC driver!",
                        job_ring->jr_id);
    SEC_DEBUG("Jr[%p]. Disabled IRQs on jr id %d", job_ring, job_ring->jr_id);
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
