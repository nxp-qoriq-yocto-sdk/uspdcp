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

#ifndef SEC_SG_CONTEXTS_H
#define SEC_SG_CONTEXTS_H

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

/** Validation bit pattern. A valid sec_context_t item would contain
 * this pattern at predefined position/s in the item itself. */
#define CONTEXT_VALIDATION_PATTERN  0xF0A955CD

/** Maximum number of entries that can be used */
#define SEC_MAX_SG_TBL_ENTRIES  0x20

/* TODO: Write something meaningful here */
#define SG_CONTEXT_IN_TBL_EN(sg_ctx)        ( (sg_ctx)->in_sg_tbl_en == 1 )

/* TODO: Write something meaningful here */
#define SG_CONTEXT_OUT_TBL_EN(sg_ctx)       ( (sg_ctx)->out_sg_tbl_en == 1 )

/* TODO: Write something meaningful here */
#define SG_CONTEXT_GET_LEN_IN(sg_ctx)               ( (sg_ctx)->in_total_length )

/* TODO: Write something meaningful here */
#define SG_CONTEXT_GET_LEN_OUT(sg_ctx)               ( (sg_ctx)->out_total_length )

/* TODO: Write something meaningful here */
#define SG_CONTEXT_GET_TBL_IN(sg_ctx)               ( (sg_ctx)->in_sg_tbl )

/* TODO: Write something meaningful here */
#define SG_CONTEXT_GET_TBL_OUT(sg_ctx)               ( (sg_ctx)->out_sg_tbl )

#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#warning "Update for 36 bits addresses"
#error "36 bit not supported"
#else
#define SG_TBL_SET_ADDRESS(sg_entry,address)    ((sg_entry).addr_ls = \
                                                (dma_addr_t)(address) )
#endif // defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)

#define SG_TBL_SET_LENGTH(sg_entry,len)         ((sg_entry).length = (len) )
#define SG_TBL_SET_OFFSET(sg_entry,off)         ((sg_entry).offset = (off) )
#define SG_TBL_SET_FINAL(sg_entry)              ( (sg_entry).final = 1 )

#define DUMP_SG_TBL(sg_tbl) {                                               \
    int __i = 0;                                                            \
    do{                                                                     \
        int __j = 0;                                                        \
        for(__j = 0;                                                        \
            __j < sizeof(struct sec_sg_tbl_entry)/sizeof(uint32_t);         \
            __j++ ){                                                        \
            SEC_DEBUG("0x%08x %08x",                                        \
                    (uint32_t)((uint32_t*)(((&((sg_tbl)[__i])))) + __j),    \
                    *((uint32_t*)(&((sg_tbl)[__i])) + __j) );               \
        }                                                                   \
    }while((sg_tbl)[__i++].final != 1);                                     \
}
/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** Status of a SEC Scatter Gather context. */
typedef enum sec_sg_context_usage_e
{
    SEC_SG_CONTEXT_UNUSED = 0,  /*< SEC context is unused and is located in the free list. */
    SEC_SG_CONTEXT_USED,        /*< SEC context is used and is located in the in-use list. */
}sec_sg_context_usage_t;

/** Direction of a SG table */
typedef enum sec_sg_context_direction_e{
    SEC_SG_CONTEXT_DIR_IN = 0,
    SEC_SG_CONTEXT_DIR_OUT
}sec_sg_context_direction_t;

/** The declaration of a context pool. */
typedef struct sec_sg_contexts_pool_s
{
    /* The list of free contexts */
    list_t free_list;

    /* The list of in use contexts */
    list_t in_use_list;

    /* Total number of contexts available in all three lists. */
    uint16_t no_of_contexts;
    /* Flag indicating if this pool was initialized. Can be #TRUE or #FALSE. */
    uint16_t is_initialized;
    /* SEC contexts in this pool */
    struct sec_sg_context_t *sec_sg_contexts;

}sec_sg_contexts_pool_t;

struct sec_sg_tbl_entry{
    /* TODO: Write something meaningful here */
    uint32_t    res0:28;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    /* TODO: Write something meaningful here */
    uint32_t    addr_ms:4;
#else
    /* TODO: Write something meaningful here */
    uint32_t    res1:4;
#endif
    /* TODO: Write something meaningful here */
    uint32_t    addr_ls;
    /* TODO: Write something meaningful here */
    uint32_t    ext:1;
    /* TODO: Write something meaningful here */
    uint32_t    final:1;
    /* TODO: Write something meaningful here */
    uint32_t    length:30;
    /* TODO: Write something meaningful here */
    uint32_t    res2:19;
    /* TODO: Write something meaningful here */
    uint32_t    offset:13;
} PACKED;

struct sec_sg_context_t
{
    list_node_t node;
    /** Validation pattern at start of structure.
     * @note The first member from sec_context_t structure MUST be the node element.
     *       This allows for an optimized conversion from list_node_t to sec_context_t! */
    uint32_t start_pattern;

    /* TODO: Write something meaningful here */
    sec_sg_contexts_pool_t *pool;

    /* TODO: Write something meaningful here */
    volatile sec_sg_context_usage_t state;

    /* TODO: Write something meaningful here */
    volatile uint32_t   in_total_length;

    /* TODO: Write something meaningful here */
    volatile uint32_t   out_total_length;

    /* TODO: Write something meaningful here */
    uint8_t in_sg_tbl_en;

    /* TODO: Write something meaningful here */
    struct sec_sg_tbl_entry  *in_sg_tbl;

    /* TODO: Write something meaningful here */
    uint8_t out_sg_tbl_en;

    /* TODO: Write something meaningful here */
    struct sec_sg_tbl_entry  *out_sg_tbl;

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
 * @param [in] number_of_contexts  The number of contexts to allocated for this pool.
 * @param [in] thread_safe         Configure the thread safeness.
 *                                 Valid values: #THREAD_SAFE_POOL, #THREAD_UNSAFE_POOL
 */
sec_return_code_t init_sg_contexts_pool(sec_sg_contexts_pool_t *pool,
                                     uint32_t number_of_contexts,
                                     void ** dma_mem);

/** @brief Destroy a pool of sec contexts.
 *
 *  Destroy the lists and free any memory allocated.
 *
 *  @param [in] pool                Pointer to a sec context pool structure.
 * */
void destroy_sg_contexts_pool(sec_sg_contexts_pool_t *pool);

/** @brief Get a free context from the pool.
 *
 *  @note If the pool was configured as thread safe, then this function CAN be called
 *  by multiple threads simultaneously.
 *  @note If the pool was not configured as thread safe, then this function CANNOT be called
 *  by multiple threads simultaneously.
 *
 *  @param [in] pool                Pointer to a sec context pool structure.
 * */
sec_sg_context_t* get_free_sg_context(sec_sg_contexts_pool_t *pool);

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

sec_return_code_t free_sg_context(sec_sg_contexts_pool_t *pool, sec_sg_context_t *ctx);

/** TODO: Write something meaningful here */
sec_return_code_t build_sg_context(sec_sg_context_t *sg_ctx,
                                   const sec_packet_t *packet,
                                   sec_sg_context_direction_t dir);
/*================================================================================================*/


/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //SEC_CONTEXTS_H
