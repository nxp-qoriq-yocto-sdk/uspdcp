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
#include <stdio.h>
#include "fsl_sec.h"

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/
//sec_out_cbk pdcp_ready_packet_handler;

int main(void)
{
    int ret = 0;
    int job_rings_no = 2;
    sec_job_ring_t *job_ring_handles[2];
    sec_context_handle_t pdcp_ctx_handle = NULL;
    sec_pdcp_context_info_t pdcp_ctx_cfg_data;

    /////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC user space driver requesting 2 Job Rings
    /////////////////////////////////////////////////////////////////////
    ret = sec_init(job_rings_no, (sec_job_ring_t**)&job_ring_handles);
    if (ret != SEC_SUCCESS)
    {
        printf("sec_init::Error %d", ret);
        return 1;
    }

    /////////////////////////////////////////////////////////////////////
    // 2. Create a PDCP context affined to first Job Ring.
    /////////////////////////////////////////////////////////////////////
    ret = sec_create_pdcp_context (job_ring_handles[0],
                                   &pdcp_ctx_cfg_data,
                                   &pdcp_ctx_handle);
    if (ret != SEC_SUCCESS)
    {
        printf("sec_create_pdcp_context::Error %d", ret);
        return 1;
    }



    /////////////////////////////////////////////////////////////////////
    // x. Initialize SEC user space driver requesting 2 Job Rings
    /////////////////////////////////////////////////////////////////////

    ret = sec_release();
    if (ret != SEC_SUCCESS)
    {
        printf("sec_release::Error %d", ret);
        return 1;
    }

    return 0;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
