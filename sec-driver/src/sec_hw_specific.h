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

#ifndef SEC_HW_SPECIFIC_H
#define SEC_HW_SPECIFIC_H

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include "fsl_sec.h"
#include "sec_utils.h"

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/

/** Maximum length in words (4 bytes)  for IV(Initialization Vector) */
// TODO: Modify back to be 6 bytes...cannot do MAC-I checking with AES anyway....
// will do memcmp in software.
#define SEC_IV_MAX_LENGTH          12

/** Maximum length in words (4 bytes)  for IV (Initialization Vector) template.
 * Stores F8 IV followed by F9 IV. */
#define SEC_IV_TEMPLATE_MAX_LENGTH 8

/*****************************************************************
 * SEC REGISTER CONFIGURATIONS
 *****************************************************************/

/** Memory range assigned for registers of a job ring */
#ifdef SEC_HW_VERSION_3_1
#define SEC_CH_REG_RANGE                    0x100
#else // SEC_HW_VERSION_4_4
#define SEC_CH_REG_RANGE                    0x1000
#endif

#ifdef SEC_HW_VERSION_3_1
/** Offset to the registers of a job ring, as mapped by default by SEC engine */
#define SEC_CH_REG_RANGE_START_NORMAL       0x1000
/** Offset to the registers of a job ring, mapped in an alternate 4k page 
 * by SEC engine  if configured to do so with MCR (Master Control Register)*/
#define SEC_CH_REG_RANGE_START_ALTERNATE    0x0000
/** Offset to the registers of a job ring.
 * Is different for each job ring. */
#define CHAN_BASE(job_ring) ((    (job_ring->alternate_register_range == TRUE) ? \
                    SEC_CH_REG_RANGE_START_ALTERNATE : SEC_CH_REG_RANGE_START_NORMAL) \
                    + (job_ring->jr_id * SEC_CH_REG_RANGE))
#else // SEC_HW_VERSION_4_4
//#define CHAN_BASE(job_ring) (job_ring->jr_id * SEC_CH_REG_RANGE)
#define CHAN_BASE(job_ring) (0)
#endif // SEC_HW_VERSION_4_4
/*****************************************************************
 * ISR(Interrupt Status Register) offset
 *****************************************************************/

/** Offset to higher 32 bits of ISR register.*/
#define SEC_REG_ISR         0x1010
/** Offset to lower 32 bits of ISR register */
#define SEC_REG_ISR_LO      0x1014

/*****************************************************************
 * ISR(Interrupt Status Register) values
 *****************************************************************/

/** Job ring errors mask */
#define SEC_REG_ISR_CHERR   0xaa
/** Job ring done mask */
#define SEC_REG_ISR_CHDONE  0x55

/** Mask to check if DONE interrupt was generated for a certain job ring */
#define SEC_REG_GET_VAL_ISR_DONE(job_ring_id) (1 << (job_ring_id)* 2)
/** Mask to check if Error interrupt was generated for a certain job ring */
#define SEC_REG_GET_VAL_ISR_ERR(job_ring_id) (2 << (job_ring_id)* 2)



/*****************************************************************
 * IER(Interrupt Enable Register) offset
 *****************************************************************/

/** Offset from the SEC register base address for IER register.
 * Enables/disables done/error interrupts per channel, at controller level.*/
#define SEC_REG_IER     0x1008
/** Offset to lower 32 bits of IER register */
#define SEC_REG_IER_LO  0x100C


/*****************************************************************
 * IER(Interrupt Enable Register) values
 *****************************************************************/

/** Enable DONE interrupts for all 4 job rings */
#define SEC_REG_VAL_IER_DONE    0x00055
/** Enable DONE interrupts for a certain job ring */
#define SEC_REG_SET_VAL_IER_DONE(job_ring_id) (1 << (job_ring_id)* 2)
/** Enable Error interrupts for a certain job ring */
#define SEC_REG_SET_VAL_IER_ERR(job_ring_id) (2 << (job_ring_id)* 2)

/*****************************************************************
 * CCR(Channel Configuration Register) offset
 *****************************************************************/

/** Offset to higher 32 bits of CCR for a job ring */
#define SEC_REG_CCCR(jr)        (CHAN_BASE(jr) + SEC_REG_CCCR_OFFSET_HI)
/** Offset to lower 32 bits of CCR for a job ring */
#define SEC_REG_CCCR_LO(jr)     (CHAN_BASE(jr) + SEC_REG_CCCR_OFFSET_LO)

/** Offset to higher 32 bits of CCR, Channel Configuration Register */
#define SEC_REG_CCCR_OFFSET_HI  0x0108
/** Offset to lower 32 bits of CCR, Channel Configuration Register */
#define SEC_REG_CCCR_OFFSET_LO  0x010C

#ifdef SEC_HW_VERSION_4_4

#define JR_REG_IRSR(jr)        (CHAN_BASE(jr) + JR_REG_IRSR_OFFSET)

#define JR_REG_ORSR(jr)        (CHAN_BASE(jr) + JR_REG_ORSR_OFFSET)

#define JR_REG_IRSR_OFFSET      0x000C

#define JR_REG_ORSR_OFFSET      0x002C

#define JR_REG_IRBA(jr)             (CHAN_BASE(jr) + JR_REG_IRBA_OFFSET_HI)

#define JR_REG_IRBA_LO(jr)          (CHAN_BASE(jr) + JR_REG_IRBA_OFFSET_LO)

#define JR_REG_IRBA_OFFSET_HI       0x0000

#define JR_REG_IRBA_OFFSET_LO       0x0004

#define JR_REG_ORBA(jr)             (CHAN_BASE(jr) + JR_REG_ORBA_OFFSET_HI)

#define JR_REG_ORBA_LO(jr)          (CHAN_BASE(jr) + JR_REG_ORBA_OFFSET_LO)

#define JR_REG_ORBA_OFFSET_HI       0x0020

#define JR_REG_ORBA_OFFSET_LO       0x0024

#define JR_REG_IRJA(jr)             (CHAN_BASE(jr) + JR_REG_IRJA_OFFSET)

#define JR_REG_IRJA_OFFSET          0x001C

#define JR_REG_IRRI(jr)             (CHAN_BASE(jr) + JR_REG_IRJA_OFFSET)

#define JR_REG_IRRI_OFFSET          0x005C

#define JR_REG_ORJR(jr)                 (CHAN_BASE(jr) + JR_REG_ORJR_OFFSET)

#define JR_REG_ORJR_OFFSET          0x0034

#define JR_REG_JRCR(jr)                 (CHAN_BASE(jr) + JR_REG_JRCR_OFFSET)

#define JR_REG_JRCR_OFFSET          0x006C

#define JR_REG_JRCFG_LO(jr)                (CHAN_BASE(jr) + JR_REG_JRCFG_OFFSET_LO)

#define JR_REG_JRCFG_OFFSET_LO      0x0050

#define JR_REG_JRINT(jr)                (CHAN_BASE(jr) + JR_REG_JRINT_OFFSET)

#define JR_REG_JRINT_OFFSET         0x004C

#endif // SEC_HW_VERSION_4_4


/*****************************************************************
 * CCR(Channel Configuration Register) values
 *****************************************************************/

/* Job ring reset */
#ifdef SEC_HW_VERSION_3_1
#define SEC_REG_CCCR_VAL_RESET      0x1
#else
#define JR_REG_JRCR_VAL_RESET       0x00000001

#define JR_REG_JRCFG_LO_IMSK        0x00000001

#define JRINT_ERR_HALT_MASK         0x0C

#define JRINT_ERR_HALT_INPROGRESS   0x04

#endif
/* Job ring continue. Do same operations as for reset
but do not reset FIFO with jobs. See SEC 3.1 reference manual for more details. */
#define SEC_REG_CCCR_VAL_CONTINUE   0x2
/* Extended address enable (36bit) */
#define SEC_REG_VAL_CCCR_LO_EAE     0x20
/* Enable done writeback */
#define SEC_REG_VAL_CCCR_LO_CDWE    0x10
/* Enable done IRQ */
#define SEC_REG_VAL_CCCR_LO_CDIE    0x2
/* Enable ICV writeback if descriptor configured for ICV */
#define SEC_REG_VAL_CCCR_LO_IWSE    0x80

/*****************************************************************
 * FFER(Fetch Fifo Enqueue Register) offset
 *****************************************************************/

/**  Offset to higher 32 bits of fetch fifo enqueue register
 * (FFER) for a job ring */
#define SEC_REG_FFER(jr)        (CHAN_BASE(jr) + SEC_REG_FFER_OFFSET_HI)
/**  Offset to lower 32 bits of fetch fifo enqueue register
 * (FFER) for a job ring */
#define SEC_REG_FFER_LO(jr)     (CHAN_BASE(jr) + SEC_REG_FFER_OFFSET_LO)


/**  Offset to higher 32 bits of fetch fifo enqueue register (FFER) */
#define SEC_REG_FFER_OFFSET_HI  0x0148
/**  Offset to lower 32 bits of fetch fifo enqueue register (FFER) */
#define SEC_REG_FFER_OFFSET_LO  0x014C

/*****************************************************************
 * CSR(Channel Status Register) offset
 *****************************************************************/

/**  Offset to higher 32 bits of CSR register for a job ring */
#define SEC_REG_CSR(jr)       (CHAN_BASE(jr) + SEC_REG_CSR_OFFSET_HI)
/**  Offset to lower 32 bits of CSR register for a job ring */
#define SEC_REG_CSR_LO(jr)    (CHAN_BASE(jr) + SEC_REG_CSR_OFFSET_LO)


/**  Offset to higher 32 bits of CSR */
#define SEC_REG_CSR_OFFSET_HI   0x0110
/**  Offset to lower 32 bits of CSR */
#define SEC_REG_CSR_OFFSET_LO   0x0114

/*****************************************************************
 * CDPR(Current Descriptor Pointer Register) offset
 *****************************************************************/

/**  Offset to higher 32 bits of CDPR register for a job ring */
#define SEC_REG_CDPR(jr)        (CHAN_BASE(jr) +  SEC_REG_CDPR_OFFSET_HI)
/**  Offset to lower 32 bits of CDPR register for a job ring */
#define SEC_REG_CDPR_LO(jr)     (CHAN_BASE(jr) +  SEC_REG_CDPR_OFFSET_LO)


/**  Offset to higher 32 bits of CDPR */
#define SEC_REG_CDPR_OFFSET_HI  0x0140
/**  Offset to lower 32 bits of CDPR */
#define SEC_REG_CDPR_OFFSET_LO  0x0144

/*****************************************************************
 * CSR(Channel Status Register) values
 *****************************************************************/

#define SEC_REG_CSR_ERROR_MASK  0xFFFF /* Extract error field from CSR */

/* Specific error codes */
#define SEC_REG_CSR_LO_DOF    0x8000 /* double FF write oflow error */
#define SEC_REG_CSR_LO_SOF    0x4000 /* single FF write oflow error */
#define SEC_REG_CSR_LO_MDTE   0x2000 /* master data transfer error */
#define SEC_REG_CSR_LO_IDH    0x0400 /* illegal desc hdr error */
#define SEC_REG_CSR_LO_EU     0x0100 /* EU error detected */
#define SEC_REG_CSR_LO_WDT    0x0080 /* watchdog timeout */
#define SEC_REG_CSR_LO_SGML   0x0040 /* scatter/gather length mismatch error */
#define SEC_REG_CSR_LO_RSI    0x0020 /* RAID size incorrect error */
#define SEC_REG_CSR_LO_RSG    0x0010 /* RAID scatter/gather error */

#ifdef SEC_HW_VERSION_4_4
#define JQSTA_SSRC_SHIFT            28
#define JQSTA_SSRC_MASK             0xf0000000

#define JQSTA_SSRC_NONE             0x00000000
#define JQSTA_SSRC_CCB_ERROR        0x20000000
#define JQSTA_SSRC_JUMP_HALT_USER   0x30000000
#define JQSTA_SSRC_DECO             0x40000000
#define JQSTA_SSRC_JQERROR          0x60000000
#define JQSTA_SSRC_JUMP_HALT_CC     0x70000000

#define JQSTA_DECOERR_JUMP          0x08000000
#define JQSTA_DECOERR_INDEX_SHIFT   8
#define JQSTA_DECOERR_INDEX_MASK    0xff00
#define JQSTA_DECOERR_ERROR_MASK    0x00ff

#define JQSTA_DECOERR_NONE          0x00
#define JQSTA_DECOERR_LINKLEN       0x01
#define JQSTA_DECOERR_LINKPTR       0x02
#define JQSTA_DECOERR_JQCTRL        0x03
#define JQSTA_DECOERR_DESCCMD       0x04
#define JQSTA_DECOERR_ORDER         0x05
#define JQSTA_DECOERR_KEYCMD        0x06
#define JQSTA_DECOERR_LOADCMD       0x07
#define JQSTA_DECOERR_STORECMD      0x08
#define JQSTA_DECOERR_OPCMD         0x09
#define JQSTA_DECOERR_FIFOLDCMD     0x0a
#define JQSTA_DECOERR_FIFOSTCMD     0x0b
#define JQSTA_DECOERR_MOVECMD       0x0c
#define JQSTA_DECOERR_JUMPCMD       0x0d
#define JQSTA_DECOERR_MATHCMD       0x0e
#define JQSTA_DECOERR_SHASHCMD      0x0f
#define JQSTA_DECOERR_SEQCMD        0x10
#define JQSTA_DECOERR_DECOINTERNAL  0x11
#define JQSTA_DECOERR_SHDESCHDR     0x12
#define JQSTA_DECOERR_HDRLEN        0x13
#define JQSTA_DECOERR_BURSTER       0x14
#define JQSTA_DECOERR_DESCSIGNATURE 0x15
#define JQSTA_DECOERR_DMA           0x16
#define JQSTA_DECOERR_BURSTFIFO     0x17
#define JQSTA_DECOERR_JQRESET       0x1a
#define JQSTA_DECOERR_JOBFAIL       0x1b
#define JQSTA_DECOERR_DNRERR        0x80
#define JQSTA_DECOERR_UNDEFPCL      0x81
#define JQSTA_DECOERR_PDBERR        0x82
#define JQSTA_DECOERR_ANRPLY_LATE   0x83
#define JQSTA_DECOERR_ANRPLY_REPLAY 0x84
#define JQSTA_DECOERR_SEQOVF        0x85
#define JQSTA_DECOERR_INVSIGN       0x86
#define JQSTA_DECOERR_DSASIGN       0x87

#define JQSTA_CCBERR_JUMP           0x08000000
#define JQSTA_CCBERR_INDEX_MASK     0xff00
#define JQSTA_CCBERR_INDEX_SHIFT    8
#define JQSTA_CCBERR_CHAID_MASK     0x00f0
#define JQSTA_CCBERR_CHAID_SHIFT    4
#define JQSTA_CCBERR_ERRID_MASK     0x000f

#define JQSTA_CCBERR_CHAID_AES      (0x01 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_DES      (0x02 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_ARC4     (0x03 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_MD       (0x04 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_RNG      (0x05 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_SNOW     (0x06 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_KASUMI   (0x07 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_PK       (0x08 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_CRC      (0x09 << JQSTA_CCBERR_CHAID_SHIFT)

#define JQSTA_CCBERR_ERRID_NONE     0x00
#define JQSTA_CCBERR_ERRID_MODE     0x01
#define JQSTA_CCBERR_ERRID_DATASIZ  0x02
#define JQSTA_CCBERR_ERRID_KEYSIZ   0x03
#define JQSTA_CCBERR_ERRID_PKAMEMSZ 0x04
#define JQSTA_CCBERR_ERRID_PKBMEMSZ 0x05
#define JQSTA_CCBERR_ERRID_SEQUENCE 0x06
#define JQSTA_CCBERR_ERRID_PKDIVZRO 0x07
#define JQSTA_CCBERR_ERRID_PKMODEVN 0x08
#define JQSTA_CCBERR_ERRID_KEYPARIT 0x09
#define JQSTA_CCBERR_ERRID_ICVCHK   0x0a
#define JQSTA_CCBERR_ERRID_HARDWARE 0x0b
#define JQSTA_CCBERR_ERRID_CCMAAD   0x0c
#define JQSTA_CCBERR_ERRID_INVCHA   0x0f

#endif

/*****************************************************************
 * STEU IMR (Interrupt Mask Register) offset.
 * STEU is execution unit implementing SNOW F8 and F9.
 *****************************************************************/

/**  Offset to higher 32 bits of STEU IMR */
#define SEC_REG_STEU_IMR        0xD038
/**  Offset to lower 32 bits of STEU IMR */
#define SEC_REG_STEU_IMR_LO     0xD03C

/*****************************************************************
 * STEU ISR (Interrupt Status Register) offset.
 * STEU is execution unit implementing SNOW F8 and F9.
 *****************************************************************/

/**  Offset to higher 32 bits of STEU ISR */
#define SEC_REG_STEU_ISR        0xD030
/**  Offset to lower 32 bits of STEU ISR */
#define SEC_REG_STEU_ISR_LO     0xD034
/*****************************************************************
 * STEU IMR (Interrupt Mask Register) values
 * STEU is execution unit implementing SNOW F8 and F9.
 *****************************************************************/

/** Disable integrity check error interrupt in STEU */
#define SEC_REG_STEU_IMR_DISABLE_ICE    0x4000


/*****************************************************************
 * AESU SR(Status Register) offset.
 * AESU is execution unit implementing AES CTR and AES CMAC.
 *****************************************************************/

/**  Offset to higher 32 bits of AESU SR */
#define SEC_REG_AESU_SR         0x4028
/**  Offset to lower 32 bits of AESU SR */
#define SEC_REG_AESU_SR_LO      0x402C

/*****************************************************************
 * AESU ISR (Interrupt Status Register) offset.
 * AESU is execution unit implementing AES CTR and AES CMAC.
 *****************************************************************/

/**  Offset to higher 32 bits of AESU ISR */
#define SEC_REG_AESU_ISR        0x4030
/**  Offset to lower 32 bits of AESU ISR */
#define SEC_REG_AESU_ISR_LO     0x4034
/*****************************************************************
 * AESU IMR (Interrupt Mask Register) offset.
 * AESU is execution unit implementing AES CTR and AES CMAC.
 *****************************************************************/

/**  Offset to higher 32 bits of AESU IMR */
#define SEC_REG_AESU_IMR        0x4038
/**  Offset to lower 32 bits of AESU IMR */
#define SEC_REG_AESU_IMR_LO     0x403C


/*****************************************************************
 * AESU IMR (Interrupt Mask Register) values
 * AESU is execution unit implementing AES CTR and AES CMAC.
 *****************************************************************/

/** Disable integrity check error interrupt in STEU */
#define SEC_REG_AESU_IMR_DISABLE_ICE    0x4000


/*****************************************************************
 * Descriptor format: Header Dword values
 *****************************************************************/

#ifdef SEC_HW_VERSION_4_4
#define JR_JOB_STATUS_MASK         0x000000FF

#define JR_DESC_STATUS_NO_ERR       0x00000000
#else
/** Written back when packet processing is done,
 * in lower 32 bits of Descriptor header */
#define SEC_DESC_HDR_DONE           0xff000000
/** Request done notification (DN) per descriptor */
#define SEC_DESC_HDR_DONE_NOTIFY    0x00000001
#endif
/** Mask used to extract ICCR0 result from lower 32 bits of Descriptor header.
 * ICCR0 indicates result for integrity check performed by primary execution unit.
 * ICCR1 does the same for secondary execution unit.
 * Only ICCR0 used by driver. */
#define SEC_DESC_HDR_ICCR0_MASK     0x18000000
/** Written back when MAC-I check passed for packet,
 * in lower 32 bits of Descriptor header */
#define SEC_DESC_HDR_ICCR0_PASS     0x08000000
/** Written back when MAC-I check failed for packet,
 * in lower 32 bits of Descriptor header */
#define SEC_DESC_HDR_ICCR0_FAIL     0x10000000

/** Determines the processing type. Outbound means encrypting packets */
#define SEC_DESC_HDR_DIR_OUTBOUND   0x00000000
/** Determines the processing type. Inbound means decrypting packets */
#define SEC_DESC_HDR_DIR_INBOUND    0x00000002


/** Mask used to extract primary EU configuration from SEC descriptor header */
#define SEC_DESC_HDR_SEL0_MASK          0xf0000000

/*****************************************************************
 * SNOW descriptor configuration
 *****************************************************************/

/**  Select STEU execution unit, the one implementing SNOW 3G */
#define SEC_DESC_HDR_EU_SEL0_STEU       0x90000000
/** Mode data used to program STEU execution unit for F8 processing */
#define SEC_DESC_HDR_MODE0_STEU_F8      0x00900000
/** Mode data used to program STEU execution unit for F9 processing,
 *  no MAC-I check */
#define SEC_DESC_HDR_MODE0_STEU_F9      0x01a00000
/** Mode data used to program STEU execution unit for F9 processing,
 *  with MAC-I check */
#define SEC_DESC_HDR_MODE0_STEU_F9_ICV  0x05a00000
/** Select SNOW 3G descriptor type = common_nonsnoop */
#define SEC_DESC_HDR_DESC_TYPE_STEU     0x00000010

/*****************************************************************
 * AES descriptor configuration
 *****************************************************************/

/**  Select AESU execution unit, the one implementing AES */
#define SEC_DESC_HDR_EU_SEL0_AESU           0x60000000
/** Mode data used to program AESU execution unit for AES CTR processing */
#define SEC_DESC_HDR_MODE0_AESU_CTR         0x00600000
/** Mode data used to program AESU execution unit for AES CMAC processing */
#define SEC_DESC_HDR_MODE0_AESU_CMAC        0x04400000
/** Mode data used to program AESU execution unit for AES CMAC processing.
 * Perform MAC-I check */
#define SEC_DESC_HDR_MODE0_AESU_CMAC_ICV    0x06400000
/** Select AES descriptor type = common_nonsnoop */
#define SEC_DESC_HDR_DESC_TYPE_AESU         0x00000010

/*****************************************************************
 * Macros manipulating descriptor header
 *****************************************************************/

/** Check if a descriptor has the DONE bits set.
 * If yes, it means the packet tied to the descriptor 
 * is processed by SEC engine already.*/
#if SEC_HW_VERSION_4_4
#define hw_job_is_done(job)          (((*((job)->out_status)) & JR_JOB_STATUS_MASK) == JR_DESC_STATUS_NO_ERR)
#else
#define hw_job_is_done(descriptor)          ((descriptor->hdr & SEC_DESC_HDR_DONE) == SEC_DESC_HDR_DONE)
#endif

/** Check if integrity check performed by primary execution unit failed */
#define hw_icv_check_failed(descriptor)     ((descriptor->hdr_lo & SEC_DESC_HDR_ICCR0_MASK) == SEC_DESC_HDR_ICCR0_FAIL)

/** Check if integrity check performed by primary execution unit passed */
#define hw_icv_check_passed(descriptor)     ((descriptor->hdr_lo & SEC_DESC_HDR_ICCR0_MASK) == SEC_DESC_HDR_ICCR0_PASS)
#ifdef SEC_HW_VERSION_3_1
/** Enable done writeback in descriptor header dword after packet is processed by SEC engine */
#define hw_job_enable_writeback(descriptor_hdr) ((descriptor_hdr) |= SEC_DESC_HDR_DONE_NOTIFY)
#endif //# SEC_HW_VERSION_3_1
/** Return 0 if no error generated on this job ring.
 * Return non-zero if error. */
#if SEC_HW_VERSION_3_1
#define hw_job_ring_error(jr) (in_be32((jr)->register_base_addr + SEC_REG_CSR_LO(jr)) & SEC_REG_CSR_ERROR_MASK)
#else // SEC_HW_VERSION_3_1
#define hw_job_error(jr)    ((*((job)->out_status)) & JR_JOB_STATUS_MASK)
#endif

 /** Some error types require that the same error bit is set to 1 to clear the error source.
  * Use this macro for this purpose.
  */
#define hw_job_ring_clear_error(jr, value) (setbits32((jr)->register_base_addr + SEC_REG_CSR_LO(jr), (value)))

#ifdef SEC_HW_VERSION_3_1
/** Read pointer to current descriptor that is beeing processed on a job ring. */
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#define hw_get_current_descriptor(jr) ((dma_addr_t)((in_be32((jr)->register_base_addr + SEC_REG_CDPR(jr)) << 32)  |  \
                                                    (in_be32((jr)->register_base_addr + SEC_REG_CDPR_LO(jr)))))
#else
#define hw_get_current_descriptor(jr) ((dma_addr_t) (in_be32((jr)->register_base_addr + SEC_REG_CDPR_LO(jr))))
#endif


/*****************************************************************
 * Macros manipulating SEC registers for a job ring/channel
 *****************************************************************/

/** @brief Enqueue descriptor into a job ring's FIFO.
 * A descriptor points to an input packet to be processed as well as
 * to an output packet where SEC will write processing result.
 * The descriptor also points to the specific cryptographic operations
 * that must be applied on the input packet.
 *
 * @param [in] job_ring     The job ring
 * @param [in] descriptor   Physical address of descriptor.
 */
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)

#define hw_enqueue_packet_on_job_ring(job_ring, descriptor) \
{\
    /* Write higher 32 bits. Only relevant when Extended address\
       is enabled(36 bit physical addresses).\
       @note address must be big endian\
    */\
    out_be32(job_ring->register_base_addr + SEC_REG_FFER(job_ring),\
             PHYS_ADDR_HI(descriptor));\
    /* Write lower 32 bits. This is the trigger to insert the descriptor\
       into the channel's FETCH FIFO.\
       @note: This is why higher 32 bits MUST ALWAYS be written prior to\
       the lower 32 bits, when 36 physical addressing is ON!\
       @note address must be big endian\
    */\
    out_be32(job_ring->register_base_addr + SEC_REG_FFER_LO(job_ring),\
             PHYS_ADDR_LO(descriptor));\
}

#else //#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)

#define hw_enqueue_packet_on_job_ring(job_ring, descriptor) \
{\
    /* Write lower 32 bits of FFER. This is the trigger to insert the descriptor\
       into the channel's FETCH FIFO.\
       @note: This is why higher 32 bits MUST ALWAYS be written prior to\
       the lower 32 bits, when 36 physical addressing is ON!\
       @note address must be big endian\
    */\
    out_be32(job_ring->register_base_addr + SEC_REG_FFER_LO(job_ring),\
             (descriptor));\
}
#endif
#else // SEC_HW_VERSION_3_1

#define hw_get_current_inp_index(jr)        (( dma_addr_t)(in_be32((jr)->register_base_addr + JR_REG_IRRI(jr))) )

#define hw_get_current_out_index(jr)        (( dma_addr_t)(in_be32((jr)->register_base_addr + JR_REG_ORWI(jr))) )

#define hw_get_no_out_entries(jr)           ((uint32_t)(in_be32(jr)->register_base_addr + JR_REG_ORSF(jr)))

/** Read pointer to current descriptor that is being processed on a job ring. */
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#define hw_get_inp_queue_base(jr)   ( (dma_addr_t)((in_be32((jr)->register_base_addr + JR_REG_IRBA(jr)) << 32) | \
                                       (in_be32((jr)->register_base_addr + JR_REG_IRBA_LO(jr)))))
#define hw_get_inp_queue_base(jr)   ( (dma_addr_t)((in_be32((jr)->register_base_addr + JR_REG_ORBA(jr)) << 32) | \
                                       (in_be32((jr)->register_base_addr + JR_REG_ORBA_LO(jr)))))
#else
#define hw_get_inp_queue_base(jr)   ( (dma_addr_t)(in_be32((jr)->register_base_addr + JR_REG_IRBA_LO(jr))))

#define hw_get_out_queue_base(jr)   ( (dma_addr_t)((in_be32((jr)->register_base_addr + JR_REG_ORBA_LO(jr)))))
#endif

#define hw_get_current_inp_descriptor(jr) ( (dma_addr_t)(hw_get_inp_queue_base(jr) + \
                                         hw_get_current_inp_index(jr)))

#define hw_get_current_out_descriptor(jr) ( (dma_addr_t)(hw_get_out_queue_base(jr) + \
                                         hw_get_current_out_index(jr)))

#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#define hw_enqueue_packet_on_job_ring(job_ring, descriptor) \
{ \
    out_be32( (job_ring)->register_base_addr + JR_REG_IRBA(jr),PHYS_ADDR_HI(descriptor) ); \
    out_be32( (job_ring)->register_base_addr + JR_REG_IRBA_LO(jr),PHYS_ADDR_LO(descriptor) ); \
    out_be32(job_ring->register_base_addr + JR_REG_IRJA(job_ring),1);\
}
#else
#define hw_enqueue_packet_on_job_ring(job_ring, descriptor) \
{ \
    out_be32((job_ring)->register_base_addr + JR_REG_IRBA_LO(job_ring),PHYS_ADDR_LO(descriptor)); \
    out_be32((job_ring)->register_base_addr + JR_REG_IRJA(job_ring),1); \
}
#endif
#endif //SEC_HW_VERSION_3_1

#ifdef SEC_HW_VERSION_4_4

#define hw_set_input_ring_size(job_ring,size) \
{   \
    out_be32(job_ring->register_base_addr + JR_REG_IRSR(job_ring),\
             (size));\
}

#define hw_set_output_ring_size(job_ring,size) \
{   \
    out_be32(job_ring->register_base_addr + JR_REG_ORSR(job_ring),\
             (size));\
}

#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#define hw_set_input_ring_start_addr(job_ring, descriptor) \
{   \
    out_be32(job_ring->register_base_addr + JR_REG_IRBA(job_ring),\
             PHYS_ADDR_HI(descriptor));\
    out_be32(job_ring->register_base_addr + JR_REG_IRBA_LO(job_ring),\
             PHYS_ADDR_LO(descriptor));\
}

#define hw_set_output_ring_start_addr(job_ring, descriptor) \
{   \
    out_be32(job_ring->register_base_addr + JR_REG_ORBA(job_ring),\
             PHYS_ADDR_HI(descriptor));\
    out_be32(job_ring->register_base_addr + JR_REG_ORBA_LO(job_ring),\
             PHYS_ADDR_LO(descriptor));\
}

#else //#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#define hw_set_input_ring_start_addr(job_ring, descriptor) \
{   \
    out_be32(job_ring->register_base_addr + JR_REG_IRBA(job_ring),\
             (descriptor));\
}

#define hw_set_output_ring_start_addr(job_ring, descriptor) \
{   \
    out_be32(job_ring->register_base_addr + JR_REG_ORBA(job_ring),\
             (descriptor));\
}
#endif

#define JR_HW_REMOVE_ONE_ENTRY(jr) ( out_be32((jr)->register_base_addr + JR_REG_ORJR(jr),1) )

#define JR_HW_REMOVE_ENTRIES(jr,no_entries) ( out_be32((jr)->register_base_addr + JR_REG_ORJR(jr),(no_entries)))

#endif // SEC_HW_VERSION_4_4
/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
/** Forward structure declaration */
typedef struct sec_job_ring_t sec_job_ring_t;

#ifdef SEC_HW_VERSION_3_1
/** SEC Descriptor pointer entry */
struct sec_ptr {
    uint16_t len;       /*< length */
    uint8_t j_extent;   /*< jump to sg link table and/or extent */
    uint8_t eptr;       /*< extended address */
    uint32_t ptr;       /*< address */
};

/** A descriptor that instructs SEC engine how to process a packet.
 * On SEC 3.1 a descriptor consists of 8 double-words: 
 * one header dword and seven pointer dwords. */
struct sec_descriptor_t
{
    volatile uint32_t hdr;      /*< header high bits */
    uint32_t hdr_lo;            /*< header low bits */
    struct sec_ptr ptr[7];      /*< ptr/len pair array */
    /*< Initialization Vector. Need to have it here because it 
     * is updated with info from each packet!!! */
    uint32_t __CACHELINE_ALIGNED iv[SEC_IV_MAX_LENGTH];

} __CACHELINE_ALIGNED;
#else
struct preheader_s {
    union {
        uint32_t word;
        struct {
            uint16_t rsvd63_48;
            unsigned int rsvd47_39:9;
            unsigned int idlen:7;
        } field;
    } __CACHELINE_ALIGNED hi;

    union {
        uint32_t word;
        struct {
            unsigned int rsvd31_30:2;
            unsigned int fsgt:1;
            unsigned int lng:1;
            unsigned int offset:2;
            unsigned int abs:1;
            unsigned int add_buf:1;
            uint8_t pool_id;
            uint16_t pool_buffer_size;
        } field;
    } __CACHELINE_ALIGNED lo;
} __CACHELINE_ALIGNED;

struct init_descriptor_header_s {
    union {
        uint32_t word;
        struct {
            unsigned int ctype:5;
            unsigned int rsvd26_25:2;
            unsigned int dnr:1;
            unsigned int one:1;
            unsigned int start_idx:7;
            unsigned int zro:1;
            unsigned int rsvd14_12:3;
            unsigned int propogate_dnr:1;
            unsigned int share:3;
            unsigned int rsvd7:1;
            unsigned int desc_len:7;
        } field;
    } __CACHELINE_ALIGNED command;
} __CACHELINE_ALIGNED;

struct key_command_s {
    union {
        uint32_t word;
        struct {
            unsigned int ctype:5;
            unsigned int cls:2;
            unsigned int sgf:1;
            unsigned int imm:1;
            unsigned int enc:1;
            unsigned int rsvd21_18:4;
            unsigned int kdest:2;
            unsigned int rsvd15_10:6;
            unsigned int length:10;
        } field;
    } __CACHELINE_ALIGNED command;
} __CACHELINE_ALIGNED;

struct algorithm_operation_command_s {
    union {
        uint32_t word;
        struct {
            unsigned int ctype:5;
            unsigned int optype:3;
            uint8_t alg;
            unsigned int rsvd16_18:3;
            unsigned int aai:9;
            unsigned int as:2;
            unsigned int icv:1;
            unsigned int enc:1;
        } field;
    } __CACHELINE_ALIGNED command;
} __CACHELINE_ALIGNED;

struct sec_descriptor_t {
    struct preheader_s prehdr;
    struct init_descriptor_header_s deschdr;
    struct key_command_s keycmd;
    uint32_t key[6];
    struct algorithm_operation_command_s opcmd;
    uint32_t rsv[15];   //TODO: fill it for iv, LOAD, MATH, FIFO LOAD etc.
} __CACHELINE_ALIGNED;

#endif
/** Cryptographic data belonging to a SEC context.
 * Can be considered a joint venture between:
 * - a 'shared descriptor' (the descriptor header word)
 * - a PDB(protocol data block) */
typedef struct sec_crypto_pdb_s
{
    uint32_t crypto_hdr;    /*< Higher 32 bits of Descriptor Header dword, used for encrypt/decrypt operations.
                                Lower 32 bits are reserved and unused. */
    uint32_t auth_hdr;      /*< Higher 32 bits of Descriptor Header dword, used for authentication operations.
                                Lower 32 bits are reserved and unused. */
    uint32_t iv_template[SEC_IV_TEMPLATE_MAX_LENGTH];       /*< Template for Initialization Vector. 
                                                                HFN is stored and maintained here. */
    uint32_t hfn_threshold; /*< Threshold for HFN configured by User Application. 
                                Bitshifted left to skip SN bits from first word of IV. */
    uint32_t hfn_mask;      /*< Mask applied on IV to extract HFN */
    uint32_t sn_mask;       /*< Mask applied on PDCP header to extract SN */
    uint8_t sns;            /*< SN Short. Is set to 1 if short sequence number is used:
                                - 5 bit for PDCP c-plane and 7 bit for PDCP d-plane.
                                Is set to 0 if long SN is used:
                                - 12 bit for PDCP d-plane */
    uint8_t pdcp_hdr_len;   /*< Length in bytes for PDCP header. */
    uint8_t is_inbound;     /*< Is #TRUE for inbound data flows(performing decapsulation on packets)
                                and #FALSE for outbound data flows(performing encapsulation on packets). */

}sec_crypto_pdb_t;
/*==============================================================================
                                 CONSTANTS
==============================================================================*/

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/** @brief Initialize a job ring/channel in SEC device.
 * Write configuration register/s to properly initialize a job ring.
 *
 * @param [in] job_ring     The job ring
 *
 * @retval 0 for success
 * @retval other for error
 */
int hw_reset_job_ring(sec_job_ring_t *job_ring);

/** @brief Reset a job ring/channel in SEC device.
 * Write configuration register/s to reset a job ring.
 *
 * @param [in] job_ring     The job ring
 *
 * @retval 0 for success
 * @retval -1 in case job ring reset failed
 */
int hw_shutdown_job_ring(sec_job_ring_t *job_ring);

/** @brief Reset and continue for a job ring/channel in SEC device.
 * Write configuration register/s to reset some settings for a job ring
 * but still continue processing. FIFO with all already submitted jobs
 * will be kept.
 *
 * @param [in] job_ring     The job ring
 *
 * @retval 0 for success
 * @retval -1 in case job ring reset and continue failed
 */
int hw_reset_and_continue_job_ring(sec_job_ring_t *job_ring);

/** @brief Handle a job ring/channel error in SEC device.
 * Identify the error type and clear error bits if required.
 * Return information if job ring must be restarted.
 *
 * @param [in]  job_ring        The job ring
 * @param [in]  sec_error_code  The job ring's error code as first read from SEC engine
 * @param [out] reset_required  If set to #TRUE, the job ring must be reset.
 */
#ifdef SEC_HW_VERSION_3_1
void hw_handle_job_ring_error(sec_job_ring_t *job_ring,
                              uint32_t sec_error_code,
                              uint32_t *reset_required);
#else // SEC_HW_VERSION_3_1
void hw_handle_job_ring_error (uint32_t sec_error_code,
                              uint32_t *reset_required);
#endif
/*============================================================================*/


#endif  /* SEC_HW_SPECIFIC_H */
