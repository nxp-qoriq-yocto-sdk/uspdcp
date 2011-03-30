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

#ifndef SEC_PDCP_H
#define SEC_PDCP_H

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include "fsl_sec.h"

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/

/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
/** Forward structure declaration */
typedef struct sec_job_t sec_job_t;
/** Forward structure declaration */
typedef struct sec_descriptor_t sec_descriptor_t;
/** Forward structure declaration */
typedef struct sec_context_t sec_context_t;

/*==============================================================================
                                 CONSTANTS
==============================================================================*/

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/** @brief Updates a SEC context with PDCP specific cryptographic data
 * for a certain SEC context.
 *
 * @param [in,out] ctx          SEC context
 * @param [in]     crypto_pdb   PDCP crypto descriptor and PDB
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
int sec_pdcp_context_set_crypto_info(sec_context_t *ctx,
                                     const sec_pdcp_context_info_t *crypto_pdb);

/** @brief Updates a SEC descriptor for each packet with pointers to
 * input packet, output packet and crypto information.
 *
 * @param [in]     ctx          SEC context
 * @param [in,out] job          The job structure
 * @param [in,out] descriptor   SEC descriptor
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
int sec_pdcp_context_update_descriptor(sec_context_t *ctx,
                                       sec_job_t *job,
                                       sec_descriptor_t *descriptor);
/*============================================================================*/


#endif  /* SEC_PDCP_H */
