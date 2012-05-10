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
/** Macro for checking if thre is a Scatter-Gather input table
 * for particular job.
 */
#define SG_CONTEXT_IN_TBL_EN(sg_ctx)        ( (sg_ctx)->in_sg_tbl_en == 1 )

/** Macro for checking if there is a Scatter-Gather output table
 * for a particular job.
 */
#define SG_CONTEXT_OUT_TBL_EN(sg_ctx)       ( (sg_ctx)->out_sg_tbl_en == 1 )

/** Macro for getting the total input length of a packet submitted as fragments.
 */
#define SG_CONTEXT_GET_LEN_IN(sg_ctx)               ( (sg_ctx)->in_total_length )

/** Macro for getting the total output length of a packet submitted as fragments.
 */
#define SG_CONTEXT_GET_LEN_OUT(sg_ctx)               ( (sg_ctx)->out_total_length )

/** Macro for getting a pointer to the input Scatter-Gather table of a job.
 */
#define SG_CONTEXT_GET_TBL_IN(sg_ctx)               ( (sg_ctx)->in_sg_tbl )

/** Macro for getting a pointer to the output Scatter-Gather table of a job.
 */
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

#if (SEC_DRIVER_LOGGING == ON) && (SEC_DRIVER_LOGGING_LEVEL == SEC_DRIVER_LOG_DEBUG)
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
#else //(SEC_DRIVER_LOGGING == ON) && (SEC_DRIVER_LOGGING_LEVEL == SEC_DRIVER_LOG_DEBUG)
#define DUMP_SG_TBL(sg_tbl)
#endif // (SEC_DRIVER_LOGGING == ON) && (SEC_DRIVER_LOGGING_LEVEL == SEC_DRIVER_LOG_DEBUG)
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
typedef enum sec_sg_context_type_e{
    SEC_SG_CONTEXT_TYPE_IN = 0,
    SEC_SG_CONTEXT_TYPE_OUT
}sec_sg_context_type_t;

/** Structure defining a Scatter Gather table entry */
struct sec_sg_tbl_entry{
    /** 28 bits reserved */
    uint32_t    res0:28;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    /** Most significant 4 bits of a 36 bit address */
    uint32_t    addr_ms:4;
#else
    /** 4 bits reserved */
    uint32_t    res1:4;
#endif
    /** Lower 32 bits of data address */
    uint32_t    addr_ls;
    /** Signals if this entry points to another Scatter-Gather table or to data */
    uint32_t    ext:1;
    /** If this bit is set, this is the last entry of the Scatter-Gather table */
    uint32_t    final:1;
    /** Length of the data pointed by this fragment */
    uint32_t    length:30;
    /** 19 bits reserved */
    uint32_t    res2:19;
    /* Offset of the current data in the reassembled buffer */
    uint32_t    offset:13;
} PACKED;

/** Structure defining a Scatter-Gather context for a job */
typedef struct{

    /** Total input data length */
    uint32_t   in_total_length;

    /** Total output data length */
    uint32_t   out_total_length;

    /** If set to 1, signals that there is a Scatter-Gather table
     * enabled for the input packet of this job
     */
    uint8_t in_sg_tbl_en;

    /** Input scatter-gather table, as a pointer instead of array */
    struct sec_sg_tbl_entry  *in_sg_tbl;

    /** If set to 1, signals that there is a Scatter-Gather table
     * enabled for the output packet of this job
     */
    uint8_t out_sg_tbl_en;

    /** Output scatter-gather table, as a pointer instead of array */
    struct sec_sg_tbl_entry  *out_sg_tbl;

}sec_sg_context_t;


/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/** @brief Populates the relevant data structures of a Scatter Gather contex for a job.
 * The function checks if there is a need to construct a Scatter-Gather table for input
 * or output packets (depending if there is 1 or more fragments)
 * @param [in]  sg_ctx      The Scatter Gather context to be populated.
 *
 * @param [in]  packet      Pointer to a packet for which the Scatter Gather 
 *                          table has to be constructed
 *
 * @param [in]  dir         The 'direction' of the packet (input or output)
 *
 * @param [in]  vtop        The V2P function to be used for translating the
                            the Scatter Gather table packets' virtual addresses
                            to physical addresses
 */
sec_return_code_t build_sg_context(sec_sg_context_t *sg_ctx,
                                   const sec_packet_t *packet,
                                   sec_sg_context_type_t dir);
/*================================================================================================*/


/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //SEC_CONTEXTS_H
