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

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

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

    setbits32(job_ring->register_base_addr + SEC_REG_CCCR_LO(job_ring), reg_val);

    // TODO: integrity check required for PDCP processing ?
    return 0;
}

int hw_shutdown_job_ring(sec_job_ring_t *job_ring)
{
    unsigned int timeout = SEC_TIMEOUT;
    int usleep_interval = 10;

    ASSERT(job_ring->register_base_addr != NULL);

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
    return 0;
}
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
