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
#define LOOPBACK_TESTING

//////////////////////////////////////////////////////////////////////////
// !!!!!!!!!!!!!!!!!       IMPORTANT !!!!!!!!!!!!!!!!
//Select one and only one algorithm at a time, from below
//////////////////////////////////////////////////////////////////////////

// Ciphering
//#define PDCP_TEST_SCENARIO  PDCP_TEST_SNOW_F8_ENC
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

// Test data plane PDCP with NULL-crypto(EEA0) algorithm set
//#define PDCP_TEST_SCENARIO  PDCP_TEST_DATA_PLANE_NULL_ALGO

// Test control plane PDCP with NULL-crypto(EEA0)
// and NULL-authentication(EIA0) algorithms set
//#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_NULL_ALGO

// Test PDCP control plane encapsulation with SNOW F8 ciphering
// and SNOW F9 authentication
#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_ENC
// Test PDCP control plane decapsulation with SNOW F8 ciphering
// and SNOW F9 authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_DEC


// Test PDCP control plane encapsulation with AES CTR ciphering
// and AES CMAC authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_ENC
// Test PDCP control plane decapsulation with AES CTR ciphering
// and AES CMAC authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_DEC


// Test PDCP control plane encapsulation with AES CTR ciphering
// and SNOW F9 authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_ENC
// Test PDCP control plane decapsulation with AES CTR ciphering
// and SNOW F9 authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_DEC

// Test PDCP control plane encapsulation with SNOW F8 ciphering
// and AES CMAC authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_ENC
// Test PDCP control plane decapsulation with SNOW F8 ciphering
// and AES CMAC authentication
//#define PDCP_TEST_SCENARIO  PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_DEC




//////////////////////////////////////////////////////////////////////////////
// Number of PDCP contexts options
//////////////////////////////////////////////////////////////////////////////

// The number of PDCP contexts used. Each DRB will have 2 PDCP contexts, one for each direction.
// Generate randomly the number of PDCP contexts, in the range
// #MIN_PDCP_CONTEXT_NUMBER ... #MAX_PDCP_CONTEXT_NUMBER

// Higher limit for number of PDCP contexts
//#define MAX_PDCP_CONTEXT_NUMBER         15
#define MAX_PDCP_CONTEXT_NUMBER         1
// Lower limit for number of PDCP contexts
//#define MIN_PDCP_CONTEXT_NUMBER         5
#define MIN_PDCP_CONTEXT_NUMBER         1


// The maximum number of packets processed per context
// This test application will process a random number of packets per context ranging
// from a #MIN_PACKET_NUMBER_PER_CTX to #MAX_PACKET_NUMBER_PER_CTX value.
// Higher limit for number of packets per PDCP context
//#define MAX_PACKET_NUMBER_PER_CTX   10
#define MAX_PACKET_NUMBER_PER_CTX   1
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
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/

/*==============================================================================
                                 CONSTANTS
==============================================================================*/
/** List of algorithms supported by this test */
#define PDCP_TEST_SNOW_F8_ENC                       0
#define PDCP_TEST_SNOW_F8_DEC                       1
#define PDCP_TEST_SNOW_F9_ENC                       2
#define PDCP_TEST_SNOW_F9_DEC                       3
#define PDCP_TEST_AES_CTR_ENC                       4
#define PDCP_TEST_AES_CTR_DEC                       5
#define PDCP_TEST_AES_CMAC_ENC                      6
#define PDCP_TEST_AES_CMAC_DEC                      7
#define PDCP_TEST_DATA_PLANE_NULL_ALGO              8
#define PDCP_TEST_CTRL_PLANE_NULL_ALGO              9
#define PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_ENC    10
#define PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_DEC    11
#define PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_ENC   12
#define PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_DEC   13
#define PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_ENC    14
#define PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_DEC    15
#define PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_ENC   16
#define PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_DEC   17
/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/*============================================================================*/


#endif  /* TEST_SEC_DRIVER */
