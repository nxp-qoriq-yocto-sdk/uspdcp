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
#ifdef USDPAA
#include <flib/protoshared.h>
#else
#include "sec_pdcp.h"
#endif
#include "sec_contexts.h"
#include "sec_job_ring.h"
#include "sec_hw_specific.h"
#include "sec_utils.h"


/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
#ifndef USDPAA
/** Define for use in the array of descriptor creating functions. This is the number of
 * rows in the array
 */
#define NUM_CIPHER_ALGS sizeof(sec_crypto_alg_t)

/** Define for use in the array of descriptor creating functions. This is the number of
 * columns in the array
 */
#define NUM_INT_ALGS sizeof(sec_crypto_alg_t)
#endif
/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
#ifndef USDPAA
/** Typedef for function pointer forcreating a shared descriptor on a context */
typedef int (*create_desc_fp)(sec_context_t *ctx);

/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
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
#endif
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
#ifndef USDPAA
/** @brief Fill PDB with initial data for this context: HFN threshold, HFN mask, etc
 * @param [in,out] ctx          SEC context
 */
static void sec_pdcp_create_pdb(sec_context_t *ctx);
#endif
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
#ifdef USDPAA
static int sec_pdcp_context_create_descriptor(sec_context_t *ctx)
{
	unsigned int bufsize;
	const sec_pdcp_context_info_t *pdcp_crypto_info;
	struct alginfo cipher, integrity;
	uint32_t *descbuf;

	ASSERT(ctx != NULL);
	ASSERT(ctx->crypto_info.pdcp_crypto_info != NULL);

	pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;
	memset(&cipher, 0x00, sizeof(struct alginfo));
	memset(&integrity, 0x00, sizeof(struct alginfo));

	cipher.algtype = pdcp_crypto_info->cipher_algorithm;
	cipher.key = g_sec_vtop(pdcp_crypto_info->cipher_key);
	cipher.keylen = pdcp_crypto_info->cipher_key_len;

	integrity.algtype = pdcp_crypto_info->integrity_algorithm;
	integrity.key = g_sec_vtop(pdcp_crypto_info->integrity_key);
	integrity.keylen = pdcp_crypto_info->integrity_key_len;

	descbuf = (uint32_t*)ctx->sh_desc;
	switch (pdcp_crypto_info->user_plane) {
	case PDCP_CONTROL_PLANE:
		if (pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
			cnstr_shdsc_pdcp_c_plane_encap(descbuf,
					&bufsize,
					1,
					pdcp_crypto_info->hfn,
					pdcp_crypto_info->bearer,
					pdcp_crypto_info->packet_direction,
					pdcp_crypto_info->hfn_threshold,
					&cipher,
					&integrity,
					0);
		else
			cnstr_shdsc_pdcp_c_plane_decap(descbuf,
					&bufsize,
					1,
					pdcp_crypto_info->hfn,
					pdcp_crypto_info->bearer,
					pdcp_crypto_info->packet_direction,
					pdcp_crypto_info->hfn_threshold,
					&cipher,
					&integrity,
					0);
		break;
	case PDCP_DATA_PLANE:
		if (pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
			cnstr_shdsc_pdcp_u_plane_encap(descbuf,
					&bufsize,
					1,
					pdcp_crypto_info->sn_size,
					pdcp_crypto_info->hfn,
					pdcp_crypto_info->bearer,
					pdcp_crypto_info->packet_direction,
					pdcp_crypto_info->hfn_threshold,
					&cipher,
					0);
		else
			cnstr_shdsc_pdcp_u_plane_decap(descbuf,
					&bufsize,
					1,
					pdcp_crypto_info->sn_size,
					pdcp_crypto_info->hfn,
					pdcp_crypto_info->bearer,
					pdcp_crypto_info->packet_direction,
					pdcp_crypto_info->hfn_threshold,
					&cipher,
					0);
		break;
	default:
		SEC_ERROR("Context [%p] invalid user/control plane indication",
			  ctx);
		return SEC_INVALID_INPUT_PARAM;
	}

	SEC_DUMP_DESC(descbuf);
	return SEC_SUCCESS;
}
#else
static void sec_pdcp_create_pdb(sec_context_t *ctx)
{
    sec_pdcp_pdb_t *sec_pdb;
    struct cplane_pdb_s *cplane_pdb = NULL;
    struct uplane_pdb_s *uplane_pdb = NULL;
    const sec_pdcp_context_info_t *pdcp_crypto_info;

    ASSERT( ctx != NULL );
    ASSERT( ctx->crypto_info.pdcp_crypto_info != NULL);
    ASSERT( ctx->sh_desc != NULL);

    pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;

    sec_pdb = &((struct sec_pdcp_sd_t*)ctx->sh_desc)->pdb;
    memset(sec_pdb, 0x00, sizeof(sec_pdcp_pdb_t));

    if (pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE)
    {
        cplane_pdb = &sec_pdb->pdb_content.cplane_pdb;

        ASSERT(pdcp_crypto_info->sn_size == SEC_PDCP_SN_SIZE_5 );
#warning "This seems to be an issue in HW, bit 30 should be set to 0 for C-Plane"
        cplane_pdb->res1 = 0x00000002;
        cplane_pdb->hfn = pdcp_crypto_info->hfn;
        cplane_pdb->bearer = pdcp_crypto_info->bearer;
        cplane_pdb->dir = pdcp_crypto_info->packet_direction;
        cplane_pdb->threshold = pdcp_crypto_info->hfn_threshold;
    }
    else
    {// data plane
        uplane_pdb = &sec_pdb->pdb_content.uplane_pdb;
        uplane_pdb->bearer = pdcp_crypto_info->bearer;
        uplane_pdb->dir = pdcp_crypto_info->packet_direction;

        if( pdcp_crypto_info->sn_size ==  SEC_PDCP_SN_SIZE_12 )
        {
            uplane_pdb->sns = 0;
            uplane_pdb->hfn.hfn_l.hfn = pdcp_crypto_info->hfn;
            uplane_pdb->threshold.threshold_l.threshold = pdcp_crypto_info->hfn_threshold;
        }
        else
        {
            uplane_pdb->sns = 1;
            uplane_pdb->hfn.hfn_s.hfn = pdcp_crypto_info->hfn;
            uplane_pdb->threshold.threshold_s.threshold = pdcp_crypto_info->hfn_threshold;
        }
    }
}

static int sec_pdcp_context_create_descriptor(sec_context_t *ctx)
{
    const sec_pdcp_context_info_t *pdcp_crypto_info;
    int ret = SEC_SUCCESS;

    ASSERT(ctx != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info != NULL);

    pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;

    // Create a Protocol Data Blob (PDB)
    sec_pdcp_create_pdb(ctx);

    ret = (pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE) ?             \
            (c_plane_create_desc[pdcp_crypto_info->cipher_algorithm]        \
                          [pdcp_crypto_info->integrity_algorithm](ctx)) :    \
            (u_plane_create_desc[pdcp_crypto_info->cipher_algorithm](ctx));

    return ret;
}

static int create_c_plane_hw_acc_desc(sec_context_t *ctx)
{
    struct sec_pdcp_sd_t *pdcp_sd = NULL;
    const sec_pdcp_context_info_t *pdcp_crypto_info = NULL;

    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info != NULL);

    pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;

    ASSERT(pdcp_crypto_info->cipher_key != NULL);
    ASSERT(pdcp_crypto_info->integrity_key != NULL);

    pdcp_sd = (struct sec_pdcp_sd_t*)ctx->sh_desc;

    SEC_INFO("Creating C-PLANE HW Acc. descriptor w/alg %s & auth alg %s",
              pdcp_crypto_info->cipher_algorithm == SEC_ALG_AES ? "AES" : "SNOW",
              pdcp_crypto_info->cipher_algorithm == SEC_ALG_AES ? "AES" : "SNOW");

    SEC_PDCP_INIT_CPLANE_SD(pdcp_sd);

    /* Plug-in the HFN override in descriptor from DPOVRD */
    
    pdcp_sd->hfn_ov_desc[0] = 0xAC574F08;
    pdcp_sd->hfn_ov_desc[1] = 0x80000000;
    
    pdcp_sd->hfn_ov_desc[2] = 0xA0000405;
    
    pdcp_sd->hfn_ov_desc[3] = 0xA8774004;
    pdcp_sd->hfn_ov_desc[4] = 0x00000005;
    pdcp_sd->hfn_ov_desc[5] = 0xA8900008;
    pdcp_sd->hfn_ov_desc[6] = 0x78430804;

    SEC_PDCP_SD_SET_KEY2(pdcp_sd,
                         pdcp_crypto_info->integrity_key,
                         pdcp_crypto_info->integrity_key_len);

    SEC_PDCP_SD_SET_KEY1(pdcp_sd,
                         pdcp_crypto_info->cipher_key,
                         pdcp_crypto_info->cipher_key_len);

    SEC_PDCP_SD_SET_PROT_DIR(pdcp_sd,
                             pdcp_crypto_info->protocol_direction);

    // Doesn't matter which alg, since they're the same in this case
    SEC_PDCP_SD_SET_PROT_ALG(pdcp_sd,pdcp_crypto_info->cipher_algorithm);

    SEC_DUMP_DESC(ctx->sh_desc);
    return SEC_SUCCESS;
}

static int create_u_plane_hw_acc_desc(sec_context_t *ctx)
{
    struct sec_pdcp_sd_t *pdcp_sd = NULL;
    const sec_pdcp_context_info_t *pdcp_crypto_info = NULL;

    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info->cipher_key != NULL);

    pdcp_sd = (struct sec_pdcp_sd_t*)ctx->sh_desc;
    pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;

    SEC_INFO("Creating U-PLANE HW Acc. descriptor w/alg %s",
              pdcp_crypto_info->cipher_algorithm == SEC_ALG_AES ? "AES" : "SNOW");

    SEC_PDCP_INIT_UPLANE_SD(pdcp_sd);

    /* Plug-in the HFN override in descriptor from DPOVRD */
    pdcp_sd->hfn_ov_desc[0] = 0xAC574F08;
    pdcp_sd->hfn_ov_desc[1] = 0x80000000;
    pdcp_sd->hfn_ov_desc[2] = 0xA0000405;
    pdcp_sd->hfn_ov_desc[3] = 0xA8774004;

    /* I avoid to do complicated things in CAAM, thus I 'hardcode'
     * the operations to be done on the HFN as per the SN size. Doing
     * a generic descriptor that would look at the PDB and then decide
     * on the actual values to shift would have made the descriptors too
     * large and slow [ if-then-else construct in CAAM means 2 JUMPS ]
     */
    if(pdcp_crypto_info->sn_size == SEC_PDCP_SN_SIZE_7)
    {
        pdcp_sd->hfn_ov_desc[4] = 0x00000007;
    }
    else
    {
        pdcp_sd->hfn_ov_desc[4] = 0x0000000C;
    }
    pdcp_sd->hfn_ov_desc[5] = 0xA8900008;
    pdcp_sd->hfn_ov_desc[6] = 0x78430804;

    SEC_PDCP_SD_SET_KEY1(pdcp_sd,
                         pdcp_crypto_info->cipher_key,
                         pdcp_crypto_info->cipher_key_len);

    SEC_PDCP_SD_SET_PROT_DIR(pdcp_sd,
                             pdcp_crypto_info->protocol_direction);

    SEC_PDCP_SD_SET_PROT_ALG(pdcp_sd,pdcp_crypto_info->cipher_algorithm);

    SEC_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int create_c_plane_mixed_desc(sec_context_t *ctx)
{
    int i = 1;
    const sec_pdcp_context_info_t *pdcp_crypto_info = NULL;

    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info->cipher_key != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info->integrity_key != NULL);

    pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;

    ((struct sec_pdcp_sd_t*)ctx->sh_desc)->deschdr.command.word  = 0xB8830100;

    /* Copy HFN from PDB to the beginning of the descriptor */
    *((uint32_t*)ctx->sh_desc + i++) = *((uint32_t*)ctx->sh_desc + 2);
    /* Copy Bearer and Direction from PDB to the beginning of the descriptor */
    *((uint32_t*)ctx->sh_desc + i++) = *((uint32_t*)ctx->sh_desc + 3);
    
    /* Plug-in the HFN override in descriptor from DPOVRD */    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC574F08;
    *((uint32_t*)ctx->sh_desc + i++) = 0x80000000;
    *((uint32_t*)ctx->sh_desc + i++) = 0xA0000405;

    *((uint32_t*)ctx->sh_desc + i++) = 0xA8774004;
    *((uint32_t*)ctx->sh_desc + i++) = 0x00000005;
        
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8900008;
    
    *((uint32_t*)ctx->sh_desc + i++) = 0x78430404;

    switch(pdcp_crypto_info->integrity_algorithm )
    {
        case SEC_ALG_SNOW:
            SEC_INFO(" Creating AES CTR/SNOW f9 descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    pdcp_crypto_info->cipher_key_len; // key1, len = cipher_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
            {
                dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->cipher_key);
                *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
            }
#else
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->cipher_key);
#endif
            *((uint32_t*)ctx->sh_desc + i++) = 0x04000000 |
                    pdcp_crypto_info->integrity_key_len;  // key2, len = integrity_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
            {
                dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->integrity_key);
                *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
            }
#else
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->integrity_key);
#endif

            *((uint32_t*)ctx->sh_desc + i++) = 0x1e080701;      // seq load, class 3, offset 7, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm

            //*((uint32_t*)ctx->sh_desc + i++) = 0xF0200001;      // seqinptr: len=1 rto
            *((uint32_t*)ctx->sh_desc + i++) = 0x78490701;      // move: math0+7 -> class2-alnblk, len=1

            *((uint32_t*)ctx->sh_desc + i++) = 0xac504108;      // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;      // M1 = M0 & 0x00000000_0000001f

            *((uint32_t*)ctx->sh_desc + i++) = 0x5e080701;      // seq store, class 3, offset 7, length = 1, src=m0

            *((uint32_t*)ctx->sh_desc + i++) = 0xA8911108;      // math: (<math1> shld math1)->math1 len=8
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79370404;      // move: descbuf+4[01] -> math3, len=4
            *((uint32_t*)ctx->sh_desc + i++) = 0xA8231F08;      // math: (math3 - math1)->none len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0xA0010805;      // jump: jsl0 all-mismatch[math-n] offset=5 local->[06]
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360408;      // move: descbuf+4[01] -> math2, len=8
            *((uint32_t*)ctx->sh_desc + i++) = 0xAC024208;      // math: (math2 + imm1)->math2 len=4, ifb
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000020;      // imm1=32
            *((uint32_t*)ctx->sh_desc + i++) = 0x79630408;      // move: math2 -> descbuf+4[01], len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0x78530404;      // move: math1 -> descbuf+4[01], len=4
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360408;      // move from desc buf to math2, wc=1,offset = 4, len 8

            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412108;      // M1 = M2 | M1
            *((uint32_t*)ctx->sh_desc + i++) = 0x78501008;      // M1 -> class1 ctx, len 8, offset 16
            *((uint32_t*)ctx->sh_desc + i++) = 0x78510004;      // M1 -> class2 ctx, len 4, offset 0

            *((uint32_t*)ctx->sh_desc + i++) = 0xa8514204;      // Mask "Bearer" bit in IV, put result in M2
            *((uint32_t*)ctx->sh_desc + i++) = 0x04000000;      //

            *((uint32_t*)ctx->sh_desc + i++) = 0xa8514304;      // Mask "Direction" bit in IV, put result in M3
            *((uint32_t*)ctx->sh_desc + i++) = 0xF8000000;      //

            *((uint32_t*)ctx->sh_desc + i++) = 0xA8933308;      // "Rotate" M3 into position for SNOW f9 IV

            *((uint32_t*)ctx->sh_desc + i++) = 0x7862040C;      // Put SNOW f9 IV in OFIFO since there's no way to 
                                                                // specifiy two offset in the MOVE command (for both source
                                                                // and destination: move: math2+4 -> ofifo, len=12
            *((uint32_t*)ctx->sh_desc + i++) = 0x7821040C;      // move: ofifo -> class2-ctx+4, len=12

            if(pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xA8284104;  // M1 = SIL-
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;  // 4
                
                *((uint32_t*)ctx->sh_desc + i++) = 0xA821FB04;  // VSOL = M1
                
                *((uint32_t*)ctx->sh_desc + i) = 0x79350006
                                                        | (((i + 5)*4) << 8);
                i++;

                *((uint32_t*)ctx->sh_desc + i) = 0x79530008 
                                                        | (((i + 4)*4) << 8);
                i++;
                
                *((uint32_t*)ctx->sh_desc + i++) = 0x69B00000;  // seqfifostr: msgdata len=vsol, cont
            }
            else
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xA8084B04;  // VSOL = SIL+
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;  // 4
                
                *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;  // seqfifostr: msgdata len=vsol
            }

            *((uint32_t*)ctx->sh_desc + i++) = 0xA828FA04;  // VSIL = SIL

            *((uint32_t*)ctx->sh_desc + i++) = 0x84A00C8C |
                    ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                                           0 : CMD_ALGORITHM_ICV ); // operation: optype = 4 (class 2), alg = 0x60 (snow), aai = 0xC8 (f9), as = 11 (int/fin), icv = 1, enc = 0

            *((uint32_t*)ctx->sh_desc + i++) = 0x8210000c | \
                    ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                       CMD_ALGORITHM_ENCRYPT : 0 );  // operation: optype = 2 (class 1), alg = 0x10 (aes), aai = 0x00 (ctr), as = 11 (int/fin), icv = 0, enc = 1
            
            if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2F140000;      // seqfifold: both msg-last2 vlf

                *((uint32_t*)ctx->sh_desc + i++) = 0x7E180004;      // move: class2-ctx+0 -> class1-alnblk, len=4 a_ms=flush a_ls=last
            }
            else
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2E1C0000;      // seqfifold: both msg1->2-last2 len=0
                
                *((uint32_t*)ctx->sh_desc + i++) = 0x2A130004;      // Decrypt the  ICV. Due to continuation bit
                                                                    // set in SEQSTORE command above, the last 4 bytes
                                                                    // won't be flushed.
                                                                    // seqfifold: class1 msg-last1-flush1 len=4
#if 0
                *((uint32_t*)ctx->sh_desc + i++) = 0x79250004;      // move: ofifo -> math0+0, len=4 wait
                *((uint32_t*)ctx->sh_desc + i++) = 0x10FA0004;      // ld: ind-nfsl len=4 offs=0 imm
                *((uint32_t*)ctx->sh_desc + i++) = 0xA0A00004;      //     <nfifo_entry: ififo->class2 type=icv lc2 len=4>

                *((uint32_t*)ctx->sh_desc + i++) = 0x795A0004;       // move: math0+0 -> ififo, len=4 wait
#else
                // *((uint32_t*)ctx->sh_desc + i++) = 0x782A0004;
                // *((uint32_t*)ctx->sh_desc + i++) = 0x10FA0004;
                // *((uint32_t*)ctx->sh_desc + i++) = 0xA0A00004;
                *((uint32_t*)ctx->sh_desc + i++) = 0xA3000601;
                *((uint32_t*)ctx->sh_desc + i++) = 0x79240004;
                *((uint32_t*)ctx->sh_desc + i++) = 0x10fa0004;
                *((uint32_t*)ctx->sh_desc + i++) = 0xa0a00004;
                *((uint32_t*)ctx->sh_desc + i++) = 0x794a0004;
#endif
            }
            break;
        case SEC_ALG_AES:
            SEC_INFO("Creating SNOW f8/AES CMAC descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x1e080701;      // seq load, class 3, offset 7, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm
            
            if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xF0200001;      // seqinptr: len=1 rto
            }

            *((uint32_t*)ctx->sh_desc + i++) = 0xac504108;      // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;      // M1 = M0 & 0x00000000_0000001f

            *((uint32_t*)ctx->sh_desc + i++) = 0xA8911108;      // math: (<math1> shld math1)->math1 len=8

#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79370404;      // move: descbuf+4[01] -> math3, len=4
            *((uint32_t*)ctx->sh_desc + i++) = 0xA8231F08;      // math: (math3 - math1)->none len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0xA0010805;      // jump: jsl0 all-mismatch[math-n] offset=5 local->[06]
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360408;      // move: descbuf+4[01] -> math2, len=8
            *((uint32_t*)ctx->sh_desc + i++) = 0xAC024208;      // math: (math2 + imm1)->math2 len=4, ifb
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000020;      // imm1=32
            *((uint32_t*)ctx->sh_desc + i++) = 0x79630408;      // move: math2 -> descbuf+4[01], len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0x78530404;      // move: math1 -> descbuf+4[01], len=4
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360408;      // move from desc buf to math2, wc=1,offset = 4, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;      // M2 = M2 | M1
            
            *((uint32_t*)ctx->sh_desc + i++) = 0x5e080701;     // seq store, class 3, offset 7, length = 1, src=m0

            if(pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION)
            {
            
                *((uint32_t*)ctx->sh_desc + i++) = 0x78600008;      //  move: math2 -> class1-ctx+0, len=8
                *((uint32_t*)ctx->sh_desc + i++) = 0x78010008;      //  move: class1-ctx+0 -> class2-ctx, len=8

                *((uint32_t*)ctx->sh_desc + i++) = 0xA828FA04;      // VSIL = SIL
                *((uint32_t*)ctx->sh_desc + i++) = 0xA8284B04;      // VSOL = SIL -
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;      // 0x04

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                *((uint32_t*)ctx->sh_desc + i++) = 0x168B0004;

                *((uint32_t*)ctx->sh_desc + i++) = 0xA0000000 | (uint8_t)(-14); // jump: all-match[] always-jump

                *((uint32_t*)ctx->sh_desc + i++) = 0x79350014 | 
                                                (51 * 4 << 8);   // Read SEQ OUT, SEQ OUT, PTR & EXT LEN in M1,M2 & M3_HI

#else
                *((uint32_t*)ctx->sh_desc + i) = 0x78370008 | 
                                                   (((i + 16) * 4) << 8); //  move: descbuf -> math3, len = 8
                i++;

                *((uint32_t*)ctx->sh_desc + i++) = 0x79350010 | 
                                                (48 * 4 << 8);   // Read SEQ OUT, SEQ OUT, PTR & EXT LEN in M1,M2
#endif
                *((uint32_t*)ctx->sh_desc + i++) = 0xa8614104;   // change SEQ OUT to SEQ IN
                *((uint32_t*)ctx->sh_desc + i++) = 0x08000000;

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                *((uint32_t*)ctx->sh_desc + i++) = 0x78530018 | 
                                                (51 * 4 << 8);   // Overwrite SEQ OUT PTR & EXT LEN in JD
#else
                *((uint32_t*)ctx->sh_desc + i++) = 0x78530014 | 
                                                (48 * 4 << 8);   // Overwrite SEQ OUT PTR & EXT LEN in JD
#endif

                *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    pdcp_crypto_info->cipher_key_len;       // key1, len = cipher_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                {
                    dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->cipher_key);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
                }
#else
                *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->cipher_key);
#endif

                *((uint32_t*)ctx->sh_desc + i++) = 0x82600c0c | \
                        ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                        CMD_ALGORITHM_ENCRYPT : 0 );      // operation, optype = 2 (class 1), alg = 0x60 (snow), aai = 0xC0 (f8), as = 11 (int/fin), icv = 0, enc = 0

                *((uint32_t*)ctx->sh_desc + i++) = 0x69B00000;      // seqfifostr: msg len=vsol cont

                *((uint32_t*)ctx->sh_desc + i++) = 0x2B130000;      // seqfifold: class1 msg-last1-flush1 vlf

                *((uint32_t*)ctx->sh_desc + i++) = 0x78270004;      // Save decrypted ICV: move: ofifo -> math3+0, len=4

                // Here ends SNOW f8 processing
                *((uint32_t*)ctx->sh_desc + i++) = 0x10880004;
                *((uint32_t*)ctx->sh_desc + i++) = 0x2000006D;

                *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                        pdcp_crypto_info->integrity_key_len;   // key1, len = integrity_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                {
                    dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->integrity_key);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
                }
#else
                *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->integrity_key);
#endif

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                *((uint32_t*)ctx->sh_desc + i++) = 0xA000000B; // jump: all-match[] always-jump offset=11
#else
                *((uint32_t*)ctx->sh_desc + i++) = 0xA000000B; // jump: all-match[] always-jump offset=10
                
                /* Non-reachable code. Needed here because load imm @ offset 4 in M3 doesn't work. */
                *((uint32_t*)ctx->sh_desc + i++) = 0xA0000000 | (uint8_t)(-12); // jump: all-match[] always-jump
#endif

                *((uint32_t*)ctx->sh_desc + i++) = 0x8210060C | \
                                                   ( (pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                                                   0 : CMD_ALGORITHM_ICV );         // operation: optype = 2 (class 1), alg = 0x10 (aes), aai = 0x60 (cmac), as = 11 (int/fin), icv = 0, enc = 0 (ignored)

                *((uint32_t*)ctx->sh_desc + i++) = 0xA828FA04;      // VSIL = SIL

                *((uint32_t*)ctx->sh_desc + i++) = 0x78180008;      //  move: class2-ctx+0 -> class1-alnblk, len=8

                *((uint32_t*)ctx->sh_desc + i++) = 0x2B130000;      // seqfifold: class1 msg-last1-flush1 vlf

                *((uint32_t*)ctx->sh_desc + i++) = 0x10F00004;      // ld: ind-nfsl len=4 offs=0 imm
                *((uint32_t*)ctx->sh_desc + i++) = 0x54A04004;      //      <nfifo_entry: altsource->class1 type=icv/rsvd0A lc1 fc1 ast len=4>
                *((uint32_t*)ctx->sh_desc + i++) = 0x787F0004;      // move: math3+0 -> altsource, len=4
            }
            else
            {
            
                *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                        pdcp_crypto_info->integrity_key_len;       // key1, len = integrity_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                {
                    dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->integrity_key);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
                }
#else
                *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->integrity_key);
#endif

                *((uint32_t*)ctx->sh_desc + i++) = 0x78680008;      // move math2 to class1 input fifo(IV=64bits)
                
                *((uint32_t*)ctx->sh_desc + i++) = 0xA828FA04;      // VSIL = SIL-0x00
                *((uint32_t*)ctx->sh_desc + i++) = 0xA80A4B04;      // VSOL = VSIL +
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000003;      // 0x03

                *((uint32_t*)ctx->sh_desc + i++) = 0xA828F104;      // M1 = SIL - 0x00

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                *((uint32_t*)ctx->sh_desc + i) = 0x78350006
                                                        | (((i + 10)*4) << 8);
#else
                *((uint32_t*)ctx->sh_desc + i) = 0x78350006
                                                        | (((i + 9)*4) << 8);
#endif
                i++;

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                *((uint32_t*)ctx->sh_desc + i) = 0x79530008
                                                        | (((i + 9)*4) << 8);
#else
                *((uint32_t*)ctx->sh_desc + i) = 0x79530008
                                                        | (((i + 8)*4) << 8);
#endif
                i++;

                *((uint32_t*)ctx->sh_desc + i++) = 0x8210060C | \
                                                   ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                                                   0 : CMD_ALGORITHM_ICV );         // operation: optype = 2 (class 1), alg = 0x10 (aes), aai = 0x60 (cmac), as = 11 (int/fin), icv = 0, enc = 0 (ignored)
                                                       
                *((uint32_t*)ctx->sh_desc + i++) = 0x2B130000;      // seqfifold: class1 msg-last1-flush1 vlf

                // Save the MAC-I for future encryption
                *((uint32_t*)ctx->sh_desc + i++) = 0x78070004;      //  move: class1-ctx+0 -> math3, len=4

                // Here ends the AES-CMAC processsing.
                *((uint32_t*)ctx->sh_desc + i++) = 0x10880004;
                *((uint32_t*)ctx->sh_desc + i++) = 0x2000006D;

                *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    pdcp_crypto_info->cipher_key_len;       // key1, len = cipher_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                {
                    dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->cipher_key);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
                }
#else
                *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->cipher_key);
#endif

                *((uint32_t*)ctx->sh_desc + i++) = 0x78600008;      //  move: math2 -> class1-ctx+0, len=8

                *((uint32_t*)ctx->sh_desc + i++) = 0xF0200000;      // seqinptr: len=X rto (restart from beginning, length is replaced above)

                *((uint32_t*)ctx->sh_desc + i++) = 0xA828CA04;      // VSIL = SIL-0x01
                
                *((uint32_t*)ctx->sh_desc + i++) = 0x82600c0c | \
                        ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                        CMD_ALGORITHM_ENCRYPT : 0 );      // operation, optype = 2 (class 1), alg = 0x60 (snow), aai = 0xC0 (f8), as = 11 (int/fin), icv = 0, enc = 0

                *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;      // seq fifo store, vlf
                *((uint32_t*)ctx->sh_desc + i++) = 0x28000001;      // seqfifold: skip len=1
                *((uint32_t*)ctx->sh_desc + i++) = 0x2B100000;      // seqfifold: class1 msg vlf

                *((uint32_t*)ctx->sh_desc + i++) = 0x10F00004;      // ld: ind-nfsl len=4 offs=0 imm
                *((uint32_t*)ctx->sh_desc + i++) = 0x54F04004;      //      <nfifo_entry: altsource->class1 type=msg/rsvd0F lc1 fc1 ast len=4>
                *((uint32_t*)ctx->sh_desc + i++) = 0x787F0004;      // move: math3+0 -> altsource, len=4
            }

            break;
        default:
            SEC_INFO(" Unknown integrity algorithm requested: %d", pdcp_crypto_info->integrity_algorithm);
            ASSERT(0);
            break;
    }
    
    
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
    *((uint32_t*)ctx->sh_desc + i++) = 0x78340408;      // move: descbuf+8[01] -> math0, len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0x79350C08;      // move: descbuf+16[04] -> math1, len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8210104;      // math: (math1 - math0)->none len=8
    
    *((uint32_t*)ctx->sh_desc + i++) = 0x56420104;      // str: deco-shrdesc+1 len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm

    *((uint32_t*)ctx->sh_desc + i++) = 0xA0C20CF1;      // jump: jsl0 any-match[math-n,math-z] halt-user status=241
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
    // update descriptor length
    ((struct sec_pdcp_sd_t*)ctx->sh_desc)->deschdr.command.sd.desclen = i;

    SEC_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int create_c_plane_auth_only_desc(sec_context_t *ctx)
{
    const sec_pdcp_context_info_t *pdcp_crypto_info = NULL;
    int i = 5;
    
    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info->integrity_key != NULL);

    ((struct sec_pdcp_sd_t*)ctx->sh_desc)->deschdr.command.word  = 0xB8851100;
    pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;

    /* Plug-in the HFN override in descriptor from DPOVRD */    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC574F08;
    *((uint32_t*)ctx->sh_desc + i++) = 0x80000000;
    *((uint32_t*)ctx->sh_desc + i++) = 0xA0000405;

    *((uint32_t*)ctx->sh_desc + i++) = 0xA8774004;
    *((uint32_t*)ctx->sh_desc + i++) = 0x00000005;
        
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8900008;
    
    *((uint32_t*)ctx->sh_desc + i++) = 0x78430804;
    
    switch( pdcp_crypto_info->integrity_algorithm )
    {
        case SEC_ALG_SNOW:
            SEC_INFO(" Creating NULL/SNOW f9 descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x04000000 |
                    pdcp_crypto_info->integrity_key_len;  // key2, len = integrity_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                {
                    dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->integrity_key);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
                }
#else
                *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->integrity_key);
#endif

            *((uint32_t*)ctx->sh_desc + i++) = 0x1e080701;      // seq load, class 3, offset 7, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm

            *((uint32_t*)ctx->sh_desc + i++) = 0xF0200001;      // seqinptr: len=1 rto

            *((uint32_t*)ctx->sh_desc + i++) = 0xac504108;      // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;      // M1 = M0 & 0x00000000_0000001f

            *((uint32_t*)ctx->sh_desc + i++) = 0xA8911108;      // math: (<math1> shld math1)->math1 len=8
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79370404;      // move: descbuf+4[01] -> math3, len=4
            *((uint32_t*)ctx->sh_desc + i++) = 0xA8231F08;      // math: (math3 - math1)->none len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0xA0010805;      // jump: jsl0 all-mismatch[math-n] offset=5 local->[06]
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360408;      // move: descbuf+4[01] -> math2, len=8
            *((uint32_t*)ctx->sh_desc + i++) = 0xAC024208;      // math: (math2 + imm1)->math2 len=4, ifb
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000020;      // imm1=32
            *((uint32_t*)ctx->sh_desc + i++) = 0x79630408;      // move: math2 -> descbuf+4[01], len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0x78530404;      // move: math1 -> descbuf+4[01], len=4
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360808;      // move from desc buf to math2, wc=1,offset = 8, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8524208;      // Mask "Bearer" bit in IV
            *((uint32_t*)ctx->sh_desc + i++) = 0xffffffff;      //
            *((uint32_t*)ctx->sh_desc + i++) = 0x04000000;      //
            *((uint32_t*)ctx->sh_desc + i++) = 0x79370c04;      // move from desc buf to math3, wc=1,offset = 0x0C, len 4
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8534308;      // Mask "Direction" bit in IV
            *((uint32_t*)ctx->sh_desc + i++) = 0xf8000000;      //
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000000;      //

            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;      // M2 = M2 | M1

            *((uint32_t*)ctx->sh_desc + i++) = 0x7961000c;      // load, class2 ctx, math2, wc = 1, len 12, offset 0,

            if(pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION)
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

            *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;  // seqfifostr: msgdata len=vsol

            *((uint32_t*)ctx->sh_desc + i++) = 0x84A00C8C |
                    ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                                           0 : CMD_ALGORITHM_ICV ); // operation: optype = 4 (class 2), alg = 0x60 (snow), aai = 0xC8 (f9), as = 11 (int/fin), icv = 1, enc = 0

            *((uint32_t*)ctx->sh_desc + i++) = 0x72A20001;  // move_len: class1-alnblk -> ofifo, len=math1 (aux_ls)
            *((uint32_t*)ctx->sh_desc + i++) = 0x2F170000;  // seqfifold: both msgdata-last2-last1-flush1 vlf

            if(pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2C3C0004;  // seqfifold: class2 icv-last2 len=4
            }

            if (pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x5C200004;  // seqstr: ccb2 ctx len=4 offs=0
            }

            break;
        case SEC_ALG_AES:
            SEC_INFO(" Creating NULL/AES-CMAC descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    pdcp_crypto_info->integrity_key_len;     // key1, len = integrity_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
                {
                    dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->integrity_key);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                    *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
                }
#else
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->integrity_key);
#endif

            *((uint32_t*)ctx->sh_desc + i++) = 0x1e080701;      // seq load, class 3, offset 7, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm

            *((uint32_t*)ctx->sh_desc + i++) = 0xF0200001;     // seqinptr: len=1 rto

            *((uint32_t*)ctx->sh_desc + i++) = 0xac504108;      // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;      // M1 = M0 & 0x00000000_0000001f

            *((uint32_t*)ctx->sh_desc + i++) = 0xA8911108;      // math: (<math1> shld math1)->math1 len=8
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79370404;      // move: descbuf+4[01] -> math3, len=4
            *((uint32_t*)ctx->sh_desc + i++) = 0xA8231F08;      // math: (math3 - math1)->none len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0xA0010805;      // jump: jsl0 all-mismatch[math-n] offset=5 local->[06]
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360408;      // move: descbuf+4[01] -> math2, len=8
            *((uint32_t*)ctx->sh_desc + i++) = 0xAC024208;      // math: (math2 + imm1)->math2 len=4, ifb
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000020;      // imm1=32
            *((uint32_t*)ctx->sh_desc + i++) = 0x79630408;      // move: math2 -> descbuf+4[01], len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0x78530404;      // move: math1 -> descbuf+4[01], len=4
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360808;      // move from desc buf to math2, wc=1,offset = 8, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;      // M2 = M2 | M1

            *((uint32_t*)ctx->sh_desc + i++) = 0x78680008;      // move math2 to class1 input fifo(IV=64bits)

            if(pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION)
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

            *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;     // seq fifo store, vlf

            *((uint32_t*)ctx->sh_desc + i++) = 0x8210060C | \
                    ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                       0 : CMD_ALGORITHM_ICV );         // operation: optype = 2 (class 1), alg = 0x10 (aes), aai = 0x60 (cmac), as = 11 (int/fin), icv = 0, enc = 0 (ignored)

            *((uint32_t*)ctx->sh_desc + i++) = 0x74A20001;  // move_len: class2-alnblk -> ofifo, len=math1 (aux_ms)
            *((uint32_t*)ctx->sh_desc + i++) = 0x2F170000;  // seqfifold: both msgdata-last2-last1-flush1 vlf

            if(pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2A3B0004;  // seqfifold: class1 icv-last1-flush1 len=4
            }

            if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x5A200004;     // seqstr: ctx1 len=4 offs=0
            }

            break;
        default:
            SEC_INFO(" Unknown integrity algorithm requested: %d", pdcp_crypto_info->integrity_algorithm);
            ASSERT(0);
            break;
    }
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
    *((uint32_t*)ctx->sh_desc + i++) = 0x78340408;      // move: descbuf+8[01] -> math0, len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0x79350C08;      // move: descbuf+16[04] -> math1, len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8210104;      // math: (math1 - math0)->none len=8
    
    *((uint32_t*)ctx->sh_desc + i++) = 0x56420104;      // str: deco-shrdesc+1 len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm

    *((uint32_t*)ctx->sh_desc + i++) = 0xA0C20CF1;      // jump: jsl0 any-match[math-n,math-z] halt-user status=241
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
    // update descriptor length
    ((struct sec_pdcp_sd_t*)ctx->sh_desc)->deschdr.command.sd.desclen = i;

    SEC_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int create_c_plane_cipher_only_desc(sec_context_t *ctx)
{
    const sec_pdcp_context_info_t *pdcp_crypto_info = NULL;
    int i = 5;

    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info->cipher_key != NULL);

    pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;
    ((struct sec_pdcp_sd_t*)ctx->sh_desc)->deschdr.command.word = 0xB8851100;  // shared header, start idx = 5

    /* Plug-in the HFN override in descriptor from DPOVRD */    
    *((uint32_t*)ctx->sh_desc + i++) = 0xAC574F08;    // math: (povrd & imm1)->none len=8 ifb
    *((uint32_t*)ctx->sh_desc + i++) = 0x80000000;    // imm
    *((uint32_t*)ctx->sh_desc + i++) = 0xA0000405;    // jump: jsl0 all-match[math-z] offset=7 local->[32]
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8774004;    // math: (math0 << imm1)->math0 len=8 ifb
    *((uint32_t*)ctx->sh_desc + i++) = 0x00000005;    // imm
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8900008;    // math: (<math0> shld math0)->math0 len=8
    *((uint32_t*)ctx->sh_desc + i++) = 0x78430804;    // move: math0 -> descbuf+8[02], len=4

    switch( pdcp_crypto_info->cipher_algorithm )
    {
        case SEC_ALG_SNOW:
            SEC_INFO(" Creating SNOW f8/NULL descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    pdcp_crypto_info->cipher_key_len; // key1, len = cipher_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
            {
                dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->cipher_key);
                *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
            }
#else
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->cipher_key);
#endif

            *((uint32_t*)ctx->sh_desc + i++) = 0x1e080701;      // seq load, class 3, offset 7, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm

            *((uint32_t*)ctx->sh_desc + i++) = 0xac504108;      // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;      // M1 = M0 & 0x00000000_0000001f
            *((uint32_t*)ctx->sh_desc + i++) = 0x5e080701;      // seq store, class 3, offset 7, length = 1, src=m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xA8911108;      // math: (<math1> shld math1)->math1 len=8

#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79370404;      // move: descbuf+4[01] -> math3, len=4
            
            *((uint32_t*)ctx->sh_desc + i++) = 0xA8231F08;      // math: (math3 - math1)->none len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0xA0010805;      // jump: jsl0 all-mismatch[math-n] offset=5 local->[06]
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360408;      // move: descbuf+4[01] -> math2, len=8
            *((uint32_t*)ctx->sh_desc + i++) = 0xAC024208;      // math: (math2 + imm1)->math2 len=4, ifb
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000020;      // imm1=32
            *((uint32_t*)ctx->sh_desc + i++) = 0x79630408;      // move: math2 -> descbuf+4[01], len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0x78530404;      // move: math1 -> descbuf+4[01], len=4
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360808;      // move from desc buf to math2, wc=1,offset = 8, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;      // M2 = M2 | M1

            *((uint32_t*)ctx->sh_desc + i++) = 0x79600008;      // load, class1 ctx, math2, wc = 1, len 8, offset 0

            *((uint32_t*)ctx->sh_desc + i++) = 0xa828fa04;      // VSIL=SIL-0x00
            if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xa8084b04;      // VSOL=SIL+0x04
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;      // 
            }
            else
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xa8284b04;      // VSOL=SIL-0x04
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;      //
            }

            *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;         // SEQ FIFO STORE, vlf

            *((uint32_t*)ctx->sh_desc + i++) = 0x82600c0c | \
                    ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                         CMD_ALGORITHM_ENCRYPT : 0 );      // operation, optype = 2 (class 1), alg = 0x60 (snow), aai = 0xC0 (f8), as = 11 (int/fin), icv = 0, enc = 0

            if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
            {
               *((uint32_t*)ctx->sh_desc + i++) = 0x2B100000;      // seqfifold: class1 msgdata vlf
               *((uint32_t*)ctx->sh_desc + i++) = 0x22930004;      // fifold: class1 msgdata-last1-flush1 len=4 imm
               *((uint32_t*)ctx->sh_desc + i++) = 0x00000000;
            }
            else
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2B130000;      // seqfifold: class1 msgdata-last1-flush1 vlf
            }

            break;

        case SEC_ALG_AES:
            SEC_INFO(" Creating AES-CTR/NULL descriptor.");

            *((uint32_t*)ctx->sh_desc + i++) = 0x02000000 |
                    pdcp_crypto_info->cipher_key_len; // key1, len = cipher_key_len
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
            {
                dma_addr_t key_addr = g_sec_vtop(pdcp_crypto_info->cipher_key);
                *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_HI(key_addr);
                *((uint32_t*)ctx->sh_desc + i++) = PHYS_ADDR_LO(key_addr);
            }
#else
            *((uint32_t*)ctx->sh_desc + i++) = g_sec_vtop(pdcp_crypto_info->cipher_key);
#endif

            *((uint32_t*)ctx->sh_desc + i++) = 0x1e080701;      // seq load, class 3, offset 7, length = 1, dest = m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm

            *((uint32_t*)ctx->sh_desc + i++) = 0xac504108;      // retain SN only (5 bits)
            *((uint32_t*)ctx->sh_desc + i++) = 0x0000001f;      // M1 = M0 & 0x00000000_0000001f
            *((uint32_t*)ctx->sh_desc + i++) = 0x5e080701;      // seq store, class 3, offset 7, length = 1, src=m0
            *((uint32_t*)ctx->sh_desc + i++) = 0xA8911108;      // math: (<math1> shld math1)->math1 len=8
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79370404;      // move: descbuf+4[01] -> math3, len=4

            *((uint32_t*)ctx->sh_desc + i++) = 0xA8231F08;      // math: (math3 - math1)->none len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0xA0010805;      // jump: jsl0 all-mismatch[math-n] offset=5 local->[06]
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360408;      // move: descbuf+4[01] -> math2, len=8
            *((uint32_t*)ctx->sh_desc + i++) = 0xAC024208;      // math: (math2 + imm1)->math2 len=4, ifb
            *((uint32_t*)ctx->sh_desc + i++) = 0x00000020;      // imm1=32
            *((uint32_t*)ctx->sh_desc + i++) = 0x79630408;      // move: math2 -> descbuf+4[01], len=8

            *((uint32_t*)ctx->sh_desc + i++) = 0x78530404;      // move: math1 -> descbuf+4[01], len=4
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
            *((uint32_t*)ctx->sh_desc + i++) = 0x79360808;      // move from desc buf to math2, wc=1,offset = 8, len 8
            *((uint32_t*)ctx->sh_desc + i++) = 0xa8412208;      // M2 = M2 | M1

            *((uint32_t*)ctx->sh_desc + i++) = 0x79601010;      // load, class1 ctx, math2, wc = 1, len 16, offset 16

            *((uint32_t*)ctx->sh_desc + i++) = 0xa828fa04;      // VSIL=SIL-0x00
            if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xa8084b04;      // VSOL=SIL+0x04
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;      // 
            }
            else
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0xa8284b04;      // VSOL=SIL-0x04
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;      //
            }

            *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;  // SEQ FIFO STORE, vlf

            *((uint32_t*)ctx->sh_desc + i++) = 0x8210000c | \
                    ((pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) ? \
                       CMD_ALGORITHM_ENCRYPT : 0 );  // operation: optype = 2 (class 1), alg = 0x10 (aes), aai = 0x00 (ctr), as = 11 (int/fin), icv = 0, enc = 1
            
            if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2B100000;      // seqfifold: class1 msgdata vlf
                *((uint32_t*)ctx->sh_desc + i++) = 0x22930004;      // fifold: class1 msgdata-last1-flush1 len=4 imm
                *((uint32_t*)ctx->sh_desc + i++) = 0x00000000;
            }
            else
            {
                *((uint32_t*)ctx->sh_desc + i++) = 0x2b130000;  // seq fifo load, class1, vlf, LC1
            }

            break;
        default:
            SEC_INFO(" Unknown ciphering algorithm requested: %d", pdcp_crypto_info->cipher_algorithm);
            ASSERT(0);
            break;
    }
#ifdef UNDER_CONSTRUCTION_HFN_THRESHOLD
    *((uint32_t*)ctx->sh_desc + i++) = 0x78340408;      // move: descbuf+8[01] -> math0, len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0x79350C08;      // move: descbuf+16[04] -> math1, len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0xA8210104;      // math: (math1 - math0)->none len=8
    
    *((uint32_t*)ctx->sh_desc + i++) = 0x56420104;      // str: deco-shrdesc+1 len=4
    *((uint32_t*)ctx->sh_desc + i++) = 0xa1001001;      // wait for calm

    *((uint32_t*)ctx->sh_desc + i++) = 0xA0C20CF1;      // jump: jsl0 any-match[math-n,math-z] halt-user status=241
#endif // UNDER_CONSTRUCTION_HFN_THRESHOLD
    // update descriptor length
    ((struct sec_pdcp_sd_t*)ctx->sh_desc)->deschdr.command.sd.desclen = i;

    SEC_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}

static int create_u_plane_null_desc(sec_context_t *ctx)
{
    int i = 0;
    
    ASSERT(ctx != NULL);

    SEC_INFO("Creating U-PLANE/ descriptor with NULL alg.");

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

static int create_c_plane_null_desc(sec_context_t *ctx)
{
    const sec_pdcp_context_info_t *pdcp_crypto_info = NULL;
    int i = 0;
    
    ASSERT(ctx != NULL);
    ASSERT(ctx->sh_desc != NULL);
    ASSERT(ctx->crypto_info.pdcp_crypto_info != NULL);
    
    pdcp_crypto_info = ctx->crypto_info.pdcp_crypto_info;

    SEC_INFO("Creating C-Plane descriptor with NULL/NULL alg.");

    *((uint32_t*)ctx->sh_desc + i++) = 0xBA800300;    // shared descriptor header -- desc len TBC later; no sharing
    
    if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
    {
        *((uint32_t*)ctx->sh_desc + i++) = 0xA80A4B04;    // Put SEQ-IN-Length into VSOL
        *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;    // and add 4 to it (4 NULL bytes at the end)
    }
    else
    {
        *((uint32_t*)ctx->sh_desc + i++) = 0xA82A4B04;    // Put SEQ-IN-Length into VSOL
        *((uint32_t*)ctx->sh_desc + i++) = 0x00000004;    // and sub 4 to it (4 NULL bytes at the end)
    }
    *((uint32_t*)ctx->sh_desc + i++) = 0x69300000;    // SEQ FIFO STORE
    *((uint32_t*)ctx->sh_desc + i++) = 0xA82A4F04;    // MATH VSIL - imm -> No DEST (to set math size flags)
    *((uint32_t*)ctx->sh_desc + i++) = 0x00000FFF;    // immediate value with maximum permitted length of frame to copy.  I arbitrarily set this to 4095...this can go up to 65535.
    *((uint32_t*)ctx->sh_desc + i++) = 0xA0C108F1;    // HALT with status if the length of the input frame (as in VSIL) is bigger than the length in the immediate value
    *((uint32_t*)ctx->sh_desc + i++) = 0xA80AF004;    // MATH ADD VSIL + 0 -> MATH 0 (to put the length of the input frame into MATH 0)
    *((uint32_t*)ctx->sh_desc + i++) = 0x70820000;    // Move Length from Deco Alignment block to Output FIFO using length from MATH 0

    if(pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
    {
        *((uint32_t*)ctx->sh_desc + i++) = 0xA8600008;    // Clear M0 (M0 XOR M0 = 0)
        *((uint32_t*)ctx->sh_desc + i++) = 0x78420004;    // Move 4 bytes of 0 to output FIFO
    }

    ((struct sec_pdcp_sd_t*)ctx->sh_desc)->deschdr.command.sd.desclen = i;
    SEC_DUMP_DESC(ctx->sh_desc);

    return SEC_SUCCESS;
}
#endif
/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/
int sec_pdcp_context_set_crypto_info(sec_context_t *ctx,
                                     const sec_pdcp_context_info_t *crypto_info)
{
    int ret = SEC_SUCCESS;

    ASSERT(crypto_info != NULL);

    // store PDCP crypto info in context
    ctx->crypto_info.pdcp_crypto_info = crypto_info;

    ret = sec_pdcp_context_create_descriptor(ctx);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Failed to create descriptor for "
               "PDCP context with bearer = %d", ctx->crypto_info.pdcp_crypto_info->bearer);

    return ret;
}

#ifndef USDPAA
/** Static array for selection of the function to be used for creating the shared descriptor on a SEC context,
 * depending on the algorithm selected by the user. This array is used for PDCP Control plane where both
 * authentication and encryption is required.
 */
static create_desc_fp c_plane_create_desc[NUM_CIPHER_ALGS][NUM_INT_ALGS] = {
        /*              NULL                                SNOW                                    AES */
        /* NULL */{create_c_plane_null_desc,          create_c_plane_auth_only_desc,      create_c_plane_auth_only_desc     },
        /* SNOW */{create_c_plane_cipher_only_desc,   create_c_plane_hw_acc_desc,         create_c_plane_mixed_desc         },
        /* AES  */{create_c_plane_cipher_only_desc,   create_c_plane_mixed_desc,          create_c_plane_hw_acc_desc        },
};

/** Static array for selection of the function to be used for creating the shared descriptor on a SEC context,
 * depending on the algorithm selected by the user. This array is used for PDCP User plane where only
 * encryption is required.
 */
static create_desc_fp u_plane_create_desc[NUM_CIPHER_ALGS] ={
        /*      NULL                   SNOW                     AES */
        create_u_plane_null_desc,   create_u_plane_hw_acc_desc,   create_u_plane_hw_acc_desc
};
#endif
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
