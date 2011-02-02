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
/** Return codes for SEC user space driver APIs */
typedef enum sec_return_code_e
{
	SEC_SUCCESS = 0,             /*< Operation executed successfully.*/
	SEC_INVALID_INPUT_PARAM,     /*< API received an invalid argument. */
	SEC_OUT_OF_MEMORY,           /*< Memory allocation failed. */
    SEC_PACKETS_IN_FLIGHT,       /*< API function indicates there are packets in flight
                                     for SEC to process that belong to a certain PDCP context.
                                     Can be returned by sec_delete_pdcp_context().*/
    SEC_PROCESSING_ERROR,        /*< Indicates a SEC processing error occurred on a Job Ring which requires a Job Ring reset.
                                     The only API that can be called after this error is sec_release(). */

}sec_return_code_t;

/** 
 * Status codes indicating SEC processing result for each packet, 
 * when SEC user space driver invokes User Application provided callback.
 * TODO: Detail error codes per source? DECO/Job ring/CCB ?
 */
typedef enum sec_status_e
{
    SEC_STATUS_SUCCESS = 0,     /*< SEC processed packet without error.*/
    SEC_STATUS_ERROR,           /*< SEC packet processing returned with error. */
    SEC_STATUS_DISCARD,         /*< Packet discarded without being processed by SEC.
                                    This can happen when a Job Ring is reset and the Input Ring is not empty. */
    SEC_STATUS_OVERDUE,         /*< Indicates a packet processed by SEC for a PDCP context that was requested
                                    to be removed by the User Application.*/
    SEC_STATUS_LAST_OVERDUE,    /*< Indicates the last packet processed by SEC for a PDCP context that was requested
                                    to be removed by the User Application. It is safe for the User Application to reuse
                                    PDCP context related data. */

}sec_status_t;

typedef enum sec_ua__return_e
{
    SEC_RETURN_SUCCESS,         /*< User Application processed response notification with success. */
    SEC_RETURN_STOP,            /*< User Application wants to return from the polling API without
                                    processing any other SEC responses. */
}sec_ua_return_t;

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
typedef uint64_t dma_addr_t;
#else
typedef uint32_t dma_addr_t;
#endif


/**
 * Data type used for specifying addressing scheme for 
 * packets submitted by User Application. Assume virtual addressing */
//typedef uint8_t*  packet_addr_t;

/**
 * Data type used for specifying addressing scheme for
 *  packets submitted by User Application. Assume physical addressing */
typedef dma_addr_t  packet_addr_t;

/** Handle to a Job Ring */
typedef void* sec_job_ring_t;

/** Handle to a SEC Context */
typedef void* sec_context_handle_t;

/** @brief Function called by SEC user space driver to notify every processed packet.
 *
 * Callback provided by the User Application when a SEC PDCP context is created.
 * Callback is invoked by SEC user space driver for each packet processed by SEC
 * belonging to a certain PDCP context.
 *
 * @param [in] buffer       Buffer containing the packet processed by SEC.
 *                          This is the output buffer provided by the User Application
 *                          with sec_process_pdcp_in/[out]_packet () APIs.
 *                          TODO: address is virtual or physical?
 * @param [in] offset       Offset within buffer where SEC result starts.
 * @param [in] buffer_len   Length of buffer.
 * @param [in] opaque       Opaque data received from User Application when packet
 *                          was submitted which is not passed back to UA.
 * @param [in] status       Status word indicating processing result for this packet.
 *                          See ::sec_status_t type for possible values.
 * @param [in] error_info   Detailed error code, as reported by SEC device. Is set to value 0 for success processing.
 *                          In case of per packet error (status is #SEC_STATUS_ERROR), this filed contains the status word
 *                          as generated by SEC device. In case of discarded packets this field represents:
 *                          - for SEC 4.4: Job Ring Interrupt Status Register (Bits 0-32).
 *                          - for SEC 3.1: Channel Status Register (Bits 32-63).
 *
 * @retval Returns values from ::sec_ua_return_t enum.
 * @retval #SEC_RET_SUCCESS for successful execution
 * @retval #SEC_RET_STOP    The User Application must return this code when it desires to stop the polling process.
 */
typedef int (*sec_out_cbk)(packet_addr_t   buffer,
                           uint32_t        offset,
                           uint32_t        buffer_len,
                           void            *opaque,
                           int32_t         status,
                           int32_t         error_info);


/**
 * PDCP context structure provided by User Application when a PDCP context is created.
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
 * @retval #SEC_SUCCESS for successful execution
 * @retval #SEC_OUT_OF_MEMORY is returned if memory allocation fails.
 * @retval >0 in case of error
 */
int sec_init(int job_rings_no, sec_job_ring_t **job_ring_handles);

/* @brief Release the resources used by the SEC user space driver.
 *
 * Reset SEC's channels indicated by the User Application at init and
 * free any memory allocated.
 * Call once during application teardown.
 *
 * @retval #SEC_SUCCESS for successful execution
 * @retval >0 in case of error
 */
int sec_release();


/** @brief Initializes a SEC PDCP context with the data provided.
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
 * @retval #SEC_SUCCESS for successful execution
 * @retval #SEC_OUT_OF_MEMORY when there is not enough internal memory to allocate the context
 * @retval >0 in case of other errors TODO: define other errors, like SEC_DRIVER_SHUTDOWN.
 */
int sec_create_pdcp_context (sec_pdcp_context_info_t *sec_ctx_info, 
                             sec_context_handle_t *sec_ctx_handle);

/** @brief Deletes a SEC PDCP context previously created.
 * Deletes a PDCP context identified with the handle provided by the function caller.
 * The handle was obtained by the caller using sec_create_pdcp_context() function.
 * Call for once for each PDCP context.
 * If called when there are still some packets awaiting to be processed by SEC for this context,
 * the API will return #SEC_PACKETS_IN_FLIGHT. All per-context packets processed by SEC after
 * this API is invoked will be raised to the User Application having status field set to #SEC_STATUS_OVERDUE.
 * The last overdue packet will have status set to #SEC_STATUS_LAST_OVERDUE.
 * Only after the last overdue packet is notified, the User Application can be sure the PDCP context
 * was removed from SEC user space driver.
 * 
 * @param [in] sec_ctx_handle     PDCP context handle.
 * 
 * @retval #SEC_SUCCESS for successful execution
 * @retval #SEC_PACKETS_IN_FLIGHT in case there are some already submitted packets
 * for this context awaiting to be processed by SEC.
 * @retval >0 in case of error
 */
int sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle);

/** @brief Polls for available packets processed by SEC on all Job Rings initialized by the User Application.
 *
 * This function polls the SEC Job Rings and delivers processed packets to the User Application.
 * Each processed packet belongs to a PDCP context and each PDCP context has a sec_out_cbk registered.
 * This sec_out_cbk is invoked for each processed packet belonging to that PDCP context.
 *
 * The Job Rings are polled in a weighted round robin fashion using a fixed weight for each Job Ring.
 * The polling is stopped when <limit> packets are notified or when there are no more packets to notify.
 * User Application has an additional mechanism to stop the polling, that is by returning #SEC_RETURN_STOP from
 * sec_out_cbk.
 *
 * @note The sec_poll() API cannot be called from within a sec_out_cbk function.
 *
 * @param [in]  limit   This value represents the maximum number of processed packets
 *                      that can be notified to the User Aplication by this API call, on all Job Rings.
 *                      Note that fewer packets may be notified if enough processed packets are not available.
 * @param [in]  weight  Assuming a weighted round robin scheduling algorithm for polling Job Rings, this weight indicates
 *                      how many packets can be notified from a Job Ring in a single scheduling step.
 *
 * @retval #SEC_SUCCESS             for successful execution.
 * @retval #SEC_PROCESSING_ERROR    indicates a fatal execution error that requires a SEC user space driver shutdown.
 */
int sec_poll(uint32_t limit,  uint32_t weight);

/** @brief Polls for available packets processed by SEC on a specific Job Ring initialized by the User Application.
 *
 * This function polls the SEC Job Rings and delivers processed packets to the User Application.
 * Each processed packet belongs to a PDCP context and each PDCP context has a sec_out_cbk registered.
 * This sec_out_cbk is invoked for each processed packet belonging to that PDCP context.
 *
 * The polling is stopped when <limit> packets are notified or when there are no more packets to notify.
 * User Application has an additional mechanism to stop the polling, that is by returning #SEC_RETURN_STOP from
 * sec_out_cbk.
 *
 * @note The sec_poll_job_ring() API cannot be called from within a sec_out_cbk function.
 *
 * @param [in]  job_ring_handle     The Job Ring handle.
 * @param [in]  limit               This value represents the maximum number of processed packets
 *                                  that can be notified to the User Aplication by this API call on this Job Ring.
 *                                  Note that fewer packets may be notified if enough processed packets are not available.
 *
 * @retval #SEC_SUCCESS             for successful execution.
 * @retval #SEC_PROCESSING_ERROR    indicates a fatal execution error that requires a SEC user space driver shutdown.
 */
int sec_poll_job_ring(sec_job_ring_t *job_ring_handle, uint32_t limit);

/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //FSL_SEC_H
