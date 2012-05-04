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
#include "sec_sg_utils.h"
#include "sec_utils.h"
#include <stdlib.h>

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

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

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

sec_return_code_t   build_sg_context(sec_sg_context_t *sg_ctx,
                                     const sec_packet_t *packet,
                                     sec_sg_context_type_t dir,
                                     sec_vtop vtop)
{
    uint8_t     *sg_tbl_en;
    uint32_t    num_fragments = 0;
    uint32_t    tmp_len = 0;
    uint32_t    total_length;
    int         i = 0;
    struct sec_sg_tbl_entry    *sg_tbl = NULL;
    sec_return_code_t ret = SEC_SUCCESS;

    // Sanity checks
    ASSERT(packet != NULL );
    ASSERT(sg_ctx != NULL);

    // Get number of fragments from first packet.
    num_fragments = packet[0].num_fragments;

    sg_tbl_en = (dir == SEC_SG_CONTEXT_TYPE_IN ? \
                       &sg_ctx->in_sg_tbl_en : &sg_ctx->out_sg_tbl_en);

    if( num_fragments == 0 )
    {
        /* If there aren't multiple fragments in this list,
         * return everything is fine
         */
        return SEC_SUCCESS;
    }

    total_length = packet[0].total_length;

    sg_tbl = (dir == SEC_SG_CONTEXT_TYPE_IN ?   \
            sg_ctx->in_sg_tbl : sg_ctx->out_sg_tbl);

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

        SG_TBL_SET_ADDRESS(sg_tbl[i],vtop(packet[i].address));
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

    if( dir == SEC_SG_CONTEXT_TYPE_IN )
        sg_ctx->in_total_length = total_length;
    else
        sg_ctx->out_total_length = total_length;
    
    // Enable SG for this direction
    *sg_tbl_en = 1;

    SEC_DEBUG("Created scatter gather table: @ 0x%08x",(uint32_t)sg_tbl);
    DUMP_SG_TBL(sg_tbl);

    return ret;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
