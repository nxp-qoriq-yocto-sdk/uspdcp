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

#define  ON  1
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
#define SEC_DRIVER_LOG_ERROR 0
/** Logging level for SEC user space driver: log both errors and info messages */
#define SEC_DRIVER_LOG_INFO  1

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


/** Name of UIO device. Each user space SEC job ring will have a corresponding UIO device
 * with the name sec-channelX, where X is the job ring id.
 * Maximum length is #SEC_UIO_MAX_DEVICE_NAME_LENGTH.
 *
 * @note  Must be kept in synch with SEC kernel driver define #SEC_UIO_DEVICE_NAME !
 */
#define SEC_UIO_DEVICE_NAME     "sec-job-ring"


/** Maximum number of PDCP contexts  per direction (uplink/downlink). */
#define SEC_MAX_PDCP_CONTEXTS_PER_DIRECTION   192

/** Maximum number of SEC PDCP contexts that can be managed
 *  simultaneously by SEC user space driver. Add 2x safety margin. */
#define SEC_MAX_PDCP_CONTEXTS   ((SEC_MAX_PDCP_CONTEXTS_PER_DIRECTION) * 2) * 2


#ifdef SEC_HW_VERSION_4_4

////////////////////////////////////////////////////////////////////////////////////////////////
// SEC 4.4
////////////////////////////////////////////////////////////////////////////////////////////////


/** Size of cryptographic context that is used directly in communicating with SEC device.
 *  SEC device works only with physical addresses. This is the maximum size for a SEC
 *  descriptor on SEC 4.4 device, 64 words.
 */
#define SEC_CRYPTO_DESCRIPTOR_SIZE  256

/** Size of job descriptor submitted to SEC device for each packet to be processed.
 *  Contains 3 DMA address pointers: to shared descriptor, to input buffer and to output buffer.
 *  The job descriptor contains other SEC specific commands as well. */
// TODO: keep count of cacheline alignment, size may be bigger
#define SEC_JOB_DESCRIPTOR_SIZE     sizeof(dma_addr_t) * 3 + 20

/** DMA memory required for an input ring of a job ring. */
#define SEC_DMA_MEM_INPUT_RING_SIZE     (SEC_JOB_DESCRIPTOR_SIZE) * (SEC_JOB_RING_SIZE)
/** DMA memory required for an output ring of a job ring.  
 *  Required extra 4 byte for status word per each entry. */
#define SEC_DMA_MEM_OUTPUT_RING_SIZE    (SEC_JOB_DESCRIPTOR_SIZE + 4) * (SEC_JOB_RING_SIZE)
/** DMA memory required for a job ring, including both input and output rings. */
#define SEC_DMA_MEM_JOB_RING_SIZE       (SEC_DMA_MEM_INPUT_RING_SIZE) + (SEC_DMA_MEM_OUTPUT_RING_SIZE)

#else //#ifdef SEC_HW_VERSION_4_4

////////////////////////////////////////////////////////////////////////////////////////////////
// SEC 3.1
////////////////////////////////////////////////////////////////////////////////////////////////


/** On SEC 3.1 device there is no concept of shared descriptor.
 *  There are only job descriptors used for every packet submitted to SEC device.
 */
#define SEC_CRYPTO_DESCRIPTOR_SIZE  0
/** Size of cryptographic context that is used directly in communicating with SEC device.
 *  SEC device works only with physical addresses. This is the maximum size for a SEC
 *  descriptor on SEC 3.1 device, 8 double words. */
// TODO: keep count of cacheline alignment, size may be bigger
#define SEC_JOB_DESCRIPTOR_SIZE     64
/** DMA memory required for a channel (similar with job ring in SEC 4.4). */
#define SEC_DMA_MEM_INPUT_RING_SIZE     (SEC_JOB_DESCRIPTOR_SIZE) * (SEC_JOB_RING_SIZE)
/** DMA memory required for a channel (similar with job ring in SEC 4.4). 
 * On SEC 3.1 there is not output ring, instead SEC directly updates jobs from channel (input ring) */
#define SEC_DMA_MEM_JOB_RING_SIZE       (SEC_DMA_MEM_INPUT_RING_SIZE)

#endif



/** When calling sec_init() UA will provide an area of virtual memory
 *  of size #SEC_DMA_MEMORY_SIZE to be  used internally by the driver
 *  to allocate data (like SEC descriptors) that needs to be passed to 
 *  SEC device in physical addressing and later on retrieved from SEC device. 
 *  At initialization the UA provides specialized ptov/vtop functions/macros to
 *  translate addresses allocated from this memory area. */
#define SEC_DMA_MEMORY_SIZE     (SEC_CRYPTO_DESCRIPTOR_SIZE) * (SEC_MAX_PDCP_CONTEXTS)  + \
                                (SEC_DMA_MEM_JOB_RING_SIZE) * (MAX_SEC_JOB_RINGS)

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
 * SEC_DRIVER_LOG_INFO - log errors and info messages
 */
#define SEC_DRIVER_LOGGING_LEVEL SEC_DRIVER_LOG_INFO

/***************************************/
/* SEC JOB RING related configuration. */
/***************************************/

#ifdef SEC_HW_VERSION_4_4

////////////////////////////////////////////////////////////////////////////////////////////////
// SEC 4.4
////////////////////////////////////////////////////////////////////////////////////////////////


/** Configure the size of the JOB RING.
 * For SEC 4.4 the maximum size of the RING is hardware limited to 1024.
 * However the number of packets in flight in a time interval of 1ms can be calculated
 * from the traffic rate (Mbps) and packet size. 
 * Here it was considered a packet size of 40 bytes. 
 *
 * @note Round up to nearest power of 2 for optimized update
 * of producer/consumer indexes of each job ring
 */
#define SEC_JOB_RING_SIZE  512
#define SEC_JOB_RING_HW_SIZE    SEC_JOB_RING_SIZE
#else

////////////////////////////////////////////////////////////////////////////////////////////////
// SEC 3.1
////////////////////////////////////////////////////////////////////////////////////////////////

/** Configure the size of the JOB RING.
 *  For SEC 3.1 the size of the FIFO (concept similar to JOB INPUT RING
 *  on SEC 4.4) is hardware fixed to 24. */
#define SEC_JOB_RING_HW_SIZE  24
/** The size of the job ring rounded up to nearest power of 2.
 *  This is an optimization for updating producer/consumer indexes 
 *  of a job ring with bitwise operations. */
#define SEC_JOB_RING_SIZE  32
#endif

/** Maximum number of job rings supported by SEC hardware */
#define MAX_SEC_JOB_RINGS         4

/************************************************/
/* Scatter/Gather support related configuration */
/************************************************/

/** Enable or disable the support for scatter/gather
 * buffers in the SEC driver.
 * Valid values:
 * ON - enable scatter gather support
 * OFF - disable scatter gather support
 */
#define SEC_ENABLE_SCATTER_GATHER OFF


/***************************************************/
/* Interrupt coalescing related configuration.     */
/* NOTE: SEC hardware enabled interrupt            */
/* coalescing is not supported on SEC version 3.1! */
/* SEC version 4.4 has support for interrupt       */
/* coalescing.                                     */
/***************************************************/

#ifdef SEC_HW_VERSION_4_4

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
#endif

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/*================================================================================================*/

/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //FSL_SEC_CONFIG_H
