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
#include "sec_atomic.h"
#include "sec_utils.h"
#include "sec_pdcp.h"
#include "sec_hw_specific.h"

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/


/** Constants used to configure a thread-safe/unsafe pool. */
#define THREAD_SAFE_POOL    THREAD_SAFE_LIST
#define THREAD_UNSAFE_POOL  THREAD_UNSAFE_LIST

/** Get state bitfield from state_packets_no member in ::sec_context_t structure.
 * First 3 bits in state_packets_no represent the state. */
#define CONTEXT_GET_STATE(value)        ((value) >> 29)
#define CONTEXT_SET_STATE(value, state) ((value) = ((value) & 0x1fffffff) | ((state) << 29))

/** Get packets_no bitfield from state_packets_no member in ::sec_context_t structure.
 * Last 29 bits in state_packets_no represent the packets_no. */
#define CONTEXT_GET_PACKETS_NO(value)   ((value) & 0x1fffffff)
#define CONTEXT_SET_PACKETS_NO(value, packets_no)   ((value) = ((value) & 0xe0000000) | (packets_no))

/** Validation bit pattern. A valid sec_context_t item would contain
 * this pattern at predefined position/s in the item itself. */
#define CONTEXT_VALIDATION_PATTERN  0xF0A955CD
/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** Function type used to update a SEC descriptor according to 
 * the processing operations that must be done on a packet. */
typedef int (*sec_update_descriptor)(sec_job_t *job, sec_descriptor_t *descriptor);

/** Status of a SEC context.
 * @note: Can have at most 4 values in the range 0...3!
 * If required to add more values, increase size of state bitfield from
 * state_packets_no member in ::sec_context_t structure! */
typedef enum sec_context_usage_e
{
    SEC_CONTEXT_UNUSED = 0,  /*< SEC context is unused and is located in the free list. */
    SEC_CONTEXT_USED,        /*< SEC context is used and is located in the in-use list. */
    SEC_CONTEXT_RETIRING,    /*< SEC context is unused and is located in the retire list. */
}sec_context_usage_t;

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
	struct sec_context_t *sec_contexts;

}sec_contexts_pool_t;


/** The declaration of a SEC context structure. */
struct sec_context_t
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
	 * @note: Do not change the position of the node member in the structure. */
	list_node_t node;
    /** Validation pattern at start of structure.
     * @note The first member from sec_context_t structure MUST be the node element.
     *       This allows for an optimized conversion from list_node_t to sec_context_t! */
    uint32_t start_pattern;
    /** The handle of the JR to which this context is affined.
     *  This handle is needed in sec_process_packet() function to identify
     *  the input JR in which the packet will be enqueued. */
    sec_job_ring_handle_t *jr_handle;
    /* The pool this context belongs to.
     * This pointer is needed when delete_pdcp_context() is received from UA
     * to be able to identify the pool from which this context was acquired.
     * A context can be taken from:
     *  - the affined JR's pool
     *  - global pool if the affined JR's pool is full */
    sec_contexts_pool_t *pool;
    /** The callback called for UA notifications. */
	sec_out_cbk notify_packet_cbk;
    /** Bitfield representing:
     *      state:3 bits | packet_no:29 bits
     *
     * @note Writes to this field MUST be done using the primitives defined
     * in sec_atomic.h when executed in a concurrent access context!
     *
     * State:
     *  The status of the sec context. Can have values from ::sec_context_usage_t enum.
     *  This status is needed in the sec_poll function, to decide which packet status
     *  to provide to UA when the notification callback is called.
     *  This field may seem redundant (because of the three lists) but it is not.
     *
     * Packets_no:
     *  Number of packets in flight for this context.
     *  The maximum value this counter can have is the maximum size of a JR.
     *  For SEC 4.4 being 1024 and for SEC 3.1 being 24.
     * */
	atomic_t state_packets_no;

    /** Crypto info received from UA.
     * TODO: replace with union when other protocols besides PDCP will be supported!*/
    sec_pdcp_context_info_t *pdcp_crypto_info;
    /** Cryptographic information that defines this SEC context.
     * The <keys> member from this structure is DMA-accesible by SEC device. */
    sec_crypto_pdb_t crypto_desc_pdb;
    /** Function used to update a crypto descriptor for a packet
     * belonging to this context. Does not apply to authentication descriptor!
     */
    sec_update_descriptor update_crypto_descriptor;
    /** Validation pattern at end of structure. */
    uint32_t end_pattern;
}__CACHELINE_ALIGNED;

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
 * @param [in,out] dma_mem         Pointer to a DMA-capable memory zone, to be used 
 *                                 for allocating SEC crypto info.
 * @param [in] number_of_contexts  The number of contexts to allocated for this pool.
 * @param [in] thread_safe         Configure the thread safeness.
 *                                 Valid values: #THREAD_SAFE_POOL, #THREAD_UNSAFE_POOL
 */
sec_return_code_t init_contexts_pool(sec_contexts_pool_t *pool,
                                     void **dma_mem,
                                     uint32_t number_of_contexts,
                                     uint8_t thread_safe);

/** @brief Destroy a pool of sec contexts.
 *
 *  Destroy the lists and free any memory allocated.
 *
 *  @param [in] pool                Pointer to a sec context pool structure.
 * */
void destroy_contexts_pool(sec_contexts_pool_t *pool);

/** @brief Get a free context from the pool.
 *
 *  @note If the pool was configured as thread safe, then this function CAN be called
 *  by multiple threads simultaneously.
 *  @note If the pool was not configured as thread safe, then this function CANNOT be called
 *  by multiple threads simultaneously.
 *
 *  @param [in] pool                Pointer to a sec context pool structure.
 * */
sec_context_t* get_free_context(sec_contexts_pool_t *pool);

/** @brief Release a context from the pool.
 *
 *  If the context has packets in flight, the context will be moved to a retire list and
 *  will not be available for reuse until all packets in flight are processed. This function
 *  should not be called again for the same context once all the packets in flight were processed.
 *  A garbage collector is called for every call to get_free_context() or free_or_retire_context()
 *  which will make the retired contexts with no packets in flight reusable.
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

sec_return_code_t free_or_retire_context(sec_contexts_pool_t *pool, sec_context_t *ctx);
/*================================================================================================*/


/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //SEC_CONTEXTS_H
