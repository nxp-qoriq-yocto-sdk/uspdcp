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

#ifndef TEST_SEC_DRIVER
#define TEST_SEC_DRIVER

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/

/******************************************************************************/
// START OF CONFIGURATION SECTION
/******************************************************************************/

/* Each PDCP context will be assigned a random scenario (ciphering + integrity
 * algorithms will be random.
 */
//#define RANDOM_TESTING

//////////////////////////////////////////////////////////////////////////////
// Number of PDCP contexts options
//////////////////////////////////////////////////////////////////////////////

// The number of PDCP contexts used. Each DRB will have 2 PDCP contexts, one for each direction.
// Generate randomly the number of PDCP contexts, in the range
// #MIN_PDCP_CONTEXT_NUMBER ... #MAX_PDCP_CONTEXT_NUMBER

#ifdef RANDOM_TESTING
// Higher limit for number of PDCP contexts
#define MAX_PDCP_CONTEXT_NUMBER         15
// Lower limit for number of PDCP contexts
#define MIN_PDCP_CONTEXT_NUMBER         5
#else
// Will use one scenario per context
#define MAX_PDCP_CONTEXT_NUMBER             (sizeof(test_scenarios)/sizeof(test_scenarios[0]))

#endif
// The maximum number of packets processed per context
// This test application will process a random number of packets per context ranging
// from a #MIN_PACKET_NUMBER_PER_CTX to #MAX_PACKET_NUMBER_PER_CTX value.
// Higher limit for number of packets per PDCP context
#define MAX_PACKET_NUMBER_PER_CTX   10
// Lower limit for number of packets per PDCP context
#define MIN_PACKET_NUMBER_PER_CTX   1

//////////////////////////////////////////////////////////////////////////////
// Logging Options
//////////////////////////////////////////////////////////////////////////////

// Disable test application logging
//#define test_printf(format, ...)

// Enable test application logging
#define test_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__)

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


#endif  /* TEST_SEC_DRIVER */
