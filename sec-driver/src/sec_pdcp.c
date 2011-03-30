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

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
/** First word of Descriptor Header that configure SEC 3.1 for SNOW F8 encryption */
#define SEC_PDCP_SNOW_F8_ENC_DESCRIPTOR_HEADER_HI   SEC_DESC_HDR_EU_SEL0_STEU | SEC_DESC_HDR_MODE0_STEU_F8  | \
                                                    SEC_DESC_HDR_DESC_TYPE_STEU | SEC_DESC_HDR_DIR_OUTBOUND

/** First word of Descriptor Header that configure SEC 3.1 for SNOW F9 encryption
 * TODO: to be defined */
#define SEC_PDCP_SNOW_F9_ENC_DESCRIPTOR_HEADER_HI   0x0

/** First word of Descriptor Header that configure SEC 3.1 for SNOW F8 decryption*/
#define SEC_PDCP_SNOW_F8_DEC_DESCRIPTOR_HEADER_HI   SEC_DESC_HDR_EU_SEL0_STEU | SEC_DESC_HDR_MODE0_STEU_F8  | \
                                                    SEC_DESC_HDR_DESC_TYPE_STEU | SEC_DESC_HDR_DIR_INBOUND

/** First word of Descriptor Header that configure SEC 3.1 for SNOW F9 decryption
 * TODO: to be defined */
#define SEC_PDCP_SNOW_F9_DEC_DESCRIPTOR_HEADER_HI   0x0

/** First word of Descriptor Header that configure SEC 3.1 for AES CTR encryption*/
#define SEC_PDCP_AES_CTR_ENC_DESCRIPTOR_HEADER_HI   SEC_DESC_HDR_EU_SEL0_AESU | SEC_DESC_HDR_MODE0_AESU_CTR  | \
                                                    SEC_DESC_HDR_DIR_OUTBOUND

/** First word of Descriptor Header that configure SEC 3.1 for AES CMAC encryption
 * TODO: to be defined */
#define SEC_PDCP_AES_CMAC_ENC_DESCRIPTOR_HEADER_HI  0x0

/** First word of Descriptor Header that configure SEC 3.1 for AES CTR decryption*/
#define SEC_PDCP_AES_CTR_DEC_DESCRIPTOR_HEADER_HI   SEC_DESC_HDR_EU_SEL0_AESU | SEC_DESC_HDR_MODE0_AESU_CTR  | \
                                                    SEC_DESC_HDR_DIR_INBOUND

/** First word of Descriptor Header that configure SEC 3.1 for AES CMAC decryption
 * TODO: to be defined */
#define SEC_PDCP_AES_CMAC_DEC_DESCRIPTOR_HEADER_HI  0x0

/** Length in bytes of IV(Initialization Vector) for SNOW F8.
 * Same length is used for IV(Initialization Vector actually representing ICV = Initial Counter Value)
 * in case of AES CTR(encrypt/decrypt).
 * 
 * For AES CTR, PDCP standard state an IV 128 bits long, having trailing 64 bits set to 0. 
 * First 64 bits constructed as for SNOW 3G F8 IV.
 * @note SEC 3.1 knows to append the trailing 64 bits of zero, 
 *       so just create an IV like for SNOW F8.
 * */
#define PDCP_SNOW_F8_AES_CTR_IV_LENGTH  8

/** Length in BITS for bearer field that is part of IV */
#define PDCP_BEARER_LENGTH      5

/** Length in bytes of PDCP header for control plane packets and
 * for data plane packets when short SN is used. */
#define PDCP_HEADER_LENGTH_SHORT    1

/** Length in bytes of PDCP header for data plane packets when long SN is used. */
#define PDCP_HEADER_LENGTH_LONG     2

/** Mask for extracting HFN from IV for control plane packets */
#define PDCP_HFN_MASK_CONTROL_PLANE         0xFFFFFF80

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
extern ptov_function sec_ptov;
extern vtop_function sec_vtop;

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
int sec_pdcp_create_snow_f8_aes_ctr_descriptor(sec_context_t *ctx);
/** @brief Create a SNOW F9 (authentication) descriptor.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
int sec_pdcp_create_snow_f9_descriptor(sec_context_t *ctx);
/** @brief Create a AES CMAC (authentication) descriptor.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
int sec_pdcp_create_aes_cmac_descriptor(sec_context_t *ctx);

/** @brief  Creates a SEC descriptor for the algorithm
 * configured for this context.
 *
 * @param [in,out] ctx          SEC context
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
int sec_pdcp_context_create_descriptor(sec_context_t *ctx);

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
int sec_pdcp_context_update_snow_f8_aes_ctr_descriptor(sec_job_t *job, sec_descriptor_t *descriptor);

/** @brief  Updates the Initialization Vector required by SEC to
 * run SNOW F8 /AES CTR algorithms for a PDCP packet.
 * IV is updated with:
 *  - SN(seq no) extracted from PDCP header
 *  - D/C bit extracted from PDCP header
 *  - HFN is incremented if SN rolls over
 *
 * @param [in,out] job          SEC job
 * @param [in,out] descriptor   SEC descriptor
 *
 * @retval SEC_SUCCESS                  for success      
 * @retval SEC_HFN_THRESHOLD_REACHED    if HFN reached the threshold configured for 
 *                                      the SEC context this PDB, IV, packet belong to.
 *                                      It is not error, but an indication to User App that
 *                                      crypto keys must be renegotiated.
 * @retval other for error
 */
int sec_pdcp_update_iv(uint32_t iv[], sec_crypto_pdb_t *sec_pdb, uint8_t *pdcp_header);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

int sec_pdcp_create_snow_f8_aes_ctr_descriptor(sec_context_t *ctx)
{
    sec_crypto_pdb_t *sec_pdb = &ctx->crypto_desc_pdb;

    // Copy crypto data into PDB
    ASSERT(ctx->pdcp_crypto_info->cipher_key_len <= SEC_CRYPTO_KEY_MAX_LENGTH);

    // Copy crypto key
    memcpy(sec_pdb->keys->crypto_key, ctx->pdcp_crypto_info->cipher_key, ctx->pdcp_crypto_info->cipher_key_len);
    sec_pdb->crypto_key_len = ctx->pdcp_crypto_info->cipher_key_len;

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

    // Construct (template) IV (64 bits) =  HFN | SN | Bearer | D/C | 26 bits of 0
    // control plane:           HFN 27 bits, SN 5 bit
    // data plane, short SN:    HFN 25 bits, SN 7 bit
    // data plane, long SN:     HFN 20 bits, SN 12 bit
    
    // Set HFN.
    // SN will be updated for each packet. When SN rolls over, HFN is incremented.
    sec_pdb->iv_template[0] = ctx->pdcp_crypto_info->hfn << ctx->pdcp_crypto_info->sn_size;

    // Set bearer
    sec_pdb->iv_template[1] = ctx->pdcp_crypto_info->bearer << (32 - PDCP_BEARER_LENGTH);

    // Set D/C bit : Consider data PDU in template. 
    // This bit will be updated for each packet. 
    // It will be cleared for control PDUs.
    // Mind that for data plane packets we can have data PDUs and control PDU's!
    // Do not mistake with control plane PDUs!
    sec_pdb->iv_template[1] = sec_pdb->iv_template[1] | (1 << (32 - PDCP_BEARER_LENGTH - 1));

    // Enable SEC engine to do write-back.
    // When SEC finished processing packet, the higer 32 bits 
    // from descriptor will be written-back by SEC to indicate job DONE.
    hw_job_enable_writeback(sec_pdb->crypto_hdr);

    return SEC_SUCCESS;
}

int sec_pdcp_create_snow_f9_descriptor(sec_context_t *ctx)
{
    // Create descriptor for encrypt
    if (ctx->pdcp_crypto_info->packet_direction == PDCP_DOWNLINK)
    {
        SEC_INFO("Creating SNOW F9 DL descriptor");
        ctx->crypto_desc_pdb.auth_hdr = SEC_PDCP_SNOW_F9_ENC_DESCRIPTOR_HEADER_HI;
    }
    else
    {
        SEC_INFO("Creating SNOW F9 UL descriptor");
        // Create descriptor for decrypt
        ctx->crypto_desc_pdb.auth_hdr = SEC_PDCP_SNOW_F9_DEC_DESCRIPTOR_HEADER_HI;
    }

    // TODO: create IV template for F9...

    // Enable SEC engine to do write-back
    hw_job_enable_writeback(ctx->crypto_desc_pdb.auth_hdr);

    return SEC_SUCCESS;
}

int sec_pdcp_create_aes_cmac_descriptor(sec_context_t *ctx)
{
    // Create descriptor for encrypt
    if (ctx->pdcp_crypto_info->packet_direction == PDCP_DOWNLINK)
    {
        SEC_INFO("Creating AES CMAC DL descriptor");
        ctx->crypto_desc_pdb.auth_hdr = SEC_PDCP_AES_CMAC_ENC_DESCRIPTOR_HEADER_HI;
    }
    else
    {
        SEC_INFO("Creating AES CMAC UL descriptor");
        // Create descriptor for decrypt
        ctx->crypto_desc_pdb.auth_hdr = SEC_PDCP_AES_CMAC_DEC_DESCRIPTOR_HEADER_HI;
    }

    // TODO: create IV template for AES CMAC...

    // Enable SEC engine to do write-back
    hw_job_enable_writeback(ctx->crypto_desc_pdb.auth_hdr);
    return SEC_SUCCESS;
}

int sec_pdcp_context_create_descriptor(sec_context_t *ctx)
{
    int ret = SEC_SUCCESS;

    if (ctx->pdcp_crypto_info->algorithm == SEC_ALG_SNOW)
    {
        // Configure SEC descriptor for ciphering/deciphering
        if (ctx->pdcp_crypto_info->packet_direction == PDCP_DOWNLINK)
        {
            SEC_INFO("Creating SNOW F8 DL descriptor");
            ctx->crypto_desc_pdb.crypto_hdr = SEC_PDCP_SNOW_F8_ENC_DESCRIPTOR_HEADER_HI;
        }
        else
        {
            SEC_INFO("Creating SNOW F8 UL descriptor");
            // Create descriptor for decrypt
            ctx->crypto_desc_pdb.crypto_hdr = SEC_PDCP_SNOW_F8_DEC_DESCRIPTOR_HEADER_HI;
        }

        ret = sec_pdcp_create_snow_f8_aes_ctr_descriptor(ctx);
        SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create SNOW F8 descriptor");

        // Set pointer to the function that will be called to 
        // update the SEC descriptor for each packet.
        // SNOW F8 will always be the first processing step for both data plane (SNOW F8)
        // and control plane (SNOW F8 + SNOW F9).
        ctx->update_crypto_descriptor = sec_pdcp_context_update_snow_f8_aes_ctr_descriptor;

        // If control plane context, also configure authentication descriptor
        if (ctx->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE)
        {
            SEC_ASSERT(0, SEC_INVALID_INPUT_PARAM, "PDCP Control plane feature not implemented!");
            ret = sec_pdcp_create_snow_f9_descriptor(ctx);
            SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create SNOW F9 descriptor");
            // TODO: maintain a second function pointer for updating authentication descriptor ?
            // TODO: F9 is done before F8 on DL. Reverse for uplink !
        }
    }
    else if (ctx->pdcp_crypto_info->algorithm == SEC_ALG_AES)
    {
        // Configure SEC descriptor for ciphering/deciphering
        if (ctx->pdcp_crypto_info->packet_direction == PDCP_DOWNLINK)
        {
            SEC_INFO("Creating AES CTR DL descriptor");
            ctx->crypto_desc_pdb.crypto_hdr = SEC_PDCP_AES_CTR_ENC_DESCRIPTOR_HEADER_HI;
        }
        else
        {
            SEC_INFO("Creating AES CTR UL descriptor");
            // Create descriptor for decrypt
            ctx->crypto_desc_pdb.crypto_hdr = SEC_PDCP_AES_CTR_DEC_DESCRIPTOR_HEADER_HI;
        }
        ret = sec_pdcp_create_snow_f8_aes_ctr_descriptor(ctx);
        SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create AES CTR descriptor");

        // Set pointer to the function that will be called to 
        // update the SEC descriptor for each packet.
        ctx->update_crypto_descriptor = sec_pdcp_context_update_snow_f8_aes_ctr_descriptor;

        // If control plane context, also configure authentication descriptor
        if (ctx->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE)
        {
            SEC_ASSERT(0, SEC_INVALID_INPUT_PARAM, "PDCP Control plane feature not implemented!");
            ret = sec_pdcp_create_aes_cmac_descriptor(ctx);
            SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create AES CMAC descriptor");
            // TODO: maintain a second function pointer for updating authentication descriptor ?
            // TODO: AES CMAC is done before AES CTR on DL. Reverse for uplink !
        }
    }

    return SEC_SUCCESS;
}

int sec_pdcp_context_update_snow_f8_aes_ctr_descriptor(sec_job_t *job, sec_descriptor_t *descriptor)
{
    int ret = SEC_SUCCESS;
    sec_crypto_pdb_t *sec_pdb = &job->sec_context->crypto_desc_pdb;
    uint8_t pdcp_hdr_len = 0;
    dma_addr_t phys_addr = 0;

    // Configure SEC descriptor header for crypto operation
    descriptor->hdr = sec_pdb->crypto_hdr;
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
    ret = sec_pdcp_update_iv(descriptor->iv,
                             sec_pdb,
                             job->in_packet->address + job->in_packet->offset); // PDCP header pointer
    // set pointer to IV
    descriptor->ptr[1].ptr = PHYS_ADDR_LO(phys_addr);

    //////////////////////////////////////////////////////////////
    // set cipher key
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(sec_pdb->keys->crypto_key);
    descriptor->ptr[2].len = sec_pdb->crypto_key_len;
    // no s/g
    descriptor->ptr[2].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[2].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to cipher key
    descriptor->ptr[2].ptr = PHYS_ADDR_LO(phys_addr);

    // calculate PDCP header length
    pdcp_hdr_len = sec_pdb->sns ? PDCP_HEADER_LENGTH_SHORT : PDCP_HEADER_LENGTH_LONG;


    //////////////////////////////////////////////////////////////
    // set input buffer
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(job->in_packet->address) + job->in_packet->offset + pdcp_hdr_len;
    descriptor->ptr[3].len = job->in_packet->length - job->in_packet->offset - pdcp_hdr_len;
    // no s/g
    descriptor->ptr[3].j_extent = 0;
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
    // Update for 36bit physical address
    descriptor->ptr[3].eptr = PHYS_ADDR_HI(phys_addr);
#endif
    // pointer to input buffer. Skip UA offset and PDCP header
    descriptor->ptr[3].ptr = PHYS_ADDR_LO(phys_addr);

    //////////////////////////////////////////////////////////////
    // copy PDCP header from input packet into output packet
    //////////////////////////////////////////////////////////////

    memcpy(job->out_packet->address + job->out_packet->offset, 
           job->in_packet->address + job->in_packet->offset,
           pdcp_hdr_len);


    //////////////////////////////////////////////////////////////
    // set output buffer
    //////////////////////////////////////////////////////////////

    phys_addr = sec_vtop(job->out_packet->address) + job->out_packet->offset + pdcp_hdr_len;
    descriptor->ptr[4].len = job->out_packet->length - job->out_packet->offset - pdcp_hdr_len;
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

int sec_pdcp_update_iv(uint32_t iv[],
                       sec_crypto_pdb_t *sec_pdb,
                       uint8_t *pdcp_header)
{
    int ret = SEC_SUCCESS;
    uint16_t seq_no = 0;
    uint16_t last_sn = 0;
    uint16_t max_sn = 0;
    uint8_t data_control_bit = 0;

    // extract sequence number from PDCP header located in input packet
    seq_no = PDCP_HEADER_GET_SN(pdcp_header, sec_pdb);

    // extract last SN from IV
    last_sn = sec_pdb->iv_template[0] & (~sec_pdb->hfn_mask);
    // max_sn = ALL SN bits set to 1 = SN mask = NOT HFN mask
    max_sn = ~sec_pdb->hfn_mask;

    // first clear old SN
    sec_pdb->iv_template[0] = sec_pdb->iv_template[0] & sec_pdb->hfn_mask;

    // set new SN in IV first word. 
    sec_pdb->iv_template[0] = sec_pdb->iv_template[0] | seq_no;

    // check if SN rolled over
    if ((last_sn == max_sn) && (seq_no == 0))
    {
        // increment HFN: 
        // max_sn + 1 = ALL SN bits set to 0 and IV[0] bit at position length(SN) + 1,
        // when counting from right to left, is set to 1. This is LSB bit of HFN.
        sec_pdb->iv_template[0] = sec_pdb->iv_template[0] + (1 + max_sn);
    }

    // check if HFN reached threshold
    ret = ((sec_pdb->iv_template[0] & sec_pdb->hfn_mask) >= sec_pdb->hfn_threshold) ? 
          SEC_HFN_THRESHOLD_REACHED : SEC_SUCCESS;

    // extract D/C bit from PDCP header
    data_control_bit = PDCP_HEADER_GET_D_C(pdcp_header);

    // set D/C bit in IV: data PDU or control PDU (both possible for data plane)
    sec_pdb->iv_template[1] = sec_pdb->iv_template[1] & 
                                (PDCP_IV_WORD_BEARER_MASK  | 
                                (data_control_bit << (32 - PDCP_BEARER_LENGTH - 1)));

    iv[0] = sec_pdb->iv_template[0];
    iv[1] = sec_pdb->iv_template[1];

    return ret;
}
/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int sec_pdcp_context_set_crypto_info(sec_context_t *ctx,
                                     sec_pdcp_context_info_t *crypto_info)
{
    int ret = SEC_SUCCESS;

    // store PDCP crypto info in context
    ctx->pdcp_crypto_info = crypto_info;

    ret = sec_pdcp_context_create_descriptor(ctx);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create descriptor for "
               "PDCP context with bearer = %d", ctx->pdcp_crypto_info->bearer);

    return SEC_SUCCESS;
}

int sec_pdcp_context_update_descriptor(sec_context_t *ctx, sec_job_t *job, sec_descriptor_t *descriptor)
{
    int ret = SEC_SUCCESS;

    // Call function pointer to update descriptor for: 
    // SNOW F8/F9 or AES CTR/CMAC
    ret = ctx->update_crypto_descriptor(job, descriptor);

    return ret;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
