/* Copyright 2013 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the names of its 
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.      
 * 
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.                                                                   
 *                                                                                  
 * This software is provided by Freescale Semiconductor "as is" and any
 * express or implied warranties, including, but not limited to, the implied
 * warranties of merchantability and fitness for a particular purpose are
 * disclaimed. In no event shall Freescale Semiconductor be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential damages
 * (including, but not limited to, procurement of substitute goods or services;
 * loss of use, data, or profits; or business interruption) however caused and
 * on any theory of liability, whether in contract, strict liability, or tort
 * (including negligence or otherwise) arising in any way out of the use of
 * this software, even if advised of the possibility of such damage.
 */

#ifndef SEC_RLC_H
#define SEC_RLC_H

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include "fsl_sec.h"
#include "sec_contexts.h"

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/
#define SEC_RLC_PDB_OPT_SNS_SHIFT   1
#define SEC_RLC_PDB_OPT_SNS_AM      (0 << SEC_RLC_PDB_OPT_SNS_SHIFT)
#define SEC_RLC_PDB_OPT_SNS_UM      (1 << SEC_RLC_PDB_OPT_SNS_SHIFT)

#define SEC_RLC_PDB_OPT_4B_SHIFT    0
#define SEC_RLC_PDB_OPT_4B_SHIFT_EN (1 << SEC_RLC_PDB_OPT_4B_SHIFT)

#define SEC_RLC_PDB_HFN_SHIFT_UM    7
#define SEC_RLC_PDB_HFN_SHIFT_AM    12

#define SEC_RLC_PDB_BEARER_SHIFT    27
#define SEC_RLC_PDB_DIR_SHIFT       26

/** Macro for initializing the common part of a SD performing RLC data
 * plane encap or decap. It is useful only for combinations supported by the 
 * PROTOCOL operation in SEC.
 */
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
#define SEC_RLC_INIT_SD(descriptor){ \
        /* CTYPE = shared job descriptor
         * RIF = 0
         * DNR = 0
         * ONE = 1
         * Start Index = 5, in order to jump over PDB
         * ZRO,CIF = 0
         * SC = 1
         * PD = 0, SHARE = WAIT
         * Descriptor Length = 10
         */                                                                  \
        (descriptor)->deschdr.command.word  = 0xB8851110;                    \
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
         * Protocol ID = 3G RLC PDU
         * Protocol Info = 0 (to be completed @ runtime)
         */                                                                  \
        (descriptor)->protocol.command.word = 0x80320000;                    \
}
#else
#define SEC_RLC_INIT_SD(descriptor){ \
        /* CTYPE = shared job descriptor
         * RIF = 0
         * DNR = 0
         * ONE = 1
         * Start Index = 5, in order to jump over PDB
         * ZRO,CIF = 0
         * SC = 1
         * PD = 0, SHARE = WAIT
         * Descriptor Length = 10
         */                                                                  \
        (descriptor)->deschdr.command.word  = 0xB885110E;                    \
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
         * Protocol ID = 3G RLC PDU
         * Protocol Info = 0 (to be completed @ runtime)
         */                                                                  \
        (descriptor)->protocol.command.word = 0x80320000;                    \
}
#endif // defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
/** Macro for setting the pointer and length of the key to be used for
 * encryption in a SD performing RLC encap/decap for control or data plane.
 */
#define SEC_RLC_SD_SET_KEY1(descriptor,key,len) {                           \
        (descriptor)->key1_cmd.command.field.length = (len);                \
        (descriptor)->key1_ptr = g_sec_vtop((key));                         \
}

/** Macro for setting the direction (encapsulation or decapsulation)
 * for descriptors implementing the PROTOCOL operation command for RLC.
 */
#define SEC_RLC_SD_SET_PROT_DIR(descriptor,dir)                             \
        ((descriptor)->protocol.command.field.optype =                      \
        ((dir) == RLC_ENCAPSULATION) ? CMD_PROTO_ENCAP : CMD_PROTO_DECAP)

/** Macro for setting the algorithm combination
 * for descriptors implementing the PROTOCOL operation command for RLC.
 */
#define SEC_RLC_SD_SET_PROT_ALG(descriptor,alg)                             \
        ((descriptor)->protocol.command.field.protinfo =                    \
        (alg == SEC_ALG_RLC_CRYPTO_KASUMI) ? CMD_PROTO_RLC_KASUMI_ALG :     \
        (alg == SEC_ALG_RLC_CRYPTO_SNOW) ? CMD_PROTO_RLC_SNOW_ALG : -1)

/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
/** SEC PDB structure, as defined for RLC processing */
struct rlc_pdb_s{
    uint32_t res_opt;
    uint32_t hfn_res;
    uint32_t bearer_dir_res;
    uint32_t hfn_thr_res;
} __packed;

/** Structure for aggregating the two types of
 * PDBs existing in PDCP Driver.
 */
typedef struct sec_rlc_pdb_s
{
    union {
        uint32_t    content[4];
        struct rlc_pdb_s rlc_pdb;
    }__packed pdb_content;
}__packed sec_rlc_pdb_t;

/** Structure for a RLC shared descriptor */
struct sec_rlc_sd_t{
    struct descriptor_header_s  deschdr;
    sec_rlc_pdb_t               pdb;
    struct key_command_s        key1_cmd;
    dma_addr_t                  key1_ptr;
    uint32_t                    hfn_ov_desc[7];
    struct protocol_operation_command_s protocol;
} __packed;
/*==============================================================================
                                 CONSTANTS
==============================================================================*/

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/** @brief Updates a SEC context with RLC specific cryptographic data
 * for a certain SEC context.
 *
 * @param [in,out] ctx          SEC context
 * @param [in]     crypto_pdb   RLC crypto descriptor and PDB
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
int sec_rlc_context_set_crypto_info(sec_context_t *ctx,
                                     const sec_rlc_context_info_t *crypto_pdb);

/*============================================================================*/


#endif  /* SEC_RLC_H */
