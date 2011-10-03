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
#include "sec_sg_contexts.h"
#include "sec_utils.h"
#include <stdlib.h>

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
//#define GET_CONTEXT_FROM_LIST_NODE(list_node) container_of(list_node, sec_context_t, node)
#define GET_SG_CONTEXT_FROM_LIST_NODE(list_node) ((sec_sg_context_t*)(list_node))
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

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
/** @brief Destroy one of the lists from the pool.
 *
 * @param [in] list            The list to destroy.
 * */
static void destroy_sg_pool_list(list_t * list);

/** @brief Free a used context.
 *
 * Move the context from the in-use list to the free list
 *
 * @param [in] pool           Pointer to a sec context pool structure.
 * @param [in] ctx            The context to retire.
 * */
static void free_in_use_sg_context(sec_sg_contexts_pool_t * pool, sec_sg_context_t * ctx);

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static void destroy_sg_pool_list(list_t * list)
{
    sec_sg_context_t *ctx = NULL;
    list_node_t * node = NULL;

    ASSERT(list != NULL);

    while(!list->is_empty(list))
    {
        node = list->remove_first(list);
        ctx = GET_SG_CONTEXT_FROM_LIST_NODE(node);

        memset(ctx->in_sg_tbl, 0, sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES);

        memset(ctx->out_sg_tbl, 0, sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES);

        memset(ctx, 0, sizeof(sec_sg_context_t));
        ctx->state = SEC_SG_CONTEXT_UNUSED;
    }
    list_destroy(list);
}

static void free_in_use_sg_context(sec_sg_contexts_pool_t * pool, sec_sg_context_t * ctx)
{
    ASSERT(ctx != NULL);
    ASSERT(pool != NULL);

    // remove context from in use list
    pool->in_use_list.delete_node(&pool->in_use_list, &ctx->node);

    // modify contex's usage before adding it to the free list
    // once it is added to the free list it can be retrieved and
    // used by other threads ... and we should not interfere
    ctx->state = SEC_SG_CONTEXT_UNUSED;

    // add context to free list
    // TODO: maybe add new context to head -> better chance for a cache hit if same element is reused next
    pool->free_list.add_tail(&pool->free_list, &ctx->node);
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

sec_return_code_t init_sg_contexts_pool(sec_sg_contexts_pool_t * pool,
                                     uint32_t number_of_sg_contexts,
                                     void ** dma_mem)
{
    int i = 0;
    sec_sg_context_t * ctx = NULL;

    ASSERT(pool != NULL);
    ASSERT(dma_mem != NULL);

    if (number_of_sg_contexts == 0)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    // init lists
    list_init(&pool->free_list, THREAD_SAFE_LIST);
    list_init(&pool->in_use_list, THREAD_SAFE_LIST);

    // Allocate memory for this pool from heap
    // The pool is allocated at startup so this should not impact the runtime performance.
    pool->sec_sg_contexts = malloc(number_of_sg_contexts * sizeof(struct sec_sg_context_t));
    if (pool->sec_sg_contexts == NULL)
    {
        // failed to allocate memory
        return SEC_OUT_OF_MEMORY;
    }

    // fill up free list with free contexts from the statically defined array of contexts
    for (i = 0; i < number_of_sg_contexts; i++)
    {
        // get a context from the statically allocated global array
        ctx = &pool->sec_sg_contexts[i];

        // initialize the sec_context with valid values
        memset(ctx, 0, sizeof(struct sec_sg_context_t));
        ctx->state = SEC_SG_CONTEXT_UNUSED;
        ctx->pool = pool;

        ctx->in_total_length = 0;
        ctx->out_total_length = 0;

        ctx->in_sg_tbl_en = 0;
        ctx->out_sg_tbl_en = 0;

        // Need two SG tables, one for input packet, the other for the output
        ctx->in_sg_tbl = (struct sec_sg_tbl_entry*)*dma_mem;
        memset(ctx->in_sg_tbl, 0, sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES);
        *dma_mem += sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES;

        ctx->out_sg_tbl = (struct sec_sg_tbl_entry*)*dma_mem;
        memset(ctx->out_sg_tbl, 0, sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES);
        *dma_mem += sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES;

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

void destroy_sg_contexts_pool(sec_sg_contexts_pool_t * pool)
{
    ASSERT(pool != NULL);

    if (pool->is_initialized == FALSE)
    {
        SEC_DEBUG("Pool not yet initialized\n");
        return;
    }

    // destroy the lists
    destroy_sg_pool_list(&pool->free_list);
    destroy_sg_pool_list(&pool->in_use_list);

    // free the memory allocated for the contexts
    if(pool->sec_sg_contexts != NULL)
    {
        free(pool->sec_sg_contexts);
        pool->sec_sg_contexts = NULL;
    }

    memset(pool, 0, sizeof(sec_sg_contexts_pool_t));
}

sec_sg_context_t* get_free_sg_context(sec_sg_contexts_pool_t * pool)
{
    sec_sg_context_t * ctx = NULL;
    list_node_t * node = NULL;

    ASSERT(pool != NULL);

    if (pool->free_list.is_empty(&pool->free_list))
    {
        return NULL;
    }

    // remove first element from the free list
    node = pool->free_list.remove_first(&pool->free_list);
    ctx = GET_SG_CONTEXT_FROM_LIST_NODE(node);

    ASSERT(ctx->state == SEC_SG_CONTEXT_UNUSED);

    // Reset SG tables
    memset(ctx->in_sg_tbl, 0, sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES);
    memset(ctx->out_sg_tbl, 0, sizeof(struct sec_sg_tbl_entry) * SEC_MAX_SG_TBL_ENTRIES);

    ctx->state = SEC_SG_CONTEXT_USED;

    // add the element to the tail of the in use list
    pool->in_use_list.add_tail(&pool->in_use_list, node);

    return ctx;
}

sec_return_code_t free_sg_context(sec_sg_contexts_pool_t * pool, sec_sg_context_t * ctx)
{
    ASSERT(pool != NULL);
    ASSERT(ctx != NULL);
    ASSERT(ctx->state == SEC_SG_CONTEXT_USED);

    free_in_use_sg_context(pool, ctx);

    return SEC_SUCCESS;
}

sec_return_code_t   build_sg_context(sec_sg_context_t *sg_ctx,
                                     const sec_packet_t *packet,
                                     sec_sg_context_direction_t dir)
{
    uint8_t     *sg_tbl_en;;
    uint32_t    num_fragments = 0;
    uint32_t    tmp_len = 0;
    uint32_t    total_length;
    int         i = 0;
    struct sec_sg_tbl_entry    *sg_tbl = NULL;
    sec_return_code_t ret = SEC_SUCCESS;

    // Sanity checks
    ASSERT(packet != NULL );
    ASSERT( sg_ctx != NULL );

    // Get number of fragments from first packet.
    num_fragments = packet[0].num_fragments;

    sg_tbl_en = dir == SEC_SG_CONTEXT_DIR_IN ? \
                       &sg_ctx->in_sg_tbl_en : &sg_ctx->out_sg_tbl_en;

    if( num_fragments == 0 )
    {
        /* If there aren't multiple fragments in this list,
         * return everything is fine
         */
        return SEC_SUCCESS;
    }

    total_length = packet[0].total_length;

    sg_tbl = dir == SEC_SG_CONTEXT_DIR_IN ?   \
            sg_ctx->in_sg_tbl : sg_ctx->out_sg_tbl;

    do{
        SEC_DEBUG("Processing SG fragment %d",i);
        tmp_len += packet[i].length;
        SEC_ASSERT(tmp_len <= total_length, SEC_INVALID_INPUT_PARAM,
                   "Fragment %d length would make total length exceed the total buffer length (%d)",
                   i, total_length);
        SEC_ASSERT( packet[i].address != NULL, SEC_INVALID_INPUT_PARAM,
                    "Fragment %d pointer is NULL",i);

        SEC_ASSERT(packet[i].offset < packet[i].length, SEC_INVALID_INPUT_PARAM,
                   "Fragment %i offset (%d) is larger than its length (%d)",
                   i,packet[i].offset, packet[i].length);

        SG_TBL_SET_ADDRESS(sg_tbl[i],packet[i].address);
        SG_TBL_SET_OFFSET(sg_tbl[i], packet[i].offset);
        SG_TBL_SET_LENGTH(sg_tbl[i],packet[i].length);
    }while( ++i <= num_fragments);

    SG_TBL_SET_FINAL(sg_tbl[--i]);

    if( tmp_len != total_length )
    {
        SEC_ERROR("Packets' fragment length (%d) is not equal to the total buffer length (%d)",
                  tmp_len,total_length);
        return SEC_INVALID_INPUT_PARAM;
    }

    if( dir == SEC_SG_CONTEXT_DIR_IN )
        sg_ctx->in_total_length = total_length;
    else
        sg_ctx->out_total_length = total_length;

    SEC_DEBUG("Created scatter gather table:");
    DUMP_SG_TBL(sg_tbl);

    return ret;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
