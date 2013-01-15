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

#ifndef FSL_SEC_CONFIG_H
#define FSL_SEC_CONFIG_H

#ifdef __cplusplus
/* *INDENT-OFF* */

extern "C"{
/* *INDENT-ON* */
#endif


/**
    @defgroup SecUserSpaceDriverAPI SEC user space driver API
    Contains all the information that defines SEC user space driver API.

    @defgroup SecUserSpaceDriverConfigurationSection SEC Driver Configuration
    @ingroup SecUserSpaceDriverAPI
    This sub-module documents how SEC user space driver can be configured.

    @defgroup SecUserSpaceDriverFunctions SEC Driver Programming
    @ingroup SecUserSpaceDriverAPI
    This sub-module documents how SEC user space driver can be programmed.

    @defgroup SecUserSpaceDriverExternal SEC Driver External Dependencies
    @ingroup SecUserSpaceDriverAPI
    This sub-module documents SEC user space driver external dependencies.

    @defgroup SecUserSpaceDriverExternalMemoryManagement SEC Driver External Memory Management Mechanism
    @ingroup SecUserSpaceDriverExternal
    This sub-module documents SEC user space driver external memory management mechanism.

    @defgroup SecUserSpaceDriverManagementFunctions SEC Driver Management Functions
    @ingroup SecUserSpaceDriverFunctions
    This sub-module documents SEC user space driver management functions.

    @defgroup SecUserSpaceDriverContextFunctions SEC Driver Context Handling Functions
    @ingroup SecUserSpaceDriverFunctions
    This sub-module documents SEC user space driver functions that allow handling of cryptographic contexts.
    Currently only PDCP contexts are supported.

    @defgroup SecUserSpaceDriverPacketFunctions SEC Driver Packet Processing Functions
    @ingroup SecUserSpaceDriverFunctions
    This sub-module documents SEC user space driver functions that allow packet processing operations.

    @defgroup SecUserSpaceDriverErrorFunctions SEC Driver Error Handling Functions
    @ingroup SecUserSpaceDriverFunctions
    This sub-module documents SEC user space driver error handling functions.
 */

 /** @mainpage SEC User Space Driver API Main Page
     This is a generated Doxygen documentation that describes SEC user space driver API.
     <br><br>See @ref SecUserSpaceDriverAPI "SEC user space driver API"
  */


/**
    @addtogroup SecUserSpaceDriverConfigurationSection
    @{
 */

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/***************************************/
/*       Helper defines                */
/***************************************/

/** Define used for setting a flag on */
#define  ON  1
/** Define used for setting a flag off */
#define  OFF 0

/** Maximum length for #SEC_UIO_DEVICE_NAME. */
#define SEC_UIO_MAX_DEVICE_NAME_LENGTH  30


/** SEC is configured to start work in polling mode,
 *  when configured for NAPI notification style. */
#define SEC_STARTUP_POLLING_MODE     0
/** SEC is configured to start work in interrupt mode,
 *  when configured for NAPI notification style. */
#define SEC_STARTUP_INTERRUPT_MODE   1


/** Logging level for SEC user space driver: log only errors */
#define SEC_DRIVER_LOG_ERROR    0
/** Logging level for SEC user space driver: log both errors and info messages */
#define SEC_DRIVER_LOG_INFO     1
/** Logging level for SEC user space driver: log errors, info and debug messages */
#define SEC_DRIVER_LOG_DEBUG    2

/** SEC driver will use NAPI model to receive notifications
 * for processed packets from SEC engine hardware:
 * - IRQ for low traffic
 * - polling for high traffic. */
#define SEC_NOTIFICATION_TYPE_NAPI  0
/** SEC driver will use ONLY interrupts to receive notifications
 * for processed packets from SEC engine hardware. */
#define SEC_NOTIFICATION_TYPE_IRQ   1
/** SEC driver will use ONLY polling to receive notifications
 * for processed packets from SEC engine hardware. */
#define SEC_NOTIFICATION_TYPE_POLL  2



/** Bit mask for Job Ring id 0 in DTS Job Ring mapping.
 * DTS Job Ring mapping distributes SEC's job rings among SEC user space driver and SEC kernel driver. */
#define SEC_JOB_RING_0  0x8
/** Bit mask Job Ring id 1 in DTS Job Ring mapping.
 * DTS Job Ring mapping distributes SEC's job rings among SEC user space driver and SEC kernel driver. */
#define SEC_JOB_RING_1  0x4
/** Bit mask for Job Ring id 2 in DTS Job Ring mapping.
 * DTS Job Ring mapping distributes SEC's job rings among SEC user space driver and SEC kernel driver. */
#define SEC_JOB_RING_2  0x2
/** Bit mask for Job Ring id 3 in DTS Job Ring mapping.
 * DTS Job Ring mapping distributes SEC's job rings among SEC user space driver and SEC kernel driver. */
#define SEC_JOB_RING_3  0x1

/** Calculate the number of Job Rings enabled from the DTS Job Ring mapping. */
#define SEC_NUMBER_JOB_RINGS(mask)  (((mask) & SEC_JOB_RING_3) + \
                                    (((mask) & SEC_JOB_RING_2) >> 1)) + \
                                    (((mask) & SEC_JOB_RING_1) >> 2) + \
                                    (((mask) & SEC_JOB_RING_0) >> 3)


/************************************************/
/* SEC USER SPACE DRIVER related configuration. */
/************************************************/

/** Determines how SEC user space driver will receive notifications
 * for processed packets from SEC engine.
 * Valid values are: #SEC_NOTIFICATION_TYPE_POLL, #SEC_NOTIFICATION_TYPE_IRQ
 * and #SEC_NOTIFICATION_TYPE_NAPI. */
#define SEC_NOTIFICATION_TYPE   SEC_NOTIFICATION_TYPE_POLL

/************************************************/
/* Scatter/Gather support related configuration */
/************************************************/

/** Enable or disable the support for scatter/gather
 * buffers in the SEC driver.
 * Valid values:
 * ON - enable scatter gather support
 * OFF - disable scatter gather support
 */
#define SEC_ENABLE_SCATTER_GATHER ON

/** Name of UIO device. Each user space SEC job ring will have a corresponding UIO device
 * with the name sec-channelX, where X is the job ring id.
 * Maximum length is #SEC_UIO_MAX_DEVICE_NAME_LENGTH.
 *
 * @note  Must be kept in synch with SEC kernel driver define #SEC_UIO_DEVICE_NAME !
 */
#define SEC_UIO_DEVICE_NAME     "sec-job-ring"

/** Maximum number of job rings supported by SEC hardware */
#define MAX_SEC_JOB_RINGS         4

/** Maximum number of PDCP contexts  per direction (uplink/downlink). */
#define SEC_MAX_PDCP_CONTEXTS_PER_DIRECTION   256

/** Maximum number of SEC PDCP contexts that can be managed
 *  simultaneously by SEC user space driver. Add 2x safety margin. */
#define SEC_MAX_PDCP_CONTEXTS   ((SEC_MAX_PDCP_CONTEXTS_PER_DIRECTION) * 2) * 2

/** Size of cryptographic context that is used directly in communicating with SEC device.
 *  SEC device works only with physical addresses. This is the maximum size for a SEC
 *  descriptor ( = 64 words).
 */
#define SEC_CRYPTO_DESCRIPTOR_SIZE  256

/** Size of job descriptor submitted to SEC device for each packet to be processed.
 *  Job descriptor contains 3 DMA address pointers:
 *      - to shared descriptor, to input buffer and to output buffer.
 *  The job descriptor contains other SEC specific commands as well:
 *      - HEADER command, SEQ IN PTR command SEQ OUT PTR command and opaque data,
 *        each measuring 4 bytes.
 *  Job descriptor size, depending on physical address representation:
 *      - 32 bit - size is 28 bytes - cacheline-aligned size is 64 bytes
 *      - 36 bit - size is 40 bytes - cacheline-aligned size is 64 bytes
 *  @note: Job descriptor must be cacheline-aligned to ensure efficient memory access.
 *  @note: If other format is used for job descriptor, then the size must be revised.
 */
#define SEC_JOB_DESCRIPTOR_SIZE     64

/** Size of one entry in the input ring of a job ring.
 *  Input ring contains pointers to job descriptors.
 *  The memory used for an input ring and output ring must be physically contiguous.
 */
#define SEC_JOB_INPUT_RING_ENTRY_SIZE  sizeof(dma_addr_t)

/** Size of one entry in the output ring of a job ring.
 *  Output ring entry is a pointer to a job descriptor followed by a 4 byte status word.
 *  The memory used for an input ring and output ring must be physically contiguous.
 *  @note If desired to use also the optional SEQ OUT indication in output ring entries,
 *  then 4 more bytes must be added to the size.
*/
#define SEC_JOB_OUTPUT_RING_ENTRY_SIZE  SEC_JOB_INPUT_RING_ENTRY_SIZE + 4

/** DMA memory required for an input ring of a job ring. */
#define SEC_DMA_MEM_INPUT_RING_SIZE     (SEC_JOB_INPUT_RING_ENTRY_SIZE) * (SEC_JOB_RING_SIZE)

/** DMA memory required for an output ring of a job ring.
 *  Required extra 4 byte for status word per each entry. */
#define SEC_DMA_MEM_OUTPUT_RING_SIZE    (SEC_JOB_OUTPUT_RING_ENTRY_SIZE) * (SEC_JOB_RING_SIZE)

/** DMA memory required for descriptors of a job ring. */
#define SEC_DMA_MEM_DESCRIPTORS         (SEC_CRYPTO_DESCRIPTOR_SIZE)*(SEC_JOB_RING_SIZE)

/** DMA memory required for a job ring, including both input and output rings. */
#define SEC_DMA_MEM_JOB_RING_SIZE       ((SEC_DMA_MEM_INPUT_RING_SIZE) + (SEC_DMA_MEM_OUTPUT_RING_SIZE) + (SEC_DMA_MEM_DESCRIPTORS))

#if (SEC_ENABLE_SCATTER_GATHER == ON)

/** Size of a SG table, in bytes */
#define SEC_SG_TBL_SIZE                 16

/** Maximum number of entries in a SG table, per direction (in/out) */
#define SEC_MAX_SG_TBL_ENTRIES          32

/** DMA memory required for SG tables: number of entries in the job ring
 * multiplied by the maximum number of entries in a SG table multiplied by
 * the the number of SG tables per job (one for the input packet, one for the
 * output packet) i.e. 2
 */
#define SEC_DMA_MEM_SG_SIZE         (SEC_JOB_RING_SIZE * SEC_MAX_SG_TBL_ENTRIES * SEC_SG_TBL_SIZE * 2)

#endif // (SEC_ENABLE_SCATTER_GATHER == ON)

/** When calling sec_init() UA will provide an area of virtual memory
 *  of size #SEC_DMA_MEMORY_SIZE to be  used internally by the driver
 *  to allocate data (like SEC descriptors) that needs to be passed to
 *  SEC device in physical addressing and later on retrieved from SEC device.
 *  At initialization the UA provides specialized ptov/vtop functions/macros to
 *  translate addresses allocated from this memory area. */
#if (SEC_ENABLE_SCATTER_GATHER == ON)
#define SEC_DMA_MEMORY_SIZE     ( (SEC_CRYPTO_DESCRIPTOR_SIZE) * (SEC_MAX_PDCP_CONTEXTS) + \
                                  (SEC_DMA_MEM_JOB_RING_SIZE) * (MAX_SEC_JOB_RINGS) + \
                                  (SEC_DMA_MEM_SG_SIZE) * (MAX_SEC_JOB_RINGS) )
#else // (SEC_ENABLE_SCATTER_GATHER == ON)
#define SEC_DMA_MEMORY_SIZE     ( (SEC_CRYPTO_DESCRIPTOR_SIZE) * (SEC_MAX_PDCP_CONTEXTS) + \
                                  (SEC_DMA_MEM_JOB_RING_SIZE) * (MAX_SEC_JOB_RINGS) )
#endif // (SEC_ENABLE_SCATTER_GATHER == ON)

/** PDCP sequence number length */
#define SEC_PDCP_SN_SIZE_5  5
/** PDCP sequence number length */
#define SEC_PDCP_SN_SIZE_7  7
/** PDCP sequence number length */
#define SEC_PDCP_SN_SIZE_12 12


/************************************************/
/* SEC DEVICE related configuration.            */
/************************************************/

/** SEC device is configured for 36 bit physical addressing.
 *  Both the SEC user space driver and the SEC kernel driver
 *  need to have this configuration enable at the same time!
 */
//#define CONFIG_PHYS_64BIT

/***************************************************/
/* Library Internal Logging related configuration. */
/***************************************************/

/** Enable/Disable logging support at compile time.
 * Valid values:
 * ON - enable logging
 * OFF - disable logging
 *
 * The messages are logged at stdout.
 */
#define SEC_DRIVER_LOGGING ON

/** Configure logging level at compile time.
 * Valid values:
 * SEC_DRIVER_LOG_ERROR - log only errors
 * SEC_DRIVER_LOG_INFO  - log errors and info messages
 * SEC_DRIVER_LOG_DEBUG - log errors, info and debug messages
 */
#define SEC_DRIVER_LOGGING_LEVEL SEC_DRIVER_LOG_ERROR

/***************************************/
/* SEC JOB RING related configuration. */
/***************************************/

/** Configure the size of the JOB RING.
 * The maximum size of the ring is hardware limited to 1024.
 * However the number of packets in flight in a time interval of 1ms can be calculated
 * from the traffic rate (Mbps) and packet size.
 * Here it was considered a packet size of 40 bytes.
 *
 * @note Round up to nearest power of 2 for optimized update
 * of producer/consumer indexes of each job ring
 *
 * \todo Should set to 750, according to the calculation above, but
 * the JR size must be power of 2, thus the next closest value must
 * be chosen (i.e. 512 since 1024 is not available)
 */
#define SEC_JOB_RING_SIZE       512

/***************************************************/
/* Interrupt coalescing related configuration.     */
/* NOTE: SEC hardware enabled interrupt            */
/* coalescing is not supported on SEC version 3.1! */
/* SEC version 4.4 has support for interrupt       */
/* coalescing.                                     */
/***************************************************/

#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL

#define SEC_INT_COALESCING_ENABLE   ON
/** Interrupt Coalescing Descriptor Count Threshold.
 * While interrupt coalescing is enabled (ICEN=1), this value determines
 * how many Descriptors are completed before raising an interrupt.
 *
 * Valid values for this field are from 0 to 255.
 * Note that a value of 1 functionally defeats the advantages of interrupt
 * coalescing since the threshold value is reached each time that a
 * Job Descriptor is completed. A value of 0 is treated in the same
 * manner as a value of 1.
 * */
#define SEC_INTERRUPT_COALESCING_DESCRIPTOR_COUNT_THRESH  10

/** Interrupt Coalescing Timer Threshold.
 * While interrupt coalescing is enabled (ICEN=1), this value determines the
 * maximum amount of time after processing a Descriptor before raising an interrupt.
 *
 * The threshold value is represented in units equal to 64 CAAM interface
 * clocks. Valid values for this field are from 1 to 65535.
 * A value of 0 results in behavior identical to that when interrupt
 * coalescing is disabled.*/
#define SEC_INTERRUPT_COALESCING_TIMER_THRESH  100
#endif // SEC_NOTIFICATION_TYPE_POLL

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/*================================================================================================*/

/*================================================================================================*/

/**
    @}
 */

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //FSL_SEC_CONFIG_H
