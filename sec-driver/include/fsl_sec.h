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

#ifndef FSL_SEC_H
#define FSL_SEC_H

#ifdef __cplusplus
/* *INDENT-OFF* */

extern "C"{
/* *INDENT-ON* */
#endif

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
/** Sequence number length */
#define SEC_PDCP_SN_SIZE_5  5
/** Sequence number length */
#define SEC_PDCP_SN_SIZE_7  7
/** Sequence number length */
#define SEC_PDCP_SN_SIZE_12 12

/*==================================================================================================
                                             ENUMS
==================================================================================================*/
/* Return codes for SEC user space driver APIs */
typedef enum sec_return_code_e
{
	SEC_SUCCESS = 0,             /*< Operation executed successfully.*/
	SEC_INVALID_INPUT_PARAM,     /*< API received an invalid argument. */
	SEC_OUT_OF_MEMORY,           /*< Memory allocation failed. */

}sec_return_code_t;

/* 
 * Status codes indicating SEC processing result for each packet, 
 * when SEC user space driver invokes User Application provided callback.
 * ET TODO: Detail error codes per source? DECO/Job ring/CCB ?
 */
typedef enum sec_status_e
{
    SEC_STATUS_SUCCESS = 0,     /*< SEC processed packet without error.*/
    SEC_STATUS_ERROR,           /*< SEC packet processing returned with error. */

}sec_status_t;

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
typedef uint64_t dma_addr_t;
#else
typedef uint32_t dma_addr_t;
#endif


/* Data type used for specifying addressing scheme for 
 * packets submitted by User Application. Assume virtual addressing */
//typedef uint8_t*  packet_addr_t;

/*  Data type used for specifying addressing scheme for
 *  packets submitted by User Application. Assume physical addressing */
typedef dma_addr_t  packet_addr_t;

/* Handle to a Job Ring */
typedef void* sec_job_ring_t;

/* Handle to a SEC Context */
typedef void* sec_context_handle_t;

/* Callback provided by the User Application when a SEC PDCP context is created.
 * Callback is invoked by SEC user space driver for each packet processed by SEC
 * belonging to a certain PDCP context.
 */
typedef void (*sec_out_cbk)(
        packet_addr_t   buffer,      /*< Buffer with SEC result. TODO: address is virtual or physical? */
        uint32_t        offset,     /*< Offset within buffer where SEC result starts. */
        uint32_t        buffer_len, /*< Length of buffer. */
        void            *opaque,    /*< Opaque data received from User Application when packet was submitted and passed back to UA. */
        int32_t         status);    /*< Status word indicating success or failure. */


/* PDCP context structure provided by User Application when a PDCP context is created.
 * User Application fills this structure with data that is used by SEC user space driver
 * to create a SEC descriptor. This descriptor is used by SEC to process all packets
 * belonging to this PDCP context.
 */
typedef struct sec_pdcp_context_info_s 
{
    uint8_t     sn_size;                /*< Sequence number can be represented on 5, 7 or 12 bits. 
                                            Select one from: SEC_PDCP_SN_SIZE_5, SEC_PDCP_SN_SIZE_7, SEC_PDCP_SN_SIZE_12. */
    uint8_t     bearer:5;               /*< Radio bearer id. */
    uint8_t     user_plane:1;           /*< Control plane versus Data plane indication TODO: confirm this!!! */
    uint8_t     packet_direction:1;     /*< Direction can be uplink(0) or downlink(1). */
    uint8_t     protocol_direction:1;   /*< Encryption/Description indication TODO: confirm this!!! */ 
    uint8_t     algorithm;              /*< Cryptographic algorithm used: SNOW/AES. TODO: confirm this!!! */
    uint32_t    hfn;                    /*< HFN for this radio bearer. Represents the most significant bits from sequence number. */
    uint8_t    *cipher_key;             /*< Ciphering key. */
    uint32_t    cipher_key_len;         /*< Ciphering key length. */
    uint8_t    *integrity_key;          /*< Integrity key. */
    uint32_t    integrity_key_len;      /*< Integrity key length. */
    void        *custom;                /*< User Application custom data for this PDCP context. Usage to be defined. */
    sec_out_cbk notify_packet;          /*< Callback function to be called for all packets processed on this context. */
} sec_pdcp_context_info_t;

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/*================================================================================================*/

/* @brief Initialize the SEC user space driver.
 *
 * This function will handle local data initialization,
 * mapping and initializing requested SEC's Job Rings.
 * Call once during application startup.
 *
 * @note Global SEC initialization is always done in SEC kernel driver.
 *
 * @param [in]  job_rings_no       The number of job rings to acquire and initialize.
 * @param [out] job_ring_handles   Array of job ring handles of size job_rings_no.
 *
 * @retval ::SEC_SUCCESS for successful execution
 * @retval ::SEC_OUT_OF_MEMORY is returned if memory allocation fails.
 * @retval >0 in case of error
 */
int sec_init(int job_rings_no, sec_job_ring_t **job_ring_handles);

/* @brief Release the resources used by the SEC user space driver.
 *
 * Reset SEC's channels indicated by the User Application at init and
 * free any memory allocated.
 * Call once during application teardown.
 *
 * @retval ::SEC_SUCCESS for successful execution
 * @retval >0 in case of error
 */
int sec_release();


/* @brief Initializes a SEC PDCP context with the data provided.
 * 
 * Creates a shared SEC descriptor that will be used by SEC to 
 * process packets submitted for this PDCP context. Context also 
 * registers a callback handler that is activated when packets are received from SEC.
 *
 * Returns back to the caller a SEC PDCP context handle.
 * This handle is passed back by the caller for all packet processing APIs.
 * Call for once for each PDCP context.
 *
 * @note SEC user space driver does not check if a PDCP context was already created with the same data!
 *
 * @param [in]  sec_ctx_info       PDCP context info filled by the caller.
 * @param [out] sec_ctx_handle     PDCP context handle returned by SEC user space driver.
 * 
 * @retval ::SEC_SUCCESS for successful execution
 * @retval ::SEC_OUT_OF_MEMORY when there is not enough internal memory to allocate the context
 * @retval >0 in case of error
 */

int sec_create_pdcp_context (sec_pdcp_context_info_t *sec_ctx_info, 
                             sec_context_handle_t *sec_ctx_handle);

/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //FSL_SEC_H
