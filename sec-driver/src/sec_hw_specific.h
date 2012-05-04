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
#ifdef SEC_HW_VERSION_3_1
/** Maximum length in words (4 bytes)  for IV(Initialization Vector) */
#define SEC_IV_MAX_LENGTH          12

/** Maximum length in words (4 bytes)  for IV (Initialization Vector) template.
 * Stores F8 IV followed by F9 IV. */
#define SEC_IV_TEMPLATE_MAX_LENGTH 8

/*****************************************************************
 * SEC REGISTER CONFIGURATIONS
 *****************************************************************/

/** Memory range assigned for registers of a job ring */
#define SEC_CH_REG_RANGE                    0x100

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

/*****************************************************************
 * CCR(Channel Configuration Register) values
 *****************************************************************/

/* Job ring reset */
#define SEC_REG_CCCR_VAL_RESET      0x1

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
#define hw_job_is_done(descriptor)          ((descriptor->hdr & SEC_DESC_HDR_DONE) == SEC_DESC_HDR_DONE)

/** Check if integrity check performed by primary execution unit failed */
#define hw_icv_check_failed(descriptor)     ((descriptor->hdr_lo & SEC_DESC_HDR_ICCR0_MASK) == SEC_DESC_HDR_ICCR0_FAIL)

/** Check if integrity check performed by primary execution unit passed */
#define hw_icv_check_passed(descriptor)     ((descriptor->hdr_lo & SEC_DESC_HDR_ICCR0_MASK) == SEC_DESC_HDR_ICCR0_PASS)

/** Enable done writeback in descriptor header dword after packet is processed by SEC engine */
#define hw_job_enable_writeback(descriptor_hdr) ((descriptor_hdr) |= SEC_DESC_HDR_DONE_NOTIFY)

/** Return 0 if no error generated on this job ring.
 * Return non-zero if error. */
#define hw_job_ring_error(jr) (in_be32((jr)->register_base_addr + SEC_REG_CSR_LO(jr)) & SEC_REG_CSR_ERROR_MASK)

 /** Some error types require that the same error bit is set to 1 to clear the error source.
  * Use this macro for this purpose.
  */
#define hw_job_ring_clear_error(jr, value) (setbits32((jr)->register_base_addr + SEC_REG_CSR_LO(jr), (value)))

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

/** Offset to the registers of a job ring.
 * Is different for each job ring. */
#define CHAN_BASE(jr)   ((jr)->register_base_addr)

/******************************************************************
 * Constants representing various job ring registers
 *****************************************************************/

#define JR_REG_IRBA_OFFSET              0x0000
#define JR_REG_IRBA_OFFSET_LO           0x0004

#define JR_REG_IRSR_OFFSET              0x000C
#define JR_REG_IRSA_OFFSET              0x0014
#define JR_REG_IRJA_OFFSET              0x001C

#define JR_REG_ORBA_OFFSET              0x0020
#define JR_REG_ORBA_OFFSET_LO           0x0024

#define JR_REG_ORSR_OFFSET              0x002C
#define JR_REG_ORJR_OFFSET              0x0034
#define JR_REG_ORSFR_OFFSET             0x003C
#define JR_REG_JROSR_OFFSET             0x0044
#define JR_REG_JRINT_OFFSET             0x004C

#define JR_REG_JRCFG_OFFSET             0x0050
#define JR_REG_JRCFG_OFFSET_LO          0x0054

#define JR_REG_IRRI_OFFSET              0x005C
#define JR_REG_ORWI_OFFSET              0x0064
#define JR_REG_JRCR_OFFSET              0x006C

/******************************************************************
 * Constants for error handling on job ring
 *****************************************************************/
#define JR_REG_JRINT_ERR_TYPE_SHIFT         8
#define JR_REG_JRINT_ERR_ORWI_SHIFT         16
#define JR_REG_JRINIT_JRE_SHIFT             1

#define JRINT_JRE                           (1 << JR_REG_JRINIT_JRE_SHIFT)
#define JRINT_ERR_WRITE_STATUS              (1 << JR_REG_JRINT_ERR_TYPE_SHIFT)
#define JRINT_ERR_BAD_INPUT_BASE            (3 << JR_REG_JRINT_ERR_TYPE_SHIFT)
#define JRINT_ERR_BAD_OUTPUT_BASE           (4 << JR_REG_JRINT_ERR_TYPE_SHIFT)
#define JRINT_ERR_WRITE_2_IRBA              (5 << JR_REG_JRINT_ERR_TYPE_SHIFT)
#define JRINT_ERR_WRITE_2_ORBA              (6 << JR_REG_JRINT_ERR_TYPE_SHIFT)
#define JRINT_ERR_RES_B4_HALT               (7 << JR_REG_JRINT_ERR_TYPE_SHIFT)
#define JRINT_ERR_REM_TOO_MANY              (8 << JR_REG_JRINT_ERR_TYPE_SHIFT)
#define JRINT_ERR_ADD_TOO_MANY              (9 << JR_REG_JRINT_ERR_TYPE_SHIFT)
#define JRINT_ERR_HALT_MASK                 0x0C
#define JRINT_ERR_HALT_INPROGRESS           0x04
#define JRINT_ERR_HALT_COMPLETE             0x08

#define JR_REG_JRCR_VAL_RESET               0x00000001

#define JR_REG_JRCFG_LO_ICTT_SHIFT           0x10
#define JR_REG_JRCFG_LO_ICDCT_SHIFT          0x08
#define JR_REG_JRCFG_LO_ICEN_EN              0x02

/******************************************************************
 * Constants for Packet Processing errors
 *****************************************************************/
#define SEC_HW_ERR_SSRC_NO_SRC              0x00
#define SEC_HW_ERR_SSRC_CCB_ERR             0x02
#define SEC_HW_ERR_SSRC_JMP_HALT_U          0x03
#define SEC_HW_ERR_SSRC_DECO                0x04
#define SEC_HW_ERR_SSRC_JR                  0x06
#define SEC_HW_ERR_SSRC_JMP_HALT_COND       0x07

#define SEC_HW_ERR_DECO_HFN_THRESHOLD       0xF1


/******************************************************************
 * Constants for descriptors
 *****************************************************************/

#define CMD_HDR_CTYPE_SD                        0x16
#define CMD_HDR_CTYPE_JD                        0x17

#define CMD_PROTO_SNOW_ALG                      0x01
#define CMD_PROTO_AES_ALG                       0x02

#define CMD_PROTO_DECAP                         0x06
#define CMD_PROTO_ENCAP                         0x07

#define CMD_ALGORITHM_ICV                       0x02

#define CMD_ALGORITHM_ENCRYPT                   0x01

#define PDCP_SD_KEY_LEN     0x4


/******************************************************************
 * Macros for extracting error codes for the job ring
 *****************************************************************/

#define JR_REG_JRINT_ERR_TYPE_EXTRACT(value)      ((value) & 0x00000F00)
#define JR_REG_JRINT_ERR_ORWI_EXTRACT(value)      (((value) & 0x3FFF0000) >> JR_REG_JRINT_ERR_ORWI_SHIFT)
#define JR_REG_JRINT_JRE_EXTRACT(value)           ((value) & JRINT_JRE)


/******************************************************************
 * Macros for managing the job ring
 *****************************************************************/

/** Read pointer to job ring input ring start address */
#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#define hw_get_inp_queue_base(jr)   ( (dma_addr_t)(GET_JR_REG(IRBA,(jr)) << 32) | \
                                       (GET_JR_REG_LO(IRBA,(jr))) )

/** Read pointer to job ring output ring start address */
#define hw_get_out_queue_base(jr)   ( (dma_addr_t)(GET_JR_REG(ORBA,(jr)) << 32) | \
                                       (GET_JR_REG_LO(ORBA,(jr))) )
#else
#define hw_get_inp_queue_base(jr)   ( (dma_addr_t)(GET_JR_REG_LO(IRBA,(jr)) ) )

#define hw_get_out_queue_base(jr)   ( (dma_addr_t)(GET_JR_REG_LO(ORBA,(jr)) ) )
#endif

/**
 * IRJA - Input Ring Jobs Added Register tells SEC 4.4
 * how many new jobs were added to the Input Ring.
 */
#define hw_enqueue_packet_on_job_ring(job_ring) \
    SET_JR_REG(IRJA, (job_ring), 1);

#define hw_set_input_ring_size(job_ring,size)   SET_JR_REG(IRSR,job_ring,(size))

#define hw_set_output_ring_size(job_ring,size)  SET_JR_REG(ORSR,job_ring,(size))

#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#define hw_set_input_ring_start_addr(job_ring, start_addr)              \
{                                                                       \
    SET_JR_REG(IRBA,job_ring,start_addr,PHYS_ADDR_HI(start_addr));      \
    SET_JR_REG_LO(IRBA,job_ring,start_addr,PHYS_ADDR_LO(start_addr));   \
}

#define hw_set_output_ring_start_addr(job_ring, start_addr)             \
{                                                                       \
    SET_JR_REG(ORBA,job_ring,start_addr,PHYS_ADDR_HI(start_addr));      \
    SET_JR_REG_LO(ORBA,job_ring,start_addr,PHYS_ADDR_LO(start_addr));   \
}

#else //#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
#define hw_set_input_ring_start_addr(job_ring, start_addr) \
        SET_JR_REG_LO(IRBA,job_ring,start_addr)

#define hw_set_output_ring_start_addr(job_ring, start_addr) \
        SET_JR_REG_LO(ORBA,job_ring,start_addr)

#endif

/** This macro will 'remove' one entry from the output ring of the Job Ring once
 * the software has finished processing the entry. */
#define hw_remove_one_entry(jr)                 hw_remove_entries(jr,1)

/** ORJR - Output Ring Jobs Removed Register tells SEC 4.4 how many jobs were
 * removed from the Output Ring for processing by software. This is done after
 * the software has processed the entries. */
#define hw_remove_entries(jr,no_entries)        SET_JR_REG(ORJR,(jr),(no_entries))

/** IRSA - Input Ring Slots Available register holds the number of entries in
 * the Job Ring's input ring. Once a job is enqueued, the value returned is decremented
 * by the hardware by the number of jobs enqueued. */
#define hw_get_available_slots(jr)              GET_JR_REG(IRSA,jr)

/** ORSFR - Output Ring Slots Full register holds the number of jobs which were processed
 * by the SEC and can be retrieved by the software. Once a job has been processed by
 * software, the user will call hw_remove_one_entry in order to notify the SEC that
 * the entry was processed */
#define hw_get_no_finished_jobs(jr)             GET_JR_REG(ORSFR, jr)


/******************************************************************
 * Macro for determining if the threshold was exceeded for a
 * context
 *****************************************************************/
#define HFN_THRESHOLD_MATCH(error)  \
    COND_EXPR1_EQ_AND_EXPR2_EQ( ((union hw_error_code)(error)).error_desc.deco_src.ssrc,    \
                                SEC_HW_ERR_SSRC_DECO,                                       \
                                ((union hw_error_code)(error)).error_desc.deco_src.desc_err,\
                                SEC_HW_ERR_DECO_HFN_THRESHOLD )

/******************************************************************
 * Macros for manipulating JR registers
 *****************************************************************/
#define JR_REG(name,jr)                    (CHAN_BASE(jr) + JR_REG_##name##_OFFSET)
#define JR_REG_LO(name,jr)                 (CHAN_BASE(jr) + JR_REG_##name##_OFFSET_LO)

#define GET_JR_REG(name,jr)                 ( in_be32( JR_REG( name,(jr) ) ) )
#define GET_JR_REG_LO(name,jr)              ( in_be32( JR_REG_LO( name,(jr) ) ) )

#define SET_JR_REG(name,jr,value)           ( out_be32( JR_REG( name,(jr) ),value  ) )
#define SET_JR_REG_LO(name,jr,value)        ( out_be32( JR_REG_LO( name,(jr) ),value ) )

/******************************************************************
 * Macros manipulating descriptors
 *****************************************************************/
#define PDCP_JD_SET_SD(descriptor,ptr,len)           {          \
    (descriptor)->sd_ptr = (ptr);                               \
    (descriptor)->deschdr.command.jd.shr_desc_len = (len);      \
}

#define PDCP_INIT_JD(descriptor)              { \
        /* CTYPE = job descriptor                               \
         * RSMS, DNR = 0
         * ONE = 1
         * Start Index = 0
         * ZRO,TD, MTD = 0
         * SHR = 1 (there's a shared descriptor referenced
         *          by this job descriptor,pointer in next word)
         * REO = 1 (execute job descr. first, shared descriptor
         *          after)
         * SHARE = DEFER
         * Descriptor Length = 0 ( to be completed @ runtime )
         *
         */                                                     \
        (descriptor)->deschdr.command.word = 0xB0801C08;        \
        /*
         * CTYPE = SEQ OUT command
         * Scater Gather Flag = 0 (can be updated @ runtime)
         * PRE = 0
         * EXT = 1 ( data length is in next word, following the
         *           command)
         * RTO = 0
         */                                                     \
        (descriptor)->seq_out.command.word = 0xF8400000; /**/   \
        /*
         * CTYPE = SEQ IN command
         * Scater Gather Flag = 0 (can be updated @ runtime)
         * PRE = 0
         * EXT = 1 ( data length is in next word, following the
         *           command)
         * RTO = 0
         */                                                     \
        (descriptor)->seq_in.command.word  = 0xF0400000; /**/   \
}

#define SEC_PDCP_INIT_CPLANE_SD(descriptor){ \
        /* CTYPE = shared job descriptor                        \
         * RIF = 0
         * DNR = 0
         * ONE = 1
         * Start Index = 5, in order to jump over PDB
         * ZRO,CIF,SC = 0
         * PD = 0, SHARE = WAIT
         * Descriptor Length = 10
         */                                                     \
        (descriptor)->deschdr.command.word  = 0xB885010A;       \
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

#define SEC_PDCP_INIT_UPLANE_SD(descriptor){ \
        /* CTYPE = shared job descriptor                        \
         * RIF = 0
         * DNR = 0
         * ONE = 1
         * Start Index = 7, in order to jump over PDB and
         *                  key2 command
         * ZRO,CIF,SC = 0
         * PD = 0, SHARE = WAIT
         * Descriptor Length = 10
         */                                                     \
        (descriptor)->deschdr.command.word  = 0xB887010A;       \
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
         * IMM = 0 (key after command)
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
        (descriptor)->protocol.command.word = 0x80420000;       \
}

#define SEC_PDCP_SD_SET_KEY1(descriptor,key,len) {              \
        (descriptor)->key1_cmd.command.field.length = (len);    \
        (descriptor)->key1_ptr = g_sec_vtop((key));              \
}

#define SEC_PDCP_SD_SET_KEY2(descriptor,key,len) {              \
        (descriptor)->key2_cmd.command.field.length = (len);    \
        (descriptor)->key2_ptr = g_sec_vtop((key));             \
}

#define PDCP_JD_SET_IN_PTR(descriptor,phys_addr,offset,length) {    \
    (descriptor)->seq_in_ptr = (phys_addr) + (offset);              \
    (descriptor)->in_ext_length = (length);                         \
}

#define PDCP_JD_SET_OUT_PTR(descriptor,phys_addr,offset,length) {   \
    (descriptor)->seq_out_ptr = (phys_addr) + (offset);             \
    (descriptor)->out_ext_length = (length);                        \
}

#define PDCP_JD_SET_SG_IN(descriptor)   ( (descriptor)->seq_in.command.field.sgf =  1 )

#define PDCP_JD_SET_SG_OUT(descriptor)  ( (descriptor)->seq_out.command.field.sgf = 1 )

#define SEC_PDCP_SH_SET_PROT_DIR(descriptor,dir)    ( (descriptor)->protocol.command.field.optype = \
                        ((dir) == PDCP_ENCAPSULATION) ? CMD_PROTO_ENCAP : CMD_PROTO_DECAP )

#define SEC_PDCP_SH_SET_PROT_ALG(descriptor,alg)    ( (descriptor)->protocol.command.field.protinfo = \
                        (alg == SEC_ALG_SNOW) ? CMD_PROTO_SNOW_ALG : (alg == SEC_ALG_AES) ? \
                         CMD_PROTO_AES_ALG : CMD_PROTO_SNOW_ALG )

#define SEC_PDCP_GET_DESC_LEN(descriptor)                                                   \
    (((struct descriptor_header_s*)(descriptor))->command.sd.ctype ==                       \
    CMD_HDR_CTYPE_SD ? ((struct descriptor_header_s*)(descriptor))->command.sd.desclen :    \
                       ((struct descriptor_header_s*)(descriptor))->command.jd.desclen )

#define SEC_PDCP_DUMP_DESC(descriptor) {                                    \
        int __i;                                                            \
        SEC_DEBUG("Descriptor @ 0x%08x",(uint32_t)((uint32_t*)(descriptor)));\
        for( __i = 0;                                                       \
            __i < SEC_PDCP_GET_DESC_LEN(descriptor);                        \
            __i++)                                                          \
        {                                                                   \
            SEC_DEBUG("0x%08x: 0x%08x",                                     \
                     (uint32_t)(((uint32_t*)(descriptor)) + __i),           \
                     *(((uint32_t*)(descriptor)) + __i));                   \
        }                                                                   \
}

#endif // SEC_HW_VERSION_3_1

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

#else // SEC_HW_VERSION_3_1

/** Union describing the possible error codes that
 * can be set in the descriptor status word
 */
union hw_error_code{
    uint32_t    error;
    union{
        struct{
            uint32_t    ssrc:4;
            uint32_t    ssed_val:28;
        }PACKED value;
        struct {
            uint32_t    ssrc:4;
            uint32_t     res:28;
        }PACKED no_status_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    jmp:1;
            uint32_t    res:11;
            uint32_t    desc_idx:8;
            uint32_t    cha_id:4;
            uint32_t    err_id:4;
        }PACKED ccb_status_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    jmp:1;
            uint32_t    res:11;
            uint32_t    desc_idx:8;
            uint32_t    offset:8;
        }PACKED jmp_halt_user_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    jmp:1;
            uint32_t    res:11;
            uint32_t    desc_idx:8;
            uint32_t    desc_err:8;
        }PACKED deco_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    res:17;
            uint32_t    naddr:3;
            uint32_t    desc_err:8;
        }PACKED jr_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    jmp:1;
            uint32_t    res:11;
            uint32_t    desc_idx:8;
            uint32_t    cond:8;
        }PACKED jmp_halt_cond_src;
    }PACKED error_desc;
}PACKED;

/** Union describing a descriptor header.
 */
struct descriptor_header_s {
    union {
        uint32_t word;
        struct {
            /* 4  */ unsigned int ctype:5;
            /* 5  */ unsigned int res1:2;
            /* 7  */ unsigned int dnr:1;
            /* 8  */ unsigned int one:1;
            /* 9  */ unsigned int res2:1;
            /* 10 */ unsigned int start_idx:6;
            /* 16 */ unsigned int res3:2;
            /* 18 */ unsigned int cif:1;
            /* 19 */ unsigned int sc:1;
            /* 20 */ unsigned int pd:1;
            /* 21 */ unsigned int res4:1;
            /* 22 */ unsigned int share:2;
            /* 24 */ unsigned int res5:2;
            /* 26 */ unsigned int desclen:6;
        } sd;
        struct {
            /* 4  */ unsigned int ctype:5;
            /* 5  */ unsigned int res1:1;
            /* 6  */ unsigned int rsms:1;
            /* 7  */ unsigned int dnr:1;
            /* 8  */ unsigned int one:1;
            /* 9  */ unsigned int res2:1;
            /* 10 */ unsigned int shr_desc_len:6;
            /* 16 */ unsigned int zero:1;
            /* 17 */ unsigned int td:1;
            /* 18 */ unsigned int mtd:1;
            /* 19 */ unsigned int shr:1;
            /* 20 */ unsigned int reo:1;
            /* 21 */ unsigned int share:3;
            /* 24 */ unsigned int res4:1;
            /* 25 */ unsigned int desclen:7;
        } jd;
    } PACKED command;
} PACKED;

/** Union describing a KEY command in a descriptor.
 */
struct key_command_s {
    union {
        uint32_t word;
        struct {
            unsigned int ctype:5;
            unsigned int cls:2;
            unsigned int sgf:1;
            unsigned int imm:1;
            unsigned int enc:1;
            unsigned int nwb:1;
            unsigned int ekt:1;
            unsigned int kdest:4;
            unsigned int tk:1;
            unsigned int rsvd1:5;
            unsigned int length:10;
        } PACKED field;
    } PACKED command;
} PACKED;

/** Union describing a PROTOCOL command
 * in a descriptor.
 */
struct protocol_operation_command_s {
    union {
        uint32_t word;
        struct {
            unsigned int ctype:5;
            unsigned int optype:3;
            unsigned char protid;
            unsigned short protinfo;
        } PACKED field;
    } PACKED command;
} PACKED;

/** Union describing a SEQIN command in a
 * descriptor.
 */
struct seq_in_command_s {
    union {
        uint32_t word;
        struct {
            unsigned int ctype:5;
            unsigned int res1:1;
            unsigned int inl:1;
            unsigned int sgf:1;
            unsigned int pre:1;
            unsigned int ext:1;
            unsigned int rto:1;
            unsigned int rjd:1;
            unsigned int res2:4;
            unsigned int length:16;
        } field;
    } PACKED command;
}PACKED;

/** Union describing a SEQOUT command in a
 * descriptor.
 */
struct seq_out_command_s{
    union {
        uint32_t word;
        struct {
            unsigned int ctype:5;
            unsigned int res1:2;
            unsigned int sgf:1;
            unsigned int pre:1;
            unsigned int ext:1;
            unsigned int rto:1;
            unsigned int res2:5;
            unsigned int length:16;
        } field;
    } PACKED command;
} PACKED;

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
typedef struct sec_crypto_pdb_s
{
    union{
        uint32_t    content[4];
        struct cplane_pdb_s cplane_pdb;
        struct uplane_pdb_s uplane_pdb;
    }PACKED pdb_content;
}PACKED sec_crypto_pdb_t;

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
    sec_crypto_pdb_t            pdb;
    struct key_command_s        key2_cmd;
#warning "Update for 36 bits addresses"
    dma_addr_t                  key2_ptr;
    struct key_command_s        key1_cmd;
#warning "Update for 36 bits addresses"
    dma_addr_t                  key1_ptr;
    struct protocol_operation_command_s protocol;

} PACKED;

/** Structure encompassing a job descriptor which processes
 * a single packet from a context. The job descriptor references
 * a shared descriptor from a SEC context.
 */
struct sec_descriptor_t {
    struct descriptor_header_s deschdr;
#warning "Update for 36 bits addresses"
    dma_addr_t    sd_ptr;
    struct seq_out_command_s seq_out;
#warning "Update for 36 bits addresses"
    dma_addr_t    seq_out_ptr;
    uint32_t      out_ext_length;
    struct seq_in_command_s seq_in;
    dma_addr_t    seq_in_ptr;
    uint32_t    in_ext_length;
} PACKED;

#endif // SEC_HW_VERSION_3_1
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

#ifdef SEC_HW_VERSION_3_1
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
#endif // SEC_HW_VERSION_3_1
/** @brief Handle a job ring/channel error in SEC device.
 * Identify the error type and clear error bits if required.
 * Return information if job ring must be restarted.
 *
 * @param [in]  job_ring        The job ring
 * @param [in]  sec_error_code  The job ring's error code as first read from SEC engine
 * @param [out] reset_required  If set to #TRUE, the job ring must be reset.
 */
void hw_handle_job_ring_error(sec_job_ring_t *job_ring,
                              uint32_t sec_error_code,
                              uint32_t *reset_required);
#ifdef SEC_HW_VERSION_4_4

/** @brief Handle a job ring error in SEC 4.4 device.
 * Identify the error type and printout a explanatory
 * messages.
 * The Job ring must be reset if return is nonzero.
 *
 * @param [in]  job_ring        The job ring
 *
 * @Caution                     If the function returns nonzero, then
 *                              the Job ring must be reset.
 */
int hw_job_ring_error(sec_job_ring_t *job_ring);

#if (SEC_INT_COALESCING_ENABLE == ON)

/** @brief Set interrupt coalescing parameters on the Job Ring.
 * @param [in]  job_ring                The job ring
 * @param [in]  irq_coalesing_timer     Interrupt coalescing timer threshold.
 *                                      This value determines the maximum
 *                                      amount of time after processing a descriptor
 *                                      before raising an interrupt.
 * @param [in]  irq_coalescing_count    Interrupt coalescing descriptor count threshold.
 *                                      This value determines how many descriptors
 *                                      are completed before raising an interrupt.
 */
int hw_job_ring_set_coalescing_param(sec_job_ring_t *job_ring,
                                     uint16_t irq_coalescing_timer,
                                     uint8_t irq_coalescing_count);

/** @brief Enable interrupt coalescing on a job ring
 * @param [in]  job_ring                The job ring
 */
int hw_job_ring_enable_coalescing(sec_job_ring_t *job_ring);

/** @brief Disable interrupt coalescing on a job ring
 * @param [in]  job_ring                The job ring
 */
int hw_job_ring_disable_coalescing(sec_job_ring_t *job_ring);

#endif // SEC_INT_COALESCING_ENABLE

#endif // SEC_HW_VERSION_4_4
/*============================================================================*/


#endif  /* SEC_HW_SPECIFIC_H */
