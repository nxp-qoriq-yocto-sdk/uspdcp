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

#ifndef SEC_CONTEXTS_H
#define SEC_CONTEXTS_H

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

/** Constants used to configure a thread-safe/unsafe pool. */
#define THREAD_SAFE_POOL    THREAD_SAFE_LIST
#define THREAD_UNSAFE_POOL  THREAD_UNSAFE_LIST
/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** Status of a SEC context */
typedef enum sec_context_usage_e
{
    SEC_CONTEXT_UNUSED = 0,  /*< SEC context is unused and is located in the free list. */
    SEC_CONTEXT_USED,        /*< SEC context is unused and is located in the in-use list. */
    SEC_CONTEXT_RETIRING,    /*< SEC context is unused and is located in the retire list. */
}sec_context_usage_t;

struct sec_contexts_pool_s;
struct sec_context_s;

/** The declaration of a context pool. */
typedef struct sec_contexts_pool_s
{
	/* The list of free contexts */
	list_t free_list;
	/* The list of retired contexts */
	list_t retire_list;
	/* The list of in use contexts */
	list_t in_use_list;

	/* Total number of contexts available in all three lists. */
	uint32_t no_of_contexts;

}sec_contexts_pool_t;

/** The declaration of a SEC context structure. */
typedef struct sec_context_s
{
	/** A node in the list which holds this sec context.
	 * @note: For the whole list concept to work the list_node_t must be defined
	 * statically in the sec_context_t structure. This is needed because the address
	 * of a sec_context is computed by subtracting the offset of the node member in
	 * the sec_context_t structure from the address of the node.
	 *
	 * To optimize this, the node is placed right at the beginning of the sec_context_t
	 * structure thus having the same address with the context itself. So no need for
	 * subtraction.
	 * @note: The macro #GET_CONTEXT_FROM_LIST_NODE is implemented with this optimization!!!!
	 * @note: Do not change the position of the node member in the structure.
	 * */
	list_node_t node;
    /** The handle of the JR to which this context is affined.
     *  This handle is needed in sec_process_packet() function to identify
     *  the input JR in which the packet will be enqueued.  */
    sec_job_ring_handle_t * jr_handle;
    /* The pool this context belongs to.
     * This pointer is needed when delete_pdcp_context() is received from UA
     * to be able to identify the pool from which this context was acquired.
     * A context can be taken from:
     *  - the affined JR's pool
     *  - global pool if the affined JR's pool is full
     *  */
    sec_contexts_pool_t * pool;
    /** The callback called for UA notifications. */
	sec_out_cbk notify_packet_cbk;
    /* Mutex used to synchronize the access to usage & packets_no members
     * from two different threads: Producer and Consumer.
     * TODO: find a solution to remove it.*/
    pthread_mutex_t mutex;
    /** The status of the sec context.
     *  This status is needed in the sec_poll function, to decide which packet status
     *  to provide to UA when the notification callback is called.
     *  This field may seem redundant (because of the three lists) but it is not. */
    sec_context_usage_t usage;
    /* Number of packets in flight for this context.
	 * The maximum value this counter can have is the maximum size of a JR.
	 * For SEC 4.4 being 1024 and for SEC 3.1 being 24.
	 * It is safe to use 2 bytes for this counter. */
	uint16_t packets_no;
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

/** @brief Initialize a pool of sec contexts.
 *
 *  The pool can be configured thread safe or not.
 *  A thread safe pool will use thread safe lists for storing the contexts.
 *
 * @param [in] pool                Pointer to a sec context pool structure.
 * @param [in] number_of_contexts  The number of contexts to allocated for this pool.
 * @param [in] thread_safe         Configure the thread safeness.
 *                                 Valid values: #THREAD_SAFE_POOL, #THREAD_UNSAFE_POOL
 */
uint32_t init_contexts_pool(sec_contexts_pool_t * pool,
		                    const uint32_t number_of_contexts,
		                    const uint8_t thread_safe);

/** @brief Destroy a pool of sec contexts.
 *
 *  Destroy the lists and free any memory allocated.
 *
 *  @param [in] pool                Pointer to a sec context pool structure.
 * */
void destroy_contexts_pool(sec_contexts_pool_t * pool);

/** @brief Get a free context from the pool.
 *
 *  @note If the pool was configured as thread safe, then this function CAN be called
 *  by multiple threads simultaneously.
 *  @note If the pool was not configured as thread safe, then this function CANNOT be called
 *  by multiple threads simultaneously.
 *
 *  @param [in] pool                Pointer to a sec context pool structure.
 * */
sec_context_t* get_free_context(sec_contexts_pool_t * pool);

/** @brief Release a context from the pool.
 *
 *  If the context has packets in flight, the context will be moved to a retire list and
 *  will not be available for reuse until all packets in flight are processed.
 *
 *  If the context has no packets in flight, the context will be moved to the free list
 *  and will be available for reuse.
 *
 *  @note If the pool was configured as thread safe, then this function CAN be called
 *  by multiple threads simultaneously.
 *  @note If the pool was not configured as thread safe, then this function CANNOT be called
 *  by multiple threads simultaneously.
 *
 *  @param [in] pool                Pointer to a sec context pool structure.
 *  @param [in] ctx                 Pointer to the sec context that should be deleted.
 * */

uint32_t free_or_retire_context(sec_contexts_pool_t * pool, sec_context_t * ctx);
/*================================================================================================*/


/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //SEC_CONTEXTS_H
