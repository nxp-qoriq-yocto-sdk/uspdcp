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
#include "list.h"
#include "sec_contexts.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

#ifndef offsetof
#ifdef __GNUC__
#define offsetof(a,b)           __builtin_offsetof(a,b)
#else
#define offsetof(type,member)   ((size_t) &((type*)0)->member)
#endif
#endif

/** @brief Compute the address of a context based on the address of the associated list node
 *
 * @note: Because the node (list_node_t) is placed right at the beginning of the sec_context_t
 * structure there is no need for subtraction: the node and the associated context have the
 * same address. */
//#define GET_CONTEXT_FROM_LIST_NODE(list_node) ((sec_context_t*)((list_node) - offsetof(sec_context_t, node)))
#define GET_CONTEXT_FROM_LIST_NODE(list_node) ((sec_context_t*)(list_node))
/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/* Statically allocate memory for SEC contexts.
 * These contexts will be used for filling the JR pools and the global pool. */
static sec_context_t sec_contexts[SEC_MAX_PDCP_CONTEXTS];
static int no_of_contexts_used = 0;
/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
/** @brief Destroy one of the lists from the pool.
 *
 * @param [in] list            The list to destroy.
 * */
static void destroy_pool_list(list_t * list);

/** @brief Retire a context.
 *
 * Move the context from the in-use list to the retire list
 *
 * @param [in] pool           Pointer to a sec context pool structure.
 * @param [in] ctx            The context to retire.
 * */
static void retire_context(sec_contexts_pool_t * pool, sec_context_t * ctx);

/** @brief Free a used context.
 *
 * Move the context from the in-use list to the free list
 *
 * @param [in] pool           Pointer to a sec context pool structure.
 * @param [in] ctx            The context to retire.
 * */
static void free_in_use_context(sec_contexts_pool_t * pool, sec_context_t * ctx);

/** @brief Run the garbage collector on the retiring list of contexts.
 *
 * The contexts that have packets in flight are not deleted immediately when
 * sec_delete_pdcp_context() is called instead they are marked as retiring.
 *
 * When all the packets in flight were notified to UA with #SEC_STATUS_OVERDUE and
 * #SEC_STATUS_LAST_OVERDUE statuses, the contexts can be deleted (ctx->packets_no == 0).
 * The function where we can find out the first time that all packets in flight
 * were notified is sec_poll().
 * To minimize the response time of the UA notifications, the contexts will not be deleted
 * in the sec_poll() function. Instead the retiring contexts with no more packets in flight
 * will be deleted whenever UA creates a new context or deletes another -> this is the purpose
 * of the garbage collector.
 *
 * The garbage collector will be called when a context is taken from the pool or when
 * a context is released to the pool.
 *
 * @param [in] pool          The pool on which the garbage collector is called.
 */
static void run_contexts_garbage_colector(sec_contexts_pool_t * pool);

/** @brief This function modifies the context associated to a list node
 * after deletion from the retiring list (the usage member is changed and
 * other members of the context structure).
 *
 * This function is needed by the list's API delete_matching_nodes.
 *
 * @param [in] node         A list node.
 *
 * */
static void node_modify_after_delete(list_node_t *node);

/** @brief This function matches the retiring contexts with
 * no packets in flight and which can be reused (moved to free list).
 *
 * This function is needed by the list's API delete_matching_nodes.
 *
 * @param [in] node         A list node.
 *
 * */
static uint8_t node_match(list_node_t *node);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static void destroy_pool_list(list_t * list)
{
	sec_context_t *ctx = NULL;
	list_node_t * node = NULL;

	assert(list != NULL);

	while(!list->is_empty(list))
	{
		node = list->remove_first(list);
		ctx = GET_CONTEXT_FROM_LIST_NODE(node);
		assert((uint32_t)ctx == (uint32_t)node);

		// destroy the context's mutex
		pthread_mutex_destroy(&(ctx->mutex));
		memset(ctx, 0, sizeof(sec_context_t));
		ctx->usage = SEC_CONTEXT_UNUSED;
	}
	list_destroy(list);
}

static void retire_context(sec_contexts_pool_t * pool, sec_context_t * ctx)
{
	assert(ctx != NULL);
	assert(pool != NULL);

	// remove context from in use list
	assert(ctx->usage == SEC_CONTEXT_USED);
	pool->in_use_list.delete_node(&pool->in_use_list, &ctx->node);

	// modify contex's usage before adding it to the retire list
	// once it is added to the retire list it can be retrieved and
	// used by other threads ... and we should not interfere
	ctx->usage = SEC_CONTEXT_RETIRING;

	// add context to retire list
	pool->retire_list.add_tail(&pool->retire_list, &ctx->node);
}

static void free_in_use_context(sec_contexts_pool_t * pool, sec_context_t * ctx)
{
	assert(ctx != NULL);
	assert(pool != NULL);

	// remove context from in use list
	assert(ctx->usage == SEC_CONTEXT_USED);
	pool->in_use_list.delete_node(&pool->in_use_list, &ctx->node);

	// modify contex's usage before adding it to the free list
	// once it is added to the free list it can be retrieved and
	// used by other threads ... and we should not interfere
	ctx->usage = SEC_CONTEXT_UNUSED;
	ctx->notify_packet_cbk = NULL;
	ctx->jr_handle = NULL;

	// add context to free list
	pool->free_list.add_tail(&pool->free_list, &ctx->node);
}

static uint8_t node_match(list_node_t *node)
{
	sec_context_t * ctx = NULL;

	assert(node != NULL);

	ctx = GET_CONTEXT_FROM_LIST_NODE(node);
	assert(ctx->usage == SEC_CONTEXT_RETIRING);

	if(ctx->packets_no == 0)
	{
		return 1;
	}

	return 0;
}

static void node_modify_after_delete(list_node_t *node)
{
	sec_context_t * ctx = NULL;

	assert(node != NULL);

	ctx = GET_CONTEXT_FROM_LIST_NODE(node);

	// modify contex's usage before adding it to the free list
	// once it is added to the free list it can be retrieved and
	// used by other threads ... and we should not interfere
	ctx->usage = SEC_CONTEXT_UNUSED;
	ctx->notify_packet_cbk = NULL;
	ctx->jr_handle = NULL;

}

static void run_contexts_garbage_colector(sec_contexts_pool_t * pool)
{
    list_t deleted_nodes;

    assert(pool != NULL);

    // if there is no retiring contexts then exit the garbage collector
    if (pool->retire_list.is_empty(&pool->retire_list))
    {
    	return;
    }

    list_init(&deleted_nodes, THREAD_UNSAFE_LIST);
    // Iterate through the list of retired contexts and
    // free the ones that have no more packets in flight
    // Delete all nodes for which the function node_match returns true.
    pool->retire_list.delete_matching_nodes(&pool->retire_list,
    		                                &deleted_nodes,
    		                                &node_match,
    		                                &node_modify_after_delete);

    if (deleted_nodes.is_empty(&deleted_nodes) == 0)
    {
    	pool->free_list.attach_list_to_tail(&pool->free_list, &deleted_nodes);
	}

    assert(deleted_nodes.is_empty(&deleted_nodes) == 1);
    list_destroy(&deleted_nodes);
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

uint32_t init_contexts_pool(sec_contexts_pool_t * pool,
		                    const uint32_t number_of_contexts,
		                    const uint8_t thread_safe)
{
	int i = 0;
	sec_context_t * ctx = NULL;

	assert(pool != NULL);

	if(no_of_contexts_used + number_of_contexts > SEC_MAX_PDCP_CONTEXTS)
	{
		// not enough contexts allocated statically
		// consider increasing #SEC_MAX_PDCP_CONTEXTS
		return 1;
	}

	// init lists
	list_init(&pool->free_list, thread_safe);
	list_init(&pool->retire_list, thread_safe);
	list_init(&pool->in_use_list, thread_safe);

	// fill up free list with free contexts from the statically defined array of contexts
	for (i = 0; i < number_of_contexts; i++)
	{
		// get a context from the statically allocated global array
		ctx = &sec_contexts[no_of_contexts_used];
		no_of_contexts_used++;

		// initialize the sec_context with valid values
		memset(ctx, 0, sizeof(sec_context_t));
		pthread_mutex_init(&(ctx->mutex), NULL);
		ctx->usage = SEC_CONTEXT_UNUSED;
		ctx->pool = pool;

		// add the context to the free list
		// WARNING: do not memset the context after adding it
		// to the list because it will override the node's next and
		// prev pointers
		pool->free_list.add_tail(&pool->free_list, &ctx->node);
	}

	pool->no_of_contexts = number_of_contexts;

	return 0;
}

void destroy_contexts_pool(sec_contexts_pool_t * pool)
{
	assert(pool != NULL);

	destroy_pool_list(&pool->free_list);
	destroy_pool_list(&pool->retire_list);
	destroy_pool_list(&pool->in_use_list);

	// TODO: Implement a safe mechanism for releasing contexts
	// In case some pools are destroyed at runtime, a simple index in the array
	// is not enough to keep track of the released/used contexts from the array
	// sec_contexts
	no_of_contexts_used -= pool->no_of_contexts;

	memset(pool, 0, sizeof(sec_contexts_pool_t));
}

sec_context_t* get_free_context(sec_contexts_pool_t * pool)
{
	sec_context_t * ctx = NULL;
	list_node_t * node = NULL;
	uint8_t run_gc = 0;

	assert(pool != NULL);

	// check if there are nodes in the free list
    if (pool->free_list.is_empty(&pool->free_list))
    {
    	// try and run the garbage collector in case no contexts are free
    	// The purpose is to free some retiring contexts for which all the packets in flight
		// were notified to UA
    	run_contexts_garbage_colector(pool);
    	// remember that we already run the garbage collector so that
    	// we do not run it again at exit
    	run_gc = 1;

    	// try again
    	if (pool->free_list.is_empty(&pool->free_list))
    	{
    		return NULL;
    	}
    }

    // remove first element from the free list
	node = pool->free_list.remove_first(&pool->free_list);

    ctx = GET_CONTEXT_FROM_LIST_NODE(node);
    assert(ctx->usage == SEC_CONTEXT_UNUSED);
    ctx->usage = SEC_CONTEXT_USED;

	// add the element to the tail of the in use list
    pool->in_use_list.add_tail(&pool->in_use_list, node);

    if (run_gc == 0)
    {
    	// run the contexts garbage collector only if it was not already run
    	// in this function call
    	run_contexts_garbage_colector(pool);
    }

    return ctx;
}

uint32_t free_or_retire_context(sec_contexts_pool_t * pool, sec_context_t * ctx)
{
	assert(pool != NULL);
	assert(ctx != NULL);
	assert(ctx->usage == SEC_CONTEXT_USED);

	// start critical area per context
	pthread_mutex_lock(&ctx->mutex);

	if (ctx->packets_no != 0)
	{
		// If packets in flight, do not release the context yet, move it to retiring.
		// The context will be deleted when all packets in flight are notified to UA
		retire_context(pool, ctx);

		// end of critical area per context
		pthread_mutex_unlock(&ctx->mutex);

		return SEC_PACKETS_IN_FLIGHT;
	}
	// end of critical area per context
	pthread_mutex_unlock(&ctx->mutex);

	// if no packets in flight then we can safely release the context
	free_in_use_context(pool, ctx);

	// run the contexts garbage collector
	run_contexts_garbage_colector(pool);

	return SEC_SUCCESS;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
