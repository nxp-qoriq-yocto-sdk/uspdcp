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
#include <string.h>
#include <assert.h>
#include "fsl_sec.h"

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/
#define PDCP_CONTEXT_NUMBER 10
#define JOB_RING_NUMBER     2
#define PACKET_NUMBER       5
/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/

#define CIRCULAR_COUNTER(x, max)   ((x) + 1) * ((x) != (max -1))

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

int pdcp_ready_packet_handler (sec_packet_t *in_packet,
                               sec_packet_t *out_packet,
                               ua_context_handle_t ua_ctx_handle,
                               uint32_t status,
                               uint32_t error_info)
{
    return SEC_RETURN_SUCCESS;
}


int main(void)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    sec_job_ring_t *job_ring_handles[JOB_RING_NUMBER];
    sec_context_handle_t pdcp_ctx_handle[PDCP_CONTEXT_NUMBER];
    sec_pdcp_context_info_t pdcp_ctx_cfg_data[PDCP_CONTEXT_NUMBER];
    sec_packet_t in_packet[PACKET_NUMBER];
    sec_packet_t out_packet[PACKET_NUMBER];
    int opaque = 3;

    memset (job_ring_handles, 0, sizeof(job_ring_handles));
    memset (pdcp_ctx_handle, 0, sizeof(pdcp_ctx_handle));
    memset (pdcp_ctx_cfg_data, 0, sizeof(pdcp_ctx_cfg_data));
    memset (in_packet, 0, sizeof(in_packet));
    memset (out_packet, 0, sizeof(out_packet));

    //////////////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC user space driver requesting #JOB_RING_NUMBER Job Rings
    //////////////////////////////////////////////////////////////////////////////
    ret = sec_init(JOB_RING_NUMBER, (sec_job_ring_t**)&job_ring_handles);
    if (ret != SEC_SUCCESS)
    {
        printf("sec_init::Error %d\n", ret);
        return 1;
    }

    for (i = 0; i < JOB_RING_NUMBER - 1; i++)
    {
        assert(job_ring_handles[i] != NULL);
    }

    /////////////////////////////////////////////////////////////////////
    // 2. Create a number of PDCP contexts affined to Job Rings, choosing
    // Job Rings round robin.
    /////////////////////////////////////////////////////////////////////

    k = 0;
    for (i = 0; i < PDCP_CONTEXT_NUMBER - 1; i++)
    {
        k = CIRCULAR_COUNTER(k, JOB_RING_NUMBER);
        ret = sec_create_pdcp_context (job_ring_handles[k],
                &pdcp_ctx_cfg_data[i],
                &pdcp_ctx_handle[i]);
        if (ret != SEC_SUCCESS)
        {
            printf("sec_create_pdcp_context::Error %d for PDCP context no %d \n", ret, i);
            return 1;
        }
    }


    /////////////////////////////////////////////////////////////////////
    // 3. Submit packets for first PDCP context.
    /////////////////////////////////////////////////////////////////////

    for (i = 0; i < PDCP_CONTEXT_NUMBER - 1; i++)
    {
        for (j = 0; j < PACKET_NUMBER - 1 ;j++)
        {
            ret = sec_process_packet(pdcp_ctx_handle[i],
                    &in_packet[j],
                    &out_packet[j],
                    (ua_context_handle_t)&opaque); 
            if (ret != SEC_SUCCESS)
            {
                printf("sec_process_packet::Error %d for PDCP context no %d \n", ret, i);
                return 1;
            }
        }
    }

    /////////////////////////////////////////////////////////////////////
    // 4. Poll for SEC results.
    /////////////////////////////////////////////////////////////////////

    // Will contain number of results received from SEC
    uint32_t out_number = 0; 
    int limit = -1; // retrieve all ready results from SEC

/*
    // Retrieve 2 results per job ring, in a round robin scheduling step.
    uint32_t weight = 2;
 
    ret = sec_poll(limit, weight, &out_number);
    if (ret != SEC_SUCCESS)
    {
        printf("sec_poll::Error %d when polling for SEC results\n", ret);
        return 1;
    }

    printf ("sec_poll:: Retrieved %d results from SEC \n", out_number);
*/

    ret = sec_poll_job_ring(job_ring_handles[0], limit, &out_number);
    if (ret != SEC_SUCCESS)
    {
        printf("sec_poll::Error %d when polling for SEC results\n", ret);
        return 1;
    }

    printf ("sec_poll:: Retrieved %d results from SEC \n", out_number);



    /////////////////////////////////////////////////////////////////////
    // x. Remove PDCP contexts
    /////////////////////////////////////////////////////////////////////
    for (i = 0; i < PDCP_CONTEXT_NUMBER - 1; i++)
    {
        ret = sec_delete_pdcp_context (pdcp_ctx_handle[i]);
        if (ret != SEC_SUCCESS)
        {
            printf("sec_delete_pdcp_context::Error %d for PDCP context no %d \n", ret, i);
            return 1;
        }
    }
    

    /////////////////////////////////////////////////////////////////////
    // x. Shutdown SEC user space driver.
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
