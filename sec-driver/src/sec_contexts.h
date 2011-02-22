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

#ifndef CONTEXTS_POOL_H
#define CONTEXTS_POOL_H

#ifdef __cplusplus
/* *INDENT-OFF* */

extern "C"{
/* *INDENT-ON* */
#endif

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
#include <unistd.h>

#include "list.h"
#include "fsl_sec.h"

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define THREAD_SAFE_POOL     OFF
#define THREAD_UNSAFE_POOL   ON
/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/* Status of a SEC context */
typedef enum sec_context_usage_e
{
    SEC_CONTEXT_UNUSED = 0,
    SEC_CONTEXT_USED,
    SEC_CONTEXT_RETIRING,
}sec_context_usage_t;

struct sec_contexts_pool_s;
struct sec_context_s;

typedef uint32_t (*free_or_retire_ctx_func_ptr)(struct sec_contexts_pool_s * pool, struct sec_context_s * ctx);
typedef struct sec_context_s* (*get_free_context_func_ptr)(struct sec_contexts_pool_s * pool);

typedef struct sec_contexts_pool_s
{
	uint32_t no_of_contexts;

	list_t free_list; // list of free contexts
	list_t retire_list; // list of retired contexts
	list_t in_use_list; // list of in use contexts

	uint8_t thread_safe; // valid values: #THREAD_SAFE_POOL, #THREAD_UNSAFE_POOL

	// pointer to a function used to free or retire a context
    free_or_retire_ctx_func_ptr free_or_retire_ctx_func;
    get_free_context_func_ptr get_free_ctx_func;
}sec_contexts_pool_t;

/* SEC context structure. */
typedef struct sec_context_s
{
	/* Data used to iterate through a list of contexts */
	list_node_t node;

    /* The callback called for UA notifications */
    sec_out_cbk notify_packet_cbk;
    /* The status of the sec context.*/
    sec_context_usage_t usage;
    /* The handle of the JR to which this context is affined */
    sec_job_ring_handle_t * jr_handle;
    /* The pool this context belongs to.
     * This info is needed when delete context is received from UA */
    sec_contexts_pool_t * pool;
    /* Number of packets in flight for this context.*/
    int packets_no;
    /* Mutex used to synchronize the access to usage & packets_no members
     * from two different threads: Producer and Consumer.
     * TODO: find a solution to remove it.*/
    pthread_mutex_t mutex;

}sec_context_t;


/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

uint32_t init_contexts_pool(sec_contexts_pool_t * pool,
		                    const uint32_t number_of_contexts,
		                    const uint8_t thread_safe);

void destroy_contexts_pool(sec_contexts_pool_t * pool);

/*================================================================================================*/


/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //CONTEXTS_POOL_H
