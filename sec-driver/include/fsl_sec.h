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
#include "fsl_sec_config.h"
#include <stdint.h>

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

/*==================================================================================================
                                             ENUMS
==================================================================================================*/
/** Return codes for SEC user space driver APIs */
typedef enum sec_return_code_e
{
    SEC_SUCCESS = 0,                 /*< Operation executed successfully.*/
    SEC_INVALID_INPUT_PARAM,         /*< API received an invalid input parameter. */
    SEC_CONTEXT_MARKED_FOR_DELETION, /*< The SEC context is scheduled for deletion and no more packets
                                         are allowed to be processed for the respective context. */
    SEC_OUT_OF_MEMORY,               /*< Memory allocation failed. */
    SEC_PACKETS_IN_FLIGHT,           /*< API function indicates there are packets in flight
                                         for SEC to process that belong to a certain PDCP context.
                                         Can be returned by sec_delete_pdcp_context().*/
    SEC_LAST_PACKET_IN_FLIGHT,       /*< API function indicates there is one last packet in flight
                                         for SEC to process that belongs to a certain PDCP context.
                                         When processed, the last packet in flight willl be notified to
                                         User Application with a status of #SEC_STATUS_SUCCESS or
                                         #SEC_STATUS_LAST_OVERDUE.
                                         Can be returned by sec_delete_pdcp_context().*/
    SEC_PROCESSING_ERROR,            /*< Indicates a SEC processing error occurred on a Job Ring which requires a 
                                         SEC user space driver shutdown. Call sec_get_last_error() to obtain specific
                                         error code, as reported by SEC device.
                                         Then the only other API that can be called after this error is sec_release(). */
    SEC_JR_RESET_FAILED,             /*< Job Ring reset failed. */
    SEC_JR_IS_FULL,                  /*< Job Ring is full. There is no more room in the JR for new packets.
                                         This can happen if the packet RX rate is higher than SEC's capacity. */
    SEC_DRIVER_RELEASE_IN_PROGRESS,  /*< SEC driver shutdown is in progress and no more context
                                         creation/deletion, packets processing or polling is allowed.*/
    SEC_DRIVER_ALREADY_INITALIZED,   /*< SEC driver is already initialized. */
    SEC_DRIVER_NOT_INITALIZED,       /*< SEC driver is NOT initialized. */
    SEC_DRIVER_NO_FREE_CONTEXTS,     /*< There are no more free contexts. Considering increasing the
                                         maximum number of contexts: #SEC_MAX_PDCP_CONTEXTS.*/
}sec_return_code_t;

/** Status codes indicating SEC processing result for each packet, 
 *  when SEC user space driver invokes User Application provided callback.
 *  TODO: Detail SEC specific error codes per source? DECO/Job ring/CCB ?
 */
typedef enum sec_status_e
{
    SEC_STATUS_SUCCESS = 0,     /*< SEC processed packet without error.*/
    SEC_STATUS_ERROR,           /*< SEC packet processing returned with error. */
    SEC_STATUS_OVERDUE,         /*< Indicates a packet processed by SEC for a PDCP context that was requested
                                    to be removed by the User Application.*/
    SEC_STATUS_LAST_OVERDUE,    /*< Indicates the last packet processed by SEC for a PDCP context that was requested
                                    to be removed by the User Application. After the last overdue packet is notified 
                                    to UA, it is safe for UA to reuse PDCP context related data. */

}sec_status_t;

/** Return codes for User Application registered callback sec_out_cbk. 
 */
typedef enum sec_ua_return_e
{
    SEC_RETURN_SUCCESS,         /*< User Application processed response notification with success. */
    SEC_RETURN_STOP,            /*< User Application wants to return from the polling API without
                                    processing any other SEC responses. */
}sec_ua_return_t;


typedef enum packet_type_e
{
    SEC_CONTIGUOUS_BUFFER = 0,
    SEC_SCATTER_GATHER_BUFFER
}packet_type_t;
/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

#if defined(__powerpc64__) || defined(CONFIG_PHYS_64BIT)
/** Physical address on 36 bits or more. MUST be kept in synch with same define from kernel! */
typedef uint64_t dma_addr_t;
#else
/** Physical address on 32 bits. MUST be kept in synch with same define from kernel!*/
typedef uint32_t dma_addr_t;
#endif

/** Physical address. */
typedef dma_addr_t  phys_addr_t;

/**Data type used for specifying addressing scheme for
 * packets submitted by User Application. Assume virtual addressing */

/** Data type used for specifying address for packets submitted
 *  by User Application. Assume physical addressing. */
typedef dma_addr_t  packet_addr_t;

/** Opaque handle to a Job Ring provided by SEC user space driver
 *  to UA when sec_init() is called. */
typedef void* sec_job_ring_handle_t;

/** Handle to a SEC PDCP Context */
typedef void* sec_context_handle_t;

/** UA opaque handle to a packet context.
 *  The handle is opaque from SEC driver's point of view. */
typedef void* ua_context_handle_t;

/** Structure used to describe an input or output packet accessed by SEC. */
typedef struct sec_packet_s
{
    packet_addr_t   address;        /*< The physical address (not virtual address) of the input buffer. */
    uint32_t        offset;         /*< Offset within packet from where SEC will access (read or write) data. */
    uint32_t        length;         /*< Packet length. */
    packet_type_t   scatter_gather; /*< A value of #SEC_SCATTER_GATHER_BUFFER indicates the packet is
                                        passed as a scatter/gather table.
                                        A value of #SEC_CONTIGUOUS_BUFFER means the packet is contiguous
                                        in memory and is accessible at the given address.
                                        TODO: export format for link table.
                                        TODO: how is offset interpreted when packet is s/g ? */
}sec_packet_t;


/** Contains Job Ring descriptor info returned to the caller when sec_init() is invoked. */
typedef struct sec_job_ring_descriptor_s
{
    sec_job_ring_handle_t job_ring_handle;     /*< The job ring handle is provided by the library for UA usage.
                                                   The handle is opaque from UA point of view.
                                                   The handle can be used by UA to poll for events per JR. */
    int job_ring_irq_fd;                       /*< File descriptor that can be used by UA with epoll() to wait for
                                                   interrupts generated by SEC for this Job Ring. */
}sec_job_ring_descriptor_t;


/** @brief      Translates the virtual address to physical address.
 *
 * @param [in] address  The virtual address which has to be mapped.
 *
 * @retval      Physical address which the passed virtual address maps to
 *              A value of -1 is returned if the passed virtual address
 *              doesn't map to any physical address.
 */
typedef phys_addr_t (*vtop_function)(void *address);

/** @brief      Translates the physical address to virtual address.
 *
 * @param [in]  address The physical address which has to be mapped.
 *
 * @retval      Virtual address to which the passed physical address maps onto.
 *              NULL is returned if the passed physical address doesn't map to any virtual address.
 */
typedef void* (*ptov_function)(phys_addr_t address);

/** @brief Function called by SEC user space driver to notify every processed packet.
 *
 * Callback provided by the User Application when a SEC PDCP context is created.
 * Callback is invoked by SEC user space driver for each packet processed by SEC
 * belonging to a certain PDCP context.
 *
 * @param [in] in_packet          Input packet read by SEC.
 * @param [in] out_packet         Output packet where SEC writes result.
 * @param [in] ua_ctx_handle      Opaque handle received from User Application when packet
 *                                was submitted for processing.
 * @param [in] status             Status word indicating processing result for this packet.
 *                                See ::sec_status_t type for possible values.
 * @param [in] error_info         Detailed error code, as reported by SEC device. Is set to value 0 for success processing.
 *                                In case of per packet error (status is #SEC_STATUS_ERROR),
 *                                this field contains the status word as generated by SEC device.
 *
 * @retval Returns values from ::sec_ua_return_t enum.
 * @retval #SEC_RETURN_SUCCESS for successful execution
 * @retval #SEC_RETURN_STOP    The User Application must return this code when it desires to stop the polling process.
 */
typedef int (*sec_out_cbk)(sec_packet_t        *in_packet,
                           sec_packet_t        *out_packet,
                           ua_context_handle_t ua_ctx_handle,
                           sec_status_t        status,
                           uint32_t            error_info);


/** PDCP context structure provided by User Application when a PDCP context is created.
 *  User Application fills this structure with data that is used by SEC user space driver
 *  to create a SEC descriptor. This descriptor is used by SEC to process all packets
 *  belonging to this PDCP context. */
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


/** Configuration data structure that must be provided by UA when SEC user space driver is initialized */
typedef struct sec_config_s
{
    void            *memory_area;
    ptov_function   ptov;
    vtop_function   vtop;
#ifdef SEC_HW_VERSION_4_4
    uint32_t        irq_coalescing_timer;   /*< Interrupt Coalescing Timer Threshold.
                                                While interrupt coalescing is enabled (ICEN=1), this value determines the
                                                maximum amount of time after processing a Descriptor before raising an interrupt.
                                                The threshold value is represented in units equal to 64 CAAM interface
                                                clocks. Valid values for this field are from 1 to 65535.
                                                A value of 0 results in behavior identical to that when interrupt
                                                coalescing is disabled.*/

    uint8_t         irq_coalescing_count;   /*< Interrupt Coalescing Descriptor Count Threshold.
                                                While interrupt coalescing is enabled (ICEN=1), this value determines
                                                how many Descriptors are completed before raising an interrupt.
                                                Valid values for this field are from 0 to 255.
                                                Note that a value of 1 functionally defeats the advantages of interrupt
                                                coalescing since the threshold value is reached each time that a
                                                Job Descriptor is completed. A value of 0 is treated in the same
                                                manner as a value of 1.*/
#endif
    uint8_t         work_mode;              /*< Choose between hardware poll vs interrupt notification when driver is initialized. 
                                                Valid values are #SEC_STARTUP_POLLING_MODE and #SEC_STARTUP_INTERRUPT_MODE.*/
}sec_config_t;


/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/**
 * @brief Initialize the SEC user space driver.
 *
 * This function will handle local data initialization,
 * mapping and initialization of requested SEC's Job Rings.
 * Call once during application startup.
 *
 * @note Global SEC initialization is done in SEC kernel driver.
 *
 * @note The hardware IDs of the initialized Job Rings are opaque to the UA.
 * The exact Job Rings used by this library are decided between SEC user
 * space driver and SEC kernel driver. A static partitioning of Job Rings is assumed.
 * See define #SEC_ASSIGNED_JOB_RINGS.
 *
 * @param [in]  sec_config_data         Configuration data required to initialize SEC user space driver.
 * @param [in]  job_rings_no            The number of job rings to acquire and initialize.
 * @param [out] job_ring_descriptors    Array of job ring descriptors of size job_rings_no. The job
 *                                      ring descriptors are provided by the library for UA usage.
 *                                      The storage for the array is allocated by the library.
 *
 * @retval #SEC_SUCCESS                     for successful execution
 * @retval #SEC_OUT_OF_MEMORY is returned   if internal memory allocation fails
 * @retval #SEC_PROCESSING_ERROR            indicates a fatal execution error that requires a SEC user space driver shutdown.
 *                                          Call sec_get_last_error() to obtain specific error code, as reported by SEC device.
 * @retval >0 in case of error
 */
sec_return_code_t sec_init(sec_config_t *sec_config_data,
                           uint8_t job_rings_no,
                           sec_job_ring_descriptor_t **job_ring_descriptors);

/**
 * @brief Release the resources used by the SEC user space driver.
 *
 * Reset and release SEC's job rings indicated by the User Application at
 * sec_init() and free any memory allocated internally.
 * Call once during application tear down.
 *
 * @note In case there are any packets in-flight (packets received by SEC driver
 * for processing and for which no response was yet provided to UA), the packets
 * are discarded without any notifications to User Application.
 *
 * @retval #SEC_SUCCESS         is returned for a successful execution
 * @retval #SEC_JR_RESET_FAILED is returned in case the job ring reset fails
 * @retval >0 in case of error
 */
sec_return_code_t sec_release();

/** @brief Initializes a SEC PDCP context with the data provided.
 * 
 * Creates a SEC descriptor that will be used by SEC to process packets
 * submitted for this PDCP context. Context also registers a callback handler
 * that is activated when packets are received from SEC.
 *
 * Returns back to the caller a SEC PDCP context handle.
 * This handle is passed back by the caller for sec_process_packet() calls.
 * Call once for each PDCP context.
 *
 * PDCP context is affined to a Job Ring. This means that every packet submitted on this
 * context will be processed by the same Job Ring. Hence, packet ordering is ensured at a
 * PDCP context level. If the caller does not specify a certain Job Ring for this context,
 * the SEC userspace driver will choose one from the available Job Rings, in a round robin fashion.
 *
 * @note SEC user space driver does not check if a PDCP context was already created with the same data!
 *
 * @param [in]  job_ring_handle    The Job Ring this PDCP context will be affined to.
 *                                 If set to NULL, the SEC user space driver will affine PDCP context
 *                                 to one from the available Job Rings, in a round robin fashion.
 * @param [in]  sec_ctx_info       PDCP context info filled by the caller.
 * @param [out] sec_ctx_handle     PDCP context handle returned by SEC user space driver.
 * 
 * @retval #SEC_SUCCESS for successful execution
 * @retval #SEC_DRIVER_NO_FREE_CONTEXTS when there are no more free contexts
 * @retval #SEC_DRIVER_RELEASE_IN_PROGRESS is returned if SEC driver release is in progress
 * @retval >0 in case of error
 */
sec_return_code_t sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                                           sec_pdcp_context_info_t *sec_ctx_info,
                                           sec_context_handle_t *sec_ctx_handle);

/** @brief Deletes a SEC PDCP context previously created.
 *
 * Deletes a PDCP context identified with the handle provided by the function caller.
 * The handle was obtained by the caller using sec_create_pdcp_context() function.
 * Call once for each PDCP context.
 * If called when there are still some packets awaiting to be processed by SEC for this context,
 * the API will return #SEC_PACKETS_IN_FLIGHT. All per-context packets processed by SEC after
 * this API is invoked will be raised to the User Application having status field set to #SEC_STATUS_OVERDUE.
 * The last overdue packet will have status set to #SEC_STATUS_LAST_OVERDUE.
 * Only after the last overdue packet is notified, the User Application can be sure the PDCP context
 * was removed from SEC user space driver.
 * 
 * @param [in] sec_ctx_handle     PDCP context handle.
 * 
 * @retval #SEC_SUCCESS                     for successful execution
 * @retval #SEC_PACKETS_IN_FLIGHT           in case there are some already submitted packets
 *                                          for this context awaiting to be processed by SEC.
 * @retval #SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress
 * @retval #SEC_INVALID_CONTEXT_HANDLE      is returned in case the SEC context handle is invalid
 * @retval >0 in case of error
 */
sec_return_code_t sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle);

/** @brief Polls for available packets processed by SEC on all Job Rings initialized by the User Application.
 *
 * This function polls the SEC Job Rings and delivers processed packets to the User Application.
 * Each processed packet belongs to a PDCP context and each PDCP context has a sec_out_cbk registered.
 * This sec_out_cbk is invoked for each processed packet belonging to that PDCP context.
 *
 * The Job Rings are polled in a weighted round robin fashion using a fixed weight for each Job Ring.
 * The polling is stopped when <limit> packets are notified or when there are no more packets to notify.
 * User Application has an additional mechanism to stop the polling, that is by returning #SEC_RETURN_STOP
 * from sec_out_cbk.
 *
 * @note In case the "limit" is negative and the rate of packet processing is high the function could poll
 * the JRs indefinitely.
 *
 * The SEC user space driver assumes a Linux NAPI style to deliver SEC ready packets to UA.
 * What this means is: if all ready packets are delivered to UA and no more ready packets 
 * are awaiting to be retrieved from SEC device then this function will enable SEC interrupt generation
 * on all job rings before it returns. In other words, IRQs are enabled if:
 * - (packets_no < limit) OR (limit < 0)
 * SEC interrupts for user space dedicated Job Rings are ALWAYS disabled in interrupt handler from SEC kernel driver.
 *
 * @note The sec_poll() API cannot be called from within a sec_out_cbk function!
 *
 * @param [in]  limit       This value represents the maximum number of processed packets
 *                          that can be notified to the User Aplication by this API call, on all Job Rings.
 *                          Note that fewer packets may be notified if enough processed packets are not available.
 *                          If limit has a negative value, then all ready packets will be notified.
 *                          A limit of zero is considered invalid.
 *                          A limit less or equal than weight is considered invalid.
 * @param [in]  weight      This value indicates how many packets can be notified from a Job Ring in
 *                          a single scheduling step of the weighted round robin scheduling algorithm.
 *                          A weight of zero is considered invalid.
 *                          A weight bigger or equal than limit is considered invalid.
 * @param [out] packets_no  Number of packets notified to the User Application during this function call.
 *                          Can be NULL if User Application does not need this information.
 *
 * @retval #SEC_SUCCESS                     for successful execution.
 * @retval #SEC_PROCESSING_ERROR            indicates a fatal execution error that requires a SEC user space driver shutdown.
 *                                          Call sec_get_last_error() to obtain specific error code, as reported by SEC device.
 * @retval #SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress
 * @retval #SEC_INVALID_INPUT_PARAM         is returned if limit == 0 or weight == 0 or
 *                                          limit <= weight or weight > #SEC_JOB_RING_SIZE
 */
sec_return_code_t sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no);

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
 * The SEC user space driver assumes a Linux NAPI style to deliver SEC ready packets to UA.
 * What this means is: if all ready packets are delivered to UA and no more ready packets
 * are awaiting to be retrieved from this SEC's job ring then this function will enable SEC interrupt generation
 * on this job ring before it returns. In other words, IRQs are enabled if:
 * - (packets_no < limit) OR (limit < 0)
 * SEC interrupts per job ring are disabled in interrupt handler from SEC kernel driver.
 * 
 * @note The sec_poll_job_ring() API cannot be called from within a sec_out_cbk function!
 *
 * @param [in]  job_ring_handle     The Job Ring handle.
 * @param [in]  limit               This value represents the maximum number of processed packets
 *                                  that can be notified to the User Aplication by this API call on this Job Ring.
 *                                  Note that fewer packets may be notified if enough processed packets are not available.
 *                                  If limit has a negative value, then all ready packets will be notified.
 * @param [out] packets_no          Number of packets notified to the User Application during this function call.
 *
 * @retval #SEC_SUCCESS                    for successful execution.
 * @retval #SEC_PROCESSING_ERROR           indicates a fatal execution error that requires a SEC user space driver shutdown.
 *                                         Call sec_get_last_error() to obtain specific error code, as reported by SEC device.
 * @retval #SEC_DRIVER_RELEASE_IN_PROGRESS is returned if SEC driver release is in progress
 */
sec_return_code_t sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle,
                                    int32_t limit,
                                    uint32_t *packets_no);

/**
 * @brief Submit a packet for SEC processing on a specified context.
 *
 * This function creates a "job" which is meant to instruct SEC HW
 * to perform the processing associated to the packet's SEC context
 * on the input buffer. The "job" is enqueued in the Job Ring associated
 * to the packet's SEC context. The function will return after the "job"
 * enqueue is finished. The function will not wait for SEC to
 * start or/and finish the "job" processing.
 *
 * After the processing is finished the SEC HW writes the processing result
 * to the provided output buffer.
 *
 * The User Application must poll SEC driver using sec_poll() or sec_poll_job_ring() to
 * receive notifications of the processing completion status. The notifications are received
 * by UA by means of callback (see ::sec_out_cbk).
 *
 *
 * @param [in]  sec_ctx_handle     The handle of the context associated to this packet.
 *                                 This handle is opaque from the User Application point of view.
 *                                 SEC driver uses this handle to identify the processing type
 *                                 required for this packet.
 * @param [in]  in_packet          Input packet read by SEC.
 * @param [in]  out_packet         Output packet where SEC writes result.
 * @param [in]  ua_ctx_handle      The handle to a User Application packet context.
 *                                 This handle is opaque from the SEC driver's point of view and
 *                                 will be provided by SEC driver in the response callback.
 *
 *
 * @retval #SEC_SUCCESS is returned for successful execution
 * @retval #SEC_INVALID_INPUT_PARAM is returned in case the SEC context handle is invalid (e.g. corrupt handle)
 *                                  or offset for input/output buffer is invalid (e.g offset > length)
 *                                  or length of the input/output buffer is invalid (e.q. zero)
 *                                  or input/output buffer address is invalid (e.g. NULL)
 *
 * @retval #SEC_JR_IS_FULL                  is returned if the JR is full
 * @retval #SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress
 * @retval #SEC_CONTEXT_MARKED_FOR_DELETION is returned if the SEC context was marked for deletion.
 * @retval #SEC_PROCESSING_ERROR            indicates a fatal execution error that requires a SEC user space driver shutdown.
 *                                          Call sec_get_last_error() to obtain specific error code, as reported by SEC device.
 * @retval >0 in case of error
 */
sec_return_code_t sec_process_packet(sec_context_handle_t sec_ctx_handle,
                                     sec_packet_t *in_packet,
                                     sec_packet_t *out_packet,
                                     ua_context_handle_t ua_ctx_handle);


/** @brief Returns the last SEC user space driver error, if any.
 *
 * Use this function to query SEC user space driver status after an
 * API function returned a #SEC_PROCESSING_ERROR error code.
 * The last error is a thread specific value. Call this function on the same
 * thread that received #SEC_PROCESSING_ERROR error.
 *
 * @note After an API returns #SEC_PROCESSING_ERROR code, besides calling sec_get_last_error()
 * the only other valid API to call is sec_release().
 *
 * @retval Returns specific error code, as reported by SEC device.
 *         On SEC 3.1, the error is extracted from Channel Status Register (CSR), bits [32:63].
 *         On SEC 4.4, the error is extracted from Job Ring Interrupt Status Register (JRINT), bits [0:32].
 */
uint32_t sec_get_last_error(void);

const char* sec_get_status_message(sec_status_t status);
const char* sec_get_error_message(sec_return_code_t error);

/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //FSL_SEC_H
