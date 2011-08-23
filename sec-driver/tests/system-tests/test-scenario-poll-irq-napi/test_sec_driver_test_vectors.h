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

#ifndef TEST_SEC_DRIVER_TEST_VECTORS
#define TEST_SEC_DRIVER_TEST_VECTORS

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include "test_sec_driver.h"

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/
// By default this test validates conformity of output packets against
// reference test vectors. However, for PDCP control-plane such validation
// is not possible because we do not have test vectors for it.
#define VALIDATE_CONFORMITY

// Must be defined only when testing PDCP control-plane encapsulation-configurations
// that will require packets beeing double-sent through SEC engine.
//
// @note Placed here only as a reference. Will be defined by every PDCP
//       c-plane specific configuration.
//#define TEST_PDCP_CONTROL_PLANE_DOUBLE_PASS_ENC

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
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

#ifndef PDCP_TEST_SCENARIO
#error "PDCP_TEST_SCENARIO is undefined!!!"
#endif

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F8_ENC
//////////////////////////////////////////////////////////////////////////////
#if PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F8_ENC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_SNOW_F8_ENC"

// Extracted from:
// Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2
// Document 3: Implementors. Test Data
// Version: 1.0
// Date: 10th January 2006
// Section 4. CONFIDENTIALITY ALGORITHM UEA2, Test Set 3

// Length of PDCP header
#define PDCP_HEADER_LENGTH 2

static uint8_t snow_f8_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// PDCP header
static uint8_t snow_f8_enc_pdcp_hdr[] = {0x8B, 0x26};

// PDCP payload not encrypted
static uint8_t snow_f8_enc_data_in[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                        0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

// PDCP payload encrypted
static uint8_t snow_f8_enc_data_out[] = {0xBA,0x0F,0x31,0x30,0x03,0x34,0xC5,0x6B, // PDCP payload encrypted
                                         0x52,0xA7,0x49,0x7C,0xBA,0xC0,0x46};

// Radio bearer id
static uint8_t snow_f8_enc_bearer = 0x3;

// Start HFN
static uint32_t snow_f8_enc_hfn = 0xFA556;

// HFN threshold
static uint32_t snow_f8_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F8_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F8_DEC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_SNOW_F8_DEC"

// Extracted from:
// Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2
// Document 3: Implementors. Test Data
// Version: 1.0
// Date: 10th January 2006
// Section 4. CONFIDENTIALITY ALGORITHM UEA2, Test Set 3

// Length of PDCP header
#define PDCP_HEADER_LENGTH 2


static uint8_t snow_f8_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// PDCP header
static uint8_t snow_f8_dec_pdcp_hdr[] = {0x8B, 0x26};

// PDCP payload not encrypted
static uint8_t snow_f8_dec_data_out[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                         0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

// PDCP payload encrypted
static uint8_t snow_f8_dec_data_in[] = { 0xBA,0x0F,0x31,0x30,0x03,0x34,0xC5,0x6B, // PDCP payload encrypted
                                         0x52,0xA7,0x49,0x7C,0xBA,0xC0,0x46};
// Radio bearer id
static uint8_t snow_f8_dec_bearer = 0x3;

// Start HFN
static uint32_t snow_f8_dec_hfn = 0xFA556;

// HFN threshold
static uint32_t snow_f8_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CTR_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CTR_ENC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_AES_CTR_ENC"

// Extracted from:
// document 3GPP TS 33.401 V9.6.0 (2010-12)
// Annex C, 128-EEA2, Test Set 1

// Length of PDCP header
#define PDCP_HEADER_LENGTH 2


static uint8_t aes_ctr_enc_key[] = {0xd3, 0xc5, 0xd5, 0x92, 0x32, 0x7f, 0xb1, 0x1c,
                                    0x40, 0x35, 0xc6, 0x68, 0x0a, 0xf8, 0xc6, 0xd1};

// PDCP header
static uint8_t aes_ctr_enc_pdcp_hdr[] = {0x89, 0xB4};

// PDCP payload not encrypted
static uint8_t aes_ctr_enc_data_in[] = {0x98, 0x1b, 0xa6, 0x82, 0x4c, 0x1b, 0xfb, 0x1a,
                                        0xb4, 0x85, 0x47, 0x20, 0x29, 0xb7, 0x1d, 0x80,
                                        0x8c, 0xe3, 0x3e, 0x2c, 0xc3, 0xc0, 0xb5, 0xfc,
                                        0x1f, 0x3d, 0xe8, 0xa6, 0xdc, 0x66, 0xb1, 0xf0};


// PDCP payload encrypted
static uint8_t aes_ctr_enc_data_out[] = {0xe9, 0xfe, 0xd8, 0xa6, 0x3d, 0x15, 0x53, 0x04,
                                         0xd7, 0x1d, 0xf2, 0x0b, 0xf3, 0xe8, 0x22, 0x14,
                                         0xb2, 0x0e, 0xd7, 0xda, 0xd2, 0xf2, 0x33, 0xdc,
                                         0x3c, 0x22, 0xd7, 0xbd, 0xee, 0xed, 0x8e, 0x78};
// Radio bearer id
static uint8_t aes_ctr_enc_bearer = 0x15;
// Start HFN
static uint32_t aes_ctr_enc_hfn = 0x398A5;

// HFN threshold
static uint32_t aes_ctr_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CTR_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CTR_DEC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_AES_CTR_DEC"

// Extracted from:
// document 3GPP TS 33.401 V9.6.0 (2010-12)
// Annex C, 128-EEA2, Test Set 1

// Length of PDCP header
#define PDCP_HEADER_LENGTH 2


static uint8_t aes_ctr_dec_key[] = {0xd3, 0xc5, 0xd5, 0x92, 0x32, 0x7f, 0xb1, 0x1c,
                                    0x40, 0x35, 0xc6, 0x68, 0x0a, 0xf8, 0xc6, 0xd1};

// PDCP header
static uint8_t aes_ctr_dec_pdcp_hdr[] = {0x89, 0xB4};

// PDCP payload encrypted
static uint8_t aes_ctr_dec_data_in[] = {0xe9, 0xfe, 0xd8, 0xa6, 0x3d, 0x15, 0x53, 0x04,
                                        0xd7, 0x1d, 0xf2, 0x0b, 0xf3, 0xe8, 0x22, 0x14,
                                        0xb2, 0x0e, 0xd7, 0xda, 0xd2, 0xf2, 0x33, 0xdc,
                                        0x3c, 0x22, 0xd7, 0xbd, 0xee, 0xed, 0x8e, 0x78};


// PDCP payload not encrypted
static uint8_t aes_ctr_dec_data_out[] = {0x98, 0x1b, 0xa6, 0x82, 0x4c, 0x1b, 0xfb, 0x1a,
                                         0xb4, 0x85, 0x47, 0x20, 0x29, 0xb7, 0x1d, 0x80,
                                         0x8c, 0xe3, 0x3e, 0x2c, 0xc3, 0xc0, 0xb5, 0xfc,
                                         0x1f, 0x3d, 0xe8, 0xa6, 0xdc, 0x66, 0xb1, 0xf0};
// Radio bearer id
static uint8_t aes_ctr_dec_bearer = 0x15;
// Start HFN
static uint32_t aes_ctr_dec_hfn = 0x398A5;

// HFN threshold
static uint32_t aes_ctr_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F9_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_ENC
#warning "F9 only !!!  Be sure to compile with #define PDCP_TEST_SNOW_F9_ONLY activated from sec_utils.h !"

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_SNOW_F9_ENC"

// Extracted from:
// Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2
// Document 3: Implementors. Test Data
// Version: 1.0
// Date: 10th January 2006
// Section 5. INTEGRITY ALGORITHM UIA2, Test Set 4

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

static uint8_t snow_f9_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

static uint8_t snow_f9_auth_enc_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                         0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t snow_f9_enc_pdcp_hdr[] = {0xD0};

// PDCP payload not encrypted
static uint8_t snow_f9_enc_data_in[] = {0xA7 ,0xD4,0x63,0xDF,0x9F,0xB2,0xB2,
                                        0x78,0x83,0x3F,0xA0,0x2E,0x23,0x5A,0xA1,
                                        0x72,0xBD,0x97,0x0C,0x14,0x73,0xE1,0x29,
                                        0x07,0xFB,0x64,0x8B,0x65,0x99,0xAA,0xA0,
                                        0xB2,0x4A,0x03,0x86,0x65,0x42,0x2B,0x20,
                                        0xA4,0x99,0x27,0x6A,0x50,0x42,0x70,0x09};

// PDCP payload encrypted
static uint8_t snow_f9_enc_data_out[] = {0x38,0xB5,0x54,0xC0};

// Radio bearer id
static uint8_t snow_f9_enc_bearer = 0x0;

// Start HFN
static uint32_t snow_f9_enc_hfn = 0xA3C9F2;

// HFN threshold
static uint32_t snow_f9_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F9_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_DEC
#warning "F9 only !!!  Be sure to compile with #define PDCP_TEST_SNOW_F9_ONLY activated from sec_utils.h !"

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_SNOW_F9_DEC"

// Extracted from:
// Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2
// Document 3: Implementors. Test Data
// Version: 1.0
// Date: 10th January 2006
// Section 5. INTEGRITY ALGORITHM UIA2, Test Set 4

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

static uint8_t snow_f9_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

static uint8_t snow_f9_auth_dec_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                         0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t snow_f9_dec_pdcp_hdr[] = { 0xD0};

// PDCP payload not encrypted
static uint8_t snow_f9_dec_data_in[] = {0xA7 ,0xD4,0x63,0xDF,0x9F,0xB2,0xB2,
                                        0x78,0x83,0x3F,0xA0,0x2E,0x23,0x5A,0xA1,
                                        0x72,0xBD,0x97,0x0C,0x14,0x73,0xE1,0x29,
                                        0x07,0xFB,0x64,0x8B,0x65,0x99,0xAA,0xA0,
                                        0xB2,0x4A,0x03,0x86,0x65,0x42,0x2B,0x20,
                                        0xA4,0x99,0x27,0x6A,0x50,0x42,0x70,0x09,
                                        // The MAC-I from packet
                                        0x38,0xB5,0x54,0xC0};

// PDCP payload encrypted
static uint8_t snow_f9_dec_data_out[] = { 0x38,0xB5,0x54,0xC0};

// Radio bearer id
static uint8_t snow_f9_dec_bearer = 0x0;

// Start HFN
static uint32_t snow_f9_dec_hfn = 0xA3C9F2;

// HFN threshold
static uint32_t snow_f9_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CMAC_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_ENC
#warning "AES CMAC only !!! Be sure to compile with #define PDCP_TEST_AES_CMAC_ONLY activated from sec_utils.h !"

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_AES_CMAC_ENC"

// Test Set 2


// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

static uint8_t aes_cmac_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

static uint8_t aes_cmac_auth_enc_key[] = {0xd3,0xc5,0xd5,0x92,0x32,0x7f,0xb1,0x1c,
                                          0x40,0x35,0xc6,0x68,0x0a,0xf8,0xc6,0xd1};

// PDCP header
static uint8_t aes_cmac_enc_pdcp_hdr[] = { 0x48};

// PDCP payload
static uint8_t aes_cmac_enc_data_in[] = {0x45,0x83,0xd5,0xaf,0xe0,0x82,0xae};

// PDCP payload encrypted
static uint8_t aes_cmac_enc_data_out[] = {0xb9,0x37,0x87,0xe6};

// Radio bearer id
static uint8_t aes_cmac_enc_bearer = 0x1a;

// Start HFN
static uint32_t aes_cmac_enc_hfn = 0x1CC52CD;

// HFN threshold
static uint32_t aes_cmac_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CMAC_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_DEC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_AES_CMAC_DEC"
#warning "AES CMAC only !!! Be sure to compile with #define PDCP_TEST_AES_CMAC_ONLY activated from sec_utils.h !"

// Test Set 2


// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

static uint8_t aes_cmac_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                    0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};

static uint8_t aes_cmac_auth_dec_key[] = {0xd3,0xc5,0xd5,0x92,0x32,0x7f,0xb1,0x1c,
                                          0x40,0x35,0xc6,0x68,0x0a,0xf8,0xc6,0xd1};
// PDCP header
static uint8_t aes_cmac_dec_pdcp_hdr[] = { 0x48};

// PDCP payload not encrypted
static uint8_t aes_cmac_dec_data_in[] = {0x45,0x83,0xd5,0xaf,0xe0,0x82,0xae,
                                         // The MAC-I from packet
                                         0xb9,0x37,0x87,0xe6};

// PDCP payload encrypted
static uint8_t aes_cmac_dec_data_out[] = {0xb9,0x37,0x87,0xe6};

// Radio bearer id
static uint8_t aes_cmac_dec_bearer = 0x1a;

// Start HFN
static uint32_t aes_cmac_dec_hfn = 0x1CC52CD;

// HFN threshold
static uint32_t aes_cmac_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_DATA_PLANE_NULL_ALGO
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_DATA_PLANE_NULL_ALGO

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_DATA_PLANE_NULL_ALGO"


// Length of PDCP header
#define PDCP_HEADER_LENGTH 2
//#define PDCP_HEADER_LENGTH 1
//#define PDCP_HEADER_LENGTH 0

// PDCP header
//static uint8_t null_crypto_pdcp_hdr[] = { 0x48};
//static uint8_t null_crypto_pdcp_hdr[] = {0x8B};
static uint8_t null_crypto_pdcp_hdr[] = {0x89, 0xB4};


// PDCP payload not encrypted
//static uint8_t null_crypto_data_in[] = {0x45,0x83,0xd5,0xaf,0xe0,0x82,0xae,
//                                         0xb9,0x37,0x87,0xe6, 0xb9,0x37,0x87,0xe6,0xb9,0x37,0x87,0xe6,0xb9,0x37,0x87,0xe6, 0xff};
//static uint8_t null_crypto_data_in[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
//                                         0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};
#if 0
static uint8_t null_crypto_data_out[] = { 0x00,0xCC,0x73,0xDE,0xC3,0x8E,0xDE,0xBF,0xEF,0xE3,0x0D,0x32,0x1B,0xC1,0x24,0x21,
        0x3D,0x2F,0x09,0x82,0x82,0x8B,0x56,0x38,0x31,0x9A,0x68,0x85,0xC7,0x5A,0xB1,0xB6,
        0xA3,0xC2,0x5A,0x5E,0xF7,0xB7,0x5E,0x28,0x89,0xAD,0x5B,0xDB,0xE8,0x01,0x99,0x5A,
        0x97,0xEE,0xE1,0x71,0xE7,0x92,0x21,0xF4,0x00,0x2F,0x85,0x18,0x7B,0xE9,0x44,0xDC,
        0x26,0xE9,0xA0,0xC4,0x51,0x50,0xAA,0xAD,0x5B,0x75,0x1E,0xBA,0x56,0xA4,0x44,0x47,
        0xE4,0xDE,0x2D,0x92,0xFB,0x1A,0xD9,0xF8,0x50,0xBC,0x1A,0x7F,0x3E,0x45,0x88,0x86,
        0x31,0x43,0xB6,0xEC,0x96,0x17,0x9A,0x64,0x49,0xA7,0x2C,0xF3,0x3D,0xDD,0xA0,0x2E,
        0xFA,0xD2,0x8F,0x43,0x73,0x7D,0xEF,0xE6,0x4F,0x6F,0x55,0xC0,0xA6,0x49,0x3C,0x6E,
        0xBC,0x65,0xDD,0xAE,0x92,0xC0,0xD9,0xD1,0x70,0x54,0x9C,0xC6,0x5A,0xD4,0xF8,0x3B,
        0x96,0xC1,0x00,0x33,0xA8,0x0D,0xB4,0xD0,0x1A,0x9A,0xAD,0x0F,0x56,0x27,0x97,0xC9,
        0x16,0xBC,0x3A,0xA6,0xDF,0x2A,0xF6,0x0C,0x5E,0x2E,0x8D,0x8B,0xA5,0xF6,0xDC,0x59,
        0x13,0xF0,0xB2,0x9E,0x90,0xC4,0xDD,0xE8,0xDA,0x51,0x23,0x4A,0x71,0xAA,0x5D,0x75,
        0xC2,0x79,0xC6,0x8B,0x85,0xFD,0xD6,0xD7,0x84,0xCD,0xFB,0x42,0xBD,0xAB,0xD0,0xAC,
        0xDB,0xD2,0x40,0x4B,0x5C,0x26,0xAA,0x80,0x78,0xDA,0x0D,0x96,0x85,0x7B,0x64,0x36,
        0x23,0xDE,0x85,0x48,0xAD,0xC6,0x60,0xAD,0x09,0xB3,0xE9,0x99,0x02,0x08,0xD7,0x96,
        0xD6,0xED,0x26,0x45,0x3E,0x94,0xAD,0xE0,0xFF,0x9C,0x89,0x7E,0xC6,0x12,0xFF,0x30,
        0x62,0x4D,0x69,0x84,0x74,0x48,0xDC,0x76,0x70,0x47,0xCB,0x9C,0xDF,0x50,0xF1,0xE1,
        0xF3,0x47,0x70,0xE3,0xB6,0xC8,0x4B,0x73,0x64,0xC4,0xBB,0xDE,0xA0,0x1E,0xD3,0xBA,
        0x96,0x52,0x51,0xC1,0xE9,0x80,0xA7,0x6A,0xB1,0x62,0xBC,0x2A,0xDB,0x1F,0x9F,0x3B,
        0xE1,0x13,0x7B,0x3C,0x78,0xFD,0xBE,0x15,0x3A,0x59,0xE4,0x09,0x41,0x61,0x4A,0xA7,
        0x9E,0x83,0x46,0xE0,0x68,0x6E,0x6A,0xE6,0x4C,0xE8,0x2A,0x44,0x35,0x36,0x23,0x54,
        0xB7,0x66,0x09,0xE1,0xA2,0x2A,0xAC,0x1E,0x9A,0xAF,0x02,0x25,0x1E,0x1E,0xF7,0x54,
        0xDC,0xEF,0x1D,0x65,0xCF,0x40,0xA2,0xE4,0xD9,0x18,0x18,0x4D,0x7D,0x73,0x87,0xED,
        0x9D,0xF0,0x01,0x04,0xF1,0x3B,0x7A,0xD0,0x1A,0x2A,0x7C,0xBF,0xEA,0xBE,0x4B,0x2B,
        0x62,0xD9,0x59,0xA9,0x7F,0x63,0xBA,0x0E,0xFA,0xB1,0xBB
};
#endif
static uint8_t null_crypto_data_in[] = {0x98, 0x1b, 0xa6, 0x82, 0x4c, 0x1b, 0xfb, 0x1a,
        0xb4, 0x85, 0x47, 0x20, 0x29, 0xb7, 0x1d, 0x80,
        0x8c, 0xe3, 0x3e, 0x2c, 0xc3, 0xc0, 0xb5, 0xfc,
        0x1f, 0x3d, 0xe8, 0xa6, 0xdc, 0x66, 0xb1, 0xf0};

// PDCP payload encrypted
//static uint8_t null_crypto_data_out[] = {0x45,0x83,0xd5,0xaf,0xe0,0x82,0xae,
//                                          0xb9,0x37,0x87,0xe6, 0xb9,0x37,0x87,0xe6,0xb9,0x37,0x87,0xe6,0xb9,0x37,0x87,0xe6, 0xff};
#if 0
static uint8_t null_crypto_data_in[] = {0x00,0xCD,0xF8,0x16,0xBF,0x49,0x1F,0xEE,0xFB,0x0C,0x31,0x24,0xBF,0x52,0x3A,0x25,
        0x20,0x0F,0x8C,0xF9,0xC9,0x6C,0x96,0xEC,0x0A,0x2D,0xFC,0xC3,0xFE,0x28,0x8D,0x90,
        0xC6,0xDD,0x13,0x6A,0x2C,0x25,0xC2,0x31,0x02,0xEE,0x66,0x8C,0x8F,0x47,0xF0,0xE8,
        0xC8,0x0E,0x1D,0xA2,0xE2,0x53,0xFF,0xCF,0x80,0xE2,0xB8,0x56,0xC9,0xBC,0x8F,0x1D,
        0x52,0x7F,0xEC,0x6A,0x1A,0xC6,0xFF,0x01,0x07,0xB7,0x27,0xC8,0x7F,0xDF,0x31,0xE5,
        0xFB,0xA2,0x30,0xCB,0x6C,0x77,0x35,0x34,0xEC,0xAE,0x32,0xFF,0x8D,0xB1,0x0E,0x92,
        0xBD,0x60,0x7F,0x81,0x45,0x76,0xAA,0xC1,0x72,0x8D,0x1E,0x56,0x1B,0xA8,0x3A,0x29,
        0x11,0x27,0xAC,0x7B,0x94,0x87,0xB3,0x55,0x01,0xB9,0xCA,0x95,0xE4,0x3D,0xA8,0xAD,
        0x5F,0xC2,0x02,0x33,0x11,0x8D,0xBC,0x41,0x36,0x06,0x67,0xA1,0x6F,0x90,0x08,0x7F,
        0x25,0x3E,0x34,0x26,0xC2,0x33,0x20,0xC7,0xEE,0xA2,0xB5,0x0D,0xD6,0x09,0x4C,0xB1,
        0x27,0xE1,0x63,0x51,0xE7,0x58,0xEF,0xEB,0x7D,0xB0,0x7F,0x09,0x84,0xB9,0x50,0x33,
        0x67,0x40,0x4E,0x8F,0x3D,0x4B,0xEA,0x82,0x70,0x89,0xE0,0xCE,0xD6,0x5D,0xE3,0xD6,
        0x8F,0x8F,0x5C,0xF6,0xDE,0x6F,0x12,0x23,0xBE,0xCD,0x31,0x06,0x87,0x57,0x2B,0xC0,
        0x07,0x33,0xE3,0x32,0x7F,0x0E,0xFD,0x07,0x89,0x8D,0xC5,0x17,0xA5,0xF0,0xF4,0x43,
        0xDD,0x90,0x03,0x26,0x97,0x14,0xB9,0xDA,0x40,0xF6,0xDD,0xAF,0x1B,0x71,0xA4,0xBA,
        0xB3,0xF3,0xD3,0x9F,0x27,0x74,0xE1,0x18,0x29,0x28,0x71,0x86,0xED,0x1C,0x80,0x9C,
        0x1D,0xE3,0x7B,0x4C,0x4C,0x2D,0x49,0xE4,0x9A,0x97,0x47,0x98,0xFB,0x87,0xC3,0xE0,
        0x81,0xFF,0xBD,0x9D,0x6B,0x98,0x77,0x49,0xC1,0x6D,0x20,0xC8,0x7C,0xAD,0xAD,0xC0,
        0xAF,0x9D,0x87,0xED,0xC8,0x34,0x99,0xBC,0x4B,0x44,0x7D,0x35,0x20,0x19,0x2B,0x7D,
        0xB1,0x8D,0x25,0x5C,0x92,0x10,0x48,0x90,0xB1,0xCA,0x55,0x4B,0x49,0x1F,0x02,0x55,
        0xAA,0xF4,0xD5,0xC9,0xAD,0x41,0xA7,0x5E,0x8D,0x0B,0x60,0x30,0x03,0x23,0x1B,0xE7,
        0x26,0x1C,0xF9,0x12,0x04,0x75,0xEE,0x8E,0xEC,0x07,0x78,0x4B,0xE5,0x32,0xD3,0xE2,
        0x10,0x2C,0x81,0xC2,0xB4,0xB6,0x91,0xE2,0x51,0x6C,0x8B,0xE9,0x4A,0x7C,0x65,0x81,
        0x5E,0xEA,0xD5,0x6F,0xF2,0xB9,0x80,0x19,0xE7,0x8E,0x2B,0x7D,0x0C,0xF9,0x80,0x26,
        0x53,0xC8,0x00,0x64,0xF0,0x5B,0x62};

static uint8_t null_crypto_data_out[] = {0x20,0xd9,0x97,0x63,0x60,0x68,0x2d,0x55,0x0e,0x8d,
                                           0x50,0x0a,0xc7,0xfc,0x5e,
                                           // The MAC-I from packet
                                           0x17,0xe3,0xe5,0xd7};
#endif
static uint8_t null_crypto_data_out[] = {0xe9, 0xfe, 0xd8, 0xa6, 0x3d, 0x15, 0x53, 0x04,
        0xd7, 0x1d, 0xf2, 0x0b, 0xf3, 0xe8, 0x22, 0x14,
        0xb2, 0x0e, 0xd7, 0xda, 0xd2, 0xf2, 0x33, 0xdc,
        0x3c, 0x22, 0xd7, 0xbd, 0xee, 0xed, 0x8e, 0x78};
// Radio bearer id
static uint8_t null_crypto_bearer = 0x1a;

// Start HFN
static uint32_t null_crypto_hfn = 0x1CC52CD;

// HFN threshold
static uint32_t null_crypto_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_NULL_ALGO
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_NULL_ALGO

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_NULL_ALGO"


// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// PDCP header
static uint8_t null_crypto_pdcp_hdr[] = { 0x48};

// PDCP payload not encrypted
static uint8_t null_crypto_data_in[] = {0x45,0x83,0xd5,0xaf,0xe0,0x82,0xae,
                                         0xb9,0x37,0x87,0xe6, 0xb9,0x37,0x87,0xe6,0xb9,0x37,0x87,0xe6,0xb9,0x37,0x87,0xe6, 0xff};

// PDCP payload encrypted
static uint8_t null_crypto_data_out[] = {0x45,0x83,0xd5,0xaf,0xe0,0x82,0xae,
                                          0xb9,0x37,0x87,0xe6, 0xb9,0x37,0x87,0xe6,0xb9,0x37,0x87,0xe6,0xb9,0x37,0x87,0xe6, 0xff};

// Radio bearer id
static uint8_t null_crypto_bearer = 0x1a;

// Start HFN
static uint32_t null_crypto_hfn = 0x1CC52CD;

// HFN threshold
static uint32_t null_crypto_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_ENC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_ENC"

// WE HAVE NO PDCP CONTROL-PLANE TEST VECTORS AVAILABLE!
// Output data was obtained by running encapsulation test.
// Then the reverse, decapsulation test was done to obtain back the original data.
// This is the method used for validating to some extent the PDCP control-plane algorithms.

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// Crypto key
static uint8_t snow_f8_f9_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                       0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// Authentication key
static uint8_t snow_f8_f9_enc_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                            0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t snow_f8_f9_enc_pdcp_hdr[] = {0x8B};

// PDCP payload not encrypted
static uint8_t snow_f8_f9_enc_data_in[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                         0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

// PDCP payload + MAC-I both encrypted
static uint8_t snow_f8_f9_enc_data_out[] =  {0x20,0xd9,0x97,0x63,0x60,0x68,0x2d,0x55,0x0e,0x8d,
                                           0x50,0x0a,0xc7,0xfc,0x5e,
                                           // The MAC-I from packet
                                           0x17,0xe3,0xe5,0xd7};
// Radio bearer id
static uint8_t snow_f8_f9_enc_bearer = 0x3;

// Start HFN
static uint32_t snow_f8_f9_enc_hfn = 0xFA556;

// HFN threshold
static uint32_t snow_f8_f9_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_DEC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_DEC"

// WE HAVE NO PDCP CONTROL-PLANE TEST VECTORS AVAILABLE!
// Output data was obtained by running encapsulation test.
// Then the reverse, decapsulation test was done to obtain back the original data.
// This is the method used for validating to some extent the PDCP control-plane algorithms.

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// Crypto key
static uint8_t snow_f8_f9_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                       0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// Authentication key
static uint8_t snow_f8_f9_dec_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                            0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t snow_f8_f9_dec_pdcp_hdr[] = {0x8B};


// PDCP payload + MAC-I both encrypted
static uint8_t snow_f8_f9_dec_data_in[] =  {0x20,0xd9,0x97,0x63,0x60,0x68,0x2d,0x55,0x0e,0x8d,
                                           0x50,0x0a,0xc7,0xfc,0x5e,
                                           // The MAC-I from packet
                                           0x17,0xe3,0xe5,0xd7};
                                           
// PDCP payload not encrypted
static uint8_t snow_f8_f9_dec_data_out[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                          0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};
                                          


// Radio bearer id
static uint8_t snow_f8_f9_dec_bearer = 0x3;

// Start HFN
static uint32_t snow_f8_f9_dec_hfn = 0xFA556;

// HFN threshold
static uint32_t snow_f8_f9_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_ENC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_ENC"

// WE HAVE NO PDCP CONTROL-PLANE TEST VECTORS AVAILABLE!
// Output data was obtained by running encapsulation test.
// Then the reverse, decapsulation test was done to obtain back the original data.
// This is the method used for validating to some extent the PDCP control-plane algorithms.

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// Crypto key
static uint8_t aes_ctr_cmac_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                       0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// Authentication key
static uint8_t aes_ctr_cmac_enc_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                            0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t aes_ctr_cmac_enc_pdcp_hdr[] = {0x8B};

// PDCP payload not encrypted
static uint8_t aes_ctr_cmac_enc_data_in[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                         0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

// PDCP payload + MAC-I both encrypted
static uint8_t aes_ctr_cmac_enc_data_out[] = {0xa1,0x05,0xfb,0xfe,0xa4,0x8d,0x74,0x3d,
                                              0x29,0x53,0x27,0x33,0xd9,0xba,0x91,
                                              // The MAC-I from packet
                                              0x89,0x46,0x96,0xd6};

// Radio bearer id
static uint8_t aes_ctr_cmac_enc_bearer = 0x3;

// Start HFN
static uint32_t aes_ctr_cmac_enc_hfn = 0xFA556;

// HFN threshold
static uint32_t aes_ctr_cmac_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_DEC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_DEC"

// WE HAVE NO PDCP CONTROL-PLANE TEST VECTORS AVAILABLE!
// Output data was obtained by running encapsulation test.
// Then the reverse, decapsulation test was done to obtain back the original data.
// This is the method used for validating to some extent the PDCP control-plane algorithms.

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// Crypto key
static uint8_t aes_ctr_cmac_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                       0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// Authentication key
static uint8_t aes_ctr_cmac_dec_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                            0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t aes_ctr_cmac_dec_pdcp_hdr[] = {0x8B};

// PDCP payload + MAC-I both encrypted
static uint8_t aes_ctr_cmac_dec_data_in[] = {0xa1,0x05,0xfb,0xfe,0xa4,0x8d,0x74,0x3d,
                                              0x29,0x53,0x27,0x33,0xd9,0xba,0x91,
                                              // The MAC-I from packet
                                              0x89,0x46,0x96,0xd6};
// PDCP payload not encrypted
static uint8_t aes_ctr_cmac_dec_data_out[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                         0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};
// Radio bearer id
static uint8_t aes_ctr_cmac_dec_bearer = 0x3;

// Start HFN
static uint32_t aes_ctr_cmac_dec_hfn = 0xFA556;

// HFN threshold
static uint32_t aes_ctr_cmac_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_ENC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_ENC"

// WE HAVE NO PDCP CONTROL-PLANE TEST VECTORS AVAILABLE!
// Output data was obtained by running encapsulation test.
// Then the reverse, decapsulation test was done to obtain back the original data.
// This is the method used for validating to some extent the PDCP control-plane algorithms.

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// Crypto key
static uint8_t aes_ctr_snow_f9_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                       0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// Authentication key
static uint8_t aes_ctr_snow_f9_enc_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                            0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t aes_ctr_snow_f9_enc_pdcp_hdr[] = {0x8B};

// PDCP payload not encrypted
static uint8_t aes_ctr_snow_f9_enc_data_in[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                         0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

// PDCP payload + MAC-I both encrypted
static uint8_t aes_ctr_snow_f9_enc_data_out[] = {0xa1,0x05,0xfb,0xfe,0xa4,0x8d,0x74,
                                                 0x3d,0x29,0x53,0x27,0x33,0xd9,0xba,0x91,
                                                 // The MAC-I from packet
                                                 0x2f,0x1b,0x86,0x12};

// Radio bearer id
static uint8_t aes_ctr_snow_f9_enc_bearer = 0x3;

// Start HFN
static uint32_t aes_ctr_snow_f9_enc_hfn = 0xFA556;

// HFN threshold
static uint32_t aes_ctr_snow_f9_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_DEC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_DEC"

// WE HAVE NO PDCP CONTROL-PLANE TEST VECTORS AVAILABLE!
// Output data was obtained by running encapsulation test.
// Then the reverse, decapsulation test was done to obtain back the original data.
// This is the method used for validating to some extent the PDCP control-plane algorithms.

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// Crypto key
static uint8_t aes_ctr_snow_f9_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                       0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// Authentication key
static uint8_t aes_ctr_snow_f9_dec_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                            0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t aes_ctr_snow_f9_dec_pdcp_hdr[] = {0x8B};

// PDCP payload + MAC-I both encrypted
static uint8_t aes_ctr_snow_f9_dec_data_in[] = {0xa1,0x05,0xfb,0xfe,0xa4,0x8d,0x74,
                                                 0x3d,0x29,0x53,0x27,0x33,0xd9,0xba,0x91,
                                                 // The MAC-I from packet
                                                 0x2f,0x1b,0x86,0x12};
// PDCP payload not encrypted
static uint8_t aes_ctr_snow_f9_dec_data_out[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                                 0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

// Radio bearer id
static uint8_t aes_ctr_snow_f9_dec_bearer = 0x3;

// Start HFN
static uint32_t aes_ctr_snow_f9_dec_hfn = 0xFA556;

// HFN threshold
static uint32_t aes_ctr_snow_f9_dec_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_ENC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_ENC"

// WE HAVE NO PDCP CONTROL-PLANE TEST VECTORS AVAILABLE!
// Output data was obtained by running encapsulation test.
// Then the reverse, decapsulation test was done to obtain back the original data.
// This is the method used for validating to some extent the PDCP control-plane algorithms.

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// Crypto key
static uint8_t snow_f8_aes_cmac_enc_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                             0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// Authentication key
static uint8_t snow_f8_aes_cmac_enc_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                                  0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t snow_f8_aes_cmac_enc_pdcp_hdr[] = {0x8B};

// PDCP payload not encrypted
static uint8_t snow_f8_aes_cmac_enc_data_in[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                                 0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};

// PDCP payload + MAC-I both encrypted
static uint8_t snow_f8_aes_cmac_enc_data_out[] =  {0x20,0xd9,0x97,0x63,0x60,0x68,0x2d,0x55,0x0e,0x8d,
                                                   0x50,0x0a,0xc7,0xfc,0x5e,
                                                   // The MAC-I from packet
                                                   0xb1,0xbe,0xf5,0x13};
// Radio bearer id
static uint8_t snow_f8_aes_cmac_enc_bearer = 0x3;

// Start HFN
static uint32_t snow_f8_aes_cmac_enc_hfn = 0xFA556;

// HFN threshold
static uint32_t snow_f8_aes_cmac_enc_hfn_threshold = 0xFF00000;

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_DEC

#define PDCP_TEST_SCENARIO_NAME "PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_DEC"

// WE HAVE NO PDCP CONTROL-PLANE TEST VECTORS AVAILABLE!
// Output data was obtained by running encapsulation test.
// Then the reverse, decapsulation test was done to obtain back the original data.
// This is the method used for validating to some extent the PDCP control-plane algorithms.

// Length of PDCP header
#define PDCP_HEADER_LENGTH 1

// Crypto key
static uint8_t snow_f8_aes_cmac_dec_key[] = {0x5A,0xCB,0x1D,0x64,0x4C,0x0D,0x51,0x20,
                                             0x4E,0xA5,0xF1,0x45,0x10,0x10,0xD8,0x52};
// Authentication key
static uint8_t snow_f8_aes_cmac_dec_auth_key[] = {0xC7,0x36,0xC6,0xAA,0xB2,0x2B,0xFF,0xF9,
                                                  0x1E,0x26,0x98,0xD2,0xE2,0x2A,0xD5,0x7E};
// PDCP header
static uint8_t snow_f8_aes_cmac_dec_pdcp_hdr[] = {0x8B};

// PDCP payload + MAC-I both encrypted
static uint8_t snow_f8_aes_cmac_dec_data_in[] =  {0x20,0xd9,0x97,0x63,0x60,0x68,0x2d,0x55,0x0e,0x8d,
                                                  0x50,0x0a,0xc7,0xfc,0x5e,
                                                  // The MAC-I from packet
                                                  0xb1,0xbe,0xf5,0x13};

// PDCP payload not encrypted
static uint8_t snow_f8_aes_cmac_dec_data_out[] = {0xAD,0x9C,0x44,0x1F,0x89,0x0B,0x38,0xC4,
                                                  0x57,0xA4,0x9D,0x42,0x14,0x07,0xE8};
// Radio bearer id
static uint8_t snow_f8_aes_cmac_dec_bearer = 0x3;

// Start HFN
static uint32_t snow_f8_aes_cmac_dec_hfn = 0xFA556;

// HFN threshold
static uint32_t snow_f8_aes_cmac_dec_hfn_threshold = 0xFF00000;

#else
#error "Unsuported test scenario!"
#endif


//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F8_ENC
//////////////////////////////////////////////////////////////////////////////
#if PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F8_ENC


#define test_crypto_key         snow_f8_enc_key
#define test_crypto_key_len     sizeof(snow_f8_enc_key)

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            snow_f8_enc_data_in
#define test_data_out           snow_f8_enc_data_out

#define test_pdcp_hdr           snow_f8_enc_pdcp_hdr
#define test_bearer             snow_f8_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_12
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                snow_f8_enc_hfn
#define test_hfn_threshold      snow_f8_enc_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F8_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F8_DEC

#define test_crypto_key         snow_f8_dec_key
#define test_crypto_key_len     sizeof(snow_f8_dec_key)

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            snow_f8_dec_data_in
#define test_data_out           snow_f8_dec_data_out

#define test_pdcp_hdr           snow_f8_dec_pdcp_hdr
#define test_bearer             snow_f8_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_12
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm    SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                snow_f8_dec_hfn
#define test_hfn_threshold      snow_f8_dec_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CTR_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CTR_ENC

#define test_crypto_key         aes_ctr_enc_key
#define test_crypto_key_len     sizeof(aes_ctr_enc_key)

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            aes_ctr_enc_data_in
#define test_data_out           aes_ctr_enc_data_out

#define test_pdcp_hdr           aes_ctr_enc_pdcp_hdr
#define test_bearer             aes_ctr_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_12
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm    SEC_ALG_AES
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                aes_ctr_enc_hfn
#define test_hfn_threshold      aes_ctr_enc_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CTR_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CTR_DEC

#define test_crypto_key         aes_ctr_dec_key
#define test_crypto_key_len     sizeof(aes_ctr_dec_key)

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            aes_ctr_dec_data_in
#define test_data_out           aes_ctr_dec_data_out

#define test_pdcp_hdr           aes_ctr_dec_pdcp_hdr
#define test_bearer             aes_ctr_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_12
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm    SEC_ALG_AES
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                aes_ctr_dec_hfn
#define test_hfn_threshold      aes_ctr_dec_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F9_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_ENC

#define test_crypto_key         snow_f9_enc_key
#define test_crypto_key_len     sizeof(snow_f9_enc_key)

#define test_auth_key           snow_f9_auth_enc_key
#define test_auth_key_len       sizeof(snow_f9_auth_enc_key)

#define test_data_in            snow_f9_enc_data_in
#define test_data_out           snow_f9_enc_data_out

#define test_pdcp_hdr           snow_f9_enc_pdcp_hdr
#define test_bearer             snow_f9_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
//#define test_cipher_algorithm   SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_SNOW
#define test_hfn                snow_f9_enc_hfn
#define test_hfn_threshold      snow_f9_enc_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_SNOW_F9_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_SNOW_F9_DEC

#define test_crypto_key         snow_f9_dec_key
#define test_crypto_key_len     sizeof(snow_f9_dec_key)

#define test_auth_key           snow_f9_auth_dec_key
#define test_auth_key_len       sizeof(snow_f9_auth_dec_key)

#define test_data_in            snow_f9_dec_data_in
#define test_data_out           snow_f9_dec_data_out

#define test_pdcp_hdr           snow_f9_dec_pdcp_hdr
#define test_bearer             snow_f9_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
#define test_integrity_algorithm SEC_ALG_SNOW
#define test_hfn                snow_f9_dec_hfn
#define test_hfn_threshold      snow_f9_dec_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CMAC_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_ENC

#define test_crypto_key         aes_cmac_enc_key
#define test_crypto_key_len     sizeof(aes_cmac_enc_key)

#define test_auth_key           aes_cmac_auth_enc_key
#define test_auth_key_len       sizeof(aes_cmac_auth_enc_key)

#define test_data_in            aes_cmac_enc_data_in
#define test_data_out           aes_cmac_enc_data_out

#define test_pdcp_hdr           aes_cmac_enc_pdcp_hdr
#define test_bearer             aes_cmac_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
#define test_integrity_algorithm SEC_ALG_AES
#define test_hfn                aes_cmac_enc_hfn
#define test_hfn_threshold      aes_cmac_enc_hfn_threshold
#define test_packet_offset      PACKET_OFFSET + AES_CMAC_SCTRATCHPAD_PACKET_AREA_LENGHT

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_AES_CMAC_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_AES_CMAC_DEC

#define test_crypto_key         aes_cmac_dec_key
#define test_crypto_key_len     sizeof(aes_cmac_dec_key)

#define test_auth_key           aes_cmac_auth_dec_key
#define test_auth_key_len       sizeof(aes_cmac_auth_dec_key)

#define test_data_in            aes_cmac_dec_data_in
#define test_data_out           aes_cmac_dec_data_out

#define test_pdcp_hdr           aes_cmac_dec_pdcp_hdr
#define test_bearer             aes_cmac_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
#define test_integrity_algorithm SEC_ALG_AES
#define test_hfn                aes_cmac_dec_hfn
#define test_hfn_threshold      aes_cmac_dec_hfn_threshold
#define test_packet_offset      PACKET_OFFSET + AES_CMAC_SCTRATCHPAD_PACKET_AREA_LENGHT

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_DATA_PLANE_NULL_ALGO
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_DATA_PLANE_NULL_ALGO

#define test_crypto_key         NULL
#define test_crypto_key_len     0

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            null_crypto_data_in
#define test_data_out           null_crypto_data_out

#define test_pdcp_hdr           null_crypto_pdcp_hdr
#define test_bearer             null_crypto_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_7
#define test_user_plane         PDCP_DATA_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                null_crypto_hfn
#define test_hfn_threshold      null_crypto_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_NULL_ALGO
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_NULL_ALGO

#define test_crypto_key         NULL
#define test_crypto_key_len     0

#define test_auth_key           NULL
#define test_auth_key_len       0

#define test_data_in            null_crypto_data_in
#define test_data_out           null_crypto_data_out

#define test_pdcp_hdr           null_crypto_pdcp_hdr
#define test_bearer             null_crypto_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_NULL
#define test_integrity_algorithm SEC_ALG_NULL
#define test_hfn                null_crypto_hfn
#define test_hfn_threshold      null_crypto_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_ENC

#define test_crypto_key         snow_f8_f9_enc_key
#define test_crypto_key_len     sizeof(snow_f8_f9_enc_key)

#define test_auth_key           snow_f8_f9_enc_auth_key
#define test_auth_key_len       sizeof(snow_f8_f9_enc_auth_key)

#define test_data_in            snow_f8_f9_enc_data_in
#define test_data_out           snow_f8_f9_enc_data_out

#define test_pdcp_hdr           snow_f8_f9_enc_pdcp_hdr
#define test_bearer             snow_f8_f9_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_SNOW
#define test_hfn                snow_f8_f9_enc_hfn
#define test_hfn_threshold      snow_f8_f9_enc_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

// Do not validate conformity against reference test vector
// as we do not have one for PDCP control-plane
//#ifdef VALIDATE_CONFORMITY
//#undef VALIDATE_CONFORMITY
//#endif

// A double pass of packets through SEC engine is requried for PDCP c-plane
// having both ciphering and authentication algorithms set.
#define TEST_PDCP_CONTROL_PLANE_DOUBLE_PASS_ENC

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_SNOW_F8_SNOW_F9_DEC

#define test_crypto_key         snow_f8_f9_dec_key
#define test_crypto_key_len     sizeof(snow_f8_f9_dec_key)

#define test_auth_key           snow_f8_f9_dec_auth_key
#define test_auth_key_len       sizeof(snow_f8_f9_dec_auth_key)

#define test_data_in            snow_f8_f9_dec_data_in
#define test_data_out           snow_f8_f9_dec_data_out

#define test_pdcp_hdr           snow_f8_f9_dec_pdcp_hdr
#define test_bearer             snow_f8_f9_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_SNOW
#define test_hfn                snow_f8_f9_dec_hfn
#define test_hfn_threshold      snow_f8_f9_dec_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_ENC

#define test_crypto_key         aes_ctr_cmac_enc_key
#define test_crypto_key_len     sizeof(aes_ctr_cmac_enc_key)

#define test_auth_key           aes_ctr_cmac_enc_auth_key
#define test_auth_key_len       sizeof(aes_ctr_cmac_enc_auth_key)

#define test_data_in            aes_ctr_cmac_enc_data_in
#define test_data_out           aes_ctr_cmac_enc_data_out

#define test_pdcp_hdr           aes_ctr_cmac_enc_pdcp_hdr
#define test_bearer             aes_ctr_cmac_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_AES
#define test_integrity_algorithm SEC_ALG_AES
#define test_hfn                aes_ctr_cmac_enc_hfn
#define test_hfn_threshold      aes_ctr_cmac_enc_hfn_threshold
#define test_packet_offset      PACKET_OFFSET + AES_CMAC_SCTRATCHPAD_PACKET_AREA_LENGHT

// A double pass of packets through SEC engine is requried for PDCP c-plane
// having both ciphering and authentication algorithms set.
#define TEST_PDCP_CONTROL_PLANE_DOUBLE_PASS_ENC

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_AES_CTR_AES_CMAC_DEC

#define test_crypto_key         aes_ctr_cmac_dec_key
#define test_crypto_key_len     sizeof(aes_ctr_cmac_dec_key)

#define test_auth_key           aes_ctr_cmac_dec_auth_key
#define test_auth_key_len       sizeof(aes_ctr_cmac_dec_auth_key)

#define test_data_in            aes_ctr_cmac_dec_data_in
#define test_data_out           aes_ctr_cmac_dec_data_out

#define test_pdcp_hdr           aes_ctr_cmac_dec_pdcp_hdr
#define test_bearer             aes_ctr_cmac_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_AES
#define test_integrity_algorithm SEC_ALG_AES
#define test_hfn                aes_ctr_cmac_dec_hfn
#define test_hfn_threshold      aes_ctr_cmac_dec_hfn_threshold
#define test_packet_offset      PACKET_OFFSET + AES_CMAC_SCTRATCHPAD_PACKET_AREA_LENGHT

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_ENC

#define test_crypto_key         aes_ctr_snow_f9_enc_key
#define test_crypto_key_len     sizeof(aes_ctr_snow_f9_enc_key)

#define test_auth_key           aes_ctr_snow_f9_enc_auth_key
#define test_auth_key_len       sizeof(aes_ctr_snow_f9_enc_auth_key)

#define test_data_in            aes_ctr_snow_f9_enc_data_in
#define test_data_out           aes_ctr_snow_f9_enc_data_out

#define test_pdcp_hdr           aes_ctr_snow_f9_enc_pdcp_hdr
#define test_bearer             aes_ctr_snow_f9_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_AES
#define test_integrity_algorithm SEC_ALG_SNOW
#define test_hfn                aes_ctr_snow_f9_enc_hfn
#define test_hfn_threshold      aes_ctr_snow_f9_enc_hfn_threshold
#define test_packet_offset      PACKET_OFFSET

// A double pass of packets through SEC engine is requried for PDCP c-plane
// having both ciphering and authentication algorithms set.
#define TEST_PDCP_CONTROL_PLANE_DOUBLE_PASS_ENC

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_AES_CTR_SNOW_F9_DEC

#define test_crypto_key         aes_ctr_snow_f9_dec_key
#define test_crypto_key_len     sizeof(aes_ctr_snow_f9_dec_key)

#define test_auth_key           aes_ctr_snow_f9_dec_auth_key
#define test_auth_key_len       sizeof(aes_ctr_snow_f9_dec_auth_key)

#define test_data_in            aes_ctr_snow_f9_dec_data_in
#define test_data_out           aes_ctr_snow_f9_dec_data_out

#define test_pdcp_hdr           aes_ctr_snow_f9_dec_pdcp_hdr
#define test_bearer             aes_ctr_snow_f9_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_AES
#define test_integrity_algorithm SEC_ALG_SNOW
#define test_hfn                aes_ctr_snow_f9_dec_hfn
#define test_hfn_threshold      aes_ctr_snow_f9_dec_hfn_threshold
#define test_packet_offset      PACKET_OFFSET



//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_ENC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_ENC

#define test_crypto_key         snow_f8_aes_cmac_enc_key
#define test_crypto_key_len     sizeof(snow_f8_aes_cmac_enc_key)

#define test_auth_key           snow_f8_aes_cmac_enc_auth_key
#define test_auth_key_len       sizeof(snow_f8_aes_cmac_enc_auth_key)

#define test_data_in            snow_f8_aes_cmac_enc_data_in
#define test_data_out           snow_f8_aes_cmac_enc_data_out

#define test_pdcp_hdr           snow_f8_aes_cmac_enc_pdcp_hdr
#define test_bearer             snow_f8_aes_cmac_enc_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_ENCAPSULATION
#define test_cipher_algorithm   SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_AES
#define test_hfn                snow_f8_aes_cmac_enc_hfn
#define test_hfn_threshold      snow_f8_aes_cmac_enc_hfn_threshold
#define test_packet_offset      PACKET_OFFSET + AES_CMAC_SCTRATCHPAD_PACKET_AREA_LENGHT

// A double pass of packets through SEC engine is requried for PDCP c-plane
// having both ciphering and authentication algorithms set.
#define TEST_PDCP_CONTROL_PLANE_DOUBLE_PASS_ENC

//////////////////////////////////////////////////////////////////////////////
// PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_DEC
//////////////////////////////////////////////////////////////////////////////
#elif PDCP_TEST_SCENARIO == PDCP_TEST_CTRL_PLANE_SNOW_F8_AES_CMAC_DEC

#define test_crypto_key         snow_f8_aes_cmac_dec_key
#define test_crypto_key_len     sizeof(snow_f8_aes_cmac_dec_key)

#define test_auth_key           snow_f8_aes_cmac_dec_auth_key
#define test_auth_key_len       sizeof(snow_f8_aes_cmac_dec_auth_key)

#define test_data_in            snow_f8_aes_cmac_dec_data_in
#define test_data_out           snow_f8_aes_cmac_dec_data_out

#define test_pdcp_hdr           snow_f8_aes_cmac_dec_pdcp_hdr
#define test_bearer             snow_f8_aes_cmac_dec_bearer
#define test_sn_size            SEC_PDCP_SN_SIZE_5
#define test_user_plane         PDCP_CONTROL_PLANE
#define test_packet_direction   PDCP_DOWNLINK
#define test_protocol_direction PDCP_DECAPSULATION
#define test_cipher_algorithm   SEC_ALG_SNOW
#define test_integrity_algorithm SEC_ALG_AES
#define test_hfn                snow_f8_aes_cmac_dec_hfn
#define test_hfn_threshold      snow_f8_aes_cmac_dec_hfn_threshold
#define test_packet_offset      PACKET_OFFSET + AES_CMAC_SCTRATCHPAD_PACKET_AREA_LENGHT

// Do not validate conformity against reference test vector
// as we do not have one for PDCP control-plane
//#ifdef VALIDATE_CONFORMITY
//#undef VALIDATE_CONFORMITY
//#endif

#else
#error "Unsuported test scenario!"
#endif

/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/*============================================================================*/


#endif  /* TEST_SEC_DRIVER_TEST_VECTORS */
