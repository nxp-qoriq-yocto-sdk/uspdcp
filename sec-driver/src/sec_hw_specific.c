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
extern sec_vtop g_sec_vtop;

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
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


/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/


/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int hw_reset_job_ring(sec_job_ring_t *job_ring)
{
    int ret = 0;

    ASSERT(job_ring->register_base_addr != NULL);

    // First reset the job ring in hw
    ret = hw_shutdown_job_ring(job_ring);
    SEC_ASSERT(ret == 0, ret, "Failed resetting job ring in hardware");

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
    hw_set_input_ring_start_addr(job_ring, g_sec_vtop(job_ring->input_ring));
    SEC_DEBUG(" Set input ring base address to: Virtual: 0x%x, Physical: 0x%x, Read from HW: 0x%08x",
            (dma_addr_t)job_ring->input_ring,
            g_sec_vtop(job_ring->input_ring),
            hw_get_inp_queue_base(job_ring));

    // Write the JR output queue start address
    hw_set_output_ring_start_addr(job_ring, g_sec_vtop(job_ring->output_ring));
    SEC_DEBUG(" Set output ring base address to: Virtual: 0x%x, Physical: 0x%x, Read from HW: 0x%08x",
            (dma_addr_t)job_ring->output_ring,
            g_sec_vtop(job_ring->output_ring),
            hw_get_out_queue_base(job_ring));

    return 0;
}


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
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
