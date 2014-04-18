/* Copyright 2013 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the names of its 
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.      
 * 
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.                                                                   
 *                                                                                  
 * This software is provided by Freescale Semiconductor "as is" and any
 * express or implied warranties, including, but not limited to, the implied
 * warranties of merchantability and fitness for a particular purpose are
 * disclaimed. In no event shall Freescale Semiconductor be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential damages
 * (including, but not limited to, procurement of substitute goods or services;
 * loss of use, data, or profits; or business interruption) however caused and
 * on any theory of liability, whether in contract, strict liability, or tort
 * (including negligence or otherwise) arising in any way out of the use of
 * this software, even if advised of the possibility of such damage.
 */


#ifndef TEST_SEC_DRIVER_WCDMA
#define TEST_SEC_DRIVER_WCDMA

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/
#ifdef USDPAA
/* The size of the DMA-able memory zone to be allocated through the test */
#define DMAMEM_SIZE	0x4000000
#endif
/******************************************************************************/
// START OF CONFIGURATION SECTION
/******************************************************************************/

/* Each RLC context will be assigned a random scenario (ciphering + integrity
 * algorithms will be random.
 */
//#define RANDOM_TESTING

//////////////////////////////////////////////////////////////////////////////
// Number of RLC contexts options
//////////////////////////////////////////////////////////////////////////////

// The number of RLC contexts used. Each DRB will have 2 RLC contexts, one for each direction.
// Generate randomly the number of RLC contexts, in the range
// #MIN_RLC_CONTEXT_NUMBER ... #MAX_RLC_CONTEXT_NUMBER

#ifdef RANDOM_TESTING
// Higher limit for number of RLC contexts
#define MAX_RLC_CONTEXT_NUMBER         15
// Lower limit for number of RLC contexts
#define MIN_RLC_CONTEXT_NUMBER         5
#else
// Will use one scenario per context
#define MAX_RLC_CONTEXT_NUMBER             (sizeof(test_scenarios)/sizeof(test_scenarios[0]))

#endif
// The maximum number of packets processed per context
// This test application will process a random number of packets per context ranging
// from a #MIN_PACKET_NUMBER_PER_CTX to #MAX_PACKET_NUMBER_PER_CTX value.
// Higher limit for number of packets per RLC context
#define MAX_PACKET_NUMBER_PER_CTX   10
// Lower limit for number of packets per RLC context
#define MIN_PACKET_NUMBER_PER_CTX   1

//////////////////////////////////////////////////////////////////////////////
// Logging Options
//////////////////////////////////////////////////////////////////////////////

// Disable test application logging
#define test_printf(format, ...)

// Enable test application logging
//#define test_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__)

/******************************************************************************/
// END OF CONFIGURATION SECTION
/******************************************************************************/

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/

/*==============================================================================
                                 CONSTANTS
==============================================================================*/

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/*============================================================================*/


#endif  /* TEST_SEC_DRIVER_WCDMA */
