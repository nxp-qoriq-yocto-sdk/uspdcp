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
#define test_printf(format, ...)
// Disable test application logging for clock cycle measurements
#define profile_printf(format, ...)

// Enable test application logging
// #define test_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__)
// Enable test application logging for clock cycle measurements
// #define profile_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__)



// Enable random number of Scatter Gather fragments
#define RANDOM_SCATTER_GATHER_FRAGMENTS

/******************************************************************************/
// END OF CONFIGURATION SECTION
/******************************************************************************/


/*==============================================================================
                                    ENUMS
==============================================================================*/
enum scenario_direction_e{
    SCENARIO_DIR_UPLINK = 0,
    SCENARIO_DIR_DOWNLINK,
    SCENARIO_DIR_INVALID
};

enum scenario_type_e{
    SCENARIO_PDCP_CPLANE_AES_CTR_AES_CMAC = 0,
    SCENARIO_PDCP_CPLANE_SNOW_F8_SNOW_F9,
    SCENARIO_PDCP_CPLANE_AES_CTR_NULL,
    SCENARIO_PDCP_CPLANE_SNOW_F8_NULL,
    SCENARIO_PDCP_CPLANE_NULL_AES_CMAC,
    SCENARIO_PDCP_CPLANE_NULL_SNOW_F9,
    SCENARIO_PDCP_CPLANE_NULL_NULL,
    SCENARIO_PDCP_UPLANE_SHORT_SN_AES_CTR,
    SCENARIO_PDCP_UPLANE_LONG_SN_AES_CTR,
    SCENARIO_PDCP_UPLANE_SHORT_SN_SNOW_F8,
    SCENARIO_PDCP_UPLANE_LONG_SN_SNOW_F8,
    SCENARIO_PDCP_UPLANE_SHORT_SN_NULL,
    SCENARIO_PDCP_UPLANE_LONG_SN_NULL,
    SCENARIO_PDCP_INVALID
};
/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
typedef struct user_params_s{
    uint8_t max_frags;
    uint16_t payload_size;
    enum scenario_type_e scenario;
    enum scenario_direction_e direction;
}users_params_t;
/*==============================================================================
                                 CONSTANTS
==============================================================================*/

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/*============================================================================*/


#endif  /* TEST_BENCHMARK_TWO_THREADS */
