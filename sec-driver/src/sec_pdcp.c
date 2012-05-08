/* Copyright (c) 2011 - 2012 Freescale Semiconductor, Inc.
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

#ifdef SEC_HW_VERSION_3_1
// For definition of sec_vtop and sec_ptov macros
#include "external_mem_management.h"
#endif // SEC_HW_VERSION_3_1

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
#ifdef SEC_HW_VERSION_3_1
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
#else // SEC_HW_VERSION_3_1

/** Define for use in the array of descriptor creating functions. This is the number of
 * rows in the array
 */
#define NUM_CIPHER_ALGS sizeof(sec_crypto_alg_t)

/** Define for use in the array of descriptor creating functions. This is the number of
 * columns in the array
 */
#define NUM_INT_ALGS sizeof(sec_crypto_alg_t)

#endif // SEC_HW_VERSION_3_1

/** Used only for testing! Simulates that the first 2 packets
 *  submitted to SEC have an invalid descriptor header.
 *  This will generate an IDH error. */
//#define SEC_SIMULATE_PACKET_PROCESSING_ERROR

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
#ifdef SEC_HW_VERSION_4_4
/** Typedef for function pointer forcreating a shared descriptor on a context */
typedef int (*create_desc_fp)(sec_context_t *ctx);
#endif // SEC_HW_VERSION_4_4
/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
#ifdef SEC_HW_VERSION_4_4

 /** Forward declaration of a static array of function pointers
  * indexed by the encryption algorithms and authentication
  * algorithms used for PDCP Control Plane.
  */
static create_desc_fp c_plane_create_desc[][NUM_INT_ALGS];

/** Forward declaration of a static array of function pointers
 * indexed by the encryption algorithms used for
 * PDCP User Plane.
 */
static create_desc_fp u_plane_create_desc[];

#endif //SEC_HW_VERSION_4_4

/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
#ifdef SEC_HW_VERSION_4_4
extern sec_vtop g_sec_vtop;
#endif
/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
#ifdef SEC_HW_VERSION_3_1
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
#else // SEC_HW_VERSION_3_1
/** @brief Function for populating a shared descriptor with the
 * required information for executing a HW supported protocol
 * acceleration for PDCP Control Plane.
 *
 * @param [in]      ctx         SEC context
 *
 * @Caution     Works only for SNOW3G/AES algorithm suites
 */
static int create_c_plane_hw_acc_desc(sec_context_t *ctx);

/** @brief Function for populating a shared descriptor with the
 * required information for executing a HW supported protocol
 * acceleration for PDCP User Plane.
 *
 * @param [in]      ctx         SEC context
 *
 * @Caution     Works only for SNOW3G/AES algorithm suites
 */
static int create_u_plane_hw_acc_desc(sec_context_t *ctx);

/** @brief Function for populating a shared descriptor with the
 * required information for executing an accelerated
 * PDCP Control Plane descriptor with mixed authentication.
 * and encryption algorithms
 *
 * @param [in]      ctx         SEC context
 *
 * @Caution     Currently not implemented, do not use
 */
static int create_c_plane_mixed_desc(sec_context_t *ctx);

/** @brief Function for populating a shared descriptor with the
 * required information for executing a PDCP Control plane descriptor
 * performing only authentication and no encryption.
 *
 * @param [in]      ctx         SEC context
 *
 * @Caution     Currently not implemented, do not use
 */
static int create_c_plane_auth_only_desc(sec_context_t *ctx);

/** @brief Function for populating a shared descriptor with the
 * required information for executing a PDCP Control plane descriptor
 * performing only encryption and no authentication.
 *
 * @param [in]      ctx         SEC context
 *
 * @Caution     Currently not implemented, do not use
 */
static int create_c_plane_cipher_only_desc(sec_context_t *ctx);

/** @brief Function for populating a shared descriptor with the
 * required information for executing a PDCP Control plane descriptor
 * which performs authentication and/or encryption using NULL algorithm
 *
 * @param [in]      ctx         SEC context
 *
 *
 */
static int create_null_desc(sec_context_t *ctx);

#endif // SEC_HW_VERSION_3_1
/** @brief Fill PDB with initial data for this context: HFN threshold, HFN mask, etc
 * @param [in,out] ctx          SEC context
 */
static void sec_pdcp_create_pdb(sec_context_t *ctx);

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
#ifdef SEC_HW_VERSION_3_1
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
    ctx->update_auth_descriptor = sec_pdcp_context_update_null_auth_descriptor;

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
#else // SEC_HW_VERSION_3_1
static void sec_pdcp_create_pdb(sec_context_t *ctx)
{
    sec_crypto_pdb_t *sec_pdb;
    struct cplane_pdb_s *cplane_pdb = NULL;
    struct uplane_pdb_s *uplane_pdb = NULL;

    ASSERT( ctx != NULL );
    ASSERT( ctx->pdcp_crypto_info != NULL);
    ASSERT( ctx->sh_desc != NULL);

    sec_pdb = &ctx->sh_desc->pdb;

    if (ctx->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE)
    {
        cplane_pdb = &sec_pdb->pdb_content.cplane_pdb;

        ASSERT( ctx->pdcp_crypto_info->sn_size == SEC_PDCP_SN_SIZE_5 );
#warning "This seems to be an issue in HW, bit 30 should be set to 0 for C-Plane"
        cplane_pdb->res1 = 0x00000002;
        cplane_pdb->hfn = ctx->pdcp_crypto_info->hfn;
        cplane_pdb->bearer = ctx->pdcp_crypto_info->bearer;
        cplane_pdb->dir = ctx->pdcp_crypto_info->packet_direction;
        cplane_pdb->threshold = ctx->pdcp_crypto_info->hfn_threshold;
    }
    else
    {// data plane
        uplane_pdb = &sec_pdb->pdb_content.uplane_pdb;
        uplane_pdb->bearer = ctx->pdcp_crypto_info->bearer;
        uplane_pdb->dir = ctx->pdcp_crypto_info->packet_direction;

        if( ctx->pdcp_crypto_info->sn_size ==  SEC_PDCP_SN_SIZE_12 )
        {
            uplane_pdb->sns = 0;
            uplane_pdb->hfn.hfn_l.hfn = ctx->pdcp_crypto_info->hfn;
            uplane_pdb->threshold.threshold_l.threshold = ctx->pdcp_crypto_info->hfn_threshold;
        }
        else
        {
            uplane_pdb->sns = 1;
            uplane_pdb->hfn.hfn_s.hfn = ctx->pdcp_crypto_info->hfn;
            uplane_pdb->threshold.threshold_s.threshold = ctx->pdcp_crypto_info->hfn_threshold;
        }
    }
}

static int sec_pdcp_context_create_descriptor(sec_context_t *ctx)
{
    int ret = SEC_SUCCESS;

    ASSERT(ctx != NULL);
    ASSERT(ctx->pdcp_crypto_info != NULL);

    // Create a Protocol Data Blob (PDB)
    sec_pdcp_create_pdb(ctx);

    ret = (ctx->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE) ?             \
            (c_plane_create_desc[ctx->pdcp_crypto_info->cipher_algorithm]        \
                          [ctx->pdcp_crypto_info->integrity_algorithm](ctx)) :    \
            (u_plane_create_desc[ctx->pdcp_crypto_info->cipher_algorithm](ctx));

    return ret;
}

static int create_c_plane_hw_acc_desc(sec_context_t *ctx)
{
    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);

    ASSERT(ctx->pdcp_crypto_info->cipher_key != NULL);
    ASSERT(ctx->pdcp_crypto_info->integrity_key != NULL);

    SEC_INFO("Creating C-PLANE HW Acc. descriptor w/alg %s & auth alg %s",
              ctx->pdcp_crypto_info->cipher_algorithm == SEC_ALG_AES ? "AES" : "SNOW",
              ctx->pdcp_crypto_info->cipher_algorithm == SEC_ALG_AES ? "AES" : "SNOW");

    SEC_PDCP_INIT_CPLANE_SD(ctx->sh_desc);

    /* Plug-in the HFN override in descriptor from DPOVRD */
    
    ctx->sh_desc->hfn_ov_desc[0] = 0xAC574F08;
    ctx->sh_desc->hfn_ov_desc[1] = 0x80000000;
    
    ctx->sh_desc->hfn_ov_desc[2] = 0xA0000407;
    
    ctx->sh_desc->hfn_ov_desc[3] = 0xAC574008;
    ctx->sh_desc->hfn_ov_desc[4] = 0x07FFFFFF;
    ctx->sh_desc->hfn_ov_desc[5] = 0xAC704008;
    ctx->sh_desc->hfn_ov_desc[6] = 0x00000005;
    ctx->sh_desc->hfn_ov_desc[7] = 0xA8900008;
    ctx->sh_desc->hfn_ov_desc[8] = 0x78430804;

    SEC_PDCP_SD_SET_KEY2(ctx->sh_desc,
                         ctx->pdcp_crypto_info->integrity_key,
                         ctx->pdcp_crypto_info->integrity_key_len);

    SEC_PDCP_SD_SET_KEY1(ctx->sh_desc,
                         ctx->pdcp_crypto_info->cipher_key,
                         ctx->pdcp_crypto_info->cipher_key_len);

    SEC_PDCP_SH_SET_PROT_DIR(ctx->sh_desc,
                             ctx->pdcp_crypto_info->protocol_direction);

    // Doesn't matter which alg, since they're the same in this case
    SEC_PDCP_SH_SET_PROT_ALG(ctx->sh_desc,ctx->pdcp_crypto_info->cipher_algorithm);

    SEC_PDCP_DUMP_DESC(ctx->sh_desc);
    return SEC_SUCCESS;
}

static int create_u_plane_hw_acc_desc(sec_context_t *ctx)
{
    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);
    ASSERT(ctx->pdcp_crypto_info->cipher_key != NULL);

    SEC_INFO("Creating U-PLANE HW Acc. descriptor w/alg %s",
              ctx->pdcp_crypto_info->cipher_algorithm == SEC_ALG_AES ? "AES" : "SNOW");

    SEC_PDCP_INIT_UPLANE_SD(ctx->sh_desc);

    /* Plug-in the HFN override in descriptor from DPOVRD */
    ctx->sh_desc->hfn_ov_desc[0] = 0xAC574F08;
    ctx->sh_desc->hfn_ov_desc[1] = 0x80000000;
    ctx->sh_desc->hfn_ov_desc[2] = 0xA0000407;
    ctx->sh_desc->hfn_ov_desc[3] = 0xAC574008;
    ctx->sh_desc->hfn_ov_desc[5] = 0xAC704008;
    
    /* I avoid to do complicated things in CAAM, thus I 'hardcode'
     * the operations to be done on the HFN as per the SN size. Doing
     * a generic descriptor that would look at the PDB and then decide
     * on the actual values to shift would have made the descriptors too
     * large and slow [ if-then-else construct in CAAM means 2 JUMPS ]
     */
    if(ctx->pdcp_crypto_info->sn_size == SEC_PDCP_SN_SIZE_7)
    {
        ctx->sh_desc->hfn_ov_desc[4] = 0x01FFFFFF;
        ctx->sh_desc->hfn_ov_desc[6] = 0x00000007;
    }
    else
    {
        ctx->sh_desc->hfn_ov_desc[4] = 0x000FFFFF;
        ctx->sh_desc->hfn_ov_desc[6] = 0x0000000C;
    }
    ctx->sh_desc->hfn_ov_desc[7] = 0xA8900008;
    ctx->sh_desc->hfn_ov_desc[8] = 0x78430804;

    SEC_PDCP_SD_SET_KEY1(ctx->sh_desc,
                         ctx->pdcp_crypto_info->cipher_key,
                         ctx->pdcp_crypto_info->cipher_key_len);

    SEC_PDCP_SH_SET_PROT_DIR(ctx->sh_desc,
                                 ctx->pdcp_crypto_info->protocol_direction);

    SEC_PDCP_SH_SET_PROT_ALG(ctx->sh_desc,ctx->pdcp_crypto_info->cipher_algorithm);

    SEC_PDCP_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int create_c_plane_mixed_desc(sec_context_t *crypto_pdb)
{
    SEC_INFO(" Called create_mixed_desc");
    ASSERT(0);

    return SEC_SUCCESS;
}

static int create_c_plane_auth_only_desc(sec_context_t *ctx)
{
    int i = 5;

    ctx->sh_desc->deschdr.command.word  = 0xB8850100;

    /* Plug-in the HFN override in descriptor from DPOVRD */    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC574F08;
    *((uint32_t*)ctx->sh_desc + i++) = 0x80000000;
    *((uint32_t*)ctx->sh_desc + i++) = 0xA0000407;
    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC574008;
    *((uint32_t*)ctx->sh_desc + i++) = 0x07FFFFFF;
    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC704008;
    *((uint32_t*)ctx->sh_desc + i++) = 0x00000005;
        
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8900008;
    
    *((uint32_t*)ctx->sh_desc + i++) = 0x78430804;
    
    switch( ctx->pdcp_crypto_info->integrity_algorithm )
    {
        case SEC_ALG_SNOW:
            SEC_INFO(" Creating NULL/SNOW f9 descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x04000000 |
                    ctx->pdcp_crypto_info->integrity_key_len;  // key2, len = integrity_key_len
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(ctx->pdcp_crypto_info->integrity_key);

            *((uint32_t*)ctx->sh_desc + i++) = 0x1E080001;  // seq load, class 3, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xA1001001;  // wait for calm

            *((uint32_t*)ctx->sh_desc + i++) = 0xF0200001;  // seqinptr: len=1 rto

            *((uint32_t*)ctx->sh_desc + i++) = 0xAC804108;  // shift right 24 bits, put result in math1
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000018;  // 24
            *((uint32_t*)ctx->sh_desc + i++) = 0xA8514108;  // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001F;  // M1 = M1 & 0x0000001f_00000000
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000000;  //

            *((uint32_t*)ctx->sh_desc + i++) = 0x79360808;  // move from desc buf to math2, wc=1,offset = 8, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8524208;  // Mask "Bearer" bit in IV
            *((uint32_t*)ctx->sh_desc + i++) = 0xffffffff;  //
            *((uint32_t*)ctx->sh_desc + i++) = 0x04000000;  //
            *((uint32_t*)ctx->sh_desc + i++) = 0x79370c04;  // move from desc buf to math3, wc=1,offset = 0x0C, len 4
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8534308;  // Mask "Direction" bit in IV
            *((uint32_t*)ctx->sh_desc + i++) = 0xf8000000;  //
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000000;  //

            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;  // M2 = M2 | M1

            *((uint32_t*)ctx->sh_desc + i++) = 0x7961000c;  // load, class2 ctx, math2, wc = 1, len 12, offset 0,

            if( ctx->pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION )
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xA8284104;  // M1 = SIL-0x04
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;  // 4
            }
            else
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xA828F104;  // M1 = SIL-0x00
            }

            *((uint32_t*)ctx->sh_desc + i++) = 0xA821FA04;  // VSIL = M1-0x00
            *((uint32_t*)ctx->sh_desc + i++) = 0xA821FB04;  // VSOL = M1-0x00

            *((uint32_t*)ctx->sh_desc + i++) = 0x84A00C8C |
                    ( (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                                           0 : CMD_ALGORITHM_ICV ); // operation: optype = 4 (class 2), alg = 0x60 (snow), aai = 0xC8 (f9), as = 11 (int/fin), icv = 1, enc = 0

            *((uint32_t*)ctx->sh_desc + i++) = 0x72A20001;  // move_len: class1-alnblk -> ofifo, len=math1 (aux_ls)
            *((uint32_t*)ctx->sh_desc + i++) = 0x2F170000;  // seqfifold: both msgdata-last2-last1-flush1 vlf

            if( ctx->pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION )
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2C3C0004;  // seqfifold: class2 icv-last2 len=4
            }

            *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;  // seqfifostr: msgdata len=vsol

            if ( ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION )
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x5C200004;  // seqstr: ccb2 ctx len=4 offs=0
            }

            break;
        case SEC_ALG_AES:
            SEC_INFO(" Creating NULL/AES-CMAC descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    ctx->pdcp_crypto_info->integrity_key_len;     // key1, len = integrity_key_len
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(ctx->pdcp_crypto_info->integrity_key);

            *((uint32_t*)ctx->sh_desc + i++) = 0x1E080001;     // seq load, class 3, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xA1001001;     // wait for calm

            *((uint32_t*)ctx->sh_desc + i++) = 0xF0200001;     // seqinptr: len=1 rto

            *((uint32_t*)ctx->sh_desc + i++) = 0xac804108;     // shift right 24 bits, put result in math1
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000018;     // 24
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8514108;     // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;     // M1 = M1 & 0x0000001f_00000000
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000000;     //

            *((uint32_t*)ctx->sh_desc + i++) = 0x79360808;     // move from desc buf to math2, wc=1,offset = 8, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;     // M2 = M2 | M1

            *((uint32_t*)ctx->sh_desc + i++) = 0x78680008;     // move math2 to class1 input fifo(IV=64bits)

            if( ctx->pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION )
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xA8284104;  // M1 = SIL-0x04 (ICV size)
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;  // 4
            }
            else
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xA828F104;     // M1 = SIL-0x00
            }

            *((uint32_t*)ctx->sh_desc + i++) = 0xA821FA04;     // VSIL = M1-0x00
            *((uint32_t*)ctx->sh_desc + i++) = 0xA821fB04;     // VSOL = M1-0x00

            *((uint32_t*)ctx->sh_desc + i++) = 0x8210060C | \
                    ( (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                       0 : CMD_ALGORITHM_ICV );         // operation: optype = 2 (class 1), alg = 0x10 (aes), aai = 0x60 (cmac), as = 11 (int/fin), icv = 0, enc = 0 (ignored)

            *((uint32_t*)ctx->sh_desc + i++) = 0x74A20001;  // move_len: class2-alnblk -> ofifo, len=math1 (aux_ms)
            *((uint32_t*)ctx->sh_desc + i++) = 0x2F170000;  // seqfifold: both msgdata-last2-last1-flush1 vlf

            if( ctx->pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION )
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2A3B0004;  // seqfifold: class1 icv-last1-flush1 len=4
            }

            *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;     // seq fifo store, vlf

            if( ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION )
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x5A200004;     // seqstr: ctx1 len=4 offs=0
            }

            break;
        default:
            SEC_INFO(" Unknown integrity algorithm requested: %d", ctx->pdcp_crypto_info->integrity_algorithm);
            ASSERT(0);
            break;
    }

    // update descriptor length
    ctx->sh_desc->deschdr.command.sd.desclen = i;

    SEC_PDCP_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int create_c_plane_cipher_only_desc(sec_context_t *ctx)
{
    int i = 5;

    ctx->sh_desc->deschdr.command.word = 0xB8850100;  // shared header, start idx = 5

    /* Plug-in the HFN override in descriptor from DPOVRD */    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC574F08;
    *((uint32_t*)ctx->sh_desc + i++) = 0x80000000;
    *((uint32_t*)ctx->sh_desc + i++) = 0xA0000407;
    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC574008;
    *((uint32_t*)ctx->sh_desc + i++) = 0x07FFFFFF;
    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC704008;
    *((uint32_t*)ctx->sh_desc + i++) = 0x00000005;
        
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8900008;
    
    *((uint32_t*)ctx->sh_desc + i++) = 0x78430804;

    switch( ctx->pdcp_crypto_info->cipher_algorithm )
    {
        case SEC_ALG_SNOW:
            SEC_INFO(" Creating SNOW f8/NULL descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    ctx->pdcp_crypto_info->cipher_key_len; // key1, len = cipher_key_len
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(ctx->pdcp_crypto_info->cipher_key);

            *((uint32_t*)ctx->sh_desc + i++) = 0x1e080001;      // seq load, class 3, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm
            *((uint32_t*)ctx->sh_desc + i++) = 0xac804108;      // shift right 24 bits, put result in math1
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000018;      // 24
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8514108;      // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;      // M1 = M1 & 0x0000001f_00000000
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000000;      //

            *((uint32_t*)ctx->sh_desc + i++) = 0x79360808;      // move from desc buf to math2, wc=1,offset = 8, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;      // M2 = M2 | M1

            *((uint32_t*)ctx->sh_desc + i++) = 0x5e080001;      // seq store, class 3, length = 1, src=m0

            *((uint32_t*)ctx->sh_desc + i++) = 0x79600008;      // load, class1 ctx, math2, wc = 1, len 8, offset 0

            *((uint32_t*)ctx->sh_desc + i++) = 0xa828fa04;      // VSIL=SIL-0x00
            *((uint32_t*)ctx->sh_desc + i++) = 0xa828fb04;      // VSOL=SOL-0x00

            *((uint32_t*)ctx->sh_desc + i++) = 0x82600c0c | \
                    ( (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
            CMD_ALGORITHM_ENCRYPT : 0 );      // operation, optype = 2 (class 1), alg = 0x60 (snow), aai = 0xC0 (f8), as = 11 (int/fin), icv = 0, enc = 0

            *((uint32_t*)ctx->sh_desc + i++) = 0x2B130000;      // seqfifold: class1 msgdata-last1-flush1 vlf
            *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;      // SEQ FIFO STORE, vlf

            break;

        case SEC_ALG_AES:
            SEC_INFO(" Creating AES-CTR/NULL descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    ctx->pdcp_crypto_info->cipher_key_len; // key1, len = cipher_key_len
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(ctx->pdcp_crypto_info->cipher_key);

            *((uint32_t*)ctx->sh_desc + i++) = 0x1E080001;  // seq load, class 3, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xA1001001;  // wait for calm
            *((uint32_t*)ctx->sh_desc + i++) = 0xac804108;  // shift right 24 bits, put result in math1
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000018;  // 24
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8514108;  // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;  // M1 = M1 & 0x0000001f_00000000
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000000;  //

            *((uint32_t*)ctx->sh_desc + i++) = 0x79360808;  // move from desc buf to math2, wc=1,offset = 8, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;  // M2 = M2 | M1

            *((uint32_t*)ctx->sh_desc + i++) = 0x5e080001;  // seq store, class 3, length = 1, src=m0

            *((uint32_t*)ctx->sh_desc + i++) = 0x79601010;  // load, class1 ctx, math2, wc = 1, len 16, offset 16

            *((uint32_t*)ctx->sh_desc + i++) = 0xa828fa04;  // VSIL=SIL-0x00
            *((uint32_t*)ctx->sh_desc + i++) = 0xa828fb04;  // VSOL=SIL-0x00

            *((uint32_t*)ctx->sh_desc + i++) = 0x8210000c | \
                    ( (ctx->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                       CMD_ALGORITHM_ENCRYPT : 0 );  // operation: optype = 2 (class 1), alg = 0x10 (aes), aai = 0x00 (ctr), as = 11 (int/fin), icv = 0, enc = 1
            *((uint32_t*)ctx->sh_desc + i++) = 0x2b170000;  // seq fifo load, class1, vlf, LC1,LC2
            *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;  // SEQ FIFO STORE, vlf

            break;
        default:
            SEC_INFO(" Unknown ciphering algorithm requested: %d", ctx->pdcp_crypto_info->cipher_algorithm);
            ASSERT(0);
            break;
    }

    // update descriptor length
    ctx->sh_desc->deschdr.command.sd.desclen = i;

    SEC_PDCP_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int create_null_desc(sec_context_t *ctx)
{
    int i = 0;
    SEC_INFO("Creating U-PLANE/C-Plane descriptor with NULL alg.");

    *((uint32_t*)ctx->sh_desc + i++) = 0xBA800008;    // shared descriptor header -- desc len is 8; no sharing
    *((uint32_t*)ctx->sh_desc + i++) = 0xA80AFB04;    // Put SEQ-IN-Length into VSOL
    *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;    // SEQ FIFO STORE
    *((uint32_t*)ctx->sh_desc + i++) = 0xA82A4F04;    // MATH VSIL - imm -> No DEST (to set math size flags)
    *((uint32_t*)ctx->sh_desc + i++) = 0x00000FFF;    // immediate value with maximum permitted length of frame to copy.  I arbitrarily set this to 4095...this can go up to 65535.
    *((uint32_t*)ctx->sh_desc + i++) = 0xA0C108F1;    // HALT with status if the length of the input frame (as in VSIL) is bigger than the length in the immediate value
    *((uint32_t*)ctx->sh_desc + i++) = 0xA80AF004;    // MATH ADD VSIL + 0 -> MATH 0 (to put the length of the input frame into MATH 0)
    *((uint32_t*)ctx->sh_desc + i++) = 0x70820000;    // Move Length from Deco Alignment block to Output FIFO using length from MATH 0

    SEC_PDCP_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}
#endif // SEC_HW_VERSION_3_1
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

#ifdef SEC_HW_VERSION_3_1
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
    SEC_INFO("Update crypto descriptor return code = %d", ret);

    return ret;
}
#else // SEC_HW_VERSION_3_1
int sec_pdcp_context_update_descriptor(sec_context_t *ctx,
                                       sec_job_t *job,
                                       sec_descriptor_t *descriptor,
                                       uint8_t do_integrity_check)
{
    dma_addr_t  phys_addr;
    uint32_t    offset = 0;
    uint32_t    length = 0;
    int ret = SEC_SUCCESS;

    PDCP_INIT_JD(descriptor);
    PDCP_JD_SET_SD(descriptor,
                   job->sec_context->sh_desc_phys,
                   SEC_PDCP_GET_DESC_LEN(job->sec_context->sh_desc));

#if (SEC_ENABLE_SCATTER_GATHER == ON)
    if( SG_CONTEXT_OUT_TBL_EN(job->sg_ctx ))
    {
        PDCP_JD_SET_SG_OUT(descriptor);
        phys_addr = g_sec_vtop(SG_CONTEXT_GET_TBL_OUT(job->sg_ctx));
        offset = 0;
        length = SG_CONTEXT_GET_LEN_OUT(job->sg_ctx);
    }
    else
#endif // SEC_ENABLE_SCATTER_GATHER == ON
    {
        phys_addr = job->sec_context->out_pkt_vtop(job->out_packet->address);
        offset = job->out_packet->offset;
        length = job->out_packet->length;
    }

    PDCP_JD_SET_OUT_PTR(descriptor,
                        phys_addr,
                        offset,
                        length);

#if (SEC_ENABLE_SCATTER_GATHER == ON)
    if( SG_CONTEXT_IN_TBL_EN(job->sg_ctx ))
    {
        PDCP_JD_SET_SG_IN(descriptor);
        phys_addr = g_sec_vtop(SG_CONTEXT_GET_TBL_IN(job->sg_ctx));
        offset = 0;
        length = SG_CONTEXT_GET_LEN_IN(job->sg_ctx);
    }
    else
#endif // (SEC_ENABLE_SCATTER_GATHER == ON)
    {
        phys_addr = job->sec_context->in_pkt_vtop(job->in_packet->address);
        offset = job->in_packet->offset;
        length = job->in_packet->length;
    }

    PDCP_JD_SET_IN_PTR(descriptor,
                       phys_addr,
                       offset,
                       length);

/* In order to be compatible with QI scenarios, the DPOVRD value loaded
 * must be formated like this:
 * HFN_Ov_En(1b) | Res(1b) | HFN Value (right aligned)
 */
    descriptor->load_dpovrd.command.word = 0x16870004;   // ld: deco-povrd len=4 offs=0 imm
    if( job->sec_context->hfn_ov_en == TRUE)
    {
        descriptor->dpovrd = CMD_DPOVRD_HFN_OV_EN | job->hfn_ov_value;
    }
    else
    {
        descriptor->dpovrd = 0x00000000;
    }

    SEC_PDCP_DUMP_DESC(descriptor);

    SEC_INFO("Update crypto descriptor return code = %d", ret);

    return ret;
}

/** Static array for selection of the function to be used for creating the shared descriptor on a SEC context,
 * depending on the algorithm selected by the user. This array is used for PDCP Control plane where both
 * authentication and encryption is required.
 */
static create_desc_fp c_plane_create_desc[NUM_CIPHER_ALGS][NUM_INT_ALGS] = {
        /*              NULL                                SNOW                                    AES */
        /* NULL */{create_null_desc,                  create_c_plane_auth_only_desc,      create_c_plane_auth_only_desc     },
        /* SNOW */{create_c_plane_cipher_only_desc,   create_c_plane_hw_acc_desc,         create_c_plane_mixed_desc         },
        /* AES  */{create_c_plane_cipher_only_desc,   create_c_plane_mixed_desc,          create_c_plane_hw_acc_desc        },
};

/** Static array for selection of the function to be used for creating the shared descriptor on a SEC context,
 * depending on the algorithm selected by the user. This array is used for PDCP User plane where only
 * encryption is required.
 */
static create_desc_fp u_plane_create_desc[NUM_CIPHER_ALGS] ={
        /*      NULL                   SNOW                     AES */
        create_null_desc,   create_u_plane_hw_acc_desc,   create_u_plane_hw_acc_desc
};

#endif // SEC_HW_VERSION_3_1
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
