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
#define SEC_HW_ERR_CCB_ICV_CHECK_FAIL       0x0A


/******************************************************************
 * Constants for descriptors
 *****************************************************************/

#define CMD_HDR_CTYPE_SD                        0x16
#define CMD_HDR_CTYPE_JD                        0x17

#define CMD_PROTO_SNOW_ALG                      0x01
#define CMD_PROTO_AES_ALG                       0x02

/* RLC specific defines */
#define CMD_PROTO_RLC_NULL_ALG                  0x00
#define CMD_PROTO_RLC_KASUMI_ALG                0x01
#define CMD_PROTO_RLC_SNOW_ALG                  0x02

#define CMD_PROTO_DECAP                         0x06
#define CMD_PROTO_ENCAP                         0x07

#define CMD_ALGORITHM_ICV                       0x02

#define CMD_ALGORITHM_ENCRYPT                   0x01

#define CMD_DPOVRD_EN                           (1<<31)

/** The maximum size of a SEC descriptor, in WORDs (32 bits). */
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
#define MAX_DESC_SIZE_WORDS                     51
#else
#define MAX_DESC_SIZE_WORDS                     53
#endif

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
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
#define hw_get_inp_queue_base(jr)   ( (((dma_addr_t)GET_JR_REG(IRBA,(jr))) << 32) | \
                                       (GET_JR_REG_LO(IRBA,(jr))) )

/** Read pointer to job ring output ring start address */
#define hw_get_out_queue_base(jr)   ( ((dma_addr_t)(GET_JR_REG(ORBA,(jr))) << 32) | \
                                       (GET_JR_REG_LO(ORBA,(jr))) )
#else
#define hw_get_inp_queue_base(jr)   ( (dma_addr_t)(GET_JR_REG_LO(IRBA,(jr)) ) )

#define hw_get_out_queue_base(jr)   ( (dma_addr_t)(GET_JR_REG_LO(ORBA,(jr)) ) )
#endif

/**
 * IRJA - Input Ring Jobs Added Register shows 
 * how many new jobs were added to the Input Ring.
 */
#define hw_enqueue_packet_on_job_ring(job_ring) \
    SET_JR_REG(IRJA, (job_ring), 1);

#define hw_set_input_ring_size(job_ring,size)   SET_JR_REG(IRSR,job_ring,(size))

#define hw_set_output_ring_size(job_ring,size)  SET_JR_REG(ORSR,job_ring,(size))

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
#define hw_set_input_ring_start_addr(job_ring, start_addr)  \
{                                                           \
    SET_JR_REG(IRBA,job_ring,PHYS_ADDR_HI(start_addr));     \
    SET_JR_REG_LO(IRBA,job_ring,PHYS_ADDR_LO(start_addr));  \
}

#define hw_set_output_ring_start_addr(job_ring, start_addr) \
{                                                           \
    SET_JR_REG(ORBA,job_ring,PHYS_ADDR_HI(start_addr));     \
    SET_JR_REG_LO(ORBA,job_ring,PHYS_ADDR_LO(start_addr));  \
}

#else //#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
#define hw_set_input_ring_start_addr(job_ring, start_addr) \
        SET_JR_REG_LO(IRBA,job_ring,start_addr)

#define hw_set_output_ring_start_addr(job_ring, start_addr) \
        SET_JR_REG_LO(ORBA,job_ring,start_addr)

#endif

/** This macro will 'remove' one entry from the output ring of the Job Ring once
 * the software has finished processing the entry. */
#define hw_remove_one_entry(jr)                 hw_remove_entries(jr,1)

/** ORJR - Output Ring Jobs Removed Register shows how many jobs were
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
    ( COND_EXPR1_EQ_AND_EXPR2_EQ( ((union hw_error_code)(error)).error_desc.deco_src.ssrc,             \
                                  SEC_HW_ERR_SSRC_DECO,                                                \
                                  ((union hw_error_code)(error)).error_desc.deco_src.desc_err,         \
                                  SEC_HW_ERR_DECO_HFN_THRESHOLD )                                      \
    ||                                                                                                 \
    COND_EXPR1_EQ_AND_EXPR2_EQ( ((union hw_error_code)(error)).error_desc.jmp_halt_user_src.ssrc,      \
                                SEC_HW_ERR_SSRC_JMP_HALT_U,                                            \
                                ((union hw_error_code)(error)).error_desc.jmp_halt_user_src.offset,    \
                                SEC_HW_ERR_DECO_HFN_THRESHOLD )                                        \
    )
    
#define ICV_CHECK_FAIL(error)  \
    COND_EXPR1_EQ_AND_EXPR2_EQ( ((union hw_error_code)(error)).error_desc.ccb_status_src.ssrc,  \
                                SEC_HW_ERR_SSRC_CCB_ERR,                                        \
                                ((union hw_error_code)(error)).error_desc.ccb_status_src.err_id,\
                                SEC_HW_ERR_CCB_ICV_CHECK_FAIL )

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
/** Macro for setting the SD pointer in a JD. Common for all protocols
 * supported by the SEC driver.
 */
#define SEC_JD_SET_SD(descriptor,ptr,len)           {          \
    (descriptor)->sd_ptr = (ptr);                               \
    (descriptor)->deschdr.command.jd.shr_desc_len = (len);      \
}

/** Macro for setting a pointer to the job which this descriptor processes.
 * It eases the lookup procedure for identifying the descriptor that has
 * completed.
 */
#define SEC_JD_SET_JOB_PTR(descriptor,ptr) \
        (descriptor)->job_ptr = (ptr)

/** Macro for setting up a JD. The structure of the JD is common across all
 * supported protocols, thus its structure is identical.
 */
#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
#define SEC_JD_INIT(descriptor)              { \
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
        (descriptor)->deschdr.command.word = 0xB0801C0D;        \
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
         /* 
          * In order to be compatible with QI scenarios, the DPOVRD value
          * loaded must be formated like this:
          * DPOVRD_EN (1b) | Res| DPOVRD Value (right aligned).
          */                                                    \
        (descriptor)->load_dpovrd.command.word = 0x16870004;    \
        /* By default, DPOVRD mechanism is disabled, thus the value to be
         * LOAD-ed through the above descriptor command will be 0x0000_0000. 
         */                                                     \
        (descriptor)->dpovrd = 0x00000000;                      \
    }
#else
#define SEC_JD_INIT(descriptor)              { \
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
        (descriptor)->deschdr.command.word = 0xB0801C0A;        \
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
         /* 
          * In order to be compatible with QI scenarios, the DPOVRD value
          * loaded must be formated like this:
          * DPOVRD_EN (1b) | Res| DPOVRD Value (right aligned).
          */                                                    \
        (descriptor)->load_dpovrd.command.word = 0x16870004;    \
        /* By default, DPOVRD mechanism is disabled, thus the value to be
         * LOAD-ed through the above descriptor command will be 0x0000_0000. 
         */                                                     \
        (descriptor)->dpovrd = 0x00000000;                      \
    }
#endif

/** Macro for setting the pointer to the input buffer in the JD, according to 
 * the parameters set by the user in the ::sec_packet_t structure.
 */
#define SEC_JD_SET_IN_PTR(descriptor,phys_addr,offset,length) {     \
    (descriptor)->seq_in_ptr = (phys_addr) + (offset);              \
    (descriptor)->in_ext_length = (length);                         \
}

/** Macro for setting the pointer to the output buffer in the JD, according to
 * the parameters set by the user in the ::sec_packet_t structure.
 */
#define SEC_JD_SET_OUT_PTR(descriptor,phys_addr,offset,length) {    \
    (descriptor)->seq_out_ptr = (phys_addr) + (offset);             \
    (descriptor)->out_ext_length = (length);                        \
}

/** Macro for setting the Scatter-Gather flag in the SEQ IN command. Used in 
 * case the input buffer is split in multiple buffers, according to the user
 * specification.
 */
#define SEC_JD_SET_SG_IN(descriptor) \
     ( (descriptor)->seq_in.command.field.sgf =  1 )

/** Macro for setting the Scatter-Gather flag in the SEQ OUT command. Used in 
 * case the output buffer is split in multiple buffers, according to the user
 * specification.
 */
#define SEC_JD_SET_SG_OUT(descriptor) \
   ( (descriptor)->seq_out.command.field.sgf = 1 )

#define SEC_JD_SET_DPOVRD(descriptor) \

/** Macro for retrieving a descriptor's length. Works for both SD and JD. */
#define SEC_GET_DESC_LEN(descriptor)                                                        \
    (((struct descriptor_header_s*)(descriptor))->command.sd.ctype ==                       \
    CMD_HDR_CTYPE_SD ? ((struct descriptor_header_s*)(descriptor))->command.sd.desclen :    \
                       ((struct descriptor_header_s*)(descriptor))->command.jd.desclen )

/** Helper macro for dumping the hex representation of a descriptor */
#define SEC_DUMP_DESC(descriptor) {                                         \
        int __i;                                                            \
        SEC_DEBUG("Descriptor @ 0x%08x",(uint32_t)((uint32_t*)(descriptor)));\
        for( __i = 0;                                                       \
            __i < SEC_GET_DESC_LEN(descriptor);                             \
            __i++)                                                          \
        {                                                                   \
            SEC_DEBUG("0x%08x: 0x%08x",                                     \
                     (uint32_t)(((uint32_t*)(descriptor)) + __i),           \
                     *(((uint32_t*)(descriptor)) + __i));                   \
        }                                                                   \
}
/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
/** Forward structure declaration */
typedef struct sec_job_ring_t sec_job_ring_t;

/** Union describing the possible error codes that
 * can be set in the descriptor status word
 */
union hw_error_code{
    uint32_t    error;
    union{
        struct{
            uint32_t    ssrc:4;
            uint32_t    ssed_val:28;
        }__packed value;
        struct {
            uint32_t    ssrc:4;
            uint32_t     res:28;
        }__packed no_status_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    jmp:1;
            uint32_t    res:11;
            uint32_t    desc_idx:8;
            uint32_t    cha_id:4;
            uint32_t    err_id:4;
        }__packed ccb_status_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    jmp:1;
            uint32_t    res:11;
            uint32_t    desc_idx:8;
            uint32_t    offset:8;
        }__packed jmp_halt_user_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    jmp:1;
            uint32_t    res:11;
            uint32_t    desc_idx:8;
            uint32_t    desc_err:8;
        }__packed deco_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    res:17;
            uint32_t    naddr:3;
            uint32_t    desc_err:8;
        }__packed jr_src;
        struct {
            uint32_t    ssrc:4;
            uint32_t    jmp:1;
            uint32_t    res:11;
            uint32_t    desc_idx:8;
            uint32_t    cond:8;
        }__packed jmp_halt_cond_src;
    }__packed error_desc;
}__packed;

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
    } __packed command;
} __packed;

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
        } __packed field;
    } __packed command;
} __packed;

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
        } __packed field;
    } __packed command;
} __packed;

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
    } __packed command;
}__packed;

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
    } __packed command;
} __packed;

struct load_command_s{
    union {
        uint32_t word;
        struct {
            unsigned int ctype:5;
            unsigned int class:2;
            unsigned int sgf:1;
            unsigned int imm:1;
            unsigned int dst:7;
            unsigned char offset;
            unsigned char length;
        } fields;
    } __packed command;
} __packed;

/** Structure encompassing a general shared descriptor of maximum
 * size (64 WORDs). Usually, other specific shared descriptor structures 
 * will be type-casted to this one
 * this one.
 */
struct sec_sd_t {
    uint32_t rsvd[MAX_DESC_SIZE_WORDS];
} ____cacheline_aligned __packed;

/** Structure encompassing a job descriptor which processes
 * a single packet from a context. The job descriptor references
 * a shared descriptor from a SEC context.
 */
struct sec_descriptor_t {
    struct descriptor_header_s deschdr;
    dma_addr_t    sd_ptr;
    struct seq_out_command_s seq_out;
    dma_addr_t    seq_out_ptr;
    uint32_t      out_ext_length;
    struct seq_in_command_s seq_in;
    dma_addr_t    seq_in_ptr;
    uint32_t      in_ext_length;
    struct load_command_s load_dpovrd;
    uint32_t      dpovrd;
    struct sec_job_t *job_ptr;
//#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
//    uint32_t      pad[2];
//#else
//    uint32_t      pad[5];
//#endif
} ____cacheline_aligned __packed;
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

/** @brief Handle a job ring error in the device.
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

/*============================================================================*/


#endif  /* SEC_HW_SPECIFIC_H */
