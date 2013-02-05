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

#ifndef SEC_PDCP_H
#define SEC_PDCP_H

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include "fsl_sec.h"
#include "sec_contexts.h"
#include "sec_hw_specific.h"

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/
#ifdef SEC_HW_VERSION_3_3
/** Length in bytes for PDCP control plane MAC-I */
#define PDCP_MAC_I_LENGTH  4
#endif

/** Macro for initializing the common part of a SD performing PDCP control
 * plane encap or decap. It is useful only for combinations supported by the 
 * PROTOCOL operation in SEC.
 */
#define SEC_PDCP_INIT_CPLANE_SD(descriptor){ \
        /* CTYPE = shared job descriptor                        \
         * RIF = 0
         * DNR = 0
         * ONE = 1
         * Start Index = 5, in order to jump over PDB
         * ZRO,CIF = 0
         * SC = 1
         * PD = 0, SHARE = WAIT
         * Descriptor Length = 10
         */                                                     \
        (descriptor)->deschdr.command.word  = 0xB8851113;       \
        /* CTYPE = Key
         * Class = 2 (authentication)
         * SGF = 0
         * IMM = 0 (key ptr after command)
         * ENC, NWB, EKT, KDEST = 0
         * TK = 0
         * Length = 0 (to be completed at runtime)
         */                                                     \
        (descriptor)->key2_cmd.command.word = 0x04000000;       \
        /* CTYPE = Key
         * Class = 1 (encryption)
         * SGF = 0
         * IMM = 0 (key ptr after command)
         * ENC, NWB, EKT, KDEST = 0
         * TK = 0
         * Length = 0 (to be completed at runtime)
         */                                                     \
        (descriptor)->key1_cmd.command.word = 0x02000000;       \
        /* CTYPE = Protocol operation
         * OpType = 0 (to be completed @ runtime)
         * Protocol ID = PDCP - C-Plane
         * Protocol Info = 0 (to be completed @ runtime)
         */                                                     \
        (descriptor)->protocol.command.word = 0x80430000;       \
}

/** Macro for initializing the common part of a SD performing PDCP data
 * plane encap or decap. It is useful only for combinations supported by the 
 * PROTOCOL operation in SEC.
 */
#define SEC_PDCP_INIT_UPLANE_SD(descriptor){ \
        /* CTYPE = shared job descriptor
         * RIF = 0
         * DNR = 0
         * ONE = 1
         * Start Index = 7, in order to jump over PDB and
         *                  key2 command
         * ZRO,CIF = 0
         * SC = 1
         * PD = 0, SHARE = WAIT
         * Descriptor Length = 10
         */                                                                  \
        (descriptor)->deschdr.command.word  = 0xB8871113;                    \
        /* CTYPE = Key
         * Class = 2 (authentication)
         * SGF = 0
         * IMM = 0 (key ptr after command)
         * ENC, NWB, EKT, KDEST = 0
         * TK = 0
         * Length = 0 (to be completed at runtime)
         */                                                                  \
        (descriptor)->key2_cmd.command.word = 0x04000000;                    \
        /* CTYPE = Key
         * Class = 1 (encryption)
         * SGF = 0
         * IMM = 0 (key after command)
         * ENC, NWB, EKT, KDEST = 0
         * TK = 0
         * Length = 0 (to be completed at runtime)
         */                                                                  \
        (descriptor)->key1_cmd.command.word = 0x02000000;                    \
        /* CTYPE = Protocol operation
         * OpType = 0 (to be completed @ runtime)
         * Protocol ID = PDCP - C-Plane
         * Protocol Info = 0 (to be completed @ runtime)
         */                                                                  \
        (descriptor)->protocol.command.word = 0x80420000;                    \
}

/** Macro for setting the pointer and length of the key to be used for
 * encryption in a SD performing PDCP encap/decap for control or data plane.
 */
#define SEC_PDCP_SD_SET_KEY1(descriptor,key,len) {              \
        (descriptor)->key1_cmd.command.field.length = (len);    \
        (descriptor)->key1_ptr = g_sec_vtop((key));              \
}

/** Macro for setting the pointer and length of the key to be used for
 * integrity in a SD performing PDCP encap/decap for control or data plane.
 */
#define SEC_PDCP_SD_SET_KEY2(descriptor,key,len) {              \
        (descriptor)->key2_cmd.command.field.length = (len);    \
        (descriptor)->key2_ptr = g_sec_vtop((key));             \
}

/** Macro for setting the direction (encapsulation or decapsulation)
 * for descriptors implementing the PROTOCOL operation command for PDCP.
 */
#define SEC_PDCP_SD_SET_PROT_DIR(descriptor,dir)                             \
        ((descriptor)->protocol.command.field.optype =                       \
        ((dir) == PDCP_ENCAPSULATION) ? CMD_PROTO_ENCAP : CMD_PROTO_DECAP)

/** Macro for setting the algorithm combination
 * for descriptors implementing the PROTOCOL operation command for PDCP.
 */
#define SEC_PDCP_SD_SET_PROT_ALG(descriptor,alg)                             \
        ((descriptor)->protocol.command.field.protinfo =                     \
        (alg == SEC_ALG_SNOW) ? CMD_PROTO_SNOW_ALG : (alg == SEC_ALG_AES) ?  \
                 CMD_PROTO_AES_ALG : CMD_PROTO_SNOW_ALG )

/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
/** Forward structure declaration */
//typedef struct sec_job_t sec_job_t;
/** Forward structure declaration */
//typedef struct sec_descriptor_t sec_descriptor_t;
/** Forward structure declaration */
//typedef struct sec_context_t sec_context_t;

/** Structure describing the PDB (Protocol Data Block)
 * needed by protocol acceleration descriptors for
 * PDCP Control Plane.
 */
struct cplane_pdb_s{
    unsigned int res1;
    unsigned int hfn:27;
    unsigned int res2:5;
    unsigned int bearer:5;
    unsigned int dir:1;
    unsigned int res3:26;
    unsigned int threshold:27;
    unsigned int res4:5;
}PACKED;

/** Structure describing the PDB (Protocol Data Block)
 * needed by protocol acceleration descriptors for
 * PDCP User Plane.
 */
struct uplane_pdb_s{
    unsigned int res1:30;
    unsigned int sns:1;
    unsigned int res2:1;
    union{
        struct{
            unsigned int hfn:20;
            unsigned int res:12;
        }PACKED hfn_l;
        struct{
            unsigned int hfn:25;
            unsigned int res:7;
        }PACKED hfn_s;
        unsigned int word;
    } PACKED hfn;
    unsigned int bearer:5;
    unsigned int dir:1;
    unsigned int res3:26;
    union{
        struct{
            unsigned int threshold:20;
            unsigned int res:12;
        }PACKED threshold_l;
        struct{
            unsigned int threshold:25;
            unsigned int res:7;
        }PACKED threshold_s;
    } PACKED threshold;
}PACKED;

/** Structure for aggregating the two types of
 * PDBs existing in PDCP Driver.
 */
typedef struct sec_pdcp_pdb_s
{
    union{
        uint32_t    content[4];
        struct cplane_pdb_s cplane_pdb;
        struct uplane_pdb_s uplane_pdb;
    }PACKED pdb_content;
}PACKED sec_pdcp_pdb_t;

/** Structure encompassing a shared descriptor,
 * containing all the information needed by a
 * SEC for processing the packets belonging to
 * the same context:
 *  - key for authentication
 *  - key for encryption
 *  - protocol data block (for Hyper Frame Number, etc.)
 */
struct sec_pdcp_sd_t{
    struct descriptor_header_s  deschdr;
    sec_pdcp_pdb_t              pdb;
    struct key_command_s        key2_cmd;
#warning "Update for 36 bits addresses"
    dma_addr_t                  key2_ptr;
    struct key_command_s        key1_cmd;
#warning "Update for 36 bits addresses"
    dma_addr_t                  key1_ptr;
    uint32_t                    hfn_ov_desc[9];
    struct protocol_operation_command_s protocol;
} PACKED;
/*==============================================================================
                                 CONSTANTS
==============================================================================*/

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/** @brief Updates a SEC context with PDCP specific cryptographic data
 * for a certain SEC context.
 *
 * @param [in,out] ctx          SEC context
 * @param [in]     crypto_info  PDCP protocol related cryptographic information
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
int sec_pdcp_context_set_crypto_info(sec_context_t *ctx,
                                     const sec_pdcp_context_info_t *crypto_info);
/*============================================================================*/


#endif  /* SEC_PDCP_H */
