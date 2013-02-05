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

#ifndef TEST_SEC_DRIVER_WCDMA_TEST_VECTORS
#define TEST_SEC_DRIVER_WCDMA_TEST_VECTORS

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include "test_sec_driver_wcdma.h"

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/
#define TEST_KEY_LEN                            16

#define RLC_HEADER_LENGTH_UNACKED_MODE          1
#define RLC_HEADER_LENGTH_ACKED_MODE            2

#define MAX_NUM_SCENARIOS     (sizeof(test_scenarios)/sizeof(test_scenarios[0]))
/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
struct test_scenario{
    uint8_t cipher_algorithm;
};

/*==============================================================================
                                 CONSTANTS
==============================================================================*/

/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/
static struct test_scenario test_scenarios[] = {
    {
        /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_KASUMI,
     },
    {
        /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_KASUMI,
     },
    {
        /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_KASUMI,
     },
    {
        /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_KASUMI,
     },
    {
        /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_SNOW,
     },
    {
        /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_SNOW,
     },
    {
        /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_SNOW,
     },
    {
        /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_SNOW,
    },
    {
        /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_NULL,
    },
    {
        /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
        .cipher_algorithm = SEC_ALG_RLC_CRYPTO_NULL,
    },

};

static uint32_t test_hfn[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    0x000fa557,
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    0x000fa557,
};

static uint32_t test_hfn_threshold[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    0x000fa558,
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    0x000fa558,
};

static uint8_t test_bearer[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    0x03,
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    0x03,
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    0x03,
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    0x03,
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    0x03,
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    0x03,
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    0x03,
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    0x03,
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    0x03,
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    0x03,

};

static uint8_t test_packet_direction[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    RLC_UPLINK,
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    RLC_DOWNLINK,
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    RLC_UPLINK,
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    RLC_DOWNLINK,
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    RLC_UPLINK,
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    RLC_DOWNLINK,
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    RLC_UPLINK,
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    RLC_DOWNLINK,
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    RLC_DOWNLINK,
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    RLC_UPLINK,

};

static uint8_t test_data_mode[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    RLC_UNACKED_MODE,
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    RLC_UNACKED_MODE,
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    RLC_ACKED_MODE,
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    RLC_ACKED_MODE,
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    RLC_UNACKED_MODE,
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    RLC_UNACKED_MODE,
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    RLC_ACKED_MODE,
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    RLC_ACKED_MODE,
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    RLC_UNACKED_MODE,
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    RLC_ACKED_MODE,

};

static uint8_t *test_crypto_key[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    (uint8_t[]){0x5a,0xcb,0x1d,0x64,0x4c,0x0d,0x51,0x20,0x4e,0xa5,0xf1,0x45,0x10,0x10,0xd8,0x52},
};

static uint8_t *test_hdr[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x02},
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x02},
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x80,0x08},
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x80,0x08},
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x02},
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x02},
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x80,0x08},
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x80,0x08},
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    (uint8_t[]){0x02},
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    (uint8_t[]){0x80,0x08},
};

static uint32_t test_data_in_len[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    15,
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    15,
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    15,
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    15,
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    15,
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    15,
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    15,
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    15,
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    15,
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    15,
};

static uint8_t *test_data_in[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
};

static uint32_t test_data_out_len[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    15,
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    15,
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    15,
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    15,
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    15,
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    15,
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    15,
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    15,
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    15,
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    15
};

static uint8_t *test_data_out[] = {
    /* RLC w/KASUMI f8 encryption UPLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x7d,0x8c,0xba,0xe7,0x0c,0xb0,0x29,0xbe,0xf0,0x5c,0xe1,0xcd,0xf5,0xab,0xbd},
    /* RLC w/KASUMI f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0xcf,0xcb,0xcc,0xe6,0x87,0x89,0x01,0x19,0x9f,0xc2,0xf2,0x40,0xa7,0x2f,0xd7},
    /* RLC w/KASUMI f8 encryption UPLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0xd5,0x21,0x83,0x81,0xeb,0xf5,0xda,0xd0,0xf9,0xc0,0x5c,0x58,0x1e,0xe1,0x58},
    /* RLC w/KASUMI f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x23,0x95,0x5a,0xee,0xbc,0x03,0xb1,0xad,0x9f,0x2f,0x0a,0x2e,0xee,0x8c,0x6a},
    /* RLC w/SNOW f8 encryption UPLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x1f,0x2b,0xf3,0x1c,0x55,0x6c,0x30,0xf1,0x5a,0x2a,0xe5,0x0b,0xfa,0x7f,0x64},
    /* RLC w/SNOW f8 encryption DOWNLINK UNACKNOWLEDGED MODE */
    (uint8_t[]){0x71,0x7c,0xb6,0xf7,0x26,0xc1,0xd8,0x19,0xa1,0x3c,0x8b,0xc1,0x90,0x17,0xa4},
    /* RLC w/SNOW f8 encryption UPLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0x81,0x57,0x17,0xcf,0x42,0x9d,0x2e,0x4c,0xab,0x5b,0x6f,0x66,0x9f,0xe2,0x5f},
    /* RLC w/SNOW f8 encryption DOWNLINK ACKNOWLEDGED MODE */
    (uint8_t[]){0xe2,0xf0,0xe0,0x0e,0x53,0x25,0xac,0xcd,0x0b,0xc2,0xf0,0x10,0x80,0x3f,0x9d},
    /* RLC w/NULL encryption DOWNLINK UNACKNOWLEDGED MODE*/
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
    /* RLC w/NULL encryption UPLINK ACKNOWLEDGED MODE*/
    (uint8_t[]){0xad,0x9c,0x44,0x1f,0x89,0x0b,0x38,0xc4,0x57,0xa4,0x9d,0x42,0x14,0x07,0xe8},
};

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/*============================================================================*/


#endif  /* TEST_SEC_DRIVER_WCDMA_TEST_VECTORS */
