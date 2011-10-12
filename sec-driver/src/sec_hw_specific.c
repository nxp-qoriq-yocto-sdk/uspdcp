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
#ifdef SEC_HW_VERSION_4_4
#include "external_mem_management.h"
#endif

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
#ifdef SEC_HW_VERSION_3_1
/** @brief Identify the SEC descriptor that generated an error
 * on this job ring. Also identify the exact EU that generated the error.
 *
 * @param [in]  job_ring        The job ring
 */
static void hw_handle_eu_error(sec_job_ring_t *job_ring);
#else // SEC_HW_VERSION_3_1
/** @brief Process Jump Halt Condition related errors
 *
 * @param [in]  error_code        The error code in the descriptor status word
 */
static inline void hw_handle_jmp_halt_cond_err(union hw_error_code error_code)
{
    SEC_DEBUG("JMP: %d, Descriptor Index: 0x%x, Condition: 0x%x",
              error_code.error_desc.jmp_halt_cond_src.jmp,
              error_code.error_desc.jmp_halt_cond_src.desc_idx,
              error_code.error_desc.jmp_halt_cond_src.cond);
}

/** @brief Process DECO related errors
 *
 * @param [in]  error_code        The error code in the descriptor status word
 */
static inline void hw_handle_deco_err(union hw_error_code error_code)
{
    SEC_DEBUG("JMP: %d, Descriptor Index: 0x%x",
            error_code.error_desc.deco_src.jmp,
            error_code.error_desc.deco_src.desc_idx);

    switch( error_code.error_desc.deco_src.desc_err )
    {
        case SEC_HW_ERR_DECO_HFN_THRESHOLD:
            SEC_DEBUG(" Warning: Descriptor completed normally, but 3GPP HFN matches or exceeds the Threshold ");
            break;
        default:
            SEC_DEBUG("Error 0x%04x not implemented",error_code.error_desc.deco_src.desc_err);
            break;
    }
}

/** @brief Process  Jump Halt User Status related errors
 *
 * @param [in]  error_code        The error code in the descriptor status word
 */
static inline void hw_handle_jmp_halt_user_err(union hw_error_code error_code)
{
    SEC_DEBUG(" Not implemented");
}

/** @brief Process CCB related errors
 *
 * @param [in]  error_code        The error code in the descriptor status word
 */
static inline void hw_handle_ccb_err(union hw_error_code hw_error_code)
{
    SEC_DEBUG(" Not implemented");
}

/** @brief Process Job Ring related errors
 *
 * @param [in]  error_code        The error code in the descriptor status word
 */
static inline void hw_handle_jr_err(union hw_error_code hw_error_code)
{
    SEC_DEBUG(" Not implemented");
}

#endif // SEC_HW_VERSION_3_1
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
#ifdef SEC_HW_VERSION_3_1
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
#endif // SEC_HW_VERSION_3_1

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int hw_reset_job_ring(sec_job_ring_t *job_ring)
{
    int ret = 0;
#ifdef SEC_HW_VERSION_3_1
    uint32_t reg_val = 0;
#endif

    ASSERT(job_ring->register_base_addr != NULL);

    // First reset the job ring in hw
    ret = hw_shutdown_job_ring(job_ring);
    SEC_ASSERT(ret == 0, ret, "Failed resetting job ring in hardware");
#ifdef SEC_HW_VERSION_3_1
    reg_val = SEC_REG_VAL_CCCR_LO_CDWE;
    // Set done writeback enable at job ring level.

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
#else // SEC_HW_VERSION_3_1
    /* In order to have the HW JR in a workable state
     * after a reset, I need to re-write the input
     * queue size, input start address, output queue
     * size and output start address
     */
    // Write the JR input queue size to the HW register
    hw_set_input_ring_size(job_ring,SEC_JOB_RING_SIZE);

    // Write the JR output queue size to the HW register
    hw_set_output_ring_size(job_ring,SEC_JOB_RING_SIZE);

    // Write the JR input queue start address
    hw_set_input_ring_start_addr(job_ring, sec_vtop(job_ring->input_ring));
    SEC_DEBUG(" Set input ring base address to: Virtual: 0x%x, Physical: 0x%x, Read from HW: 0x%08x",
            (dma_addr_t)job_ring->input_ring,
            sec_vtop(job_ring->input_ring),
            hw_get_inp_queue_base(job_ring));

    // Write the JR output queue start address
    hw_set_output_ring_start_addr(job_ring, sec_vtop(job_ring->output_ring));
    SEC_DEBUG(" Set output ring base address to: Virtual: 0x%x, Physical: 0x%x, Read from HW: 0x%08x",
            (dma_addr_t)job_ring->output_ring,
            sec_vtop(job_ring->output_ring),
            hw_get_out_queue_base(job_ring));

    // Set HW read index to 0
    job_ring->hw_cidx = 0;

    // Set HW write index to 0
    job_ring->hw_pidx = 0;

#endif // SEC_HW_VERSION_3_1
    return 0;
}

#ifdef SEC_HW_VERSION_3_1
int hw_shutdown_job_ring(sec_job_ring_t *job_ring)
{
    unsigned int timeout = SEC_TIMEOUT;
    int usleep_interval = 10;
    int i = 0;

    if(job_ring->register_base_addr == NULL)
    {
        SEC_ERROR("Jr[%p] Jr id %d has register base address NULL."
                  "Probably driver not properly initialized",
                  job_ring, job_ring->jr_id);
        return 0;
    }

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
#else // SEC_HW_VERSION_3_1
int hw_shutdown_job_ring(sec_job_ring_t *job_ring)
{
    unsigned int timeout = SEC_TIMEOUT;
    uint32_t    tmp = 0;
    int usleep_interval = 10;

    if(job_ring->register_base_addr == NULL)
    {
        SEC_ERROR("Jr[%p] Jr id %d has register base address NULL."
                  "Probably driver not properly initialized",
                  job_ring, job_ring->jr_id);
            return 0;
    }

    SEC_INFO("Resetting Job ring %d", job_ring->jr_id);

    /*
     * Mask interrupts since we are going to poll
     * for reset completion status
     * Also, at POR, interrupts are ENABLED on a JR, thus
     * this is the point where I can disable them without
     * changing the code logic too much
     */
    uio_job_ring_disable_irqs(job_ring);

    /* initiate flush (required prior to reset) */
    SET_JR_REG(JRCR,job_ring, JR_REG_JRCR_VAL_RESET);

    // dummy read
    tmp = GET_JR_REG(JRCR,job_ring);

    do
    {
        tmp = GET_JR_REG(JRINT,job_ring);
        usleep(usleep_interval);
    }while ( ((tmp  & JRINT_ERR_HALT_MASK) == JRINT_ERR_HALT_INPROGRESS) && \
               --timeout);

    if( (tmp & JRINT_ERR_HALT_MASK) != JRINT_ERR_HALT_COMPLETE || \
            timeout == 0)
    {
        SEC_ERROR("Failed to flush hw job ring with id %d", job_ring->jr_id);
        SEC_DEBUG("0x%x, %d",tmp, timeout);
#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
        /* unmask interrupts */
        uio_job_ring_enable_irqs(job_ring);
#endif
        return -1;
    }

    /* Initiate reset */
    timeout = SEC_TIMEOUT;
    SET_JR_REG(JRCR,job_ring,JR_REG_JRCR_VAL_RESET);

     do
     {
        tmp = GET_JR_REG(JRCR,job_ring);
        usleep(usleep_interval);
     }while ( (tmp & JR_REG_JRCR_VAL_RESET) && --timeout);

     if( timeout ==  0)
     {
         SEC_ERROR("Failed to reset hw job ring with id  %d\n", job_ring->jr_id);
#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
         /* unmask interrupts */
         uio_job_ring_enable_irqs(job_ring);
#endif
         return -1;
     }

#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
     /* unmask interrupts */
     uio_job_ring_enable_irqs(job_ring);
#endif
    return 0;

}
#endif
#ifdef SEC_HW_VERSION_3_1
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
#else // SEC_HW_VERSION_3_1

void hw_handle_job_ring_error(sec_job_ring_t *job_ring,
                              uint32_t error_code,
                              uint32_t *reset_required)
{
    union hw_error_code hw_err_code;

    ASSERT( reset_required != NULL);

    hw_err_code.error = error_code;

    switch( hw_err_code.error_desc.value.ssrc )
    {
        case SEC_HW_ERR_SSRC_NO_SRC:
            ASSERT(hw_err_code.error_desc.no_status_src.res == 0);
            *reset_required = FALSE;
            SEC_ERROR("No Status Source ");
            break;
        case SEC_HW_ERR_SSRC_CCB_ERR:
            SEC_DEBUG("CCB Status Source");
            *reset_required = FALSE;
            hw_handle_ccb_err(hw_err_code);
            break;
        case SEC_HW_ERR_SSRC_JMP_HALT_U:
            SEC_ERROR("Jump Halt User Status Source");
            *reset_required = FALSE;
            hw_handle_jmp_halt_user_err(hw_err_code);
            break;
        case SEC_HW_ERR_SSRC_DECO:
            SEC_ERROR("DECO Status Source");
            *reset_required = FALSE;
            hw_handle_deco_err(hw_err_code);
            break;
        case SEC_HW_ERR_SSRC_JR:
            SEC_ERROR("Job Ring Status Source");
            *reset_required = FALSE;
            hw_handle_jr_err(hw_err_code);
            break;
        case SEC_HW_ERR_SSRC_JMP_HALT_COND:
            *reset_required = FALSE;
            SEC_ERROR("Jump Halt Condition Codes");
            hw_handle_jmp_halt_cond_err(hw_err_code);
            break;
        default:
            ASSERT(0);
            SEC_ERROR("Unknown SSRC");
            break;
    }
}

int hw_job_ring_error(sec_job_ring_t* job_ring)
{
    uint32_t    jrint_error_code;

    ASSERT(job_ring != NULL);

    if( JR_REG_JRINT_JRE_EXTRACT(GET_JR_REG(JRINT,job_ring)) == 0)
    {
        return 0;
    }

    jrint_error_code = JR_REG_JRINT_ERR_TYPE_EXTRACT(GET_JR_REG(JRINT,job_ring));
    switch( jrint_error_code )
    {
        case JRINT_ERR_WRITE_STATUS:
            SEC_ERROR("Error writing status to Output Ring "
                    "on job ring %d [%p], index %d",
                    job_ring->jr_id,
                    job_ring,
                    (GET_JR_REG(JRINT,job_ring) & 0x3FFF0000) >> 16);
            break;
        case JRINT_ERR_BAD_INPUT_BASE:
            SEC_ERROR("Bad Input Ring Base (%p) (not on a 4-byte boundary) "
                    "on job ring %d [%p]",
                    job_ring->input_ring,
                    job_ring->jr_id,
                    job_ring);
            break;
        case JRINT_ERR_BAD_OUTPUT_BASE:
            SEC_ERROR("Bad Output Ring Base (%p) (not on a 4-byte boundary) "
                    "on job ring %d [%p]",
                      job_ring->output_ring,
                      job_ring->jr_id,
                      job_ring);
            break;
        case JRINT_ERR_WRITE_2_IRBA:
            SEC_ERROR("Invalid write to Input Ring Base Address Register "
                    "on job ring %d [%p]",
                    job_ring->jr_id,
                    job_ring);
            break;
        case JRINT_ERR_WRITE_2_ORBA:
            SEC_ERROR("Invalid write to Output Ring Base Address Register "
                    "on job ring %d [%p]",
                    job_ring->jr_id,
                    job_ring);
            break;
        case JRINT_ERR_RES_B4_HALT:
            SEC_ERROR("Job Ring %d [%p] released before Job Ring is halted",
                    job_ring->jr_id,
                    job_ring);
            break;
        case JRINT_ERR_REM_TOO_MANY:
            SEC_ERROR("Removed too many jobs from job ring %d [%p]",
                    job_ring->jr_id,
                    job_ring);
            break;
        case JRINT_ERR_ADD_TOO_MANY:
            SEC_ERROR("Added too many jobs on job ring %d [%p]",
                    job_ring->jr_id,
                    job_ring);
            break;
        default:
            SEC_ERROR(" Unknown SEC JR Error :%d", jrint_error_code);
            ASSERT(0);
            break;
    }
    /* Acknowledge the error by writing one to JRE bit */
    setbits32(JR_REG(JRINT,job_ring),JRINT_JRE);

    return jrint_error_code;
}

#if (SEC_INT_COALESCING_ENABLE == ON)
int hw_job_ring_set_coalescing_param(sec_job_ring_t *job_ring,
                                     uint16_t irq_coalescing_timer,
                                     uint8_t  irq_coalescing_count)
{
    uint32_t reg_val = 0;

    ASSERT(job_ring != NULL);

    if(job_ring->register_base_addr == NULL)
    {
        SEC_ERROR("Jr[%p] Jr id %d has register base address NULL."
                  "Probably driver not properly initialized",
                  job_ring, job_ring->jr_id);
        return -1;
    }

    // Set descriptor count coalescing
    reg_val |= (irq_coalescing_count << JR_REG_JRCFG_LO_ICDCT_SHIFT);

    // Set coalescing timer value
    reg_val |=  (irq_coalescing_timer << JR_REG_JRCFG_LO_ICTT_SHIFT);

    // Update parameters in HW
    SET_JR_REG_LO(JRCFG,job_ring,reg_val);

    SEC_DEBUG("Jr[%p]. Set coalescing params on jr id %d "
              "(timer: %d, descriptor count: %d",
              job_ring,
              job_ring->jr_id,
              irq_coalescing_timer,
              irq_coalescing_timer);

    return 0;
}
int hw_job_ring_enable_coalescing(sec_job_ring_t *job_ring)

{
    uint32_t reg_val = 0;

    ASSERT(job_ring != NULL );
    if(job_ring->register_base_addr == NULL)
    {
        SEC_ERROR("Jr[%p] Jr id %d has register base address NULL."
                  "Probably driver not properly initialized",
                  job_ring, job_ring->jr_id);
        return -1;
    }

    /* Get the current value of the register */
    reg_val = GET_JR_REG_LO(JRCFG,job_ring);

    // Enable coalescing
    reg_val |= JR_REG_JRCFG_LO_ICEN_EN;

    //Write in hw
    SET_JR_REG_LO(JRCFG,job_ring,reg_val);

    SEC_DEBUG("Jr[%p]. Enabled coalescing on jr id %d ",
              job_ring,
              job_ring->jr_id);

    return 0;
}

int hw_job_ring_disable_coalescing(sec_job_ring_t *job_ring)
{
    uint32_t reg_val = 0;
    ASSERT(job_ring != NULL );

    if(job_ring->register_base_addr == NULL)
    {
        SEC_ERROR("Jr[%p] Jr id %d has register base address NULL."
                  "Probably driver not properly initialized",
                  job_ring, job_ring->jr_id);
        return -1;
    }

    /* Get the current value of the register */
    reg_val = GET_JR_REG_LO(JRCFG,job_ring);

    // Disable coalescing
    reg_val &= ~JR_REG_JRCFG_LO_ICEN_EN;

    //Write in hw
    SET_JR_REG_LO(JRCFG,job_ring,reg_val);

    SEC_DEBUG("Jr[%p]. Disabled coalescing on jr id %d ",
              job_ring,
              job_ring->jr_id);

    return 0;

}
#endif // SEC_INT_COALESCING_ENABLE == ON
#endif // SEC_HW_VERSION_3_1
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
