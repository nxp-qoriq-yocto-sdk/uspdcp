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
/**
    @addtogroup SecUserSpaceDriverFunctions
    @{
 */

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
#include "fsl_sec_config.h"
#include <stdint.h>

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
/** Indicates a data plane PDCP context.
 * Value assigned to user_plane member from ::sec_pdcp_context_info_t */
#define PDCP_DATA_PLANE     1
/** Indicates a control plane PDCP context.
 * Value assigned to user_plane member from ::sec_pdcp_context_info_t */
#define PDCP_CONTROL_PLANE  0

/** Indicates an uplink PDCP context.
 * Value assigned to packet_direction from ::sec_pdcp_context_info_t */
#define PDCP_UPLINK         0
/** Indicates a downlink PDCP context.
 * Value assigned to packet_direction from ::sec_pdcp_context_info_t */
#define PDCP_DOWNLINK       1

/** Indicates a PDCP context that will encapsulate (encrypt) packets */
#define PDCP_ENCAPSULATION  0
/** Indicates a PDCP context that will decapsulate (decrypt) packets */
#define PDCP_DECAPSULATION  1
/*==================================================================================================
                                             ENUMS
==================================================================================================*/
/** Return codes for SEC user space driver APIs */
typedef enum sec_return_code_e
{
    SEC_SUCCESS = 0,                 /**< Operation executed successfully.*/
    SEC_INVALID_INPUT_PARAM,         /**< API received an invalid input parameter. */
    SEC_CONTEXT_MARKED_FOR_DELETION, /**< The SEC context is scheduled for deletion and no more packets
                                          are allowed to be processed for the respective context. */
    SEC_OUT_OF_MEMORY,               /**< Memory allocation failed. */
    SEC_PACKETS_IN_FLIGHT,           /**< API function indicates there are packets in flight
                                          for SEC to process that belong to a certain PDCP context.
                                          Can be returned by sec_delete_pdcp_context().*/
    SEC_LAST_PACKET_IN_FLIGHT,       /**< API function indicates there is one last packet in flight
                                          for SEC to process that belongs to a certain PDCP context.
                                          When processed, the last packet in flight willl be notified to
                                          User Application with a status of ::SEC_STATUS_SUCCESS or
                                          ::SEC_STATUS_LAST_OVERDUE.
                                          Can be returned by sec_delete_pdcp_context().*/
    SEC_PROCESSING_ERROR,            /**< Indicates a SEC processing error occurred on a Job Ring which requires a
                                          SEC user space driver shutdown. Can be returned from sec_poll() or sec_poll_job_ring().
                                          Call sec_get_last_error() to obtain specific error code, as reported by SEC device.
                                          Then the only other API that can be called after this error is sec_release(). */
    SEC_PACKET_PROCESSING_ERROR,     /**< Indicates a SEC packet processing error occurred on a Job Ring.
                                          Can be returned from sec_poll() or sec_poll_job_ring().
                                          Call sec_get_last_error() to obtain specific error code, as reported by SEC device.
                                          The driver was able to reset job ring and job ring can be used like in a normal case. */
    SEC_JR_IS_FULL,                  /**< Job Ring is full. There is no more room in the JR for new packets.
                                          This can happen if the packet RX rate is higher than SEC's capacity. */
    SEC_DRIVER_RELEASE_IN_PROGRESS,  /**< SEC driver shutdown is in progress and no more context
                                          creation/deletion, packets processing or polling is allowed.*/
    SEC_DRIVER_ALREADY_INITIALIZED,  /**< SEC driver is already initialized. */
    SEC_DRIVER_NOT_INITIALIZED,      /**< SEC driver is NOT initialized. */
    SEC_JOB_RING_RESET_IN_PROGRESS,  /**< Job ring is resetting due to a per-packet SEC processing error ::SEC_PACKET_PROCESSING_ERROR.
                                          Reset is finished when sec_poll() or sec_poll_job_ring() return.
                                          Then the job ring can be used again. */
    SEC_DRIVER_NO_FREE_CONTEXTS,     /**< There are no more free contexts. Considering increasing the
                                          maximum number of contexts: #SEC_MAX_PDCP_CONTEXTS.*/
    /* END OF VALID VALUES */

    SEC_RETURN_CODE_MAX_VALUE,       /**< Invalid value for return code. It is used to mark the end of the return code values.
                                          @note ALL new return code values MUST be added before ::SEC_RETURN_CODE_MAX_VALUE! */
}sec_return_code_t;

/** Status codes indicating SEC processing result for each packet,
 *  when SEC user space driver invokes User Application provided callback.
 */
typedef enum sec_status_e
{
    SEC_STATUS_SUCCESS = 0,             /**< SEC processed packet without error.*/
    SEC_STATUS_ERROR,                   /**< SEC packet processing returned with error. */
    SEC_STATUS_OVERDUE,                 /**< Indicates a packet processed by SEC for a PDCP context that was requested
                                             to be removed by the User Application.*/
    SEC_STATUS_LAST_OVERDUE,            /**< Indicates the last packet processed by SEC for a PDCP context that was requested
                                             to be removed by the User Application. After the last overdue packet is notified
                                             to UA, it is safe for UA to reuse PDCP context related data. */

    SEC_STATUS_HFN_THRESHOLD_REACHED,   /**< Indicates HFN reached the threshold configured for the SEC context. Keys must be
                                             renegotiated at earliest convenience. */
    SEC_STATUS_MAC_I_CHECK_FAILED,      /**< Integrity check failed for this packet. */

    /* END OF VALID VALUES */

    SEC_STATUS_MAX_VALUE,               /**< Invalid value for status. It is used to mark the end of the status values.
                                             @note ALL new status values MUST be added before ::SEC_STATUS_MAX_VALUE! */
}sec_status_t;

/** Return codes for User Application registered callback ::sec_out_cbk.
 */
typedef enum sec_ua_return_e
{
    SEC_RETURN_SUCCESS = 0,     /**< User Application processed response notification with success. */
    SEC_RETURN_STOP,            /**< User Application wants to return from the polling API without
                                     processing any other SEC responses. */
}sec_ua_return_t;

/** Cryptographic/Integrity check algorithms.
 *  @note The id values for each algorithm are synchronized with those
 *  specified in 3gpp spec 33.401.
 */
typedef enum sec_crypto_alg_e
{
    SEC_ALG_NULL = 0,       /**< Use EEA0 for confidentiality or EIA0 for integrity protection
                                 (no confidentiality and no integrity).*/
    SEC_ALG_SNOW = 1,       /**< Use SNOW algorithm for confidentiality(EEA1) or integrity protection(EIA1) */
    SEC_ALG_AES = 2,        /**< Use AES algorithm for confidentiality(EEA2) or integrity protection(EIA2)) */

}sec_crypto_alg_t;

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
/** Physical address on 36 bits or more. MUST be kept in synch with same define from kernel! */
typedef uint64_t dma_addr_t;
#else
/** Physical address on 32 bits. MUST be kept in synch with same define from kernel!*/
typedef uint32_t dma_addr_t;
#endif

/** Data type used for specifying address for packets submitted
 *  by User Application. Assume physical addressing. */
typedef dma_addr_t  packet_addr_t;

/** Opaque handle to a Job Ring provided by SEC user space driver
 *  to UA when sec_init() is called. */
typedef const void* sec_job_ring_handle_t;

/** Handle to a SEC PDCP Context */
typedef const void* sec_context_handle_t;

/** UA opaque handle to a packet context.
 *  The handle is opaque from SEC driver's point of view. */
typedef const void* ua_context_handle_t;

/**
    @addtogroup SecUserSpaceDriverExternalMemoryManagement
    @{
 */
/** @brief Function type used for virtual to physical address conversions
 *
 * The user application, upon  intialization of the SEC driver, must provide a
 * pointer to a function with the following signature to be used for performing
 * virtual to physical translations for the internal SEC driver structures.
 * This is needed because the ::sec_packet_t structure can contain addresses
 * from different memory partitions, so the initial assumption that the packets
 * and the internal data structures of the SEC driver are allocated from the
 * same memory partition is no longer true. Thus the SEC driver can no longer
 * perform directly the V2P translations for either the input/output packets
 * or for its internal structures.
 *
 * @param [in] v          Virtual address to be converted.
 *
 * @retval Returns the corresponding physical address.
*/
typedef dma_addr_t (*sec_vtop)(void *v);
/**
    @}
 */

/** Structure used to describe an input or output packet accessed by SEC. */
typedef struct sec_packet_s
{
    dma_addr_t      address;        /**< The PHYSICAL address of the buffer. */
    uint32_t        length;         /**< Packet data length, excluding head room offset. In previous versions
                                         of the driver, this included the packet offset(headroom).
                                         This is no longer true, now length represents just the length of
                                         PDCP header + PDCP payload */
    uint32_t        offset;         /**< Offset within packet from where SEC will access (read or write) data. */
    uint32_t        tail_offset;    /**< Offset from buffer head where tail room is reserved. */
    uint32_t        total_length;   /**< Total Data Length in all fragments including the parent buffer. */
    uint32_t        num_fragments;  /**< Is set only in the first fragment from a s/g packet.
                                         It excludes the parent buffer. */
    uint32_t        pad[2];         /**< Padding to multiple of CACHE_LINE_SIZE. */

}sec_packet_t;

/** Structure used to retrieve statistics from the US SEC PDCP driver. */
typedef struct sec_statistics_s
{
    uint32_t consumer_index;        /**< Index in the Job Ring from where
                                         the next job will be dequeued. */
    uint32_t producer_index;        /**< Index in the Job Ring where
                                         the next job will be enqueued. */
    uint32_t slots_available;       /**< Slots available for jobs to be enqueued. */
    uint32_t jobs_waiting_dequeue;  /**< Number of jobs available to be dequeued by the UA. */
    uint32_t reserved[2];           /**< Reserved for future additions. */
} __attribute__ ((aligned (32))) sec_statistics_t;

/** Contains Job Ring descriptor info returned to the caller when sec_init() is invoked. */
typedef struct sec_job_ring_descriptor_s
{
    sec_job_ring_handle_t job_ring_handle;     /**< The job ring handle is provided by the library for UA usage.
                                                    The handle is opaque from UA point of view.
                                                    The handle can be used by UA to poll for events per JR. */
    int job_ring_irq_fd;                       /**< File descriptor that can be used by UA with epoll() to wait for
                                                    interrupts generated by SEC for this Job Ring. */
}sec_job_ring_descriptor_t;

/**
    @}
 */

/**
    @addtogroup SecUserSpaceDriverPacketFunctions
    @{
 */

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
 *                                In case of per packet error (when status is ::SEC_STATUS_ERROR),
 *                                this field contains the status word as generated by SEC device.
 *
 * @retval Returns values from ::sec_ua_return_t enum.
 * @retval ::SEC_RETURN_SUCCESS          for successful execution
 * @retval ::SEC_RETURN_STOP             The User Application must return this code when it desires to stop the polling process.
 */
typedef int (*sec_out_cbk)(const sec_packet_t  *in_packet,
                           const sec_packet_t  *out_packet,
                           ua_context_handle_t ua_ctx_handle,
                           sec_status_t        status,
                           uint32_t            error_info);

/**
    @}
 */

/**
    @addtogroup SecUserSpaceDriverFunctions
    @{
 */


/** PDCP context structure provided by User Application when a PDCP context is created.
 *  User Application fills this structure with data that is used by SEC user space driver
 *  to create a SEC descriptor. This descriptor is used by SEC to process all packets
 *  belonging to this PDCP context. */
typedef struct sec_pdcp_context_info_s
{
    uint8_t     sn_size;                /**< Sequence number can be represented on 5, 7 or 12 bits.
                                             Select one from: #SEC_PDCP_SN_SIZE_5, #SEC_PDCP_SN_SIZE_7, #SEC_PDCP_SN_SIZE_12.
                                             The value #SEC_PDCP_SN_SIZE_5 is valid only for control plane contexts! */
    uint8_t     bearer:5;               /**< Radio bearer id. */
    uint8_t     user_plane:1;           /**< Control plane versus Data plane indication.
                                             Possible values: #PDCP_DATA_PLANE, #PDCP_CONTROL_PLANE. */
    uint8_t     packet_direction:1;     /**< Direction can be uplink(#PDCP_UPLINK) or downlink(#PDCP_DOWNLINK). */
    uint8_t     protocol_direction:1;   /**< Can be encapsulation(#PDCP_ENCAPSULATION) or decapsulation(#PDCP_DECAPSULATION)*/
    uint8_t     cipher_algorithm;       /**< Cryptographic algorithm used: NULL/SNOW(F8)/AES(CTR).
                                             Can have values from ::sec_crypto_alg_t enum. */
    uint8_t     integrity_algorithm;    /**< Integrity algorithm used: NULL/SNOW(F9)/AES(CMAC).
                                             Can have values from ::sec_crypto_alg_t enum. */
    uint32_t    hfn;                    /**< HFN for this radio bearer. Represents the most significant bits from sequence number. */
    uint32_t    hfn_threshold;          /**< HFN threshold for this radio bearer. If HFN matches or exceeds threshold,
                                             sec_out_cbk will  be called with status ::SEC_STATUS_HFN_THRESHOLD_REACHED. */
    uint8_t    *cipher_key;             /**< Ciphering key. Must be provided by User Application as DMA-capable memory,
                                             just as it's done for packets.*/
    uint8_t    cipher_key_len;          /**< Ciphering key length. */
    uint8_t    *integrity_key;          /**< Integrity key. Must be provided by User Application as DMA-capable memory,
                                             just as it's done for packets.*/
    uint8_t    integrity_key_len;       /**< Integrity key length. */
    uint32_t   hfn_ov_en;               /**< Enables HFN override by user for this context */
    void        *custom;                /**< User Application custom data for this PDCP context. Usage to be defined. */
    sec_out_cbk notify_packet;          /**< Callback function to be called for all packets processed on this context. */
} sec_pdcp_context_info_t;


/** Configuration data structure that must be provided by UA when SEC user space driver is initialized */
typedef struct sec_config_s
{
    void            *memory_area;           /**< UA provided- virtual memory of size #SEC_DMA_MEMORY_SIZE to be used internally
                                                 by the driver to allocate data (like SEC descriptors) that needs to be passed to
                                                 SEC device in physical addressing. */

    uint32_t        irq_coalescing_timer;   /**< Interrupt Coalescing Timer Threshold.

                                                 While interrupt coalescing is enabled (ICEN=1), this value determines the
                                                 maximum amount of time after processing a Descriptor before raising an interrupt.
                                                 The threshold value is represented in units equal to 64 CAAM interface
                                                 clocks. Valid values for this field are from 1 to 65535.
                                                 A value of 0 results in behavior identical to that when interrupt
                                                 coalescing is disabled.*/

    uint8_t         irq_coalescing_count;   /**< Interrupt Coalescing Descriptor Count Threshold.

                                                 While interrupt coalescing is enabled (ICEN=1), this value determines
                                                 how many Descriptors are completed before raising an interrupt.
                                                 Valid values for this field are from 0 to 255.
                                                 Note that a value of 1 functionally defeats the advantages of interrupt
                                                 coalescing since the threshold value is reached each time that a
                                                 Job Descriptor is completed. A value of 0 is treated in the same
                                                 manner as a value of 1.*/

    uint8_t         work_mode;              /**< Choose between hardware poll vs interrupt notification when driver is initialized.
                                                 Valid values are #SEC_STARTUP_POLLING_MODE and #SEC_STARTUP_INTERRUPT_MODE.*/
    
    sec_vtop        sec_drv_vtop;           /**< Function to be used internally by the driver for virtual to physical 
                                                 address translation for internal structures. */
}sec_config_t;

/**
    @}
 */

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
    @addtogroup SecUserSpaceDriverManagementFunctions
    @{
 */


/**
 * @brief Initialize the SEC user space driver.
 *
 * This function will handle configuration and initialization for
 * requested number of JRs, as well as local data initialization.
 * Call once during application startup.
 *
 * @note Global SEC initialization is done in SEC kernel driver.
 *
 * @note The hardware IDs of the initialized Job Rings are opaque to the UA.
 * The exact Job Rings used by this library are decided between SEC user
 * space driver and SEC kernel driver. A static partitioning of Job Rings is assumed,
 * configured in DTS(device tree specification) file.
 *
 * @param [in]  sec_config_data         Configuration data required to initialize SEC user space driver.
 * @param [in]  job_rings_no            The number of job rings to acquire and initialize.
 * @param [out] job_ring_descriptors    Array of job ring descriptors of size job_rings_no. The job
 *                                      ring descriptors are provided by the library for UA usage.
 *                                      The storage for the array is allocated by the library.
 *
 * @retval ::SEC_SUCCESS                     for successful execution
 * @retval ::SEC_DRIVER_ALREADY_INITIALIZED  when driver is already initialized
 * @retval ::SEC_OUT_OF_MEMORY               is returned if internal memory allocation fails
 * @retval ::SEC_INVALID_INPUT_PARAM         when at least one invalid parameter was provided
 */
sec_return_code_t sec_init(const sec_config_t *sec_config_data,
                           uint8_t job_rings_no,
                           const sec_job_ring_descriptor_t **job_ring_descriptors);

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
 * @retval ::SEC_SUCCESS                     is returned for a successful execution
 * @retval ::SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress
 */
sec_return_code_t sec_release();


/**
    @}
 */


/**
    @addtogroup SecUserSpaceDriverContextFunctions
    @{
 */


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
 * @param [in]  sec_ctx_info       PDCP context info filled by the caller. User application will not touch
 *                                 this data until the SEC context is deleted with sec_delete_pdcp_context().
 * @param [out] sec_ctx_handle     PDCP context handle returned by SEC user space driver.
 *
 * @retval ::SEC_SUCCESS for successful execution
 * @retval ::SEC_INVALID_INPUT_PARAM         when at least one invalid parameter was provided
 * @retval ::SEC_DRIVER_NO_FREE_CONTEXTS     when there are no more free contexts
 * @retval ::SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress
 * @retval ::SEC_DRIVER_NOT_INITIALIZED      is returned if SEC driver is not yet initialized.
 */
sec_return_code_t sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                                           const sec_pdcp_context_info_t *sec_ctx_info,
                                           sec_context_handle_t *sec_ctx_handle);

/** @brief Deletes a SEC PDCP context previously created.
 *
 * Deletes a PDCP context identified with the handle provided by the function caller.
 * The handle was obtained by the caller using sec_create_pdcp_context() function.
 * Call once for each PDCP context.
 * If called when there are still some packets awaiting to be processed by SEC for this context,
 * the API will return #SEC_PACKETS_IN_FLIGHT. All per-context packets processed by SEC after
 * this API is invoked will be raised to the User Application having status field set to ::SEC_STATUS_OVERDUE.
 * The last overdue packet will have status set to ::SEC_STATUS_LAST_OVERDUE.
 * Only after the last overdue packet is notified, the User Application can be sure the PDCP context
 * was removed from SEC user space driver.
 *
 * @param [in] sec_ctx_handle     PDCP context handle.
 *
 * @retval ::SEC_SUCCESS                     for successful execution
 * @retval ::SEC_PACKETS_IN_FLIGHT           in case there are some already submitted packets
 *                                           for this context awaiting to be processed by SEC.
 * @retval ::SEC_LAST_PACKET_IN_FLIGHT       in case there is only one already submitted packet.
 * @retval ::SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress.
 * @retval ::SEC_INVALID_INPUT_PARAM         is returned in case the SEC context handle is invalid.
 * @retval ::SEC_DRIVER_NOT_INITIALIZED      is returned if SEC driver is not yet initialized.
 */
sec_return_code_t sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle);


/**
    @}
 */

/**
    @addtogroup SecUserSpaceDriverPacketFunctions
    @{
 */

/** @brief Polls for available packets processed by SEC on all Job Rings initialized by the User Application.
 *
 * This function polls the SEC Job Rings and delivers processed packets to the User Application.
 * Each processed packet belongs to a PDCP context and each PDCP context has a sec_out_cbk registered.
 * This sec_out_cbk is invoked for each processed packet belonging to that PDCP context.
 *
 * The Job Rings are polled in a weighted round robin fashion using a fixed weight for each Job Ring.
 * The polling is stopped when "limit" packets are notified or when there are no more packets to notify.
 * User Application has an additional mechanism to stop the polling, that is by returning ::SEC_RETURN_STOP
 * from sec_out_cbk.
 *
 * @note In case the "limit" is negative and the rate of packet processing is high the function could poll
 * the JRs indefinitely.
 *
 * The SEC user space driver assumes a Linux NAPI style to deliver SEC ready packets to UA.
 * What this means is: if all ready packets are delivered to UA and no more ready packets
 * are awaiting to be retrieved from SEC device then this function will enable SEC interrupt generation
 * on all job rings before it returns. In other words, IRQs are enabled if:
 * -> (packets_no < limit) OR (limit < 0)
 * SEC interrupts for user space dedicated Job Rings are ALWAYS disabled in interrupt handler from SEC kernel driver.
 * SEC interrupts per job ring are enabled in SEC kernel driver. User-space drive requests this operation
 * using UIO control.
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
 * @retval ::SEC_SUCCESS                     for successful execution.
 * @retval ::SEC_DRIVER_NOT_INITIALIZED      is returned if SEC driver is not yet initialized.
 * @retval ::SEC_PROCESSING_ERROR            indicates a fatal execution error that requires a SEC user space driver shutdown.
 *                                           Call sec_get_last_error() to obtain specific error code, as reported by SEC device.
 * @retval ::SEC_PACKET_PROCESSING_ERROR     indicates a SEC packet processing error occurred on a Job Ring.
 *                                           The driver was able to reset job ring and job ring can be used like in a normal case.
 * @retval ::SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress
 * @retval ::SEC_INVALID_INPUT_PARAM         is returned if limit == 0 or weight == 0 or
 *                                           limit <= weight or weight > #SEC_JOB_RING_HW_SIZE
 */
sec_return_code_t sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no);

/** @brief Polls for available packets processed by SEC on a specific Job Ring initialized by the User Application.
 *
 * This function polls the SEC Job Rings and delivers processed packets to the User Application.
 * Each processed packet belongs to a PDCP context and each PDCP context has a sec_out_cbk registered.
 * This sec_out_cbk is invoked for each processed packet belonging to that PDCP context.
 *
 * The polling is stopped when "limit" packets are notified or when there are no more packets to notify.
 * User Application has an additional mechanism to stop the polling, that is by returning ::SEC_RETURN_STOP from
 * sec_out_cbk.
 *
 * The SEC user space driver assumes a Linux NAPI style to deliver SEC ready packets to UA.
 * What this means is: if all ready packets are delivered to UA and no more ready packets
 * are awaiting to be retrieved from this SEC's job ring then this function will enable SEC interrupt generation
 * on this job ring before it returns. In other words, IRQs are enabled if:
 * -> (packets_no < limit) OR (limit < 0)
 * SEC interrupts per job ring are ALWAYS disabled in interrupt handler from SEC kernel driver.
 * SEC interrupts per job ring are enabled in SEC kernel driver. User-space drive requests this operation
 * using UIO control.
 *
 * @note The sec_poll_job_ring() API cannot be called from within a sec_out_cbk function!
 *
 * @param [in]  job_ring_handle     The Job Ring handle.
 * @param [in]  limit               This value represents the maximum number of processed packets
 *                                  that can be notified to the User Aplication by this API call on this Job Ring.
 *                                  Note that fewer packets may be notified if enough processed packets are not available.
 *                                  If limit has a negative value, then all ready packets will be notified.
 * @param [out] packets_no          Number of packets notified to the User Application during this function call.
 *                                  Can be NULL if User Application does not need this information.
 *
 * @retval ::SEC_SUCCESS                    for successful execution.
 * @retval ::SEC_DRIVER_NOT_INITIALIZED     is returned if SEC driver is not yet initialized.
 * @retval ::SEC_PROCESSING_ERROR           indicates a fatal execution error that requires a SEC user space driver shutdown.
 *                                          Call sec_get_last_error() to obtain specific error code, as reported by SEC device.
 * @retval ::SEC_PACKET_PROCESSING_ERROR    indicates a SEC packet processing error occurred on a Job Ring.
 *                                          The driver was able to reset job ring and job ring can be used like in a normal case.
 * @retval ::SEC_DRIVER_RELEASE_IN_PROGRESS is returned if SEC driver release is in progress
 * @retval ::SEC_INVALID_INPUT_PARAM        is returned if limit == 0 or weight == 0 or
 *                                          limit > #SEC_JOB_RING_HW_SIZE or job ring handle is NULL.
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
 * @note The input packet and output packet must not both point to the same memory location!
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
 * @retval ::SEC_SUCCESS is returned for successful execution
 * @retval ::SEC_INVALID_INPUT_PARAM when at least one invalid parameter was provided. Example:
 *                                   - the SEC context handle is invalid (e.g. corrupt handle)
 *                                   - input/output buffer address is invalid (e.g. NULL)
 *                                   - etc
 *
 * @retval ::SEC_JR_IS_FULL                  is returned if the JR is full
 * @retval ::SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress
 * @retval ::SEC_DRIVER_NOT_INITIALIZED      is returned if SEC driver is not yet initialized.
 * @retval ::SEC_CONTEXT_MARKED_FOR_DELETION is returned if the SEC context was marked for deletion.
 *                                           This can happen if sec_delete_pdcp_context() was called on the context.
 * @retval ::SEC_JOB_RING_RESET_IN_PROGRESS  indicates job ring is resetting due to a per-packet SEC processing error ::SEC_PACKET_PROCESSING_ERROR.
 *                                           Reset is finished when sec_poll() or sec_poll_job_ring() return.
 *                                           Then, sec_process_packet() can be called again.
 */
sec_return_code_t sec_process_packet(sec_context_handle_t sec_ctx_handle,
                                     const sec_packet_t *in_packet,
                                     const sec_packet_t *out_packet,
                                     ua_context_handle_t ua_ctx_handle);

/**
 * @brief Submit a packet for SEC processing on a specified context using a
          specified value for HFN.
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
 * @note The input packet and output packet must not both point to the same memory location!
 *
 * @param [in]  sec_ctx_handle     The handle of the context associated to this packet.
 *                                 This handle is opaque from the User Application point of view.
 *                                 SEC driver uses this handle to identify the processing type
 *                                 required for this packet.
 * @param [in]  in_packet          Input packet read by SEC.
 * @param [in]  out_packet         Output packet where SEC writes result.
 * @param [in]  hfn_ov_val         The value of HFN to be used by SEC for processing the input packet.
 *
 * @note It is the user responsability to provide an adequate bit-sized value for HFN, right aligned:
 *       - For ::sec_pdcp_context_info_t::user_plane set to #PDCP_DATA_PLANE:
 *         - For ::sec_pdcp_context_info_t::sn_size set to #SEC_PDCP_SN_SIZE_7, the user must provide
 *           in the least significant 25 bits of hfn_ov_val the desired HFN value to be used.
 *         - For ::sec_pdcp_context_info_t::sn_size set to #SEC_PDCP_SN_SIZE_12, the user must provide
 *           in the least significant 20 bits of hfn_ov_val the desired HFN value to be used.
 *       - For ::sec_pdcp_context_info_t::user_plane set to #PDCP_CONTROL_PLANE, the user must provide
 *         in the least significant 27 bits of hfn_ov_val the desired HFN value to be used.
 *
 * @param [in]  ua_ctx_handle      The handle to a User Application packet context.
 *                                 This handle is opaque from the SEC driver's point of view and
 *                                 will be provided by SEC driver in the response callback.
 *
 *
 * @retval ::SEC_SUCCESS is returned for successful execution
 * @retval ::SEC_INVALID_INPUT_PARAM when at least one invalid parameter was provided. Example:
 *                                   - the SEC context handle is invalid (e.g. corrupt handle)
 *                                   - input/output buffer address is invalid (e.g. NULL)
 *                                   - etc
 *
 * @retval ::SEC_JR_IS_FULL                  is returned if the JR is full
 * @retval ::SEC_DRIVER_RELEASE_IN_PROGRESS  is returned if SEC driver release is in progress
 * @retval ::SEC_DRIVER_NOT_INITIALIZED      is returned if SEC driver is not yet initialized.
 * @retval ::SEC_CONTEXT_MARKED_FOR_DELETION is returned if the SEC context was marked for deletion.
 *                                           This can happen if sec_delete_pdcp_context() was called on the context.
 * @retval ::SEC_JOB_RING_RESET_IN_PROGRESS  indicates job ring is resetting due to a per-packet SEC processing error ::SEC_PACKET_PROCESSING_ERROR.
 *                                           Reset is finished when sec_poll() or sec_poll_job_ring() return.
 *                                           Then, sec_process_packet() can be called again.
 */
sec_return_code_t sec_process_packet_hfn_ov(sec_context_handle_t sec_ctx_handle,
                                            const sec_packet_t *in_packet,
                                            const sec_packet_t *out_packet,
                                            uint32_t hfn_ov_val,
                                            ua_context_handle_t ua_ctx_handle);
/**
    @}
 */

/**
    @addtogroup SecUserSpaceDriverErrorFunctions
    @{
 */


/** @brief Returns the last SEC user space driver error, if any.
 *
 * Use this function to query SEC user space driver status after an
 * API function returned a ::SEC_PROCESSING_ERROR error code.
 * The last error is a thread specific value. Call this function on the same
 * thread that received ::SEC_PROCESSING_ERROR error.
 *
 * @note After an API returns ::SEC_PROCESSING_ERROR code, besides calling sec_get_last_error()
 * the only other valid API to call is sec_release().
 *
 * @retval -1 if local-per-thread error variable is not initialized.
 * @retval 0 if no error code is reported by SEC device.
 * @retval Returns specific error code, as reported by SEC device.
 *         The error is extracted from Job Ring Interrupt Status Register
 *         (JRINT), bits [0:32].
 */
int32_t sec_get_last_error(void);


/** @brief Return string representation for a status code.
 *
 * @param [in] status   The status code.
 * @retval string representation
 */
const char* sec_get_status_message(sec_status_t status);

/** @brief Return string representation for an error code.
 *
 * @param [in] return_code   The error code.
 * @retval string representation
 */
const char* sec_get_error_message(sec_return_code_t return_code);

/**
    @}
 */

/**
    @addtogroup SecUserSpaceDriverManagementFunctions
    @{
 */
/** @brief Retrieves statistics on a SEC Job Ring.
 *
 * This function retrieves some statistics from the CAAM driver. This can provide
 * useful information, like the number of slots available, the number of jobs waiting
 * to be dequeued or the indexes used for writing or reading from the Job Ring.
 *
 * @param [in]  job_ring_handle The Job Ring handle.
 *
 * @param [in]  jr_stats        Pointer to a statistics structure.
 *
 * @retval ::SEC_SUCCESS                    for successful execution.
 *
 */
sec_return_code_t sec_get_stats(sec_job_ring_handle_t job_ring_handle,
                                sec_statistics_t * jr_stats);
/**
    @}
 */

/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //FSL_SEC_H
