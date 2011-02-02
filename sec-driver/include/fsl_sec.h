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

}sec_return_code_t;

/** Job Ring related events notified to User Application. */
typedef enum sec_job_ring_event_e
{
	SEC_JOB_RING_RESET = 0,      /* A HW error occurred and Job Ring needs to be reset */

}sec_job_ring_event_t;
/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/**
 * Opaque handle to a Job Ring provided by sec user space driver
 * to UA when sec_init() is called.
 * */
typedef void* sec_job_ring_t;

/**
 * Callback called to notify UA of Job Ring related events. The packet related
 * events are notified with a different callback.
 *
 * In case a JR needs to be reset due to a HW error, the UA can be informed
 * of this by means of this callback.
 * TODO: This callback will be called from within the pool function.
 *
 * TODO: Specify what happens with the packets in-flight when the JR is reset.
 * Are the packets discarded? Is UA notified of discarded packets? Before or after reset?
 *
 * TODO: Specify if sec driver expects any information from UA when this
 * callback is called (e.g. go/no-go decision for reset).
 *
 * TODO: Should provide to a UA a known UA handle ... a handle received from it.
 *
 * @param [in] job_ring_handle  The opaque handle of the JR for which a notification
 *                              is raised. The JR handle was provided to User Application
 *                              by the SEC user space driver at sec_init().
 *                              TODO: Should add a User Application specific handle?
 * @param [in] jr_event         The JR related event notified to User Application.
 *                              Valid values: see enum ::sec_job_ring_event_t
 * @param [in] jr_status        The HW status that triggered the event.
 *                              This field is informational only. Is useful in debugging
 *                              the cause of the event.
 *                              In case the event is #SEC_JOB_RING_RESET, this param contains
 *                              the following data:
 *                               - For SEC 4.4: Job Ring Interrupt Status Register (Bits 0-32).
 *                               - For SEC 3.1: Channel Status Register (Bits 32-63).
 */
typedef void (*job_ring_callback_t)(
		sec_job_ring_t	*jr_handle,
		sec_job_ring_event_t   jr_event,
		int jr_status);

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
 * The exact Job Rings used by this library is decided between SEC user
 * space driver and SEC kernel driver.
 *
 * @param [in]  job_rings_no       The number of job rings to acquire and initialize.
 * @param [out] job_ring_handles   Array of job ring handles of size job_rings_no.
 *
 * @retval #SEC_SUCCESS for successful execution
 * @retval #SEC_OUT_OF_MEMORY is returned if internal memory allocation fails.
 * @retval >0 in case of error
 */
int sec_init(int job_rings_no,
		     job_ring_callback_t job_ring_callback,
		     sec_job_ring_t **job_ring_handles);

/**
 * @brief Release the resources used by the SEC user space driver.
 *
 * Reset SEC's job rings indicated by the User Application at sec_init and
 * free any memory allocated internally.
 * Call once during application teardown.
 *
 * TODO: Specify what happens with the in-flight packets, in case there are any,
 * when this API is called.
 *
 * @retval #SEC_SUCCESS for successful execution
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
