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

#ifndef TEST_BENCHMARK_TWO_THREADS
#define TEST_BENCHMARK_TWO_THREADS

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/

/******************************************************************************/
// START OF CONFIGURATION SECTION
/******************************************************************************/

//////////////////////////////////////////////////////////////////////////
// !!!!!!!!!!!!!!!!!       IMPORTANT !!!!!!!!!!!!!!!!
//Select one and only one algorithm at a time, from below
//////////////////////////////////////////////////////////////////////////

// Ciphering
#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F8_ENC
// Deciphering
//#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F8_DEC

// Authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F9_ENC
// Authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F9_DEC

// Ciphering
//#define PDCP_TEST_SCENARIO  PDCP_TEST_AES_CTR_ENC
// Deciphering
//#define PDCP_TEST_SCENARIO  PDCP_TEST_AES_CTR_DEC

// Authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_AES_CMAC_ENC
// Authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_AES_CMAC_DEC

//////////////////////////////////////////////////////////////////////////////
// Number of PDCP contexts options
//////////////////////////////////////////////////////////////////////////////

// The number of PDCP contexts used. Each DRB will have 2 PDCP contexts, one for each direction.
// This is the number of PDCP context on one direction:DL/UL.
// The total number of PDCP contexts used is PDCP_CONTEXT_NUMBER * 2.
#define PDCP_CONTEXT_NUMBER         (UE_NUMBER * DRB_PER_UE)
//#define PDCP_CONTEXT_NUMBER         1


// Enable this if test should run in infinite loop.
// Use this for measurements of CPU load with <top> tool from Linux.
//#define TEST_TYPE_INFINITE_LOOP

//////////////////////////////////////////////////////////////////////////////
// Logging Options
//////////////////////////////////////////////////////////////////////////////

// Disable test application logging
//#define test_printf(format, ...)
// Disable test application logging for clock cycle measurements
//#define profile_printf(format, ...)

// Enable test application logging
#define test_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__)
// Enable test application logging for clock cycle measurements
#define profile_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__)

// The length of a single SG fragment
#define SCATTER_GATHER_BUF_SIZE     128

// Enable this to have Scatter Gather fragments of random lengths
// The buffer sizes will be 1..SCATTER_GATHER_BUF_SIZE
#define RANDOM_SCATTER_GATHER_BUFFER_SIZE

// Enable this to have 512 pkts with more than 1 SG fragment and 
// next 512 pkts with one SG fragment, repeated over the number
// of packets to send
//#define ONE_SCATTER_GATHER_FRAGMENT_MIX

/******************************************************************************/
// END OF CONFIGURATION SECTION
/******************************************************************************/


/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/

/*==============================================================================
                                 CONSTANTS
==============================================================================*/
/** List of algorithms supported by this test */
#define PDCP_TEST_SNOW_F8_ENC   0
#define PDCP_TEST_SNOW_F8_DEC   1
#define PDCP_TEST_SNOW_F9_ENC   2
#define PDCP_TEST_SNOW_F9_DEC   3
#define PDCP_TEST_AES_CTR_ENC   4
#define PDCP_TEST_AES_CTR_DEC   5
#define PDCP_TEST_AES_CMAC_ENC  6
#define PDCP_TEST_AES_CMAC_DEC  7

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/*============================================================================*/


#endif  /* TEST_BENCHMARK_TWO_THREADS */
