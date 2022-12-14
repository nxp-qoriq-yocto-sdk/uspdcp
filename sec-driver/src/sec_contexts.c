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
#include <stdlib.h>
#include <inttypes.h>
#include "list.h"
#include "sec_contexts.h"
#include "sec_utils.h"

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/


/** @brief Compute the address of a context based on the address of the associated list node
 *
 * @note: Because the node (list_node_t) is placed right at the beginning of the sec_context_t
 * structure there is no need for subtraction: the node and the associated context have the
 * same address. */
#define GET_CONTEXT_FROM_LIST_NODE(list_node) ((sec_context_t*)(list_node))
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
extern sec_vtop g_sec_vtop;

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
 * When all the packets in flight were notified to UA,
 * the contexts can be deleted (ctx packets_no == 0).
 *
 * Inside this function we can find out the first time that all packets in flight
 * on a context were notified is sec_poll().
 *
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
 * after deletion from the retiring list (the state member is changed and
 * other members of the context structure).
 *
 * This function is needed by the list's API delete_matching_nodes.
 *
 * @param [in] node         A list node.
 *
 * */
static void node_modify_after_delete(struct list_head *node);

/** @brief This function matches the retiring contexts with
 * no packets in flight and which can be reused (moved to free list).
 *
 * This function is needed by the list's API delete_matching_nodes.
 *
 * @param [in] node         A list node.
 *
 * */
static uint8_t node_match(struct list_head *node);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static void destroy_pool_list(list_t * list)
{
    sec_context_t *ctx = NULL;
    struct list_head *node = NULL;

    ASSERT(list != NULL);

    while(!list->is_empty(list))
    {
        node = list->remove_first(list);
        ctx = GET_CONTEXT_FROM_LIST_NODE(node);
        memset(ctx, 0, sizeof(sec_context_t));
        ctx->state = SEC_CONTEXT_UNUSED;
    }
    list_destroy(list);
}

static void retire_context(sec_contexts_pool_t * pool, sec_context_t * ctx)
{
    ASSERT(ctx != NULL);
    ASSERT(pool != NULL);

    // remove context from in use list
    pool->in_use_list.delete_node(&pool->in_use_list, &ctx->node);

    // add context to retire list
    pool->retire_list.add_tail(&pool->retire_list, &ctx->node);
}

static void free_in_use_context(sec_contexts_pool_t * pool, sec_context_t * ctx)
{
    ASSERT(ctx != NULL);
    ASSERT(pool != NULL);

    // remove context from in use list
    pool->in_use_list.delete_node(&pool->in_use_list, &ctx->node);

    // modify contex's usage before adding it to the free list
    // once it is added to the free list it can be retrieved and
    // used by other threads ... and we should not interfere
    ctx->state = SEC_CONTEXT_UNUSED;
    ctx->pi = 0;
    ctx->ci = 0;
    ctx->notify_packet_cbk = NULL;
    ctx->jr_handle = NULL;
    ctx->crypto_info.pdcp_crypto_info = NULL;

    // add context to free list
    // TODO: maybe add new context to head -> better chance for a cache hit if same element is reused next
    pool->free_list.add_tail(&pool->free_list, &ctx->node);
}

static uint8_t node_match(struct list_head *node)
{
    sec_context_t * ctx = NULL;

    ASSERT(node != NULL);

    ctx = GET_CONTEXT_FROM_LIST_NODE(node);
    ASSERT(ctx->state == SEC_CONTEXT_RETIRING);

    // Context is retiring. Test if it still has packets in flight.
    if(CONTEXT_GET_PACKETS_NO(ctx) == 0)
    {
        return 1;
    }

    return 0;
}

static void node_modify_after_delete(struct list_head *node)
{
    sec_context_t * ctx = NULL;

    ASSERT(node != NULL);

    ctx = GET_CONTEXT_FROM_LIST_NODE(node);

    // modify contex's usage before adding it to the free list
    // once it is added to the free list it can be retrieved and
    // used by other threads ... and we should not interfere
    ctx->state = SEC_CONTEXT_UNUSED;
    ctx->pi = 0;
    ctx->ci = 0;
    ctx->notify_packet_cbk = NULL;
    ctx->jr_handle = NULL;

}

static void run_contexts_garbage_colector(sec_contexts_pool_t * pool)
{
    list_t deleted_nodes;

    ASSERT(pool != NULL);

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
        // TODO: maybe add new context to head -> better chance for a cache hit if same element is reused next
        pool->free_list.attach_list_to_tail(&pool->free_list, &deleted_nodes);
    }

    ASSERT(deleted_nodes.is_empty(&deleted_nodes) == 1);
    list_destroy(&deleted_nodes);
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

sec_return_code_t init_contexts_pool(sec_contexts_pool_t * pool,
                                     uint32_t number_of_contexts,
                                     void **dma_mem,
                                     uint8_t thread_safe)
{
    int i = 0;
    sec_context_t * ctx = NULL;

    ASSERT(pool != NULL);
    ASSERT(thread_safe == THREAD_SAFE_POOL || thread_safe == THREAD_UNSAFE_POOL);

    if (number_of_contexts == 0)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    // init lists
    list_init(&pool->free_list, thread_safe);
    list_init(&pool->retire_list, thread_safe);
    list_init(&pool->in_use_list, thread_safe);

    // Allocate memory for this pool from heap
    // The pool is allocated at startup so this should not impact the runtime performance.
    pool->sec_contexts = malloc(number_of_contexts * sizeof(struct sec_context_t));
    if (pool->sec_contexts == NULL)
    {
        // failed to allocate memory
        return SEC_OUT_OF_MEMORY;
    }

    // fill up free list with free contexts from the statically defined array of contexts
    for (i = 0; i < number_of_contexts; i++)
    {
        // get a context from the statically allocated global array
        ctx = &pool->sec_contexts[i];

        // initialize the sec_context with valid values
        memset(ctx, 0, sizeof(struct sec_context_t));
        ctx->state = SEC_CONTEXT_UNUSED;
        ctx->pi = 0;
        ctx->ci = 0;
        ctx->pool = pool;

        SEC_ASSERT ((uintptr_t)*dma_mem % L1_CACHE_BYTES == 0,
                      SEC_INVALID_INPUT_PARAM,
                      "Current memory position is not cacheline aligned."
                      "Context= %p", ctx);
        ctx->sh_desc = (struct sec_sd_t*)*dma_mem;
        memset(ctx->sh_desc, 0, SEC_CRYPTO_DESCRIPTOR_SIZE);
        *dma_mem += SEC_CRYPTO_DESCRIPTOR_SIZE;

        ctx->sh_desc_phys = g_sec_vtop(ctx->sh_desc);

        SEC_DEBUG("Created shared descriptor @ 0x%04x (phys: 0x%" PRIx64 ")",
                (uintptr_t)ctx->sh_desc,
                ctx->sh_desc_phys);

        // initialize validation patterns
        ctx->start_pattern = CONTEXT_VALIDATION_PATTERN;
        ctx->end_pattern = CONTEXT_VALIDATION_PATTERN;

        // Add the context to the free list
        // WARNING: do not memset with zero the context after adding it
        // to the list because it will override the node's next and
        // prev pointers.
        pool->free_list.add_tail(&pool->free_list, &ctx->node);
    }

    pool->is_initialized = TRUE;

    return SEC_SUCCESS;
}

void destroy_contexts_pool(sec_contexts_pool_t * pool)
{
    ASSERT(pool != NULL);

    if (pool->is_initialized == FALSE)
    {
        SEC_DEBUG("Pool not yet initialized\n");
        return;
    }

    // destroy the lists
    destroy_pool_list(&pool->free_list);
    destroy_pool_list(&pool->retire_list);
    destroy_pool_list(&pool->in_use_list);

    // free the memory allocated for the contexts
    if(pool->sec_contexts != NULL)
    {
        free(pool->sec_contexts);
        pool->sec_contexts = NULL;
    }

    memset(pool, 0, sizeof(sec_contexts_pool_t));
}

sec_context_t* get_free_context(sec_contexts_pool_t * pool)
{
    sec_context_t * ctx = NULL;
    struct list_head *node = NULL;
    uint8_t run_gc = 0;

    ASSERT(pool != NULL);

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

    ASSERT(ctx->state == SEC_CONTEXT_UNUSED);
    ASSERT(ctx->pi == 0);
    ASSERT(ctx->ci == 0);

    ctx->state = SEC_CONTEXT_USED;

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

sec_return_code_t free_or_retire_context(sec_contexts_pool_t * pool, sec_context_t * ctx)
{
    uint32_t packets_no = 0;
    ASSERT(pool != NULL);
    ASSERT(ctx != NULL);
    ASSERT(ctx->state == SEC_CONTEXT_USED);
    ASSERT(SEC_CONTEXT_USED < SEC_CONTEXT_RETIRING);

    // Set state to retire.
    ctx->state = SEC_CONTEXT_RETIRING;

    // If packets in flight, do not release the context yet, move it to retiring.
    // The context will be moved to free list when all packets in flight are notified to UA
    packets_no = CONTEXT_GET_PACKETS_NO(ctx);
    if (packets_no != 0)
    {
        retire_context(pool, ctx);
        return (packets_no == 1 ? SEC_LAST_PACKET_IN_FLIGHT : SEC_PACKETS_IN_FLIGHT);
    }

    // If no packets in flight then we can safely release the context
    free_in_use_context(pool, ctx);

    // Run the contexts garbage collector
    run_contexts_garbage_colector(pool);
    return SEC_SUCCESS;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
