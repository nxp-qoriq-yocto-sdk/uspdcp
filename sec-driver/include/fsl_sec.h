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

#ifndef FSL_SEC_H
#define FSL_SEC_H

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
/** Return codes for SEC user space driver APIs */
typedef enum sec_return_code_e
{
	SEC_SUCCESS = 0,             /* Operation executed successfully.*/
	SEC_INVALID_INPUT_PARAM,     /* API received an invalid argument. */
	SEC_OUT_OF_MEMORY,           /* Memory allocation failed. */
	SEC_JR_RESET_FAILED,         /* Job Ring reset failed. */

}sec_return_code_t;

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/**
 * Opaque handle to a Job Ring provided by SEC user space driver
 * to UA when sec_init() is called.
 * */
typedef void* sec_job_ring_t;

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/*================================================================================================*/

/**
 * @brief Initialize the SEC user space driver.
 *
 * This function will handle local data initialization,
 * mapping and initialization of requested SEC's Job Rings.
 * Call once during application startup.
 *
 * @note Global SEC initialization is always done in SEC kernel driver.
 *
 * @note The hardware IDs of the initialized Job Rings are opaque to the UA.
 * The exact Job Rings used by this library are decided between SEC user
 * space driver and SEC kernel driver.
 *
 * @param [in]  job_rings_no       The number of job rings to acquire and initialize.
 * @param [out] job_ring_handles   Array of job ring handles of size job_rings_no. The job
 *                                 ring handles are provided by the library for UA. The
 *                                 handles are opaque from UA point of view.
 *                                 Theses handles can be used by UA to associate a context
 *                                 to a certain JR or poll for events per JR.
 *
 * @retval #SEC_SUCCESS for successful execution
 * @retval #SEC_OUT_OF_MEMORY is returned if internal memory allocation fails
 * @retval #SEC_JR_RESET_FAILED is returned in case the job ring reset fails
 * @retval >0 in case of error
 */
int sec_init(int job_rings_no,
		     sec_job_ring_t **job_ring_handles);

/**
 * @brief Release the resources used by the SEC user space driver.
 *
 * Reset and release SEC's job rings indicated by the User Application at
 * sec_init() and free any memory allocated internally.
 * Call once during application teardown.
 *
 * @note In case there are any packets in-flight in the Job Input/Output Rings,
 * the packets are discarded without any notifications to User Application.
 *
 * @retval #SEC_SUCCESS is returned for a successful execution
 * @retval #SEC_JR_RESET_FAILED is returned in case the job ring reset fails
 * @retval >0 in case of error
 */
int sec_release();


/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //FSL_SEC_H
