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
#include "sec_hw_specific.h"
#include "sec_job_ring.h"
#include "sec_utils.h"

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/

/** Used to retry resetting a job ring in SEC hardware. */
#define SEC_TIMEOUT 100000

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

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/** @brief Identify the SEC descriptor that generated an error
 * on this job ring. Also identify the exact EU that generated the error.
 *
 * @param [in]  job_ring        The job ring
 */
static void hw_handle_eu_error(sec_job_ring_t *job_ring);

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static void hw_handle_eu_error(sec_job_ring_t *job_ring)
{
    int i;
    struct sec_job_t *job = NULL;
    dma_addr_t current_desc_phys_addr;

    // First read job ring register to get current descriptor,
    // that is also the one that generated the error.
    current_desc_phys_addr = hw_get_current_descriptor(job_ring);

    // Now search the job ring for the descriptor
    for(i = 0; i < SEC_JOB_RING_SIZE; i++)
    {
        if(job_ring->jobs[i].descr_phys_addr == current_desc_phys_addr)
        {
            job = &(job_ring->jobs[i]);
            break;
        }
    }

    // No descriptor found in the job ring, should not happen!
    if(job == NULL)
    {
        SEC_ERROR("Cannot find in job ring the descriptor with physical address 0x%x\n"
                  "which generated error on job ring id %d",
                  current_desc_phys_addr, job_ring->jr_id);
        return;
    }

    // Read descriptor header and identify the EU(Execution Unit)
    // that generated the error.
    switch (job->descr->hdr & SEC_DESC_HDR_SEL0_MASK)
    {
        case SEC_DESC_HDR_EU_SEL0_AESU:
            SEC_ERROR("Job ring %d generated error in AESU Execution Unit."
                      "AESU ISR hi = 0x%08x. AESU ISR lo = 0x%08x",
                      job_ring->jr_id,
                      in_be32(job_ring->register_base_addr + SEC_REG_AESU_ISR),
                      in_be32(job_ring->register_base_addr + SEC_REG_AESU_ISR_LO));
            break;
        case SEC_DESC_HDR_EU_SEL0_STEU:
            SEC_ERROR("Job ring %d generated error in STEU Execution Unit."
                      "STEU ISR hi = 0x%08x. STEU ISR lo = 0x%08x",
                      job_ring->jr_id,
                      in_be32(job_ring->register_base_addr + SEC_REG_STEU_ISR),
                      in_be32(job_ring->register_base_addr + SEC_REG_STEU_ISR_LO));
            break;
        default:
            SEC_ERROR("Job ring %d generated error in EU that is not used by SEC user space driver!"
                      "Descriptor header = 0x%x",
                      job_ring->jr_id,
                      job->descr->hdr);
            break;
    }
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int hw_reset_job_ring(sec_job_ring_t *job_ring)
{
    int ret = 0;
    uint32_t reg_val = 0;

    ASSERT(job_ring->register_base_addr != NULL);

    // First reset the job ring in hw
    ret = hw_shutdown_job_ring(job_ring);
    SEC_ASSERT(ret == 0, ret, "Failed resetting job ring in hardware");

    // Set done writeback enable at job ring level.
    reg_val = SEC_REG_VAL_CCCR_LO_CDWE;

#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Set 36-bit addressing if enabled
    reg_val |= SEC_REG_VAL_CCCR_LO_EAE; 
#endif

#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
    // Enable interrupt generation at job ring level 
    // ONLY if NOT in polling mode.
    reg_val |= SEC_REG_VAL_CCCR_LO_CDIE;
#endif

    // Enable integrity check if configured in descriptors.
    // Required for PDCP control plane processing.
    reg_val |= SEC_REG_VAL_CCCR_LO_IWSE;

    setbits32(job_ring->register_base_addr + SEC_REG_CCCR_LO(job_ring), reg_val);

    // Disable Integrity check error interrupt in STEU, the execution
    // unit that performs SNOW F9.
    // The driver will poll for descriptor status and read ICV failure if any.
    reg_val = SEC_REG_STEU_IMR_DISABLE_ICE; 
    setbits32(job_ring->register_base_addr + SEC_REG_STEU_IMR_LO, reg_val);

    // Disable Integrity check error interrupt in AESU, the execution
    // unit that performs AES CMAC.
    // The driver will poll for descriptor status and read ICV failure if any.
    reg_val = SEC_REG_AESU_IMR_DISABLE_ICE;
    setbits32(job_ring->register_base_addr + SEC_REG_AESU_IMR_LO, reg_val);

    return 0;
}

int hw_shutdown_job_ring(sec_job_ring_t *job_ring)
{
    unsigned int timeout = SEC_TIMEOUT;
    int usleep_interval = 10;
    int i = 0;

    ASSERT(job_ring->register_base_addr != NULL);

    // Reset job ring by setting the Reset(R) bit.
    // Values set for job ring registers will NOT be kept.
    // Better to reset twice because sometimes some registers
    // are not cleared, like it happens in case of resetting with CON bit.
    for(i = 0; i < 2; i++)
    {
        // Ask the SEC hw to reset this job ring
        out_be32(job_ring->register_base_addr + SEC_REG_CCCR(job_ring), SEC_REG_CCCR_VAL_RESET);

        // Loop until the SEC engine has finished the job ring reset
        while ((in_be32(job_ring->register_base_addr + SEC_REG_CCCR(job_ring)) & SEC_REG_CCCR_VAL_RESET) && --timeout)
        {
            usleep(usleep_interval);
        }

        if (timeout == 0)
        {
            SEC_ERROR("Failed to reset hw job ring with id  %d\n", job_ring->jr_id);
            return -1;
        }
    }
    return 0;
}

int hw_reset_and_continue_job_ring(sec_job_ring_t *job_ring)
{
    unsigned int timeout = SEC_TIMEOUT;
    int usleep_interval = 10;
    int i = 0;

    ASSERT(job_ring->register_base_addr != NULL);

    // Reset job ring by setting the Continue(CON) bit.
    // Values set for job ring registers will NOT be kept.
    // Need to reset twice because sometimes the error field
    // in CSR is not cleared after first reset!
    for(i = 0; i < 2; i++)
    {
        // Ask the SEC hw to reset this job ring
        out_be32(job_ring->register_base_addr + SEC_REG_CCCR(job_ring), SEC_REG_CCCR_VAL_CONTINUE);

        // Loop until the SEC engine has finished the job ring reset
        while ((in_be32(job_ring->register_base_addr + SEC_REG_CCCR(job_ring)) & SEC_REG_CCCR_VAL_CONTINUE) && --timeout)
        {
            usleep(usleep_interval);
        }

        if (timeout == 0)
        {
            SEC_ERROR("Failed to reset and continue for hw job ring with id  %d\n", job_ring->jr_id);
            return -1;
        }
    }
    return 0;
}

void hw_handle_job_ring_error(sec_job_ring_t *job_ring,
                              uint32_t error_code,
                              uint32_t *reset_required)
{
    uint32_t sec_error_code = sec_error_code = hw_job_ring_error(job_ring);

    // Check that the error code first read from SEC's job ring registers
    // is the same with the one we've just read now.
    ASSERT(sec_error_code == error_code);
    ASSERT(reset_required != NULL);

    if(sec_error_code & SEC_REG_CSR_LO_DOF)
    {
        // Write 1 to DOF bit
        hw_job_ring_clear_error(job_ring, SEC_REG_CSR_LO_DOF);
        // Channel is halted and we must restart it
        *reset_required = TRUE;
        SEC_ERROR("DOF error on job ring with id %d", job_ring->jr_id);
    }
    if(sec_error_code & SEC_REG_CSR_LO_SOF)
    {
        // Write 1 to SOF bit
        hw_job_ring_clear_error(job_ring, SEC_REG_CSR_LO_SOF);
        // Channel not halted, can continue processing.
        SEC_ERROR("SOF error on job ring with id %d", job_ring->jr_id);
    }
    if(sec_error_code & SEC_REG_CSR_LO_MDTE)
    {
        // Channel is halted and we must restart it
        *reset_required = TRUE;
        SEC_ERROR("MDTE error on job ring with id %d", job_ring->jr_id);
    }

    if(sec_error_code & SEC_REG_CSR_LO_IDH)
    {
        // Channel is halted and we must restart it
        *reset_required = TRUE;
        SEC_ERROR("IDH error on job ring with id %d", job_ring->jr_id);
    }

    if(sec_error_code & SEC_REG_CSR_LO_EU)
    {
        // Channel is halted and we must restart it
        // Must clear the error source in the EU that produced the error.
        *reset_required = TRUE;
        SEC_ERROR("EU error on job ring with id %d", job_ring->jr_id);

        // Identify the exact EU error
        hw_handle_eu_error(job_ring);
    }
    if(sec_error_code & SEC_REG_CSR_LO_WDT)
    {
        // Channel is halted and we must restart it
        *reset_required = TRUE;
        SEC_ERROR("WDT error on job ring with id %d", job_ring->jr_id);
    }
    if(sec_error_code & SEC_REG_CSR_LO_SGML)
    {
        // Channel is halted and we must restart it
        *reset_required = TRUE;
        SEC_ERROR("SGML error on job ring with id %d", job_ring->jr_id);
    }
    if(sec_error_code & SEC_REG_CSR_LO_RSI)
    {
        // No action required, as per SEC 3.1 Block Guide
        SEC_ERROR("RSI error on job ring with id %d", job_ring->jr_id);
    }
    if(sec_error_code & SEC_REG_CSR_LO_RSG)
    {
        // No action required, as per SEC 3.1 Block Guide
        SEC_ERROR("RSG error on job ring with id %d", job_ring->jr_id);
    }
}
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
