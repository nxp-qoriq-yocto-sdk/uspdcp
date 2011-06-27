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

#ifndef SEC_CONFIG_H
#define SEC_CONFIG_H

#ifdef __cplusplus
/* *INDENT-OFF* */

extern "C"{
/* *INDENT-ON* */
#endif

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
#include "sec_job_ring.h"
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
/** Bit mask for each job ring in a bitfield of format: jr0 | jr1 | jr2 | jr3 */
#define JOB_RING_MASK(jr_id)        (1 << (MAX_SEC_JOB_RINGS - (jr_id) -1 ))

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/** @brief Reads SEC configuration data from DTS.
 * @param [in]      job_ring_number     Number of job rings to be configured
 * @param [in,out]  job_rings           Job ring array to be updated with configuration data. 
 *
 * @retval #SEC_SUCCESS                 for successful execution
 * @retval #SEC_INVALID_INPUT_PARAM     in case of invalid configuration found in DTS
 */
int sec_configure(int job_ring_number, struct sec_job_ring_t *job_rings);

/** @brief Obtains file handle to access UIO device for a specific job ring.
 *  The file handle can be used to: 
 *  - receive job done interrupts(read number of IRQs)
 *  - request SEC engine reset (write #value TODO)
 *
 * @param [in,out]  job_ring            Job ring 
 *
 * @retval #SEC_SUCCESS                 for successful execution
 * @retval #SEC_INVALID_INPUT_PARAM     in case of invalid configuration found in DTS
 */
sec_return_code_t sec_config_uio_job_ring(struct sec_job_ring_t *job_ring);

/** @brief Uses UIO control to send commands to SEC kernel driver.
 * The mechanism is to write a command word into the file descriptor
 * that the user-space driver obtained for each user-space SEC job ring.
 * Both user-space driver and kernel driver must have the same understanding
 * about the command codes.
 *
 * @param [in]  job_ring            Job ring
 * @param [in]  uio_command         Command word
 *
 * @retval Result of write operation on the job ring's UIO file descriptor.
 *         Should be sizeof(int) for success operations.
 *         Other values can be returned and used, if desired to add special
 *         meaning to return values, but this has to be programmed in SEC
 *         kernel driver as well. No special return values are used at this time.
 */
int sec_uio_send_command(sec_job_ring_t *job_ring, int32_t uio_command);

/*================================================================================================*/


/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //SEC_CONFIG_H
