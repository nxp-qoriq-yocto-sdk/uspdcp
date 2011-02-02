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

/* SEC version supported by this user space driver: 3.1 */
#define FSL_SEC_SUPPORTED_VERSION 31

#define  ON  1
#define  OFF 0

/* SEC is configured to work in polling mode */
#define FSL_SEC_POLLING_MODE     0
/* SEC is configured to work in interrupt mode */
#define FSL_SEC_INTERRUPT_MODE   1


/************************************************/
/* SEC USER SPACE DRIVER related configuration. */
/************************************************/

/** Maximum number of SEC PDCP contexts that can be managed
 *  simultaneously by SEC user space driver. */
#define FSL_SEC_MAX_PDCP_CONTEXTS   200


/************************************************/
/* SEC DEVICE related configuration.            */
/************************************************/

/** SEC device is configured for 36 bit physical addressing.
 *  Both the SEC user space driver and the SEC kernel driver
 *  need to have this configuration enable at the same time.
 */
//#define CONFIG_PHYS_64BIT


/***************************************/
/* SEC JOB RING related configuration. */
/***************************************/

/* The size of the input JOB RING.
 * For SEC 3.1 the size of the INPUT FIFO (concept similar to JOB INPUT RING
 * on SEC 4.4) is hardware fixed to 24.
 * For SEC 4.4 the maximum size of the RING is hardware limited to 1024 */
#define FSL_SEC_JOB_INPUT_RING_SIZE  24
/* The size of the input JOB RING.
 * For SEC 3.1 there is no OUTPUT FIFO.
 * For SEC 4.4 the maximum size of the RING is hardware limited to 1024 */
#define FSL_SEC_JOB_OUTPUT_RING_SIZE 24

/*******************************************/
/* SEC working mode related configuration. */
/*******************************************/

/* SEC working mode */
#define FSL_SEC_WORKING_MODE FSL_SEC_POLLING_MODE

/************************************************/
/* Scatter/Gather support related configuration */
/************************************************/

/* Enable or disable the support for scatter/gather
 * buffers in the SEC driver. */
#define FSL_SEC_ENABLE_SCATTER_GATHER OFF


/*************************************************/
/* Interrupt coalescing related configuration.   */
/* NOTE: Interrupt coalescing is not supported   */
/* on SEC versions 3.1 !!                        */
/* SEC version 4.4 has support for interrupt     */
/* coalescing.                                   */
/*************************************************/

#if FSL_SEC_SUPPORTED_VERSION == 44

/* Interrupt Coalescing Descriptor Count Threshold.
 * While interrupt coalescing is enabled (ICEN=1), this value determines
 * how many Descriptors are completed before raising an interrupt.
 *
 * Valid values for this field are from 0 to 255.
 * Note that a value of 1 functionally defeats the advantages of interrupt
 * coalescing since the threshold value is reached each time that a
 * Job Descriptor is completed. A value of 0 is treated in the same
 * manner as a value of 1.
 * */
#define FSL_SEC_INTERRUPT_COALESCING_DESCRIPTOR_COUNT_THRESH  10

/* Interrupt Coalescing Timer Threshold.
 * While interrupt coalescing is enabled (ICEN=1), this value determines the
 * maximum amount of time after processing a Descriptor before raising an interrupt.
 * Interrupt Coalescing Timer Threshold. While interrupt coalescing is enabled (ICEN=1),
 * this value determines the maximum amount of time after processing a Descriptor
 * before raising an interrupt.
 *
 * The threshold value is represented in units equal to 64 CAAM interface
 * clocks. Valid values for this field are from 1 to 65535.
 * A value of 0 results in behavior identical to that when interrupt
 * coalescing is disabled.*/
#define FSL_SEC_INTERRUPT_COALESCING_TIMER_THRESH  100
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
