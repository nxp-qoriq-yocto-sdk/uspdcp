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
 
#ifndef __MIXED_DESCS_TESTS_H__
#define __MIXED_DESCS_TESTS_H__

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/
#define TEST_KEY_LEN                            16
#define PDCP_CTRL_PLANE_HEADER_LENGTH           1
#define PDCP_DATA_PLANE_HEADER_LENGTH_SHORT_SN  1
#define PDCP_DATA_PLANE_HEADER_LENGTH_LONG_SN   2

#define MAX_NUM_SCENARIOS     (sizeof(test_scenarios)/sizeof(test_scenarios[0]))
/*==============================================================================
                                    ENUMS
==============================================================================*/
enum test_scenarios{
    /* Control Plane w/AES CTR encryption + AES CMAC integrity UPLINK */
    TEST_SCENARIO_CTRL_PLANE_AES_CTR_AES_CMAC_UPLINK,
    /* Control Plane w/AES CTR encryption + AES CMAC integrity DOWNLINK */
    TEST_SCENARIO_CTRL_PLANE_AES_CTR_AES_CMAC_DOWNLINK,
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity DOWNLINK */
    TEST_SCENARIO_CTRL_PLANE_SNOW_F8_SNOW_F9_DOWNLINK,
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity UPLINK */
    TEST_SCENARIO_CTRL_PLANE_SNOW_F8_SNOW_F9_UPLINK,
    /* Control Plane w/SNOW f8 encryption + NULL integrity DOWNLINK */
    TEST_SCENARIO_CTRL_PLANE_SNOW_F8_NULL_DOWNLINK,
    /* Control Plane w/SNOW f8 encryption + NULL integrity UPLINK */
    TEST_SCENARIO_CTRL_PLANE_SNOW_F8_NULL_UPLINK,
    /* Control Plane w/AES CTR encryption + NULL integrity DOWNLINK */
    TEST_SCENARIO_CTRL_PLANE_AES_CTR_NULL_DOWNLINK,
    /* Control Plane w/AES CTR encryption + NULL integrity UPLINK */
    TEST_SCENARIO_CTRL_PLANE_AES_CTR_NULL_UPLINK,
    /* Control Plane w/NULL encryption + SNOW f9 integrity DOWNLINK */
    TEST_SCENARIO_CTRL_PLANE_NULL_SNOW_F9_DOWNLINK,
    /* Control Plane w/NULL encryption + SNOW f9 integrity UPLINK */
    TEST_SCENARIO_CTRL_PLANE_NULL_SNOW_F9_UPLINK,
    /* Control Plane w/NULL encryption + AES CMAC integrity DOWNLINK */
    TEST_SCENARIO_CTRL_PLANE_NULL_AES_CMAC_DOWNLINK,
    /* Control Plane w/NULL encryption + AES CMAC integrity UPLINK */
    TEST_SCENARIO_CTRL_PLANE_NULL_AES_CMAC_UPLINK,
    /* Control Plane w/NULL encryption + NULL integrity UPLINK */
    TEST_SCENARIO_CTRL_PLANE_NULL_NULL_UPLINK,
    /* User Plane w/AES CTR encryption UPLINK LONG SN */
    TEST_SCENARIO_USER_PLANE_AES_CTR_UPLINK_LONG_SN,
    /* User Plane w/AES CTR encryption DOWNLINK LONG SN */
    TEST_SCENARIO_USER_PLANE_AES_CTR_DOWNINK_LONG_SN,
    /* User Plane w/AES CTR encryption UPLINK SHORT SN */
    TEST_SCENARIO_USER_PLANE_AES_CTR_UPLINK_SHORT_SN,
    /* User Plane w/AES CTR encryption DOWNLINK SHORT SN */
    TEST_SCENARIO_USER_PLANE_AES_CTR_DOWNLINK_SHORT_SN,
    /* User Plane w/SNOW f8 encryption UPLINK LONG SN */
    TEST_SCENARIO_USER_PLANE_SNOW_F8_UPLINK_LONG_SN,
    /* User Plane w/SNOW f8 encryption DOWNLINK LONG SN */
    TEST_SCENARIO_USER_PLANE_SNOW_F8_DOWNLINK_LONG_SN,
    /* User Plane w/SNOW f8 encryption UPLINK SHORT SN */
    TEST_SCENARIO_USER_PLANE_SNOW_F8_UPLINK_SHORT_SN,
    /* User Plane w/SNOW f8 encryption DOWNLINK SHORT SN */
    TEST_SCENARIO_USER_PLANE_SNOW_F8_DOWNLINK_SHORT_SN,
    /* User Plane w/NULL encryption DOWNLINK LONG SN */
    TEST_SCENARIO_USER_PLANE_NULL_DOWNLINK_LONG_SN,
    /* User Plane w/NULL encryption DOWNLINK SHORT SN */
    TEST_SCENARIO_USER_PLANE_NULL_DOWNLINK_SHORT_SN
};

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
struct test_param{
    uint8_t type;
    uint8_t cipher_algorithm;
    uint8_t integrity_algorithm;
};

/*==============================================================================
                                 CONSTANTS
==============================================================================*/

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/
static struct test_param test_params[] = {
    {
        /* Control Plane w/AES CTR encryption + AES CMAC integrity UPLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_AES,
        .integrity_algorithm = SEC_ALG_AES,
    },
    {
        /* Control Plane w/AES CTR encryption + AES CMAC integrity DOWNLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_AES,
        .integrity_algorithm = SEC_ALG_AES,
    },
    {
        /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity DOWNLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_SNOW,
        .integrity_algorithm = SEC_ALG_SNOW,
    },
    {
        /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity UPLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_SNOW,
        .integrity_algorithm = SEC_ALG_SNOW,
    },
    {
        /* Control Plane w/SNOW f8 encryption + NULL integrity DOWNLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_SNOW,
        .integrity_algorithm = SEC_ALG_NULL,
    },
    {
        /* Control Plane w/SNOW f8 encryption + NULL integrity UPLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_SNOW,
        .integrity_algorithm = SEC_ALG_NULL,
    },
    {
        /* Control Plane w/AES CTR encryption + NULL integrity DOWNLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_AES,
        .integrity_algorithm = SEC_ALG_NULL,
    },
    {
        /* Control Plane w/AES CTR encryption + NULL integrity UPLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_AES,
        .integrity_algorithm = SEC_ALG_NULL,
    },
    {
        /* Control Plane w/NULL encryption + SNOW f9 integrity DOWNLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_NULL,
        .integrity_algorithm = SEC_ALG_SNOW,
    },
    {
        /* Control Plane w/NULL encryption + SNOW f9 integrity UPLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_NULL,
        .integrity_algorithm = SEC_ALG_SNOW,
    },
    {
        /* Control Plane w/NULL encryption + AES CMAC integrity DOWNLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_NULL,
        .integrity_algorithm = SEC_ALG_AES,
    },
    {
        /* Control Plane w/NULL encryption + AES CMAC integrity UPLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_NULL,
        .integrity_algorithm = SEC_ALG_AES,
    },
    {
        /* Control Plane w/NULL encryption + NULL integrity UPLINK */
        .type = PDCP_CONTROL_PLANE,
        .cipher_algorithm = SEC_ALG_NULL,
        .integrity_algorithm = SEC_ALG_NULL,
    },
    {
        /* User Plane w/AES CTR encryption UPLINK LONG SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_AES,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/AES CTR encryption DOWNLINK LONG SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_AES,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/AES CTR encryption UPLINK SHORT SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_AES,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/AES CTR encryption DOWNLINK SHORT SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_AES,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/SNOW f8 encryption UPLINK LONG SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_SNOW,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/SNOW f8 encryption DOWNLINK LONG SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_SNOW,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/SNOW f8 encryption UPLINK SHORT SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_SNOW,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/SNOW f8 encryption DOWNLINK SHORT SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_SNOW,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/NULL encryption DOWNLINK LONG SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_NULL,
        .integrity_algorithm = SEC_ALG_NULL,
     },
    {
        /* User Plane w/NULL encryption DOWNLINK SHORT SN*/
        .type = PDCP_DATA_PLANE,
        .cipher_algorithm = SEC_ALG_NULL,
        .integrity_algorithm = SEC_ALG_NULL,
     },
};

static uint32_t test_hfn = 0x000fa557;

static uint32_t test_hfn_threshold = 0x000fa558;

static uint8_t test_bearer = 0x03;

static uint8_t test_packet_direction[] = {
    /* Control Plane w/AES CTR encryption + AES CMAC integrity UPLINK */
    PDCP_UPLINK,
    /* Control Plane w/AES CTR encryption + AES CMAC integrity DOWNLINK */
    PDCP_DOWNLINK,
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity DOWNLINK */
    PDCP_DOWNLINK,
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity UPLINK */
    PDCP_UPLINK,
    /* Control Plane w/SNOW f8 encryption + NULL integrity DOWNLINK */
    PDCP_DOWNLINK,
    /* Control Plane w/SNOW f8 encryption + NULL integrity UPLINK */
    PDCP_UPLINK,
    /* Control Plane w/AES CTR encryption + NULL integrity DOWNLINK */
    PDCP_DOWNLINK,
    /* Control Plane w/AES CTR encryption + NULL integrity UPLINK */
    PDCP_UPLINK,
    /* Control Plane w/NULL encryption + SNOW f9 integrity DOWNLINK */
    PDCP_DOWNLINK,
    /* Control Plane w/NULL encryption + SNOW f9 integrity UPLINK */
    PDCP_UPLINK,
    /* Control Plane w/NULL encryption + AES CMAC integrity DOWNLINK */
    PDCP_DOWNLINK,
    /* Control Plane w/NULL encryption + AES CMAC integrity UPLINK */
    PDCP_UPLINK,
    /* Control Plane w/NULL encryption + NULL integrity UPLINK */
    PDCP_UPLINK,
    /* User Plane w/AES CTR encryption UPLINK LONG SN */
    PDCP_UPLINK,
    /* User Plane w/AES CTR encryption DOWNLINK LONG SN */
    PDCP_DOWNLINK,
    /* User Plane w/AES CTR encryption UPLINK SHORT SN */
    PDCP_UPLINK,
    /* User Plane w/AES CTR encryption DOWNLINK SHORT SN */
    PDCP_DOWNLINK,
    /* User Plane w/SNOW f8 encryption UPLINK LONG SN */
    PDCP_UPLINK,
    /* User Plane w/SNOW f8 encryption DOWNLINK LONG SN */
    PDCP_DOWNLINK,
    /* User Plane w/SNOW f8 encryption UPLINK SHORT SN */
    PDCP_UPLINK,
    /* User Plane w/SNOW f8 encryption DOWNLINK SHORT SN */
    PDCP_DOWNLINK,
    /* User Plane w/NULL encryption DOWNLINK LONG SN */
    PDCP_DOWNLINK,
    /* User Plane w/NULL encryption DOWNLINK SHORT SN */
    PDCP_DOWNLINK,
};

static uint8_t test_data_sns[] = {
    /* Control Plane w/AES CTR encryption + AES CMAC integrity UPLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/AES CTR encryption + AES CMAC integrity DOWNLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity DOWNLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity UPLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/SNOW f8 encryption + NULL integrity DOWNLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/SNOW f8 encryption + NULL integrity UPLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/AES CTR encryption + NULL integrity DOWNLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/AES CTR encryption + NULL integrity UPLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/NULL encryption + SNOW f9 integrity DOWNLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/NULL encryption + SNOW f9 integrity UPLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/NULL encryption + AES CMAC integrity DOWNLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/NULL encryption + AES CMAC integrity UPLINK */
    SEC_PDCP_SN_SIZE_5,
    /* Control Plane w/NULL encryption + NULL integrity UPLINK */
    SEC_PDCP_SN_SIZE_5,
    /* User Plane w/AES CTR encryption UPLINK LONG SN */
    SEC_PDCP_SN_SIZE_12,
    /* User Plane w/AES CTR encryption DOWNLINK LONG SN */
    SEC_PDCP_SN_SIZE_12,
    /* User Plane w/AES CTR encryption UPLINK SHORT SN */
    SEC_PDCP_SN_SIZE_7,
    /* User Plane w/AES CTR encryption DOWNLINK SHORT SN */
    SEC_PDCP_SN_SIZE_7,
    /* User Plane w/SNOW f8 encryption UPLINK LONG SN */
    SEC_PDCP_SN_SIZE_12,
    /* User Plane w/SNOW f8 encryption DOWNLINK LONG SN */
    SEC_PDCP_SN_SIZE_12,
    /* User Plane w/SNOW f8 encryption UPLINK SHORT SN */
    SEC_PDCP_SN_SIZE_7,
    /* User Plane w/SNOW f8 encryption DOWNLINK SHORT SN */
    SEC_PDCP_SN_SIZE_7,
    /* User Plane w/NULL encryption DOWNLINK LONG SN */
    SEC_PDCP_SN_SIZE_12,
    /* User Plane w/NULL encryption DOWNLINK SHORT SN */
    SEC_PDCP_SN_SIZE_7,
};

static uint8_t test_crypto_key[] = {0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52};

static uint8_t test_auth_key[] = {0xc7,0x36,0xc6,0xaa,0xb2,0x2b,0xff,0xf9,0x1e,0x26,0x98,0xd2,0xe2,0x2a,0xd5,0x7e};

static uint8_t *test_hdr[] = {
    /* Control Plane w/AES CTR encryption + AES CMAC integrity UPLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/AES CTR encryption + AES CMAC integrity DOWNLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity DOWNLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity UPLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/SNOW f8 encryption + NULL integrity DOWNLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/SNOW f8 encryption + NULL integrity UPLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/AES CTR encryption + NULL integrity DOWNLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/AES CTR encryption + NULL integrity UPLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/NULL encryption + SNOW f9 integrity DOWNLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/NULL encryption + SNOW f9 integrity UPLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/NULL encryption + AES CMAC integrity DOWNLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/NULL encryption + AES CMAC integrity UPLINK */
    (uint8_t[]){0x8b},
    /* Control Plane w/NULL encryption + NULL integrity UPLINK */
    (uint8_t[]){0x8b},
    /* User Plane w/AES CTR encryption UPLINK LONG SN */
    (uint8_t[]){0x8b,0x26},
    /* User Plane w/AES CTR encryption DOWNLINK LONG SN */
    (uint8_t[]){0x8b,0x26},
    /* User Plane w/AES CTR encryption UPLINK SHORT SN */
    (uint8_t[]){0x8b},
    /* User Plane w/AES CTR encryption DOWNLINK SHORT SN */
    (uint8_t[]){0x8b},
    /* User Plane w/SNOW f8 encryption UPLINK LONG SN */
    (uint8_t[]){0x8b,0x26},
    /* User Plane w/SNOW f8 encryption DOWNLINK LONG SN */
    (uint8_t[]){0x8b,0x26},
    /* User Plane w/SNOW f8 encryption UPLINK SHORT SN */
    (uint8_t[]){0x8b},
    /* User Plane w/SNOW f8 encryption DOWNLINK SHORT SN */
    (uint8_t[]){0x8b},
    /* User Plane w/NULL encryption DOWNLINK LONG SN */
    (uint8_t[]){0x8b,0x26},
    /* User Plane w/NULL encryption DOWNLINK SHORT SN */
    (uint8_t[]){0x8b},
};

static uint32_t test_data_in_len = 15;

static uint8_t test_data_in[] = {0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8};

static uint32_t test_data_out_len[] = {
    /* Control Plane w/AES CTR encryption + AES CMAC integrity UPLINK */
    19,
    /* Control Plane w/AES CTR encryption + AES CMAC integrity DOWNLINK */
    19,
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity DOWNLINK */
    19,
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity UPLINK */
    19,
    /* Control Plane w/SNOW f8 encryption + NULL integrity DOWNLINK */
    19,
    /* Control Plane w/SNOW f8 encryption + NULL integrity UPLINK */
    19,
    /* Control Plane w/AES CTR encryption + NULL integrity DOWNLINK */
    19,
    /* Control Plane w/AES CTR encryption + NULL integrity UPLINK */
    19,
    /* Control Plane w/NULL encryption + SNOW f9 integrity DOWNLINK */
    19,
    /* Control Plane w/NULL encryption + SNOW f9 integrity UPLINK */
    19,
    /* Control Plane w/NULL encryption + AES CMAC integrity DOWNLINK */
    19,
    /* Control Plane w/NULL encryption + AES CMAC integrity UPLINK */
    19,
    /* Control Plane w/NULL encryption + NULL integrity UPLINK */
    19,
    /* User Plane w/AES CTR encryption UPLINK LONG SN */
    15,
    /* User Plane w/AES CTR encryption DOWNLINK LONG SN */
    15,
    /* User Plane w/AES CTR encryption UPLINK SHORT SN */
    15,
    /* User Plane w/AES CTR encryption DOWNLINK SHORT SN */
    15,
    /* User Plane w/SNOW f8 encryption UPLINK LONG SN */
    15,
    /* User Plane w/SNOW f8 encryption DOWNLINK LONG SN */
    15,
    /* User Plane w/SNOW f8 encryption UPLINK SHORT SN */
    15,
    /* User Plane w/SNOW f8 encryption DOWNLINK SHORT SN */
    15,
    /* User Plane w/NULL encryption DOWNLINK LONG SN */
    15,
    /* User Plane w/NULL encryption DOWNLINK SHORT SN */
    15,
};

static uint8_t *test_data_out[] = {
    /* Control Plane w/AES CTR encryption + AES CMAC integrity UPLINK */
    (uint8_t[]){0x2c,0x59,0x74,0xab,0xdc,0xd8,0x36,0xf6,0x1b,0x54,0x8d,0x46,0x93,0x1c,0xff,0xc1,0x92,0x1b,0xb4},
    /* Control Plane w/AES CTR encryption + AES CMAC integrity DOWNLINK */
    (uint8_t[]){0xf2,0xb9,0x9d,0x96,0x51,0xcc,0x1e,0xe8,0x55,0x3e,0x98,0xc5,0x58,0xec,0x4c,0xcf,0xce,0x0f,0x8b},
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity DOWNLINK */
    (uint8_t[]){0x26,0xf3,0x67,0xf1,0x42,0x50,0x1a,0x85,0x02,0xb9,0x00,0xa8,0x9b,0xcf,0x06,0x4c,0xb2,0xc3,0x4a},
    /* Control Plane w/SNOW f8 encryption + SNOW f9 integrity UPLINK */
    (uint8_t[]){0x39,0xd1,0x2b,0xbd,0x2a,0x4c,0x91,0x59,0xff,0xfa,0xce,0x68,0xc0,0x7c,0x30,0x58,0xba,0x46,0x01},
    /* Control Plane w/SNOW f8 encryption + NULL integrity DOWNLINK */
    (uint8_t[]){0x26,0xf3,0x67,0xf1,0x42,0x50,0x1a,0x85,0x02,0xb9,0x00,0xa8,0x9b,0xcf,0x06,0xd1,0x2c,0x86,0x7c},
    /* Control Plane w/SNOW f8 encryption + NULL integrity UPLINK */
    (uint8_t[]){0x39,0xd1,0x2b,0xbd,0x2a,0x4c,0x91,0x59,0xff,0xfa,0xce,0x68,0xc0,0x7c,0x30,0xd0,0xc5,0x08,0x58},
    /* Control Plane w/AES CTR encryption + NULL integrity DOWNLINK */
    (uint8_t[]){0xf2,0xb9,0x9d,0x96,0x51,0xcc,0x1e,0xe8,0x55,0x3e,0x98,0xc5,0x58,0xec,0x4c,0x92,0x40,0x52,0x8e},
    /* Control Plane w/AES CTR encryption + NULL integrity UPLINK */
    (uint8_t[]){0x2c,0x59,0x74,0xab,0xdc,0xd8,0x36,0xf6,0x1b,0x54,0x8d,0x46,0x93,0x1c,0xff,0x32,0x4f,0x1a,0x6b},
    /* Control Plane w/NULL encryption + SNOW f9 integrity DOWNLINK */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8,0x9d,0x9e,0x45,0x36},
    /* Control Plane w/NULL encryption + SNOW f9 integrity UPLINK */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8,0x88,0x7f,0x4e,0x59},
    /* Control Plane w/NULL encryption + AES CMAC integrity DOWNLINK */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8,0x5d,0x8e,0x5d,0x05},
    /* Control Plane w/NULL encryption + AES CMAC integrity UPLINK */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8,0xf3,0xdd,0x01,0xdf},
    /* Control Plane w/NULL encryption + NULL integrity UPLINK */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8,0x00,0x00,0x00,0x00},
    /* User Plane w/AES CTR encryption UPLINK LONG SN */
    (uint8_t[]){0xde,0x0a,0x59,0xca,0x7d,0x93,0xa3,0xb5,0xd2,0x88,0xb3,0x04,0xa2,0x12,0x09},
    /* User Plane w/AES CTR encryption DOWNLINK LONG SN */
    (uint8_t[]){0x69,0x92,0x25,0xd8,0xe9,0xd5,0xe9,0x53,0x60,0x49,0x9f,0xe9,0x8f,0xbe,0x6a},
    /* User Plane w/AES CTR encryption UPLINK SHORT SN */
    (uint8_t[]){0x0f,0xa1,0xf2,0x56,0x6e,0xee,0x62,0x1c,0x62,0x06,0x7e,0x38,0x4a,0x02,0xa4},
    /* User Plane w/AES CTR encryption DOWNLINK SHORT SN */
    (uint8_t[]){0x00,0x8d,0x50,0x80,0x30,0xda,0xc7,0x14,0xc5,0xe0,0xc8,0xfb,0x83,0xd0,0x73},
    /* User Plane w/SNOW f8 encryption UPLINK LONG SN */
    (uint8_t[]){0x7a,0xe0,0x00,0x07,0x2a,0xa6,0xef,0xdc,0x75,0xef,0x2e,0x27,0x0f,0x69,0x3d},
    /* User Plane w/SNOW f8 encryption DOWNLINK LONG SN */
    (uint8_t[]){0x7e,0xbb,0x80,0x20,0xba,0xef,0xe7,0xf7,0xef,0x69,0x51,0x85,0x09,0xa5,0xab},
    /* User Plane w/SNOW f8 encryption UPLINK SHORT SN */
    (uint8_t[]){0x80,0xcf,0xe5,0x27,0xe2,0x88,0x2a,0xac,0xc5,0xaf,0x49,0x9b,0x3e,0x48,0x89},
    /* User Plane w/SNOW f8 encryption DOWNLINK SHORT SN */
    (uint8_t[]){0xe2,0x51,0x58,0x88,0xff,0x1a,0x00,0xe4,0x67,0x05,0x46,0x24,0x2f,0x07,0xb7},
    /* User Plane w/NULL encryption DOWNLINK LONG SN */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* User Plane w/NULL encryption DOWNLINK SHORT SN */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
};

#endif // __MIXED_DESCS_TESTS_H__
