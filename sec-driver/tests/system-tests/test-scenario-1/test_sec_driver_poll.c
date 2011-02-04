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
#include <pthread.h>
#include "fsl_sec.h"

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
int setup_sec_environment(void);
int cleanup_sec_environment(void);
int send_packets(uint8_t job_ring);
int get_results(uint8_t job_ring);
int start_sec_threads(void);
int stop_sec_threads(void);
void* sec_thread_routine(void*);


/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/
#define PDCP_CONTEXT_NUMBER 10
#define JOB_RING_NUMBER     2
#define PACKET_NUMBER       5
/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
typedef struct thread_config_s
{
    int tid;
    int job_ring_id;
}thread_config_t;

/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/

#define CIRCULAR_COUNTER(x, max)   ((x) + 1) * ((x) != (max - 1))

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
sec_job_ring_t *job_ring_handles[JOB_RING_NUMBER];
sec_context_handle_t pdcp_ctx_handle[PDCP_CONTEXT_NUMBER];
sec_pdcp_context_info_t pdcp_ctx_cfg_data[PDCP_CONTEXT_NUMBER];
sec_packet_t in_packets[PACKET_NUMBER];
sec_packet_t out_packets[PACKET_NUMBER];
int opaque[PACKET_NUMBER];

pthread_t threads[JOB_RING_NUMBER];
int job_ring_to_ctx[JOB_RING_NUMBER][PDCP_CONTEXT_NUMBER];


/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

int pdcp_ready_packet_handler (sec_packet_t *in_packet,
                               sec_packet_t *out_packet,
                               ua_context_handle_t ua_ctx_handle,
                               uint32_t status,
                               uint32_t error_info)
{
    printf ("Received packet from SEC\n");
    return SEC_RETURN_SUCCESS;
}

int setup_sec_environment(void)
{
    int ret = 0;
    int i = 0;
    int k = 0;

    memset (job_ring_handles, 0, sizeof(job_ring_handles));
    memset (pdcp_ctx_handle, 0, sizeof(pdcp_ctx_handle));
    memset (pdcp_ctx_cfg_data, 0, sizeof(pdcp_ctx_cfg_data));
    memset (in_packets, 0, sizeof(in_packets));
    memset (out_packets, 0, sizeof(out_packets));
    memset (opaque, 0, sizeof(opaque));
    memset (job_ring_to_ctx, -1, sizeof(job_ring_to_ctx));

    //////////////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC user space driver requesting #JOB_RING_NUMBER Job Rings
    //////////////////////////////////////////////////////////////////////////////
    ret = sec_init(JOB_RING_NUMBER, (sec_job_ring_t**)&job_ring_handles);
    if (ret != SEC_SUCCESS)
    {
        printf("sec_init::Error %d\n", ret);
        return 1;
    }

    for (i = 0; i < JOB_RING_NUMBER; i++)
    {
        //assert(job_ring_handles[i] != NULL);
    }

    /////////////////////////////////////////////////////////////////////
    // 2. Create a number of PDCP contexts affined to Job Rings, choosing
    // Job Rings round robin.
    /////////////////////////////////////////////////////////////////////

    k = 0;
    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        k = CIRCULAR_COUNTER(k, JOB_RING_NUMBER);
        printf ("Create & affine PDCP context %d on Job Ring %d\n", i, k);
        ret = sec_create_pdcp_context (job_ring_handles[k],
                &pdcp_ctx_cfg_data[i],
                &pdcp_ctx_handle[i]);
        if (ret != SEC_SUCCESS)
        {
            printf("sec_create_pdcp_context::Error %d for PDCP context no %d \n", ret, i);
            return 1;
        }

        // Remember what PDCP context is affined to each Job Ring.
        job_ring_to_ctx[k][i] = i;
    }

    return 0;
}

int send_packets(uint8_t job_ring)
{
    int ret = 0;
    int i = 0;
    int j = 0;

    /////////////////////////////////////////////////////////////////////
    // 3. Submit packets for PDCP context.
    /////////////////////////////////////////////////////////////////////

    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
    {
        if (job_ring_to_ctx[job_ring][i] != -1)
        {
            for (j = 0; j < PACKET_NUMBER ;j++)
            {
                ret = sec_process_packet(pdcp_ctx_handle[i],
                        &in_packets[j],
                        &out_packets[j],
                        (ua_context_handle_t)&opaque[j]);
                if (ret != SEC_SUCCESS)
                {
                    printf("sec_process_packet::Error %d for PDCP context no %d \n", ret, i);
                    return 1;
                }
                printf ("Sent packet number %d to SEC on Job Ring %d and for PDCP context %d\n",j, job_ring, i);
            }
        }
    }

    return 0;
}

int get_results(uint8_t job_ring)
{
    int ret = 0;

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

    return 0;
}

int cleanup_sec_environment(void)
{
    int ret = 0;
    int i = 0;

    /////////////////////////////////////////////////////////////////////
    // x. Remove PDCP contexts
    /////////////////////////////////////////////////////////////////////
    for (i = 0; i < PDCP_CONTEXT_NUMBER; i++)
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
        printf("sec_release::Error %d\n", ret);
        return 1;
    }

    return 0;
}

int start_sec_threads(void)
{
    int ret = 0;
    int i = 0;

    thread_config_t th_config[JOB_RING_NUMBER];


    for (i = 0; i < JOB_RING_NUMBER; i++)
    {
        th_config[i].tid = i;
        th_config[i].job_ring_id = i;
        ret = pthread_create(&threads[1], NULL, &sec_thread_routine, (void*)&th_config[i]);
        assert(ret == 0);
    }

    return 0;
}

int stop_sec_threads(void)
{
    return 0;
}

void* sec_thread_routine(void* config)
{
    thread_config_t *th_config = NULL;
    int ret = 0;

    th_config = (thread_config_t*)config;
    printf("Hello World! It's me, thread #%d!\n", th_config->tid);

    ret = send_packets(th_config->job_ring_id);



    pthread_exit(NULL);
}


/*==================================================================================================
                                        GLOBAL FUNCTIONS
=================================================================================================*/

int main(void)
{
    int ret = 0;


    /////////////////////////////////////////////////////////////////////
    // 1. Initialize SEC environment
    /////////////////////////////////////////////////////////////////////
    ret = setup_sec_environment();
    if (ret != 0)
    {
        printf("setup_sec_environment returned error\n");
        return 1;
    }



    /////////////////////////////////////////////////////////////////////
    // 2. Start worker threads
    /////////////////////////////////////////////////////////////////////
    ret = start_sec_threads();
    if (ret != 0)
    {
        printf("start_sec_threads returned error\n");
        return 1;
    }


    
    /////////////////////////////////////////////////////////////////////
    // 3. Stop worker threads
    /////////////////////////////////////////////////////////////////////
    ret = stop_sec_threads();
    if (ret != 0)
    {
        printf("stop_sec_threads returned error\n");
        return 1;
    }


    /////////////////////////////////////////////////////////////////////
    // 3. Cleanup SEC environment
    /////////////////////////////////////////////////////////////////////
    ret = cleanup_sec_environment();
    if (ret != 0)
    {
        printf("cleanup_sec_environment returned error\n");
        return 1;
    }

    pthread_exit(NULL);


    //return 0;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
