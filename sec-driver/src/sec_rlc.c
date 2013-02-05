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

#ifdef _cplusplus
extern "C" {
#endif

/*=================================================================================================
                                        INCLUDE FILES
==================================================================================================*/
#include "sec_rlc.h"
#include "sec_contexts.h"
#include "sec_job_ring.h"
#include "sec_hw_specific.h"
#include "sec_utils.h"


/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
/** Define for use in the array of descriptor creating functions. This is the number of
 * rows in the array
 */
#define NUM_RLC_CIPHER_ALGS sizeof(sec_rlc_crypto_alg_t)

#ifdef SEC_RRC_PROCESSING
/** Define for use in the array of descriptor creating functions. This is the number of
 * columns in the array
 */
#define NUM_RLC_INT_ALGS sizeof(sec_rlc_int_alg_t)
#endif // SEC_RRC_PROCESSING
/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/** Typedef for function pointer forcreating a shared descriptor on a context */
typedef int (*create_desc_fp)(sec_context_t *ctx);

/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
#ifdef SEC_RRC_PROCESSING
 /** Forward declaration of a static array of function pointers
  * indexed by the encryption algorithms and authentication
  * algorithms to be used for RRC or RLC.
  */
static create_desc_fp rrc_create_desc[][NUM_RLC_INT_ALGS];
#endif // SEC_RRC_PROCESSING

 /** Forward declaration of a static array of function pointers
  * indexed by the encryption algorithms to be used
  * for RLC.
  */
static create_desc_fp rlc_create_desc[NUM_RLC_CIPHER_ALGS];
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
/** @brief Fill PDB with initial data for this context: HFN threshold, HFN mask, etc
 * @param [in,out] ctx          SEC context
 */
static void sec_rlc_create_pdb(sec_context_t *ctx);


/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static void sec_rlc_create_pdb(sec_context_t *ctx)
{
    sec_rlc_pdb_t *sec_pdb;
    struct rlc_pdb_s *pdb_content;
    const sec_rlc_context_info_t *rlc_ctx_info;
    
    ASSERT( ctx != NULL );
    ASSERT( ctx->crypto_info.rlc_crypto_info != NULL);
    ASSERT( ctx->sh_desc != NULL);

    sec_pdb = &((struct sec_rlc_sd_t*)ctx->sh_desc)->pdb;
    memset(sec_pdb, 0x00, sizeof(sec_rlc_pdb_t));
    
    pdb_content = &sec_pdb->pdb_content.rlc_pdb;
    
    rlc_ctx_info = ctx->crypto_info.rlc_crypto_info;

    if(rlc_ctx_info->mode == RLC_ACKED_MODE)
    {
        /* AM mode for RLC */
        pdb_content->res_opt = SEC_RLC_PDB_OPT_SNS_AM;
        pdb_content->hfn_res = rlc_ctx_info->hfn << SEC_RLC_PDB_HFN_SHIFT_AM;
        pdb_content->bearer_dir_res = rlc_ctx_info->bearer << SEC_RLC_PDB_BEARER_SHIFT;
        pdb_content->bearer_dir_res |= rlc_ctx_info->packet_direction << SEC_RLC_PDB_DIR_SHIFT;
        pdb_content->hfn_thr_res = rlc_ctx_info->hfn_threshold << SEC_RLC_PDB_HFN_SHIFT_AM;
    }
    else
    {
        pdb_content->res_opt = SEC_RLC_PDB_OPT_SNS_UM;
        pdb_content->hfn_res = rlc_ctx_info->hfn << SEC_RLC_PDB_HFN_SHIFT_UM;
        pdb_content->bearer_dir_res = rlc_ctx_info->bearer << SEC_RLC_PDB_BEARER_SHIFT;
        pdb_content->bearer_dir_res |= rlc_ctx_info->packet_direction << SEC_RLC_PDB_DIR_SHIFT;
        pdb_content->hfn_thr_res = rlc_ctx_info->hfn_threshold << SEC_RLC_PDB_HFN_SHIFT_UM;
    }
}

static int create_hw_acc_rlc_desc(sec_context_t *ctx)
{
    const sec_rlc_context_info_t *rlc_ctx_info;
    struct sec_rlc_sd_t *desc;
    
    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);
    ASSERT(ctx->crypto_info.rlc_crypto_info != NULL);

    rlc_ctx_info = ctx->crypto_info.rlc_crypto_info;
    
    ASSERT(rlc_ctx_info->cipher_key != NULL);

    desc = (struct sec_rlc_sd_t*)ctx->sh_desc;
    
    SEC_INFO("Creating RLC HW Acc. descriptor w/alg %s",
              rlc_ctx_info->cipher_algorithm == SEC_ALG_RLC_CRYPTO_KASUMI ? 
              "KASUMI" : "SNOW");

    SEC_RLC_INIT_SD(desc);

    /* Plug-in the HFN override in descriptor from DPOVRD */
    desc->hfn_ov_desc[0] = 0xAC574F08;
    desc->hfn_ov_desc[1] = 0x80000000;
    desc->hfn_ov_desc[2] = 0xA0000407;
    desc->hfn_ov_desc[3] = 0xAC574008;
    desc->hfn_ov_desc[5] = 0xAC704008;
    
    /* I avoid to do complicated things in CAAM, thus I 'hardcode'
     * the operations to be done on the HFN as per the SN size. Doing
     * a generic descriptor that would look at the PDB and then decide
     * on the actual values to shift would have made the descriptors too
     * large and slow [ if-then-else construct in CAAM means 2 JUMPS ]
     */
    if(rlc_ctx_info->mode == RLC_ACKED_MODE)
    {
        desc->hfn_ov_desc[4] = 0x01FFFFFF;
        desc->hfn_ov_desc[6] = 0x00000007;
    }
    else
    {
        desc->hfn_ov_desc[4] = 0x000FFFFF;
        desc->hfn_ov_desc[6] = 0x0000000C;
    }
    desc->hfn_ov_desc[7] = 0xA8900008;
    desc->hfn_ov_desc[8] = 0x78430804;

    SEC_RLC_SD_SET_KEY1(desc,
                        rlc_ctx_info->cipher_key,
                        rlc_ctx_info->cipher_key_len);

    SEC_RLC_SD_SET_PROT_DIR(desc,
                            rlc_ctx_info->protocol_direction);

    SEC_RLC_SD_SET_PROT_ALG(desc,rlc_ctx_info->cipher_algorithm);

    SEC_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int create_rlc_null_desc(sec_context_t *ctx)
{
    int i = 0;
    SEC_INFO("Creating RLC descriptor with NULL alg.");

    *((uint32_t*)ctx->sh_desc + i++) = 0xBA800308;    // shared descriptor header -- desc len is 8; no sharing
    *((uint32_t*)ctx->sh_desc + i++) = 0xA80AFB04;    // Put SEQ-IN-Length into VSOL
    *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;    // SEQ FIFO STORE
    *((uint32_t*)ctx->sh_desc + i++) = 0xA82A4F04;    // MATH VSIL - imm -> No DEST (to set math size flags)
    *((uint32_t*)ctx->sh_desc + i++) = 0x00000FFF;    // immediate value with maximum permitted length of frame to copy.  I arbitrarily set this to 4095...this can go up to 65535.
    *((uint32_t*)ctx->sh_desc + i++) = 0xA0C108F1;    // HALT with status if the length of the input frame (as in VSIL) is bigger than the length in the immediate value
    *((uint32_t*)ctx->sh_desc + i++) = 0xA80AF004;    // MATH ADD VSIL + 0 -> MATH 0 (to put the length of the input frame into MATH 0)
    *((uint32_t*)ctx->sh_desc + i++) = 0x70820000;    // Move Length from Deco Alignment block to Output FIFO using length from MATH 0

    SEC_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int sec_pdcp_context_create_descriptor(sec_context_t *ctx)
{
    ASSERT(ctx != NULL);
    ASSERT(ctx->crypto_info.rlc_crypto_info != NULL);

    // Create a Protocol Data Blob (PDB)
    sec_rlc_create_pdb(ctx);

    switch(ctx->crypto_info.rlc_crypto_info->cipher_algorithm)
    {
        case SEC_ALG_RLC_CRYPTO_NULL:
            return create_rlc_null_desc(ctx);
            break;
        case SEC_ALG_RLC_CRYPTO_KASUMI:
        case SEC_ALG_RLC_CRYPTO_SNOW:
            return create_hw_acc_rlc_desc(ctx);
            break;
        default:
            SEC_ASSERT(0, SEC_INVALID_INPUT_PARAM,
                       "Unknown RLC encryption algorithm selected: %d\n",
                       ctx->crypto_info.rlc_crypto_info->cipher_algorithm);
            return SEC_INVALID_INPUT_PARAM;
    }
}
/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int sec_rlc_context_set_crypto_info(sec_context_t *ctx,
                                     const sec_rlc_context_info_t *crypto_info)
{
    int ret = SEC_SUCCESS;

    // store PDCP crypto info in context
    ctx->crypto_info.rlc_crypto_info = crypto_info;

    ret = sec_pdcp_context_create_descriptor(ctx);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create descriptor for "
               "PDCP context with bearer = %d", crypto_info->bearer);

    return SEC_SUCCESS;
}

#ifdef SEC_RRC_PROCESSING
/** Static array for selection of the function to be used for creating the shared descriptor on a SEC context,
 * depending on the algorithm selected by the user. This array is used for the following combinations:
 * o RRC integrity only
 * o RLC encryption & RRC integrity (two operations in one submit)
 * authentication and encryption is required.
 */
static create_desc_fp rrc_create_desc[NUM_RLC_CIPHER_ALGS][NUM_RLC_INT_ALGS] = {
        /*                      Kasumi                      Snow                       */
        /* NULL   */{   create_rrc_int_only_desc,    create_rrc_int_only_descriptor    },
        /* Kasumi */{   create_rrc_int_enc_desc,     create_rrc_int_enc_descriptor     },
        /* Snow   */{   create_rrc_int_enc_desc,     create_rrc_int_enc_descriptor     },
};
#endif // SEC_RRC_PROCESSING

/** Static array for selection of the function to be used for creating the shared descriptor on a SEC context,
 * depending on the algorithm selected by the user. This array is used for RLC encap/decap of
 * upper layer PDUs.
 */
static create_desc_fp rlc_create_desc[NUM_RLC_CIPHER_ALGS] =
    /*          NULL                    Kasumi                      Snow */
    {   create_rlc_null_desc,   create_hw_acc_rlc_desc,     create_hw_acc_rlc_desc    };

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
