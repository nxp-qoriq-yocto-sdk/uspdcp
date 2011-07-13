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
#include "sec_pdcp.h"
#include "sec_contexts.h"
#include "sec_job_ring.h"
#include "sec_hw_specific.h"
#include "sec_utils.h"

// For definition of sec_vtop and sec_ptov macros
#include "alu_mem_management.h"

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
/** First word of Descriptor Header that configure SEC 3.1 for SNOW F8 encryption */
#define SEC_PDCP_SNOW_F8_ENC_DESCRIPTOR_HEADER_HI   (SEC_DESC_HDR_EU_SEL0_STEU | SEC_DESC_HDR_MODE0_STEU_F8  | \
                                                     SEC_DESC_HDR_DESC_TYPE_STEU | SEC_DESC_HDR_DIR_OUTBOUND | \
                                                     SEC_DESC_HDR_DONE_NOTIFY)

/** First word of Descriptor Header that configure SEC 3.1 for SNOW F9 encapsulation (outbound integrity check) */
#define SEC_PDCP_SNOW_F9_ENC_DESCRIPTOR_HEADER_HI   (SEC_DESC_HDR_EU_SEL0_STEU | SEC_DESC_HDR_MODE0_STEU_F9 | \
                                                     SEC_DESC_HDR_DESC_TYPE_STEU | SEC_DESC_HDR_DIR_OUTBOUND| \
                                                     SEC_DESC_HDR_DONE_NOTIFY)

/** First word of Descriptor Header that configure SEC 3.1 for SNOW F8 decryption*/
#define SEC_PDCP_SNOW_F8_DEC_DESCRIPTOR_HEADER_HI   (SEC_DESC_HDR_EU_SEL0_STEU | SEC_DESC_HDR_MODE0_STEU_F8  | \
                                                     SEC_DESC_HDR_DESC_TYPE_STEU | SEC_DESC_HDR_DIR_INBOUND | \
                                                     SEC_DESC_HDR_DONE_NOTIFY)

/** First word of Descriptor Header that configure SEC 3.1 for SNOW F9 decapsulation (inbound integrity check) */
#define SEC_PDCP_SNOW_F9_DEC_DESCRIPTOR_HEADER_HI   (SEC_DESC_HDR_EU_SEL0_STEU | SEC_DESC_HDR_MODE0_STEU_F9_ICV | \
                                                     SEC_DESC_HDR_DESC_TYPE_STEU | SEC_DESC_HDR_DIR_INBOUND | \
                                                     SEC_DESC_HDR_DONE_NOTIFY)

/** First word of Descriptor Header that configure SEC 3.1 for AES CTR encryption*/
#define SEC_PDCP_AES_CTR_ENC_DESCRIPTOR_HEADER_HI   (SEC_DESC_HDR_EU_SEL0_AESU | SEC_DESC_HDR_MODE0_AESU_CTR  | \
                                                     SEC_DESC_HDR_DIR_OUTBOUND | SEC_DESC_HDR_DONE_NOTIFY)

/** First word of Descriptor Header that configure SEC 3.1 for AES CMAC encryption */
#define SEC_PDCP_AES_CMAC_ENC_DESCRIPTOR_HEADER_HI  (SEC_DESC_HDR_EU_SEL0_AESU | SEC_DESC_HDR_MODE0_AESU_CMAC | \
                                                     SEC_DESC_HDR_DESC_TYPE_AESU | SEC_DESC_HDR_DIR_OUTBOUND | \
                                                     SEC_DESC_HDR_DONE_NOTIFY)

/** First word of Descriptor Header that configure SEC 3.1 for AES CTR decryption*/
#define SEC_PDCP_AES_CTR_DEC_DESCRIPTOR_HEADER_HI   (SEC_DESC_HDR_EU_SEL0_AESU | SEC_DESC_HDR_MODE0_AESU_CTR  | \
                                                     SEC_DESC_HDR_DIR_INBOUND | SEC_DESC_HDR_DONE_NOTIFY)

/** First word of Descriptor Header that configure SEC 3.1 for AES CMAC decryption.
 *  Do not do ICV checking for AES CMAC on SEC 3.1 because it fails anyway.
 *  SEC 3.1 does not support ICV length of 4 bytes, as we have for PDCP c-plane.
 */
#define SEC_PDCP_AES_CMAC_DEC_DESCRIPTOR_HEADER_HI (SEC_DESC_HDR_EU_SEL0_AESU | SEC_DESC_HDR_MODE0_AESU_CMAC | \
                                                    SEC_DESC_HDR_DESC_TYPE_AESU | SEC_DESC_HDR_DIR_INBOUND | \
                                                    SEC_DESC_HDR_DONE_NOTIFY)

/** Length in bytes of IV(Initialization Vector) for SNOW F8.
 * Same length is used for IV(Initialization Vector actually representing ICV = Initial Counter Value)
 * in case of AES CTR(encrypt/decrypt).
 * 
 * For AES CTR, PDCP standard state an IV 128 bits long, having trailing 64 bits set to 0. 
 * First 64 bits constructed as for SNOW 3G F8 IV.
 * @note SEC 3.1 knows to append the trailing 64 bits of zero, 
 *       so just create an IV like for SNOW F8.
 * @note SEC 4.4 implements PDCP c-plane /d-plane protocol and 
 *       IV construction is not required in driver anymore!
 */
#define PDCP_SNOW_F8_AES_CTR_IV_LENGTH  8

/** Length in bytes of IV(Initialization Vector) for SNOW F9.
 * On P2020, 24 bytes are required even though the PDCP spec says 12 bytes. */
#define PDCP_SNOW_F9_IV_LENGTH          24

/** Length in bytes of IV(Initialization Vector) for AES CMAC. */
#define PDCP_AES_CMAC_CONTEXT_IN_LENGTH 48

/** Length in bytes for AES CMAC IV.
 * Note that AES CMAC does not use an IV in the same sense as AES CTR or SNOW F8/F9.
 * The 'IV' is prepended to the input frame and is considered part of the frame when 
 * running AES CMAC. 
 */
#define PDCP_AES_CMAC_IV_LENGTH         8

/** Position in IV/IV template where confidentiality IV starts.
 * Used for SNOW F8 and AES CTR. */
#define PDCP_CONFIDENTIALITY_IV_POS     0

/** Position in IV/IV template where integrity IV starts.
 * Used for SNOW F9 and AES CMAC. */
#define PDCP_INTEGRITY_IV_POS           2

/** Length in BITS for bearer field that is part of IV */
#define PDCP_BEARER_LENGTH      5

/** Length in bytes of PDCP header for control plane packets and
 * for data plane packets when short SN is used. */
#define PDCP_HEADER_LENGTH_SHORT    1

/** Length in bytes of PDCP header for data plane packets when long SN is used. */
#define PDCP_HEADER_LENGTH_LONG     2

/** Mask for extracting HFN from IV for control plane packets */
#define PDCP_HFN_MASK_CONTROL_PLANE         0xFFFFFFE0

/** Mask for extracting HFN from IV for data plane packets with short SN */
#define PDCP_HFN_MASK_DATA_PLANE_SHORT_SN   0xFFFFFF80

/** Mask for extracting HFN from IV for data plane packets with long SN */
#define PDCP_HFN_MASK_DATA_PLANE_LONG_SN    0xFFFFF000

/** Mask for extracting SN from PDCP header for control plane packets */
#define PDCP_SN_MASK_CONTROL_PLANE          0x1F00

/** Mask for extracting SN from PDCP header for data plane packets with short SN */
#define PDCP_SN_MASK_DATA_PLANE_SHORT_SN    0x7F00

/** Mask for extracting SN from PDCP header for data plane packets with long SN */
#define PDCP_SN_MASK_DATA_PLANE_LONG_SN     0xFFF

/** Mask for extracting bearer field from second IV word,
 * where most significant bits represent the bearer. */
#define PDCP_IV_WORD_BEARER_MASK            0xF8000000

/** Extract SN from PDCP header. For control plane packets and data plane packets 
 * with short SN, PDCP header length is 1 byte and sns field from PDB is set to 1.
 * For data plane packets with long SN, PDCP header length is 2 bytes and sns field 
 * from PDB is set to 0.
 * For all packet types will apply mask on first 2 bytes of packet and then shift right
 * 8 bits only when PDCP header length is 1 byte. */
#define PDCP_HEADER_GET_SN(hdr, pdb)(((*(uint16_t*)(hdr)) & (pdb)->sn_mask) >> ((pdb)->sns * 8) )

/** Extract D/C bit from PDCP header.
 * @note Applies only to data plane packets!*/
#define PDCP_HEADER_GET_D_C(hdr)((hdr)[0] >> 7)


/** Used only for testing! Simulates that the first 2 packets
 *  submitted to SEC have an invalid descriptor header.
 *  This will generate an IDH error. */
//#define SEC_SIMULATE_PACKET_PROCESSING_ERROR

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
/** @brief Create a SNOW F8 (ciphering/deciphering) descriptor or
 * AES CTR (ciphering/deciphering).
 *
 * @param [in,out] ctx          SEC contex
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_create_snow_f8_aes_ctr_descriptor(sec_context_t *ctx);
/** @brief Create a SNOW F9 (authentication) descriptor.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_create_snow_f9_descriptor(sec_context_t *ctx);
/** @brief Create a AES CMAC (authentication) descriptor.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_create_aes_cmac_descriptor(sec_context_t *ctx);

/** @brief  Creates SEC descriptor/s for the algorithm
 * configured for this context.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_create_descriptor(sec_context_t *ctx);

/** @brief  Creates SNOW F8 SEC descriptor as configured for this context.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_create_snow_cipher_descriptor(sec_context_t *ctx);

/** @brief  Creates NULL ciphering descriptor as configured for this context.
 * On PSC9132, will be used for PDCP data plane with NULL ciphering activated.
 * On P2020 same functionality is obtained with memcpy.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_create_null_cipher_descriptor(sec_context_t *ctx);

/** @brief  Creates AES CTR SEC descriptor as configured for this context.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_create_aes_cipher_descriptor(sec_context_t *ctx);

/** @brief  Creates SNOW F9 SEC descriptor as configured for this context.
 * SNOW F9 descriptor is created only for PDCP control plane contexts.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_create_snow_auth_descriptor(sec_context_t *ctx);

/** @brief  Creates AES CMAC SEC descriptor as configured for this context.
 * AES CMAC descriptor is created only for PDCP control plane contexts.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_create_aes_auth_descriptor(sec_context_t *ctx);

/** @brief  Creates NULL authentication descriptor as configured for this context.
 * On PSC9132, will be used for PDCP control plane with NULL ciphering activated.
 * On P2020 same functionality is obtained with memcpy.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_create_null_auth_descriptor(sec_context_t *ctx);

/** @brief  Updates a SEC descriptor for processing a PDCP packet
 * with SNOW F8 or AES CTR. Both algorithms use an identical way 
 * of configuring the descriptor. Set in descriptor pointers to:
 * input data, output data, ciphering information, etc.
 *
 * @param [in,out] job          SEC job
 * @param [in,out] descriptor   SEC descriptor
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_update_snow_f8_aes_ctr_descriptor(sec_job_t *job,
                                                              sec_descriptor_t *descriptor);

/** @brief  Updates a SEC descriptor for processing a PDCP packet
 * with SNOW F9
 * Set in descriptor pointers to:
 * input data, output data, ciphering information, etc.
 *
 * @param [in,out] job          SEC job
 * @param [in,out] descriptor   SEC descriptor
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_update_snow_f9_descriptor(sec_job_t *job, sec_descriptor_t *descriptor);

/** @brief  Updates a SEC descriptor for processing a PDCP packet
 * with AES CMAC
 * Set in descriptor pointers to:
 * input data, output data, ciphering information, etc.
 *
 * @param [in,out] job          SEC job
 * @param [in,out] descriptor   SEC descriptor
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_update_aes_cmac_descriptor(sec_job_t *job, sec_descriptor_t *descriptor);

/** @brief  Updates a SEC descriptor for processing a PDCP packet
 * with NULL authentication protocol (EIA0).
 * @note Not used on P2020 because EIA0/EEA0 is simulated with memcpy. Will be used on PSC9132.
 * Set in descriptor pointers to:
 * input data, output data, ciphering information, etc.
 *
 * @param [in,out] job          SEC job
 * @param [in,out] descriptor   SEC descriptor
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_update_null_auth_descriptor(sec_job_t *job, sec_descriptor_t *descriptor);

/** @brief  Updates a SEC descriptor for processing a PDCP packet
 * with NULL ciphering protocol (EEA0).
 * @note Not used on P2020 because EIA0/EEA0 is simulated with memcpy. Will be used on PSC9132.
 * Set in descriptor pointers to:
 * input data, output data, ciphering information, etc.
 *
 * @param [in,out] job          SEC job
 * @param [in,out] descriptor   SEC descriptor
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_pdcp_context_update_null_cipher_descriptor(sec_job_t *job, sec_descriptor_t *descriptor);

/** @brief  Updates the template Initialization Vector required by SEC to
 * run SNOW F8 / SNOW F9 / AES CTR algorithms for a PDCP packet.
 *
 * IV is updated with:
 *  - SN(seq no) extracted from PDCP header
 *  - HFN is incremented if SN rolls over
 *
 * If HFN reached the threshold configured for the SEC context this PDB belongs to,
 * once processed, the packet will be notified to User App with a status of
 * #SEC_STATUS_HFN_THRESHOLD_REACHED.
 * It is not error, but an indication to User App that crypto keys must be renegotiated.
 *
 * @param [in,out] sec_pdb      SEC protocol data for a context
 * @param [in]     pdcp_header  PDCP header
 * @param [in,out] status       SEC job status to be notified to User Application.
 *                              Can be #SEC_STATUS_SUCCESS or #SEC_STATUS_HFN_THRESHOLD_REACHED.
 * @param [in]     iv_offset    Offset from IV/IV template where the current IV to be updated starts.
 *                              IV and IV template store: confidentiality IV , integrity IV.
 */
static void sec_pdcp_update_iv_template(sec_crypto_pdb_t *sec_pdb,
                                        uint8_t *pdcp_header,
                                        sec_status_t *status,
                                        uint8_t iv_offset);

/** @brief Fill PDB with initial data for this context: HFN threshold, HFN mask, etc
 * @param [in,out] ctx          SEC context
 */
static void sec_pdcp_create_pdb(sec_context_t *ctx);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

static int sec_pdcp_create_snow_f8_aes_ctr_descriptor(sec_context_t *ctx)
{
    sec_crypto_pdb_t *sec_pdb = &ctx->crypto_desc_pdb;

    // Copy crypto data into PDB
    ASSERT(ctx->pdcp_crypto_info->cipher_key != NULL);


    // IV Template format:
    // word 0 : F8 IV[0]
    // word 1 : F8 IV[1]
    // word 2 : F9 IV[0]
    // word 3 : F9 IV[1]
    // word 4 : F9 IV[2]

    // Construct (template) IV (64 bits) =  HFN | SN | Bearer | DIRECTION | 26 bits of 0
    // control plane:           HFN 27 bits, SN 5 bit
    // data plane, short SN:    HFN 25 bits, SN 7 bit
    // data plane, long SN:     HFN 20 bits, SN 12 bit

    
    // Set HFN.
    // SN will be updated for each packet. When SN rolls over, HFN is incremented.
    sec_pdb->iv_template[PDCP_CONFIDENTIALITY_IV_POS] = ctx->pdcp_crypto_info->hfn << ctx->pdcp_crypto_info->sn_size;

    // Set bearer
    sec_pdb->iv_template[PDCP_CONFIDENTIALITY_IV_POS + 1] = ctx->pdcp_crypto_info->bearer << (32 - PDCP_BEARER_LENGTH);

    // Set DIRECTION bit: 0 for uplink, 1 for downlink
    sec_pdb->iv_template[PDCP_CONFIDENTIALITY_IV_POS + 1] = sec_pdb->iv_template[PDCP_CONFIDENTIALITY_IV_POS + 1] | 
        (ctx->pdcp_crypto_info->packet_direction << (32 - PDCP_BEARER_LENGTH - 1));

    return SEC_SUCCESS;
}

static int sec_pdcp_create_snow_f9_descriptor(sec_context_t *ctx)
{
    sec_crypto_pdb_t *sec_pdb = &ctx->crypto_desc_pdb;
    
    // Copy crypto data into PDB.
    // Copy/update only data that was not already set by data-plane corresponding function!

    ASSERT(ctx->pdcp_crypto_info->integrity_key != NULL);
    ASSERT(ctx->pdcp_crypto_info->sn_size == SEC_PDCP_SN_SIZE_5); 

    // IV Template format:
    // word 0 : F8 IV[0]
    // word 1 : F8 IV[1]
    // word 2 : F9 IV[0]
    // word 3 : F9 IV[1]
    // word 4 : F9 IV[2]

    // Construct (template) F9 IV (96 bits) =  
    //      HFN | SN | 5 bits of 0 | DIRECTION | 26 bits of 0 | Bearer | 27 bits of 0
    //
    // control plane:           HFN 27 bits, SN 5 bit
    

    // First word of F9 IV is identical with first word of F8 IV : HFN | SN
    // SN will be updated for each packet. When SN rolls over, HFN is incremented.
    sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS] = ctx->pdcp_crypto_info->hfn << ctx->pdcp_crypto_info->sn_size;

    // Set DIRECTION bit: 0 for uplink, 1 for downlink
    sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS + 1] = ctx->pdcp_crypto_info->packet_direction << 26;

    // Set bearer
    sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS + 2] = ctx->pdcp_crypto_info->bearer << (32 - PDCP_BEARER_LENGTH);

    return SEC_SUCCESS;
}

static int sec_pdcp_create_aes_cmac_descriptor(sec_context_t *ctx)
{
    sec_crypto_pdb_t *sec_pdb = &ctx->crypto_desc_pdb;

    // Copy crypto data into PDB.
    // Copy/update only data that was not already set by data-plane corresponding function!

    ASSERT(ctx->pdcp_crypto_info->integrity_key != NULL);
    ASSERT(ctx->pdcp_crypto_info->sn_size == SEC_PDCP_SN_SIZE_5);

    // IV Template format:
    // word 0 : AES CTR  IV[0]
    // word 1 : AES CTR  IV[1]
    // word 2 : AES CMAC IV[0]
    // word 3 : AES CMAC IV[1]
    // word 4 : AES CMAC IV[2]

    // Construct (template) F9 IV (96 bits) =  
    //      HFN | SN | Bearer | DIRECTION | 26 bits of 0
    //
    // control plane:           HFN 27 bits, SN 5 bit
    

    // AES CMAC 'IV' is identical with AES CTR IV
    // SN will be updated for each packet. When SN rolls over, HFN is incremented.
    sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS] = ctx->pdcp_crypto_info->hfn << ctx->pdcp_crypto_info->sn_size;

    // Set bearer
    sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS + 1] = ctx->pdcp_crypto_info->bearer << (32 - PDCP_BEARER_LENGTH);
    
    // Set DIRECTION bit: 0 for uplink, 1 for downlink
    sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS + 1] = sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS + 1] | 
        (ctx->pdcp_crypto_info->packet_direction << (32 - PDCP_BEARER_LENGTH - 1));
    
    return SEC_SUCCESS;
}

static int sec_pdcp_context_create_snow_cipher_descriptor(sec_context_t *ctx)
{
    int ret = SEC_SUCCESS;

    // PDCP data-plane descriptor is always needed
    ret = sec_pdcp_create_snow_f8_aes_ctr_descriptor(ctx);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create SNOW F8 descriptor");
    
    // Set pointer to the function that will be called to 
    // update the SEC descriptor for each packet.
    // SNOW F8 is first processing step for PDCP data plane.
    // For control plane, SNOW F8 is second processing step after SNOW F9.
    ctx->update_crypto_descriptor = sec_pdcp_context_update_snow_f8_aes_ctr_descriptor;

    // Configure SEC descriptor for ciphering/deciphering.
    if (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
    {
        SEC_INFO("Creating SNOW F8 Encapsulation descriptor");
        ctx->crypto_desc_pdb.crypto_hdr = SEC_PDCP_SNOW_F8_ENC_DESCRIPTOR_HEADER_HI;
    }
    else
    {
        SEC_INFO("Creating SNOW F8 Decapsulation descriptor");
        // Create descriptor for decrypt
        ctx->crypto_desc_pdb.crypto_hdr = SEC_PDCP_SNOW_F8_DEC_DESCRIPTOR_HEADER_HI;
    }
    return SEC_SUCCESS;
}

static int sec_pdcp_context_create_aes_cipher_descriptor(sec_context_t *ctx)
{
    int ret = SEC_SUCCESS;

    // PDCP data-plane descriptor is always needed
    ret = sec_pdcp_create_snow_f8_aes_ctr_descriptor(ctx);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create AES CTR descriptor");

    // Set pointer to the function that will be called to 
    // update the SEC descriptor for each packet.
    // AES CTR is first processing step for PDCP data plane.
    // For control plane, AES CTR is second processing step after AES CMAC.
    ctx->update_crypto_descriptor = sec_pdcp_context_update_snow_f8_aes_ctr_descriptor;


    // Configure SEC descriptor for ciphering/deciphering.
    // Required for both c-plane and d-plane.
    if (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
    {
        SEC_INFO("Creating AES CTR Encapsulation descriptor");
        ctx->crypto_desc_pdb.crypto_hdr = SEC_PDCP_AES_CTR_ENC_DESCRIPTOR_HEADER_HI;
    }
    else
    {
        SEC_INFO("Creating AES CTR Decapsulation descriptor");
        // Create descriptor for decrypt
        ctx->crypto_desc_pdb.crypto_hdr = SEC_PDCP_AES_CTR_DEC_DESCRIPTOR_HEADER_HI;
    }
    return SEC_SUCCESS;
}

static int sec_pdcp_context_create_null_cipher_descriptor(sec_context_t *ctx)
{
    // Set pointer to the function that will be called to
    // update the SEC descriptor for each packet.
    ctx->update_crypto_descriptor = sec_pdcp_context_update_null_cipher_descriptor;

    return SEC_SUCCESS;
}
static int sec_pdcp_context_create_snow_auth_descriptor(sec_context_t *ctx)
{
    int ret = SEC_SUCCESS;

    // First create snow f8 descriptor!
    // SNOW F9 descriptor is created based on info already stored
    // for data-plane descriptor!
    ret = sec_pdcp_create_snow_f9_descriptor(ctx);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create SNOW F9 descriptor");

    // Set pointer to the function that will be called to
    // update the SEC descriptor for each packet.
    ctx->update_auth_descriptor = sec_pdcp_context_update_snow_f9_descriptor;

    // Create descriptor for encrypt
    if (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
    {
        SEC_INFO("Creating SNOW F9 Encapsulation descriptor");
        ctx->crypto_desc_pdb.auth_hdr = SEC_PDCP_SNOW_F9_ENC_DESCRIPTOR_HEADER_HI;
    }
    else
    {
        SEC_INFO("Creating SNOW F9 Decapsulation descriptor");
        // Create descriptor for decrypt
        ctx->crypto_desc_pdb.auth_hdr = SEC_PDCP_SNOW_F9_DEC_DESCRIPTOR_HEADER_HI;
    }
    return SEC_SUCCESS;
}

static int sec_pdcp_context_create_aes_auth_descriptor(sec_context_t *ctx)
{
    int ret = SEC_SUCCESS;

    // First create AES CTR descriptor!
    // AES CMAC descriptor is created based on info already stored
    // for data-plane descriptor!
    ret = sec_pdcp_create_aes_cmac_descriptor(ctx);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create AES CMAC descriptor");

    // Set pointer to the function that will be called to 
    // update the SEC descriptor for each packet.
    ctx->update_auth_descriptor = sec_pdcp_context_update_aes_cmac_descriptor;

    // Create descriptor for encrypt
    if (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
    {
        SEC_INFO("Creating AES CMAC Encapsulation descriptor");
        ctx->crypto_desc_pdb.auth_hdr = SEC_PDCP_AES_CMAC_ENC_DESCRIPTOR_HEADER_HI;
    }
    else
    {
        SEC_INFO("Creating AES CMAC Decapsulation descriptor");
        // Create descriptor for decrypt
        ctx->crypto_desc_pdb.auth_hdr = SEC_PDCP_AES_CMAC_DEC_DESCRIPTOR_HEADER_HI;
    }

    return SEC_SUCCESS;
}

static int sec_pdcp_context_create_null_auth_descriptor(sec_context_t *ctx)
{
    // Set pointer to the function that will be called to 
    // update the SEC descriptor for each packet.
    ctx->update_crypto_descriptor = sec_pdcp_context_update_null_auth_descriptor;

    return SEC_SUCCESS;
}

static int sec_pdcp_context_create_descriptor(sec_context_t *ctx)
{
    int ret = SEC_SUCCESS;

    // Create a Protocol Data Blob (PDB)
    sec_pdcp_create_pdb(ctx);

    // Create ciphering descriptor.
    // Needed for both data plane and control plane
    if (ctx->pdcp_crypto_info->cipher_algorithm == SEC_ALG_SNOW)
    {
        ret = sec_pdcp_context_create_snow_cipher_descriptor(ctx);
    }
    else if (ctx->pdcp_crypto_info->cipher_algorithm == SEC_ALG_AES)
    {
        ret = sec_pdcp_context_create_aes_cipher_descriptor(ctx);
    }
    else if(ctx->pdcp_crypto_info->cipher_algorithm == SEC_ALG_NULL)
    {
        ret = sec_pdcp_context_create_null_cipher_descriptor(ctx);
    }

    // If control plane context, configure authentication descriptor
    if (ctx->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE)
    {
        if (ctx->pdcp_crypto_info->integrity_algorithm == SEC_ALG_SNOW)
        {
            ret = sec_pdcp_context_create_snow_auth_descriptor(ctx);
        }
        else if (ctx->pdcp_crypto_info->integrity_algorithm == SEC_ALG_AES)
        {
            ret = sec_pdcp_context_create_aes_auth_descriptor(ctx);
        }
        else if(ctx->pdcp_crypto_info->integrity_algorithm == SEC_ALG_NULL)
        {
            ret = sec_pdcp_context_create_null_auth_descriptor(ctx);
        }
    }
    return ret;
}

static int sec_pdcp_context_update_snow_f8_aes_ctr_descriptor(sec_job_t *job, sec_descriptor_t *descriptor)
{
    int ret = SEC_SUCCESS;
    sec_crypto_pdb_t *sec_pdb = &job->sec_context->crypto_desc_pdb;
    const sec_pdcp_context_info_t *ua_crypto_info = job->sec_context->pdcp_crypto_info;
    dma_addr_t phys_addr = 0;

#ifdef SEC_SIMULATE_PACKET_PROCESSING_ERROR
    static int counter = 0;

    // Configure SEC descriptor header for crypto operation
    if(counter < 2)
    {
        descriptor->hdr = 0;
    }
    else
    {
#endif
        descriptor->hdr = sec_pdb->crypto_hdr;
#ifdef SEC_SIMULATE_PACKET_PROCESSING_ERROR
    }
    counter++;
#endif


    //descriptor->hdr_lo = 0;

    // update the remaining 7 dwords

    //////////////////////////////////////////////////////////////
    // first dword is unused(value 0)
    //////////////////////////////////////////////////////////////

    //memset(&(descriptor->ptr[0]), 0, sizeof(struct sec_ptr));

    //////////////////////////////////////////////////////////////
    // set IV(Initialization Vector)
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(descriptor->iv);
    descriptor->ptr[1].len = PDCP_SNOW_F8_AES_CTR_IV_LENGTH;
    // no s/g
    descriptor->ptr[1].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[1].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // update IV based on SN
    sec_pdcp_update_iv_template(sec_pdb,
                                job->in_packet->address + job->in_packet->offset, // PDCP header pointer
                                &job->job_status,
                                PDCP_CONFIDENTIALITY_IV_POS);
    
    descriptor->iv[0] = sec_pdb->iv_template[PDCP_CONFIDENTIALITY_IV_POS];
    descriptor->iv[1] = sec_pdb->iv_template[PDCP_CONFIDENTIALITY_IV_POS + 1];

    // set pointer to IV
    descriptor->ptr[1].ptr = PHYS_ADDR_LO(phys_addr);
    
    SEC_DEBUG("IV[0] = 0x%x, IV[1] = 0x%x", descriptor->iv[0], descriptor->iv[1]);

    //////////////////////////////////////////////////////////////
    // set cipher key
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(ua_crypto_info->cipher_key);
    descriptor->ptr[2].len = ua_crypto_info->cipher_key_len;

    // no s/g
    descriptor->ptr[2].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[2].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to cipher key
    descriptor->ptr[2].ptr = PHYS_ADDR_LO(phys_addr);
    
    SEC_DEBUG("Cipher key (hex)= [%x %x %x %x]",
              *(uint32_t*)(ua_crypto_info->cipher_key),
              *((uint32_t*)(ua_crypto_info->cipher_key) + 1),
              *((uint32_t*)(ua_crypto_info->cipher_key) + 2),
              *((uint32_t*)(ua_crypto_info->cipher_key) + 3));

    //////////////////////////////////////////////////////////////
    // set input buffer
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(job->in_packet->address) + job->in_packet->offset + sec_pdb->pdcp_hdr_len;
    descriptor->ptr[3].len = job->in_packet->length - job->in_packet->offset - sec_pdb->pdcp_hdr_len;
    // no s/g
    descriptor->ptr[3].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[3].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to input buffer. Skip UA offset and PDCP header
    descriptor->ptr[3].ptr = PHYS_ADDR_LO(phys_addr);

    SEC_DEBUG("PDCP IN packet (len = %d bytes) (hex) = [%x %x %x %x]",
              descriptor->ptr[3].len,
              *((uint32_t*)(job->in_packet->address + job->in_packet->offset)),
              *((uint32_t*)(job->in_packet->address+job->in_packet->offset) + 1),
              *((uint32_t*)(job->in_packet->address+job->in_packet->offset)+ 2),
              *((uint32_t*)(job->in_packet->address+job->in_packet->offset) + 3));

    //////////////////////////////////////////////////////////////
    // copy PDCP header from input packet into output packet
    //////////////////////////////////////////////////////////////

    memcpy(job->out_packet->address + job->out_packet->offset, 
           job->in_packet->address + job->in_packet->offset,
           sec_pdb->pdcp_hdr_len);

    //////////////////////////////////////////////////////////////
    // set output buffer
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(job->out_packet->address) + job->out_packet->offset + sec_pdb->pdcp_hdr_len;
    descriptor->ptr[4].len = job->out_packet->length - job->out_packet->offset - sec_pdb->pdcp_hdr_len;
    // no s/g
    descriptor->ptr[4].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[4].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to output buffer. Skip UA offset and PDCP header
    descriptor->ptr[4].ptr = PHYS_ADDR_LO(phys_addr);

    
    //////////////////////////////////////////////////////////////
    // last 2 pointers unused
    //////////////////////////////////////////////////////////////

    //memset(&(descriptor->ptr[5]), 0, 2* sizeof(struct sec_ptr));

    return ret;
}

static int sec_pdcp_context_update_snow_f9_descriptor(sec_job_t *job, sec_descriptor_t *descriptor)
{
    int ret = SEC_SUCCESS;
    sec_crypto_pdb_t *sec_pdb = &job->sec_context->crypto_desc_pdb;
    const sec_pdcp_context_info_t *ua_crypto_info = job->sec_context->pdcp_crypto_info;
    dma_addr_t phys_addr = 0;

    // Depending on direction: encapsulation or decapsulation,
    // figure out where is stored the data that must be authenticated:
    // in input packet or in output packet.
    const sec_packet_t *auth_packet = (job->sec_context->double_pass == TRUE &&
                                       sec_pdb->is_inbound) ? job->out_packet : job->in_packet;

    // Configure SEC descriptor header for integrity check operation
    descriptor->hdr = sec_pdb->auth_hdr;
    //descriptor->hdr_lo = 0;

    // update the remaining 7 dwords

    //////////////////////////////////////////////////////////////
    // first dword is unused(value 0)
    //////////////////////////////////////////////////////////////

    //memset(&(descriptor->ptr[0]), 0, sizeof(struct sec_ptr));

    //////////////////////////////////////////////////////////////
    // set IV(Initialization Vector)
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(descriptor->iv);
    descriptor->ptr[1].len = PDCP_SNOW_F9_IV_LENGTH;
    // no s/g
    descriptor->ptr[1].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[1].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // update IV based on SN
    sec_pdcp_update_iv_template(sec_pdb,
                                auth_packet->address + auth_packet->offset, // PDCP header pointer
                                &job->job_status,
                                PDCP_INTEGRITY_IV_POS);

    descriptor->iv[0] = sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS];
    descriptor->iv[1] = sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS + 1];
    descriptor->iv[2] = 0;

    // For C-plane inbound, set in IV MAC-I extracted from last 4 bytes of output packet.
    // outbound:
    //      - input packet contains plain-text data + plain-text MAC-I
    //      - output packet contains encrypted data + encrypted MAC-I
    // inbound:
    //      - input packet contains encrypted data + encrypted MAC-I
    //      - output packet contains decrypted data + decrypted MAC-I
    // On inbound, SEC engine to check against generated MAC-I.
    descriptor->iv[3] = sec_pdb->is_inbound ? (*(uint32_t*)(auth_packet->address +
                                                            auth_packet->length -
                                                            PDCP_MAC_I_LENGTH))
                                            : 0;
    descriptor->iv[4] = 0; 
    descriptor->iv[5] = sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS + 2];

#if defined(PDCP_TEST_SNOW_F9_ONLY)
    // For SNOW F9 outbound test vector! Required when testing only SNOW F9.
    // We have test vectors only  for SNOW F9, not  for PDCP control plane...which
    // constructs the IV using bearer.
    // SNOW F9 spec constructs  IV using FRESH field, which is  configured below.
    // FRESH field in 3G, replaced in LTE to be BEARER | 27 bits of zero!
    descriptor->iv[5] = 0x397E8FD; 
#endif

    SEC_DEBUG("\nIV[0] = 0x%x\n IV[1] = 0x%x\n IV[2] = 0x%x\n"
              "IV[3] = 0x%x\n IV[4] = 0x%x\n IV[5] = 0x%x",
              descriptor->iv[0], descriptor->iv[1],
              descriptor->iv[2], descriptor->iv[3],
              descriptor->iv[4], descriptor->iv[5]);

    // set pointer to IV
    descriptor->ptr[1].ptr = PHYS_ADDR_LO(phys_addr);

    //////////////////////////////////////////////////////////////
    // set auth key
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(ua_crypto_info->integrity_key);

    descriptor->ptr[2].len = ua_crypto_info->integrity_key_len;
    // no s/g
    descriptor->ptr[2].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[2].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to auth key
    descriptor->ptr[2].ptr = PHYS_ADDR_LO(phys_addr);

    SEC_DEBUG("Auth key (hex)= [%x %x %x %x]",
              *(uint32_t*)(ua_crypto_info->integrity_key),
              *((uint32_t*)(ua_crypto_info->integrity_key) + 1),
              *((uint32_t*)(ua_crypto_info->integrity_key) + 2),
              *((uint32_t*)(ua_crypto_info->integrity_key) + 3));

    //////////////////////////////////////////////////////////////
    // set input buffer.
    //////////////////////////////////////////////////////////////

    // Integrity check is performed on both PDCP header + PDCP payload
    // inbound: do F9 on out packet = decrypted packet
    // outbound: do F9 on in packet = plain text packet
    phys_addr = sec_vtop(auth_packet->address) + auth_packet->offset;

    // The input and output packets must have a length that allows the driver to
    // add as trailer 4 bytes for generated MAC-I, besides offset + PDCP header + PDCP payload.
    // However, in case of single-pass c-plane packets - where only authentication is done,
    // on the outbound direction, the authenticated packet(input packet) already has the
    // length adjusted, no need to modify it.
    descriptor->ptr[3].len = auth_packet->length - auth_packet->offset - PDCP_MAC_I_LENGTH;
    if(job->sec_context->double_pass == FALSE && !sec_pdb->is_inbound)
    {
        descriptor->ptr[3].len += PDCP_MAC_I_LENGTH;
    }

    // no s/g
    descriptor->ptr[3].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[3].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to input buffer. Skip UA offset and PDCP header
    descriptor->ptr[3].ptr = PHYS_ADDR_LO(phys_addr);
    
    SEC_DEBUG("PDCP IN packet (len = %d bytes) (hex) = [%x %x %x %x]",
              descriptor->ptr[3].len,
              *((uint32_t*)(auth_packet->address + auth_packet->offset)),
              *((uint32_t*)(auth_packet->address + auth_packet->offset) + 1),
              *((uint32_t*)(auth_packet->address + auth_packet->offset)+ 2),
              *((uint32_t*)(auth_packet->address + auth_packet->offset) + 3));
    //////////////////////////////////////////////////////////////
    // next 2 pointers unused
    //////////////////////////////////////////////////////////////

    memset(&(descriptor->ptr[4]), 0, 2 * sizeof(struct sec_ptr));

    //////////////////////////////////////////////////////////////
    // set output buffer
    //////////////////////////////////////////////////////////////
    
    // MAC-I needs be generated only for outbound packets.
    // For inbound packets generated MAC-I needs to be checked against
    // MAC-I from input packet. For SNOW F9, SEC 3.1 knows to do that automatically.

    // TODO: seems that double-pass c-plane with SNOW F9 does not work unless
    // the mac-i generated code is written as output by SEC engine.
    // To be investigated at a later time.
    // However this is not a problem, since the mac_i code can be generated and
    // written by SEC engine, as SEC engine does the MAC-I validation in hw.
/*
#ifndef PDCP_TEST_SNOW_F9_ONLY
    if(sec_pdb->is_inbound)
    {
        memset(&(descriptor->ptr[6]), 0, 2 * sizeof(struct sec_ptr));
    }
    else
#endif
        */
    {
        // Generated MAC-I (4 byte).
        phys_addr = sec_vtop(job->mac_i);
        // first 4 bytes are filled with zero-s. last 4 bytes contain the MAC-I
        descriptor->ptr[6].len = 2 * PDCP_MAC_I_LENGTH; 
        // no s/g
        descriptor->ptr[6].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
        // Update for 36bit physical address
        descriptor->ptr[6].eptr = PHYS_ADDR_HI(phys_addr);
#endif
        // pointer to output buffer. Skip UA offset and PDCP header
        descriptor->ptr[6].ptr = PHYS_ADDR_LO(phys_addr);

    }
    return ret;
}

static int sec_pdcp_context_update_aes_cmac_descriptor(sec_job_t *job, sec_descriptor_t *descriptor)
{
    int ret = SEC_SUCCESS;
    sec_crypto_pdb_t *sec_pdb = &job->sec_context->crypto_desc_pdb;
    const sec_pdcp_context_info_t *ua_crypto_info = job->sec_context->pdcp_crypto_info;
    dma_addr_t phys_addr = 0;

    // Depending on direction: encapsulation or decapsulation,
    // figure out where is stored the data that must be authenticated:
    // in input packet or in output packet.
    const sec_packet_t *auth_packet = (job->sec_context->double_pass == TRUE &&
                                       sec_pdb->is_inbound) ? job->out_packet : job->in_packet;

    // Configure SEC descriptor header for integrity check operation
    descriptor->hdr = sec_pdb->auth_hdr;
    //descriptor->hdr_lo = 0;

    // update the remaining 7 dwords

    //////////////////////////////////////////////////////////////
    // first dword is unused(value 0)
    //////////////////////////////////////////////////////////////

    //memset(&(descriptor->ptr[0]), 0, sizeof(struct sec_ptr));

    //////////////////////////////////////////////////////////////
    // set IV(Initialization Vector)
    //////////////////////////////////////////////////////////////
    
    memset(&(descriptor->ptr[1]), 0, sizeof(struct sec_ptr));

    // update IV based on SN
    sec_pdcp_update_iv_template(sec_pdb,
                                auth_packet->address + auth_packet->offset, // PDCP header pointer
                                &job->job_status,
                                PDCP_INTEGRITY_IV_POS);

    // No need to configure MAC-I received for inbound packets, because SEC 3.1
    // cannot do MAC-I automatic validation as it does not support MAC-I codes less than 8 bytes.
    
    //////////////////////////////////////////////////////////////
    // set auth key
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(ua_crypto_info->integrity_key);

    descriptor->ptr[2].len = ua_crypto_info->integrity_key_len;
    // no s/g
    descriptor->ptr[2].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[2].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to auth key
    descriptor->ptr[2].ptr = PHYS_ADDR_LO(phys_addr);

    SEC_DEBUG("Auth key (hex) = [%x %x %x %x]",
              *(uint32_t*)(ua_crypto_info->integrity_key),
              *((uint32_t*)(ua_crypto_info->integrity_key) + 1),
              *((uint32_t*)(ua_crypto_info->integrity_key) + 2),
              *((uint32_t*)(ua_crypto_info->integrity_key) + 3));

    //////////////////////////////////////////////////////////////
    // set input buffer.
    //////////////////////////////////////////////////////////////

    // For AES CMAC, the initialization vector is prepended to
    // the input packet, before the PDCP header.
    // 8 bytes of scratch-pad area MUST be provided by the User Application,
    // before the PDCP header.

    // configure first word of initialisation vector
    uint32_t* temp = (uint32_t*)(auth_packet->address + auth_packet->offset - PDCP_AES_CMAC_IV_LENGTH);
    *temp = sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS];

    // configure second word of initialisation vector
    temp = (uint32_t*)(auth_packet->address + auth_packet->offset - PDCP_AES_CMAC_IV_LENGTH / 2);
    *temp = sec_pdb->iv_template[PDCP_INTEGRITY_IV_POS + 1];

    // Integrity check is performed on both PDCP header + PDCP payload
    phys_addr = sec_vtop(auth_packet->address) + auth_packet->offset - PDCP_AES_CMAC_IV_LENGTH;

    descriptor->ptr[3].len = auth_packet->length - auth_packet->offset -
                             PDCP_MAC_I_LENGTH + PDCP_AES_CMAC_IV_LENGTH;

    // The input and output packets must have a length that allows the driver to
    // add as trailer 4 bytes for generated MAC-I, besides offset + PDCP header + PDCP payload.
    // However, in case of single-pass c-plane packets - where only authentication is done,
    // on the outbound direction, the authenticated packet(input packet) already has the
    // length adjusted, no need to modify it.
    if(job->sec_context->double_pass == FALSE && !sec_pdb->is_inbound)
    {
        descriptor->ptr[3].len += PDCP_MAC_I_LENGTH;
    }

    // no s/g
    descriptor->ptr[3].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[3].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to input buffer. Skip UA offset and PDCP header
    descriptor->ptr[3].ptr = PHYS_ADDR_LO(phys_addr);
    
    // Print also 4 bytes of IV prepended to the PDCP packet (hdr + payload)
    SEC_DEBUG("PDCP IN packet + IV (len = %d bytes) (hex) = [%x %x %x %x]",
              descriptor->ptr[3].len,
              *((uint32_t*)(auth_packet->address + auth_packet->offset) - 2),
              *((uint32_t*)(auth_packet->address + auth_packet->offset) - 1),
              *((uint32_t*)(auth_packet->address + auth_packet->offset)),
              *((uint32_t*)(auth_packet->address + auth_packet->offset) + 1));
    
    //////////////////////////////////////////////////////////////
    // next 2 pointers unused
    //////////////////////////////////////////////////////////////

    memset(&(descriptor->ptr[4]), 0, 2 * sizeof(struct sec_ptr));

    //////////////////////////////////////////////////////////////
    // set output buffer
    //////////////////////////////////////////////////////////////

    // Generated MAC-I (4 byte)
    phys_addr = sec_vtop(job->mac_i);

    // If no size is given for output data = MAC-I, then SEC engine generates by default
    // a 16 byte MAC-I. Configure for 8 byte MAC-I.
    descriptor->ptr[6].len = 2 * PDCP_MAC_I_LENGTH;
    // no s/g
    descriptor->ptr[6].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[6].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to output buffer. Skip UA offset and PDCP header
    descriptor->ptr[6].ptr = PHYS_ADDR_LO(phys_addr);

    return ret;
}

static int sec_pdcp_context_update_null_auth_descriptor(sec_job_t *job, sec_descriptor_t *descriptor)
{
    return SEC_SUCCESS;
}

static int sec_pdcp_context_update_null_cipher_descriptor(sec_job_t *job, sec_descriptor_t *descriptor)
{
    return SEC_SUCCESS;
}

static void sec_pdcp_update_iv_template(sec_crypto_pdb_t *sec_pdb,
                                        uint8_t *pdcp_header,
                                        sec_status_t *status,
                                        uint8_t iv_offset)
{
    uint16_t seq_no = 0;
    uint16_t last_sn = 0;
    uint16_t max_sn = 0;


#if defined(PDCP_TEST_SNOW_F9_ONLY)
    // When testing only SNOW F9 encapsulation/decapsulation (not PDCP control plane,
    // which means SNOW F8 + SNOW F9), use an artificial SN, as extracted from test vector
    seq_no = 1;
#warning "F9 only !!!"
#elif defined(PDCP_TEST_AES_CMAC_ONLY)
#warning "AES CMAC only !!!"
    seq_no = 20;
#else
    // extract sequence number from PDCP header located in input packet
    seq_no = PDCP_HEADER_GET_SN(pdcp_header, sec_pdb);
#endif

    // extract last SN from IV
    last_sn = sec_pdb->iv_template[iv_offset] & (~sec_pdb->hfn_mask);
    // max_sn = ALL SN bits set to 1 = SN mask = NOT HFN mask
    max_sn = ~sec_pdb->hfn_mask;

    // first clear old SN
    sec_pdb->iv_template[iv_offset] = sec_pdb->iv_template[iv_offset] & sec_pdb->hfn_mask;

    // set new SN in IV first word. 
    sec_pdb->iv_template[iv_offset] = sec_pdb->iv_template[iv_offset] | seq_no;
    SEC_DEBUG("Update IV template  with SN = 0x%x", seq_no);

    // check if SN rolled over
    if ((last_sn == max_sn) && (seq_no == 0))
    {
        // increment HFN: 
        // max_sn + 1 = ALL SN bits set to 0 and IV[0] bit at position length(SN) + 1,
        // when counting from right to left, is set to 1. This is LSB bit of HFN.
        sec_pdb->iv_template[iv_offset] = sec_pdb->iv_template[iv_offset] + (1 + max_sn);
    }

    // check if HFN reached threshold
    *status = ((sec_pdb->iv_template[iv_offset] & sec_pdb->hfn_mask) >= sec_pdb->hfn_threshold) ? 
               SEC_STATUS_HFN_THRESHOLD_REACHED : SEC_STATUS_SUCCESS;
}

static void sec_pdcp_create_pdb(sec_context_t *ctx)
{
    sec_crypto_pdb_t *sec_pdb = &ctx->crypto_desc_pdb;

    // Copy HFN threshold
    sec_pdb->hfn_threshold = ctx->pdcp_crypto_info->hfn_threshold << ctx->pdcp_crypto_info->sn_size;

    // Set mask to extract HFN from IV and SN from PDCP header
    if (ctx->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE)
    {
        sec_pdb->hfn_mask = PDCP_HFN_MASK_CONTROL_PLANE;
        sec_pdb->sn_mask = PDCP_SN_MASK_CONTROL_PLANE;
    }
    else
    {
        sec_pdb->hfn_mask = (ctx->pdcp_crypto_info->sn_size == SEC_PDCP_SN_SIZE_12) ?
                            PDCP_HFN_MASK_DATA_PLANE_LONG_SN : PDCP_HFN_MASK_DATA_PLANE_SHORT_SN;

        sec_pdb->sn_mask = (ctx->pdcp_crypto_info->sn_size == SEC_PDCP_SN_SIZE_12) ?
                           PDCP_SN_MASK_DATA_PLANE_LONG_SN : PDCP_SN_MASK_DATA_PLANE_SHORT_SN;
    }

    // SNS = 1 if short SN is used
    sec_pdb->sns = (ctx->pdcp_crypto_info->sn_size ==  SEC_PDCP_SN_SIZE_12) ? 0 : 1;
    sec_pdb->pdcp_hdr_len = (ctx->pdcp_crypto_info->sn_size ==  SEC_PDCP_SN_SIZE_12) ?
                             PDCP_HEADER_LENGTH_LONG : PDCP_HEADER_LENGTH_SHORT;
    sec_pdb->is_inbound = (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? FALSE : TRUE;
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int sec_pdcp_context_set_crypto_info(sec_context_t *ctx,
                                     const sec_pdcp_context_info_t *crypto_info)
{
    int ret = SEC_SUCCESS;

    // store PDCP crypto info in context
    ctx->pdcp_crypto_info = crypto_info;

    ret = sec_pdcp_context_create_descriptor(ctx);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create descriptor for "
               "PDCP context with bearer = %d", ctx->pdcp_crypto_info->bearer);

    return SEC_SUCCESS;
}

int sec_pdcp_context_update_descriptor(sec_context_t *ctx,
                                       sec_job_t *job,
                                       sec_descriptor_t *descriptor,
                                       uint8_t do_integrity_check)
{
    int ret = SEC_SUCCESS;

    // Call function pointer to update descriptor for: 
    // SNOW F8/F9 or AES CTR/CMAC
    ret = do_integrity_check == 1 ? ctx->update_auth_descriptor(job, descriptor):
                                    ctx->update_crypto_descriptor(job, descriptor);
    //SEC_INFO("Update crypto descriptor return code = %d", ret);

    return ret;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
