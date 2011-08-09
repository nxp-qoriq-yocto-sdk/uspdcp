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

#ifdef _cplusplus
extern "C" {
#endif

/*=================================================================================================
                                        INCLUDE FILES
==================================================================================================*/

#include "fsl_sec.h"
#include "sec_utils.h"
#include "sec_contexts.h"
#include "sec_config.h"
#include "sec_job_ring.h"
#include "sec_atomic.h"
#include "sec_pdcp.h"
#include "sec_hw_specific.h"

#include <stdio.h>

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
/** Max sec contexts per pool computed based on the number of Job Rings assigned.
 *  All the available contexts will be split evenly between all the per-job-ring pools. 
 *  There is one additional global pool besides the per-job-ring pools. */
#define MAX_SEC_CONTEXTS_PER_POOL   (SEC_MAX_PDCP_CONTEXTS / (g_job_rings_no))

/** Max length of a string describing a ::sec_status_t value or a ::sec_return_code_t value */
#define MAX_STRING_REPRESENTATION_LENGTH    50


/** Define an invalid value for a pthread key. */
#define SEC_PTHREAD_KEY_INVALID     ((pthread_key_t)(~0))


/**  Macro for the initialization of the g_sec_errno thread-local key. */
#define SEC_INIT_ERRNO_KEY() \
    do{ \
        if(pthread_key_create(&g_sec_errno, NULL) != 0) \
        { \
            g_sec_errno = SEC_PTHREAD_KEY_INVALID; \
            SEC_ERROR("Error creating pthread key for g_sec_errno"); \
            return SEC_INVALID_INPUT_PARAM; \
        } \
    }while(0)

/** Macro for the destruction of the g_sec_errno thread-local. */
#define SEC_DELETE_ERRNO_KEY() \
    do{ \
        if(g_sec_errno != SEC_PTHREAD_KEY_INVALID) \
        { \
            SEC_ASSERT(pthread_key_delete(g_sec_errno) == 0, \
                       SEC_INVALID_INPUT_PARAM, \
                       "Error deleting pthread key g_sec_errno"); \
        } \
    }while(0)

/** Macro for setting the g_sec_errno thread-local variable.
 *
 * The thread-local stored value will not be the actual error code, but the address of g_sec_errno_value.
 *
 * @param errno_value    The value to be set into the thread-local errno key.
 */
#define SEC_ERRNO_SET(errno_value) \
    g_sec_errno_value = errno_value; \
    pthread_setspecific(g_sec_errno, &g_sec_errno_value)


/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/** Lists the states possible for the SEC user space driver. */
typedef enum sec_driver_state_e
{
    SEC_DRIVER_STATE_IDLE,      /*< Driver not initialized */
    SEC_DRIVER_STATE_STARTED,   /*< Driver initialized and can be used by UA */
    SEC_DRIVER_STATE_RELEASE,   /*< Driver release is in progress */
}sec_driver_state_t;

/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
/* The current state of SEC user space driver */
volatile sec_driver_state_t g_driver_state = SEC_DRIVER_STATE_IDLE;

/* The number of job rings used by SEC user space driver */
static int g_job_rings_no = 0;

/* Array of job ring handles provided to UA from sec_init() function. */
static sec_job_ring_descriptor_t g_job_ring_handles[MAX_SEC_JOB_RINGS];

/* The work mode configured for SEC user space driver at startup,
 * when NAPI notification processing is ON */
static int g_sec_work_mode = 0;

/* Last JR assigned to a context by the SEC driver using a round robin algorithm.
 * Not used if UA associates the contexts created to a certain JR.*/
static unsigned int g_last_jr_assigned = 0;

/* Global context pool */
static sec_contexts_pool_t g_ctx_pool;


/** String representation for values from  ::sec_status_t
 * @note Order of values from g_status_string MUST match the same order
 *       used to define status values in ::sec_status_t ! */
static const char g_status_string[][MAX_STRING_REPRESENTATION_LENGTH] =
{
    {"SEC_STATUS_SUCCESS"},
    {"SEC_STATUS_ERROR"},
    {"SEC_STATUS_OVERDUE"},
    {"SEC_STATUS_LAST_OVERDUE"},
    {"SEC_STATUS_HFN_THRESHOLD_REACHED"},
    {"SEC_STATUS_MAC_I_CHECK_FAILED"},
    {"Not defined"},
};

/** String representation for values from  ::sec_return_code_t
 * @note Order of values from g_return_code_descriptions MUST match the same order
 *       used to define return code values in ::sec_return_code_t ! */
static const char g_return_code_descriptions[][MAX_STRING_REPRESENTATION_LENGTH] =
{
    {"SEC_SUCCESS"},
    {"SEC_INVALID_INPUT_PARAM"},
    {"SEC_CONTEXT_MARKED_FOR_DELETION"},
    {"SEC_OUT_OF_MEMORY"},
    {"SEC_PACKETS_IN_FLIGHT"},
    {"SEC_LAST_PACKET_IN_FLIGHT"},
    {"SEC_PROCESSING_ERROR"},
    {"SEC_PACKET_PROCESSING_ERROR"},
    {"SEC_JR_RESET_FAILED"},
    {"SEC_JR_IS_FULL"},
    {"SEC_DRIVER_RELEASE_IN_PROGRESS"},
    {"SEC_DRIVER_ALREADY_INITALIZED"},
    {"SEC_DRIVER_NOT_INITALIZED"},
    {"SEC_JOB_RING_RESET_IN_PROGRESS"},
    {"SEC_DRIVER_NO_FREE_CONTEXTS"},
    {"Not defined"},
};
/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
/** Start address for DMA-able memory area configured by UA */
void* g_dma_mem_start = NULL;
/** Current address for unused DMA-able memory area configured by UA */
void* g_dma_mem_free = NULL;
/** A key used to access the SEC driver's errno variable on a thread-local basis.
 * The SEC driver uses this variable similar to libc's errno for
 * passing error codes to the User Application. */
pthread_key_t g_sec_errno = SEC_PTHREAD_KEY_INVALID;
/** Global storage for error codes returned by SEC engine.
 * @see the 'man 3 pthread_setspecific' page for further details on thread locals.
 */
uint32_t g_sec_errno_value = SEC_SUCCESS;

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/


/** @brief Enable IRQ generation for all SEC's job rings.
 */
#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
static void enable_irq();
#endif

/** @brief Poll the HW for already processed jobs in the JR
 * and notify the available jobs to UA.
 *
 * @param [in]  job_ring    The job ring to poll.
 * @param [in]  limit       The maximum number of jobs to notify.
 *                          If set to negative value, all available jobs are notified.
 * @param [out] packets_no  No of jobs notified to UA.
 *
 * @retval 0 for success
 * @retval other value for error
 */
static uint32_t hw_poll_job_ring(sec_job_ring_t *job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no);

/** @brief Poll the HW for already processed jobs in the JR
 * and silently discard the available jobs or notify them to UA
 * with indicated error code.
 * In case packets belonging to a retiring SEC context are notified,
 * the last two packets on that context are notified having status set to
 * #SEC_STATUS_OVERDUE and #SEC_STATUS_LAST_OVERDUE respectively.
 *
 * @param [in,out]  job_ring        The job ring to poll.
 * @param [in]  do_notify           Can be #TRUE or #FALSE. Indicates if packets are to be discarded
 *                                  or notified to UA with given error_code.
 * @param [in]  status              The status code.
 * @param [in]  error_code          The detailed SEC error code.
 * @param [out] notified_packets    Number of notified packets. Can be NULL if do_notify is #FALSE
 */
static void hw_flush_job_ring(sec_job_ring_t *job_ring,
                              uint32_t do_notify,
                              sec_status_t status,
                              uint32_t error_code,
                              uint32_t *notified_packets);

/** @brief Flush job rings of any processed packets.
 * The processed packets are silently dropped,
 * WITHOUT beeing notified to UA.
 */
static void flush_job_rings();
#ifdef SEC_HW_VERSION_3_1
/** @brief Checks if there are any PDCP control-plane packets waiting
 * to be processed, on a certain job ring.
 * PDCP control-plane packets may require a two step processing:
 * sent once to SEC for integrity checking, sent second time to SEC
 * for confidentiality protection.
 *
 * @param [in,out]     job_ring     Job ring
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_process_pdcp_c_plane_packets(sec_job_ring_t *job_ring);

/** @brief Copy generated MAC-I code from driver's internal location 
 * to the indicated location.
 *
 * @param [in]  job            The job
 * @param [in]  sec_context    The context
 * @param [out] out_mac_i      The MAC-I output code
 */
static void sec_handle_mac_i_generation(sec_job_t *job,
                                        sec_context_t *sec_context,
                                        uint32_t *out_mac_i);

/** @brief Check result of MAC-I validation.
 * For AES CMAC, on SEC 3.1, the validation is manually done in the driver with memcmp.
 * For SNOW, SEC 3.1 does the validation itself.
 *
 * @param [in]  job            The job
 * @param [in]  sec_context    The context
 *
 * @retval 0 for MAC-I validation success
 * @retval 1 for MAC-I validation failure
 */
static int sec_handle_mac_i_validation(sec_job_t *job, sec_context_t *sec_context);

/** @brief Handle a PDCP control packet.
 * Decide if packet is to be notified to User Application, if it is to be sent 
 * again to SEC, etc.
 *
 * @param [in]  job_ring     Job ring
 * @param [in]  job          The job
 * @param [in]  sec_context  The context
 * @param [out] status       The status to be notified to UA
 * @param [out] notify_pkt   Indicates if packet is to be notified to UA.
 */
static void sec_handle_c_plane_packet(sec_job_ring_t *job_ring,
                                      sec_job_t *job,
                                      sec_context_t *sec_context,
                                      sec_status_t *status,
                                      uint8_t *notify_pkt);

/** @brief Buffer a PDCP control packet into internal Control-plane FIFO.
 * The packet will be sent again to SEC for second processing stage:
 * confidentiality or authentication, depending on context configuration.
 * Next time sec_process_packet() API is called, the internal Control-plane FIFO
 * is checked and any buffered packets are sent to SEC.
 *
 * @param [in]  job_ring     Job ring
 * @param [in]  job          The job
 */
static void sec_buffer_c_plane_packet(sec_job_ring_t *job_ring,
                                      sec_job_t *job);

/** @brief On P2020 it is required to implement EEA0/EIA0 algorithms,
 * that is NULL crypto and NULL authentication with a memcpy-based mechanism.
 *
 * @note: TODO when migrating to PSC9132: remove this function!!
 *
 * @param [in,out]  job     The job pointing to input/output packets
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_process_packet_with_null_algo(sec_job_t *job);
#endif // SEC_HW_VERSION_3_1
/** @brief Handle packet that generated error in SEC engine.
 * Identify the exact type of error and handle the error.
 * Depending on the error type, the job ring could be reset.
 * All packets that are submitted for processing on this job ring
 * are notified to User Application with error status and detailed error code.
 *
 * @param [in]  job_ring            Job ring
 * @param [in]  sec_error_code      Error code read from job ring's Channel Status Register
 * @param [out] notified_packets    Number of notified packets. Can be NULL if do_notify is #FALSE
 * @param [out] do_driver_shutdown  If set to #TRUE, then UA is returned code #SEC_PROCESSING_ERROR
 *                                  which is indication that UA must call sec_release() after this.
 */
static void sec_handle_packet_error(sec_job_ring_t *job_ring,
                                    uint32_t sec_error_code,
                                    uint32_t *notified_packets,
                                    uint32_t *do_driver_shutdown);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
#if SEC_NOTIFICATION_TYPE != SEC_NOTIFICATION_TYPE_POLL
static void enable_irq()
{
    int i = 0;
    sec_job_ring_t * job_ring = NULL;

    for (i = 0; i < g_job_rings_no; i++)
    {
        job_ring = &g_job_rings[i];
        uio_job_ring_enable_irqs(job_ring);
    }
}
#endif

static void hw_flush_job_ring(sec_job_ring_t * job_ring,
                              uint32_t do_notify,
                              sec_status_t status,
                              uint32_t error_code,
                              uint32_t *notified_packets)
{
    sec_context_t *sec_context = NULL;
    sec_job_t *job = NULL;
    sec_job_t saved_job;
    sec_status_t new_status;
    int32_t jobs_no_to_discard = 0;
    int32_t discarded_packets_no = 0;
    int32_t number_of_jobs_available = 0;
    int ret;

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Flushing jr id %d with packet status = %d "
              "and SEC error code = 0x%x",
              job_ring, job_ring->pidx, job_ring->cidx,
              job_ring->jr_id, status, error_code);

    // compute the number of jobs available in the job ring based on the
    // producer and consumer index values.
    number_of_jobs_available = SEC_JOB_RING_NUMBER_OF_ITEMS(SEC_JOB_RING_SIZE,
                                                            job_ring->pidx,
                                                            job_ring->cidx);

    // Discard all jobs
    jobs_no_to_discard = number_of_jobs_available;
    *notified_packets = 0;

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Discarding %d packets",
              job_ring, job_ring->pidx, job_ring->cidx, jobs_no_to_discard);

    // If function was called because an error happened on the job ring,
    // then most likely the job ring is halted and it will not accept any more jobs.
    // Still, used a job ring state to reflect that job ring is in flush/reset process.
    // This way the UA will not be able to submit new jobs until reset/flush is over.
    while(jobs_no_to_discard > discarded_packets_no)
    {
        // get the first un-notified job from the job ring
        job = &job_ring->jobs[job_ring->cidx];
        sec_context = job->sec_context;

        discarded_packets_no++;

        if(do_notify == TRUE)
        {
            // copy into a temporary job the fields from the job we need to raise callback
            // this is done to free the slot before the callback is called,
            // which we cannot control in terms of how much processing it will do.
            saved_job.in_packet = job->in_packet;
            saved_job.out_packet = job->out_packet;
            saved_job.ua_handle = job->ua_handle;
        }

        // Now increment the consumer index for the current job ring,
        // AFTER saving job in temporary location!
        // Increment the consumer index for the current job ring
        job_ring->cidx = SEC_CIRCULAR_COUNTER(job_ring->cidx, SEC_JOB_RING_SIZE);

        if(do_notify == TRUE)
        {
            new_status = status;
            // If context is retiring, set a suggestive status for the packets notified to UA.
            // Doesn't matter if the status is overwritten here, if UA deleted the context,
            // it does not care about any other packet status.
            if(sec_context->state == SEC_CONTEXT_RETIRING)
            {
                // at this point, PI per context is frozen, context is retiring,
                // no more packets can be submitted for it.
                new_status = (CONTEXT_GET_PACKETS_NO(sec_context) > 1) ?
                    SEC_STATUS_OVERDUE : SEC_STATUS_LAST_OVERDUE;
            }

            // call the callback
            ret = sec_context->notify_packet_cbk(saved_job.in_packet,
                    saved_job.out_packet,
                    saved_job.ua_handle,
                    new_status,
                    error_code);

            // consume processed packet for this sec context
            CONTEXT_CONSUME_PACKET(sec_context);

            // UA requested to exit
            if (ret == SEC_RETURN_STOP)
            {
                ASSERT(notified_packets != NULL);
                *notified_packets = discarded_packets_no;
                return;
            }
        }
        else
        {
            // consume processed packet for this sec context
            CONTEXT_CONSUME_PACKET(sec_context);
        }
    }

    if(do_notify == TRUE)
    {
        ASSERT(notified_packets != NULL);
        *notified_packets = discarded_packets_no;
    }
}

static uint32_t hw_poll_job_ring(sec_job_ring_t *job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no)
{
    sec_context_t *sec_context = NULL;
    sec_job_t *job = NULL;
    sec_job_t saved_job;

    int32_t jobs_no_to_notify = 0; // the number of done jobs to notify to UA
    sec_status_t status = SEC_STATUS_SUCCESS;
    uint32_t notified_packets_no = 0;
    uint32_t error_packets_no = 0;
    uint32_t number_of_jobs_available = 0;
    uint32_t sec_error_code = 0;
    int ret = 0;
    uint8_t notify_pkt = TRUE;
    uint32_t do_driver_shutdown = FALSE;

    

    // Compute the number of notifications that need to be raised to UA
    // If limit < 0 -> notify all done jobs
    // If limit > total number of done jobs -> notify all done jobs
    // If limit = 0 -> error
    // If limit > 0 && limit < total number of done jobs -> notify a number of done jobs equal with limit

    // compute the number of jobs available in the job ring based on the
    // producer and consumer index values.

    number_of_jobs_available = SEC_JOB_RING_NUMBER_OF_ITEMS(SEC_JOB_RING_SIZE, job_ring->pidx, job_ring->cidx);

    jobs_no_to_notify = (limit < 0 || limit > number_of_jobs_available) ? number_of_jobs_available : limit;

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Jobs submitted %d.Jobs to notify %d",
              job_ring, job_ring->pidx, job_ring->cidx,
              number_of_jobs_available, jobs_no_to_notify);

    while(jobs_no_to_notify > notified_packets_no)
    {
#ifdef SEC_HW_VERSION_4_4
        if(hw_get_no_finished_jobs(job_ring) == 0 )
        {
            SEC_DEBUG("No jobs completed yet...");
            continue;
        }

        SEC_DEBUG("Found a job! Total # of jobs waiting: %d", hw_get_no_finished_jobs(job_ring) );
        {
            dma_addr_t out_descr = hw_get_current_out_descriptor(job_ring);

            out_descr -= sizeof(struct sec_outring_entry);
            SEC_DEBUG("Last output descriptor @ addr: 0x%x",out_descr);
            SEC_DEBUG("Last descriptor status: 0x%x",((struct sec_outring_entry*)out_descr)->status);

            SEC_DEBUG("Last processed descriptor status (from register): 0x%x",JR_REG_JROSR(job_ring));
        }

#else
        if(SEC_JOB_RING_NUMBER_OF_ITEMS(SEC_JOB_RING_SIZE, job_ring->pidx, job_ring->cidx) == 0)
        {
            SEC_DEBUG("Job Ring %p was emptied by moving c-plane packets into internal FIFO",
                      job_ring);
            break;
        }
#endif
        // get the first un-notified job from the job ring
        job = &job_ring->jobs[job_ring->cidx];

        // check if job is DONE
#if SEC_HW_VERSION_4_4
        if(!hw_job_is_done(job))
#else
        if(!hw_job_is_done(job->descr))
#endif
        {
            // check if any job generated error, not only this job.
#if SEC_HW_VERSION_3_1
            sec_error_code = hw_job_ring_error(job_ring);
#else
            sec_error_code = hw_job_error(job);
#endif
            if (sec_error_code)
            {
                // Set errno value to the error code returned by SEC engine.
                // Errno value is thread-local.
                SEC_ERRNO_SET(sec_error_code);

                SEC_ERROR("Packet at cidx %d generated error 0x%x on job ring with id %d\n",
                          job_ring->cidx, sec_error_code, job_ring->jr_id);
                SEC_DEBUG("Job status: 0x%x",*(uint32_t*)(job->out_status));

                // flush jobs from JR, with error code set to callback
                sec_handle_packet_error(job_ring, sec_error_code, &error_packets_no, &do_driver_shutdown);

                notified_packets_no += error_packets_no;
                *packets_no = notified_packets_no;

                return ((do_driver_shutdown == TRUE) ? SEC_PROCESSING_ERROR : SEC_PACKET_PROCESSING_ERROR);
            }
            else
            {
#warning "Do something for SEC 4.4"
#ifdef SEC_HW_VERSION_3_1
                // Packet is not processed yet, exit
                SEC_DEBUG("Jr[%p]. packet not done, exit. job descr hdr = 0x%x \n",
                          job_ring, job->descr->hdr);
#endif
                break;
            }
        }
        
        // Used only for #SEC_STATUS_HFN_THRESHOLD_REACHED code, which is set by the driver,
        // before the packet is sent to SEC.
        status = job->job_status;
        sec_context = job->sec_context;
#ifdef SEC_HW_VERSION_3_1
        // Packet processing is DONE. See if it's a data-plane or control-plane packet
        if(sec_context->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE)
        {
            sec_handle_c_plane_packet(job_ring, job, sec_context, &status, &notify_pkt);

            // If control-plane packet must be sent to SEC a second time,
            // it will not be notified now.
            if(notify_pkt == FALSE)
            {
                // increment the consumer index for the current job ring
                job_ring->cidx = SEC_CIRCULAR_COUNTER(job_ring->cidx, SEC_JOB_RING_SIZE);

                // Do not consume the packet on the context! The context still has this packet in flight,
                // even if it's in internal c-plane FIFO.
                continue;
            }
        }
#endif
        // copy into a temporary job the fields from the job we need to raise callback
        // this is done to free the slot before the callback is called,
        // which we cannot control in terms of how much processing it will do.
        saved_job.in_packet = job->in_packet;
        saved_job.out_packet = job->out_packet;
        saved_job.ua_handle = job->ua_handle;

        // now increment the consumer index for the current job ring,
        // AFTER saving job in temporary location!
        job_ring->cidx = SEC_CIRCULAR_COUNTER(job_ring->cidx, SEC_JOB_RING_SIZE);
#if SEC_HW_VERSION_4_4
        JR_HW_REMOVE_ONE_ENTRY(job_ring);
#endif

        // If context is retiring, set a suggestive status for the packets notified to UA.
        // Doesn't matter if the status is overwritten here, if UA deleted the context,
        // it does not care about any other packet status.
        if(sec_context->state == SEC_CONTEXT_RETIRING)
        {
            // at this point, PI per context is frozen, context is retiring,
            // no more packets can be submitted for it.
            status = (CONTEXT_GET_PACKETS_NO(sec_context) > 1) ?
                     SEC_STATUS_OVERDUE : SEC_STATUS_LAST_OVERDUE;
        }


        // call the callback
        ret = sec_context->notify_packet_cbk(saved_job.in_packet,
                                             saved_job.out_packet,
                                             saved_job.ua_handle,
                                             status,
                                             0); // no error

        // consume processed packet for this sec context
        CONTEXT_CONSUME_PACKET(sec_context);
        notified_packets_no++;

        // UA requested to exit
        if (ret == SEC_RETURN_STOP)
        {
            *packets_no = notified_packets_no;
            return SEC_SUCCESS; 
        }
    }

    *packets_no = notified_packets_no;

    return SEC_SUCCESS;
}

static void flush_job_rings()
{
    sec_job_ring_t * job_ring = NULL;
    int i = 0;

    for (i = 0; i < g_job_rings_no; i++)
    {
        job_ring = &g_job_rings[i];

        // Producer index is frozen. If consumer index is not equal
        // with producer index, then we must sit and wait until all
        // packets are processed by SEC on this job ring.
        while(job_ring->pidx != job_ring->cidx)
        {
            hw_flush_job_ring(job_ring,
                              FALSE,
                              SEC_STATUS_SUCCESS,
                              0, // no error
                              NULL);
        }
    }
}
#ifdef SEC_HW_VERSION_3_1
static int sec_process_pdcp_c_plane_packets(sec_job_ring_t *job_ring)
{
    int ret = SEC_SUCCESS;
    int32_t nr_of_c_plane_pkts = 0;
    sec_job_t *c_plane_job = NULL;
    sec_job_t *jr_job = NULL;
    struct fifo_t *c_plane_fifo = &job_ring->pdcp_c_plane_fifo;
    sec_context_t *sec_context = NULL;

    nr_of_c_plane_pkts = FIFO_NUMBER_OF_ITEMS(FIFO_CAPACITY, c_plane_fifo->pidx, c_plane_fifo->cidx);

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d]. C-plane FIFO pi[%d] ci[%d]",
              job_ring, job_ring->pidx, job_ring->cidx,
              c_plane_fifo->pidx, c_plane_fifo->cidx);

    while(nr_of_c_plane_pkts > 0)    
    {
        // check if the Job Ring is full
        if(SEC_JOB_RING_IS_FULL(job_ring->pidx, job_ring->cidx,
                    SEC_JOB_RING_SIZE, SEC_JOB_RING_HW_SIZE + 1))
        {
            SEC_DEBUG("Jr[%p] pi[%d] ci[%d]. Jr is full.\n", job_ring, job_ring->pidx, job_ring->cidx);
            return SEC_JR_IS_FULL;
        }

        // get the first un-notified job from the c-plane internal FIFO
        c_plane_job = c_plane_fifo->items[c_plane_fifo->cidx];

        sec_context = c_plane_job->sec_context;
        // send packet to SEC again, for second processing step:
        // integrity check or confidentiality.

        // if packet was already processed with integrity algorithm,
        // process now with confidentiality. And viceversa...
        c_plane_job->is_integrity_algo = !c_plane_job->is_integrity_algo;


        jr_job = &job_ring->jobs[job_ring->pidx];

        jr_job->in_packet = c_plane_job->in_packet;
        jr_job->out_packet = c_plane_job->out_packet;

        jr_job->sec_context = c_plane_job->sec_context;
        jr_job->ua_handle = c_plane_job->ua_handle;
        jr_job->is_integrity_algo = c_plane_job->is_integrity_algo;

        // Update SEC descriptor. If HFN reached the configured threshold, then
        // the User App callback will be raised with status set to SEC_STATUS_HFN_THRESHOLD_REACHED.
        ret = sec_pdcp_context_update_descriptor(jr_job->sec_context, jr_job, jr_job->descr, jr_job->is_integrity_algo);
        SEC_ASSERT(ret == SEC_SUCCESS, ret,
                   "sec_pdcp_context_update_descriptor returned error code %d", ret);

        // increment the producer index for the current job ring
        job_ring->pidx = SEC_CIRCULAR_COUNTER(job_ring->pidx, SEC_JOB_RING_SIZE);


        // Enqueue this descriptor into the Fetch FIFO of this JR
        // Need write memory barrier here so that descriptor is written before
        // packet is enqueued in SEC's job ring.
        // GCC's builtins offers full memry barrier, use it here.
        atomic_synchronize();
        hw_enqueue_packet_on_job_ring(job_ring, jr_job->descr_phys_addr);
        
        nr_of_c_plane_pkts--;

        // increment the consumer index for the current job ring
        c_plane_fifo->cidx = FIFO_CIRCULAR_COUNTER(c_plane_fifo->cidx, FIFO_CAPACITY);

        SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Sent c-plane packets to SEC second time.",
                  job_ring, job_ring->pidx, job_ring->cidx);
    }

    return SEC_SUCCESS;
}

static void sec_handle_mac_i_generation(sec_job_t *job,
                                        sec_context_t *sec_context,
                                        uint32_t *out_mac_i)
{
    SEC_DEBUG("SEC context[%p] in pkt[%p] out pkt[%p].",
              "Internal raw generated MAC-I: [0x%x,0x%x]",
              sec_context, job->in_packet, job->out_packet,
              *((uint32_t*)(job->mac_i->code)),
              *((uint32_t*)(job->mac_i->code + 4)));

    // SEC 3.1 generated an 8 byte long MAC-I into a per-context-memory-zone internal
    // to the driver. Copy the relevant 4 bytes of MAC-I code into output buffer.
    if(sec_context->pdcp_crypto_info->integrity_algorithm == SEC_ALG_SNOW)
    {
        // SEC 3.1 generates SNOW F9 MAC-I code that is 8 bytes long, having first 4 bytes
        // set to zero. PDCP MAC-I is in the last 4 bytes.
        *out_mac_i = *((uint32_t*)(job->mac_i->code + 4));
    }
    else if(sec_context->pdcp_crypto_info->integrity_algorithm == SEC_ALG_AES)
    {

        // SEC 3.1 generates AES CMAC MAC-I code that is 8 bytes long, 
        // having PDCP MAC-I in the first 4 bytes. Last 4 bytes unused.
        *out_mac_i = *((uint32_t*)(job->mac_i->code));
    }

    SEC_DEBUG("SEC context[%p] in pkt[%p] out pkt[%p].",
              "Generated MAC-I = 0x%x",
              sec_context, job->in_packet,
              job->out_packet, *out_mac_i);
}

static int sec_handle_mac_i_validation(sec_job_t *job, sec_context_t *sec_context)
{
    uint32_t original_mac_i;

    // Decapsulation. MAC-I validation was performed.
    if(sec_context->pdcp_crypto_info->integrity_algorithm == SEC_ALG_SNOW)
    {
        // SEC 3.1 is capable of doing MAC-I validation itself for SNOW F9
        SEC_DEBUG("Validating SNOW F9 hw status. hdr = 0x%x.\n", job->descr->hdr_lo);
        return hw_icv_check_failed(job->descr);
    }
    else if (sec_context->pdcp_crypto_info->integrity_algorithm == SEC_ALG_AES)
    {
        const sec_packet_t *auth_packet = (sec_context->double_pass == TRUE) ? job->out_packet : job->in_packet;
        // Do manually the MAC-I validation for AES CMAC.
        // SEC 3.1 does not support ICV len smaller than 8 bytes for AES CMAC.
        original_mac_i = (*(uint32_t*)(auth_packet->address +
                         auth_packet->length -
                         PDCP_MAC_I_LENGTH));

        SEC_DEBUG("SEC context[%p] in pkt[%p] out pkt[%p]."
                  "Original MAC-I = 0x%x. Generated MAC-I = 0x%x",
                  sec_context, job->in_packet, job->out_packet,
                  original_mac_i, *((uint32_t*)(job->mac_i->code)));

        return (original_mac_i != *((uint32_t*)(job->mac_i->code)));
    }
    return 0;
}

static void sec_handle_c_plane_packet(sec_job_ring_t *job_ring,
                                      sec_job_t *job,
                                      sec_context_t *sec_context,
                                      sec_status_t *status,
                                      uint8_t *notify_pkt)
{
    uint8_t mac_i_check_failed = FALSE;
    
    *notify_pkt = TRUE;

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Start processing c-plane packet done.",
               job_ring, job_ring->pidx, job_ring->cidx);

    if(sec_context->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION)
    {
        if(job->is_integrity_algo == TRUE)
        {
            if(sec_context->double_pass == TRUE)
            {
                SEC_DEBUG("SEC context[%p] in pkt[%p] out pkt[%p]."
                          "Copy MAC-I into input buffer",
                          sec_context, job->in_packet, job->out_packet);
                // for double pass c-plane packets copy generated MAC-I code into input packet:
                sec_handle_mac_i_generation(job,
                                            sec_context,
                                            (uint32_t*)(job->in_packet->address +
                                                        job->in_packet->length -
                                                        PDCP_MAC_I_LENGTH ));

                // do not notify packet now
                *notify_pkt = FALSE;

                // add packet to special control-plane fifo
                sec_buffer_c_plane_packet(job_ring, job);
            }
            else
            {
                SEC_DEBUG("SEC context[%p] in pkt[%p] out pkt[%p]."
                          "Copy MAC-I into output buffer",
                          sec_context, job->in_packet, job->out_packet);
                // For single pass c-plane packets with integrity only,
                // copy generated MAC-I code into last 4 bytes of input packet.
                sec_handle_mac_i_generation(job,
                                            sec_context,
                                            (uint32_t*)(job->out_packet->address + job->out_packet->offset));
            }
        }
    }
    else if(sec_context->pdcp_crypto_info->protocol_direction == PDCP_DECAPSULATION)
    {
        // On decapsulation we run first confidentiality algorithm
        if(job->is_integrity_algo == FALSE)
        {
            if(sec_context->double_pass == TRUE)
            {
                // do not notify packet now
                *notify_pkt = FALSE;

                // add packet to special control-plane fifo
                sec_buffer_c_plane_packet(job_ring, job);
            }
        }
        else
        {
            // integrity algorithm: validate MAC-I
            mac_i_check_failed = sec_handle_mac_i_validation(job, sec_context);
            SEC_DEBUG("SEC context[%p] in pkt[%p] out pkt[%p].",
                      "job[%p].Generated MAC-I = [0x%x,0x%x]",
                      sec_context, job->in_packet, job->out_packet, job,
                      *((uint32_t*)(job->mac_i->code)),
                      *((uint32_t*)(job->mac_i->code + 4))
                    );

            // Check if MAC-I validation failed
            if(mac_i_check_failed)
            {
                SEC_ERROR("SEC context[%p] in pkt[%p] out pkt[%p]."
                          "Integrity check FAILED!."
                          "original MAC-I[0x%x]. generated MAC-I = [0x%x,0x%x]",
                          sec_context, job->in_packet, job->out_packet,
                          (*(uint32_t*)(job->out_packet->address +
                                        job->out_packet->length -
                                        PDCP_MAC_I_LENGTH)),
                          *((uint32_t*)(job->mac_i->code)),
                          *((uint32_t*)(job->mac_i->code + 4)));

                *status = SEC_STATUS_MAC_I_CHECK_FAILED;
            }
        }
    }

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].End processing c-plane packet done."
              "job[%p].Notify to UA = %d",
               job_ring, job_ring->pidx, job_ring->cidx,
               job, *notify_pkt);
}

static void sec_buffer_c_plane_packet(sec_job_ring_t *job_ring,
                                      sec_job_t *job)
{
    struct fifo_t *c_plane_fifo = &job_ring->pdcp_c_plane_fifo;

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].job[%p]."
              "Buffering control-plane packet into internal FIFO",
              job_ring, job_ring->pidx, job_ring->cidx, job);

    // check if the c-plane fifo is full
    if(FIFO_IS_FULL(c_plane_fifo->pidx, c_plane_fifo->cidx,
                FIFO_CAPACITY, FIFO_CAPACITY))
    {
        SEC_ERROR("Jr[%p].Control-plane FIFO is FULL!"
                  "Consider increasing fifo capacity.",
                  job_ring);
    }

    // get first available job from c-plane fifo
    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].job[%p].Copy job in c-plane FIFO",
              job_ring, job_ring->pidx, job_ring->cidx, job);

    assert(c_plane_fifo->items[c_plane_fifo->pidx] != NULL);
    memcpy(c_plane_fifo->items[c_plane_fifo->pidx], job, sizeof(sec_job_t));

    // increment the producer index for the current job ring
    c_plane_fifo->pidx = FIFO_CIRCULAR_COUNTER(c_plane_fifo->pidx, FIFO_CAPACITY);
}

static int sec_process_packet_with_null_algo(sec_job_t *job)
{
    assert(job != NULL);
    assert(job->in_packet != NULL);
    assert(job->out_packet != NULL);

    SEC_ASSERT(job->in_packet->address != NULL,
            SEC_INVALID_INPUT_PARAM,
            "NULL address field specified for input packet");

    SEC_ASSERT(job->out_packet->address != NULL,
            SEC_INVALID_INPUT_PARAM,
            "NULL address field specified for output packet");

    SEC_ASSERT(job->in_packet->length - job->in_packet->offset ==
               job->out_packet->length - job->out_packet->offset,
               SEC_INVALID_INPUT_PARAM,
               "Length and offset for input packet mistmatch with those for output packet");

    // Copy input packet into output packet.
    // This is the implementation for NULL-crypto / NULL-authentication on SEC 3.1
    memcpy(job->out_packet->address + job->out_packet->offset,
           job->in_packet->address + job->in_packet->offset,
           job->in_packet->length - job->in_packet->offset);

    // Now simulate that the SEC engine marked this packet as processed.
    // Required to see the packet as DONE from sec_poll.
    job->descr->hdr |=  SEC_DESC_HDR_DONE;
    return SEC_SUCCESS;
}
#endif // SEC_HW_VERSION_3_1

static void sec_handle_packet_error(sec_job_ring_t *job_ring,
                                    uint32_t sec_error_code,
                                    uint32_t *notified_packets,
                                    uint32_t *do_driver_shutdown)
{
    int ret;
    uint32_t reset_cont_job_ring = FALSE;

    ASSERT(notified_packets != NULL);

    // Job ring will be restarted, change its state so that
    // no new packets can be submitted by UA on this job ring.
    job_ring->jr_state = SEC_JOB_RING_STATE_RESET;
    *do_driver_shutdown = FALSE;

    // Analyze the SEC error on this job ring
    // and see if a job ring reset is required.
#ifdef SEC_HW_VERSION_3_1
    hw_handle_job_ring_error(job_ring, sec_error_code, &reset_cont_job_ring);
#else
    hw_handle_job_ring_error(sec_error_code, &reset_cont_job_ring);
#endif
    // Flush the channel no matter the error type.
    // Notify all submitted jobs to UA, setting corresponding error cause
    hw_flush_job_ring(job_ring,
                      TRUE, // notify packets to UA
                      SEC_STATUS_ERROR, // the status to be set for each packet when notified to UA
                      sec_error_code,
                      notified_packets);

    // Reset job ring if required
    if(reset_cont_job_ring == TRUE)
    {
        SEC_DEBUG("Jr[%p].Error on jr id %d requires reset + continue action",
                  job_ring, job_ring->jr_id);

        // Reset job ring using 'continue' method: configuration registers and
        // already submitted jobs are kept and job ring can continue processing.
        ret = hw_reset_and_continue_job_ring(job_ring);
        if(ret != 0)
        {
            SEC_DEBUG("Jr[%p].Failed to reset and continue for jr id %d."
                      "Sending request to SEC kernel driver to reset SEC engine!",
                      job_ring, job_ring->jr_id);

            // Use UIO control to communicate with SEC kernel driver
            // and ask to reset SEC engine.
            uio_reset_sec_engine(job_ring);

            // Driver shutdown is required. UA MUST call sec_release().
            // UA must call sec_init() again if wishes to use driver again.
            *do_driver_shutdown = TRUE;
        }
        else
        {
            // Job ring can be used again by UA
            job_ring->jr_state = SEC_JOB_RING_STATE_STARTED;
        }
    }
    else
    {
        // Job ring can be used again by UA
        job_ring->jr_state = SEC_JOB_RING_STATE_STARTED;
    }
}



/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

sec_return_code_t sec_init(const sec_config_t *sec_config_data,
                           uint8_t job_rings_no,
                           const sec_job_ring_descriptor_t **job_ring_descriptors)
{
    int i = 0;
    int ret = 0;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_IDLE,
               SEC_DRIVER_ALREADY_INITALIZED,
               "Driver already initialized");

    // Validate input arguments
    SEC_ASSERT(job_rings_no <= MAX_SEC_JOB_RINGS, SEC_INVALID_INPUT_PARAM,
               "Requested number of job rings(%d) is greater than maximum hw supported(%d)",
               job_rings_no, MAX_SEC_JOB_RINGS);

    SEC_ASSERT(job_ring_descriptors != NULL, SEC_INVALID_INPUT_PARAM, "job_ring_descriptors is NULL");
    SEC_ASSERT(sec_config_data != NULL, SEC_INVALID_INPUT_PARAM, "sec_config_data is NULL");
    SEC_ASSERT(sec_config_data->memory_area != NULL, SEC_INVALID_INPUT_PARAM,
               "sec_config_data->memory_area is NULL");

    // DMA memory area must be cacheline aligned
    SEC_ASSERT ((dma_addr_t)sec_config_data->memory_area % CACHE_LINE_SIZE == 0, 
                SEC_INVALID_INPUT_PARAM,
                "Configured memory is not cacheline aligned");

    // Initialize per-thread-local errno variable

    // Delete the Errno Key here _ONLY_ if it has a valid value.
    // When sec_init is called the first time by the User Application the errno key has an invalid
    // value (SEC_PTHREAD_KEY_INVALID) and no delete will be made.
    // When sec_init is called the second, third etc. time by the User Application, the errno key
    // has a valid value (from the previous sec_init calls) and it is deleted here to be reinitialized later.
    // The errno key is not deleted in sec_release because User might want to invoke sec_get_last_error()
    // after the execution of sec_release.
    SEC_DELETE_ERRNO_KEY();

    // Initialize the errno thread-local key
    SEC_INIT_ERRNO_KEY();

    // For start, set errno with success value 0
    SEC_ERRNO_SET(SEC_SUCCESS);

    g_job_rings_no = job_rings_no;
    memset(g_job_rings, 0, sizeof(g_job_rings));
    SEC_INFO("Configuring %d number of SEC job rings", g_job_rings_no);

    // Configure DMA-capable memory area assigned to the driver by UA
    g_dma_mem_start = sec_config_data->memory_area;
    g_dma_mem_free = g_dma_mem_start;
    SEC_INFO("Using DMA memory area with start address = %p", g_dma_mem_start);
    
    // Read configuration data from DTS (Device Tree Specification).
    ret = sec_configure(g_job_rings_no, g_job_rings);
    SEC_ASSERT(ret == SEC_SUCCESS, SEC_INVALID_INPUT_PARAM, "Failed to configure SEC driver");

    // Per-job ring initialization
    for (i = 0; i < g_job_rings_no; i++)
    {
        // Initialize the context pool per JR
        // No need for thread synchronizations mechanisms for this pool because
        // one of the assumptions for this API is that only one thread will
        // create/delete contexts for a certain JR (also known as the producer of the JR).
        ret = init_contexts_pool(&(g_job_rings[i].ctx_pool),
                                 &g_dma_mem_free,
                                 MAX_SEC_CONTEXTS_PER_POOL,
                                 THREAD_UNSAFE_POOL);
        if (ret != SEC_SUCCESS)
        {
            SEC_ERROR("Failed to initialize SEC context pool "
                      "for job ring id %d. Starting sec_release()", g_job_rings[i].jr_id);
            sec_release();
            g_driver_state = SEC_DRIVER_STATE_IDLE;
            return ret;
        }

        // Configure each owned job ring with UIO data:
        // - UIO device file descriptor
        // - UIO mapping for SEC registers
        ret = sec_config_uio_job_ring(&g_job_rings[i]);
        if (ret != SEC_SUCCESS)
        {
            SEC_ERROR("Failed to configure UIO settings "
                      "for job ring id %d. Starting sec_release()",
                      g_job_rings[i].jr_id);
            sec_release();
            g_driver_state = SEC_DRIVER_STATE_IDLE;
            return ret;
        }

        // Initialize job ring
        ret = init_job_ring(&g_job_rings[i], &g_dma_mem_free, sec_config_data->work_mode);
        if (ret != SEC_SUCCESS)
        {
            SEC_ERROR("Failed to initialize job ring with id %d. "
                      "Starting sec_release()", g_job_rings[i].jr_id);
            sec_release();
            g_driver_state = SEC_DRIVER_STATE_IDLE;
            return ret;
        }

        g_job_ring_handles[i].job_ring_handle = (sec_job_ring_handle_t)&g_job_rings[i];
        g_job_ring_handles[i].job_ring_irq_fd = g_job_rings[i].uio_fd;
    }

    // Initialize the global pool of contexts also.
    // We need thread synchronizations mechanisms for this pool.
    ret = init_contexts_pool(&g_ctx_pool,
                             &g_dma_mem_free,
                             MAX_SEC_CONTEXTS_PER_POOL,
                             THREAD_SAFE_POOL);
    if (ret != SEC_SUCCESS)
    {
        SEC_ERROR("Failed to initialize global pool of SEC contexts. "
                "Starting sec_release()");
        sec_release();
        g_driver_state = SEC_DRIVER_STATE_IDLE;
        return ret;
    }

    // Remember initial work mode
    g_sec_work_mode = sec_config_data->work_mode;

    // Return handles to job rings
    *job_ring_descriptors =  &g_job_ring_handles[0];

    // Update driver state
    g_driver_state = SEC_DRIVER_STATE_STARTED;

    return SEC_SUCCESS;
}

sec_return_code_t sec_release()
{
    int i;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED ,
               "Driver release is in progress or driver not initialized");

    // Update driver state
    g_driver_state = SEC_DRIVER_STATE_RELEASE;

    // If any packets in flight for any SEC context, poll and wait
    // until all packets are received and silently discarded.
    flush_job_rings();

    for (i = 0; i < g_job_rings_no; i++)
    {
        // destroy the contexts pool per JR
        destroy_contexts_pool(&(g_job_rings[i].ctx_pool));

    	shutdown_job_ring(&g_job_rings[i]);
    }
    g_job_rings_no = 0;

    // destroy the global context pool also
    destroy_contexts_pool(&g_ctx_pool);

    memset(g_job_ring_handles, 0, sizeof(g_job_ring_handles));
    g_driver_state = SEC_DRIVER_STATE_IDLE;

    return SEC_SUCCESS;
}

sec_return_code_t sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                                           const sec_pdcp_context_info_t *sec_ctx_info,
                                           sec_context_handle_t *sec_ctx_handle)
{
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;
    sec_context_t * ctx = NULL;
    int ret = 0;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED ,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(sec_ctx_info != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_info is NULL");
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");
    SEC_ASSERT(sec_ctx_info->notify_packet != NULL, 
               SEC_INVALID_INPUT_PARAM, 
               "sec_ctx_inf has NULL notify_packet function pointer");

    // Crypto keys must come from DMA memory area and must be cacheline aligned
    SEC_ASSERT((dma_addr_t)sec_ctx_info->cipher_key % CACHE_LINE_SIZE == 0,
               SEC_INVALID_INPUT_PARAM,
               "Configured crypto key is not cacheline aligned");

    if(sec_ctx_info->integrity_key != NULL)
    {
        // Authentication keys must come from DMA memory area and must be cacheline aligned
        SEC_ASSERT((dma_addr_t)sec_ctx_info->integrity_key % CACHE_LINE_SIZE == 0,
                   SEC_INVALID_INPUT_PARAM,
                   "Configured integrity key is not cacheline aligned");
    }

    // Either UA specifies a job ring to associate with this context,
    // either the driver will choose a job ring in round robin fashion.
    if(job_ring == NULL)
    {
        /* Implement a round-robin assignment of JRs to this context */
        g_last_jr_assigned = SEC_CIRCULAR_COUNTER(g_last_jr_assigned, g_job_rings_no);
        job_ring = &g_job_rings[g_last_jr_assigned];
    }

    // Try to get a free context from the JR's pool. Run garbage collector for this JR.
    //
    // The advantage of the JR's pool is that it needs NO synchronization mechanisms,
    // because only one thread accesses it: the producer thread.
    // However, if the JR's pool is full even after running the garbage collector,
    // try to get a free context from the global pool. The disadvantage of the
    // global pool is that the access to it needs to be synchronized because it can
    // be accessed simultaneously by 2 threads (the producer thread of JR1 and the
    // producer thread for JR2).
    if((ctx = get_free_context(&job_ring->ctx_pool)) == NULL)
    {
    	// get free context from the global pool of contexts (with lock)
        if((ctx = get_free_context(&g_ctx_pool)) == NULL)
        {
			// no free contexts in the global pool
			return SEC_DRIVER_NO_FREE_CONTEXTS;
		}
    }

    ASSERT(ctx != NULL);
    ASSERT(ctx->pool != NULL);

    // set the notification callback per context
    ctx->notify_packet_cbk = sec_ctx_info->notify_packet;
    // Set the JR handle.
    ctx->jr_handle = (sec_job_ring_handle_t)job_ring;
    // Set the crypto info
    ret = sec_pdcp_context_set_crypto_info(ctx, sec_ctx_info);
    if(ret != SEC_SUCCESS)
    {
        SEC_ERROR("sec_ctx_info contains invalid data");
        free_or_retire_context(ctx->pool, ctx);
        return SEC_INVALID_INPUT_PARAM;
    }
#ifdef SEC_HW_VERSION_3_1
    // On SEC 3.1 all packets belonging to PDCP control-plane conetxts that have configured both
    // integrity check and confidentiality algorithms will be double-passed through SEC engine.
    ctx->double_pass = (ctx->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE) &&
                       ((ctx->pdcp_crypto_info->integrity_algorithm != SEC_ALG_NULL) &&
                        (ctx->pdcp_crypto_info->cipher_algorithm != SEC_ALG_NULL));

    ctx->is_null_algo = FALSE;

    if((ctx->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE &&
        ctx->pdcp_crypto_info->integrity_algorithm == SEC_ALG_NULL &&
        ctx->pdcp_crypto_info->cipher_algorithm == SEC_ALG_NULL) ||
       (ctx->pdcp_crypto_info->user_plane == PDCP_DATA_PLANE &&
        ctx->pdcp_crypto_info->cipher_algorithm == SEC_ALG_NULL))
    {
        ctx->is_null_algo = TRUE;
        SEC_DEBUG("Jr[%p].Context %p user plane = %d configured with NULL algorithm",
                  job_ring, ctx, ctx->pdcp_crypto_info->user_plane);
    }
#endif
    // provide to UA a SEC ctx handle
	*sec_ctx_handle = (sec_context_handle_t)ctx;
    return SEC_SUCCESS;
}

sec_return_code_t sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle)
{
	sec_contexts_pool_t * pool = NULL;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");

    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;

    // Validate that context handle contains valid bit patterns
    SEC_ASSERT(COND_EXPR1_EQ_AND_EXPR2_EQ(sec_context->start_pattern,
                                          CONTEXT_VALIDATION_PATTERN,
                                          sec_context->end_pattern,
                                          CONTEXT_VALIDATION_PATTERN),
               SEC_INVALID_INPUT_PARAM,
               "sec_ctx_handle is invalid");

    pool = sec_context->pool;
    ASSERT (pool != NULL);

    // Now try to free the current context. If there are packets
    // in flight the context will be retired (not freed). The context
    // will be freed in the next garbage collector call.
    return free_or_retire_context(pool, sec_context);
}

sec_return_code_t sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no)
{
    sec_job_ring_t * job_ring = NULL;
    uint32_t notified_packets_no = 0;
    uint32_t notified_packets_no_per_jr = 0;
    int32_t packets_left_to_notify = 0;
    int32_t i = 0;
    int32_t ret = SEC_SUCCESS;
    int32_t no_more_packets_on_jrs = 0;
    int32_t jr_limit = 0; // limit computed per JR

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments

    // - A limit equal with zero is considered invalid because it makes no sense to call
    // sec_poll and request for no notification.
    // - A weight of zero is considered invalid because the weighted round robing algorithm
    // used for polling the JRs needs a weight != 0.
    // To skip the round robin algorithm, UA can call sec_poll_per_jr for each JR and thus
    // implement its own algorithm.
    // - A limit smaller or equal than weight is considered invalid.
    SEC_ASSERT(!(limit == 0 || weight == 0 || (limit <= weight) || (weight > SEC_JOB_RING_HW_SIZE)),
               SEC_INVALID_INPUT_PARAM,
               "Invalid limit/weight parameter configuration");

    // Poll job rings
    // If limit < 0 -> poll JRs in round robin fashion until no more notifications are available.
    // If limit > 0 -> poll JRs in round robin fashion until limit is reached.

    // Exit from while if one of the following is true:
    //  - the required number of notifications were raised to UA.
    //  - there are no more done jobs on either of the available JRs.
    while ((notified_packets_no != limit) &&
            no_more_packets_on_jrs != 0)
    {
        no_more_packets_on_jrs = 0;
        for (i = 0; i < g_job_rings_no; i++)
        {
            job_ring = &g_job_rings[i];

            // Compute the limit for this JR
            // If there are less packets to notify than configured weight,
            // then notify the smaller number of packets.

            // how many packets do we have until reaching the budget?
            packets_left_to_notify = (limit > 0) ? (limit - notified_packets_no) : limit;
            // calculate budget per job ring
            jr_limit = (packets_left_to_notify < weight) ? packets_left_to_notify  : weight ;

            // Poll one JR
            ret = hw_poll_job_ring(job_ring, jr_limit, &notified_packets_no_per_jr);
            SEC_ASSERT(ret == SEC_SUCCESS, ret, "Error polling SEC engine job ring with id %d", job_ring->jr_id);

            // Update flag used to identify if there are no more notifications
            // in either of the available JRs.
            no_more_packets_on_jrs |= notified_packets_no_per_jr;

            // Update total number of packets notified to UA
            notified_packets_no += notified_packets_no_per_jr;
            if (notified_packets_no == limit)
            {
                break;
                // exit for loop with notified_packets_no == limit -> while loop will exit too
            }

        }
    }

    if (packets_no != NULL)
    {
        *packets_no = notified_packets_no;
    }

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_NAPI
    if (limit < 0)
    {
        enable_irq();
    }
    else if (notified_packets_no < limit)
    {
        enable_irq();
    }
#endif

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_IRQ
    // Always enable IRQ generation when in pure IRQ mode
    enable_irq();
#endif
    return SEC_SUCCESS;
}

sec_return_code_t sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle, 
                                    int32_t limit, 
                                    uint32_t *packets_no)
{
    int ret = SEC_SUCCESS;
    uint32_t notified_packets_no = 0;
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(job_ring != NULL, SEC_INVALID_INPUT_PARAM, "job_ring_handle is NULL");
    SEC_ASSERT(!((limit == 0) || (limit > SEC_JOB_RING_HW_SIZE)),
               SEC_INVALID_INPUT_PARAM,
               "Invalid limit parameter configuration");

    // Poll job ring
    // If limit < 0 -> poll JR until no more notifications are available.
    // If limit > 0 -> poll JR until limit is reached.

    // Run hw poll job ring
    ret = hw_poll_job_ring(job_ring, limit, &notified_packets_no);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Error polling SEC engine job ring with id %d", job_ring->jr_id);

    if (packets_no != NULL)
    {
        *packets_no = notified_packets_no;
    }

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_NAPI
    if (limit < 0)
    {
        uio_job_ring_enable_irqs(job_ring);
    }
    else if (notified_packets_no < limit)
    {
        uio_job_ring_enable_irqs(job_ring);
    }
#endif

#if SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_IRQ
    // Always enable IRQ generation when in pure IRQ mode
    uio_job_ring_enable_irqs(job_ring);

#endif
    return SEC_SUCCESS;
}

sec_return_code_t sec_process_packet(sec_context_handle_t sec_ctx_handle,
                                     const sec_packet_t *in_packet,
                                     const sec_packet_t *out_packet,
                                     ua_context_handle_t ua_ctx_handle)
{
    int ret = SEC_SUCCESS;
    sec_job_t *job = NULL;
    sec_job_ring_t *job_ring = NULL;

#if FSL_SEC_ENABLE_SCATTER_GATHER == ON
#error "Scatter/Gather support is not implemented!"
#endif

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ? 
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITALIZED,
               "Driver release is in progress or driver not initialized");


    // Validate input arguments
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");
    SEC_ASSERT(in_packet != NULL, SEC_INVALID_INPUT_PARAM, "in_packet is NULL");
    SEC_ASSERT(out_packet != NULL, SEC_INVALID_INPUT_PARAM, "out_packet is NULL");

    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;

    // Validate that context handle contains valid bit patterns
    SEC_ASSERT(COND_EXPR1_EQ_AND_EXPR2_EQ(sec_context->start_pattern,
                                          CONTEXT_VALIDATION_PATTERN,
                                          sec_context->end_pattern,
                                          CONTEXT_VALIDATION_PATTERN),
               SEC_INVALID_INPUT_PARAM,
               "sec_ctx_handle is invalid");

    SEC_ASSERT(!(sec_context->state == SEC_CONTEXT_RETIRING),
               SEC_CONTEXT_MARKED_FOR_DELETION,
               "SEC context is marked for deletion. "
               "Wait until all in-fligh packets are processed.");
    ASSERT(sec_context->state != SEC_CONTEXT_UNUSED);

    job_ring = (sec_job_ring_t *)sec_context->jr_handle;
    ASSERT(job_ring != NULL);

    // Check job ring state
    SEC_ASSERT(job_ring->jr_state == SEC_JOB_RING_STATE_STARTED,
               SEC_JOB_RING_RESET_IN_PROGRESS,
               "Job ring with id %d is currently resetting. "
               "Can use it again after reset is over(when sec_poll function/s return)", job_ring->jr_id);
#ifdef SEC_HW_VERSION_3_1
    // Check control-plane internal queue for packets that need be sent to SEC
    // for second processing step.
    ret = sec_process_pdcp_c_plane_packets(job_ring);
    SEC_ASSERT_CONT(ret == SEC_SUCCESS,
                    "sec_process_pdcp_c_plane_packets returned error code %d", ret);
#endif
    // check if the Job Ring is full
    if(SEC_JOB_RING_IS_FULL(job_ring->pidx, job_ring->cidx,
                            SEC_JOB_RING_SIZE, SEC_JOB_RING_HW_SIZE + 1))
    {
        SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Job Ring is full.",
                  job_ring, job_ring->pidx, job_ring->cidx);
        return SEC_JR_IS_FULL;
    }

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Before sending packet",
              job_ring, job_ring->pidx, job_ring->cidx);

    // get first available job from job ring
    job = &job_ring->jobs[job_ring->pidx];

    // update job with crypto context and in/out packet data
    job->in_packet = in_packet;
    job->out_packet = out_packet;

    job->sec_context = sec_context;
    job->ua_handle = ua_ctx_handle;
#ifdef SEC_HW_VERSION_3_1
    job->is_integrity_algo = (sec_context->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE) && 
        ((sec_context->double_pass == FALSE && sec_context->pdcp_crypto_info->integrity_algorithm != SEC_ALG_NULL) ||
         (sec_context->double_pass == TRUE && sec_context->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION) );
    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].in packet[%p].packet double-pass[%d].is_integrity_algo[%d]",
              job_ring, job_ring->pidx, job_ring->cidx,
              job->in_packet, sec_context->double_pass,
              job->is_integrity_algo);
#else
    job->is_integrity_algo = (sec_context->pdcp_crypto_info->user_plane == PDCP_CONTROL_PLANE) &&
                             (sec_context->pdcp_crypto_info->protocol_direction == PDCP_ENCAPSULATION);
    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].in packet[%p].is_integrity_algo[%d]",
              job_ring, job_ring->pidx, job_ring->cidx,
              job->in_packet,job->is_integrity_algo);
#endif

    // update descriptor with pointers to input/output data and pointers to crypto information
    ASSERT(job->descr != NULL);

    // Update SEC descriptor. If HFN reached the configured threshold, then
    // the User App callback will be raised with status set to SEC_STATUS_HFN_THRESHOLD_REACHED.
    ret = sec_pdcp_context_update_descriptor(sec_context, job, job->descr, job->is_integrity_algo);
    SEC_ASSERT(ret == SEC_SUCCESS, ret,
               "sec_pdcp_context_update_descriptor returned error code %d", ret);

    // keep count of submitted packets for this sec context 
    CONTEXT_ADD_PACKET(sec_context);

    // increment the producer index for the current job ring
    job_ring->pidx = SEC_CIRCULAR_COUNTER(job_ring->pidx, SEC_JOB_RING_SIZE);


    // Enqueue this descriptor into the Fetch FIFO of this JR
    // Need write memory barrier here so that descriptor is written before
    // packet is enqueued in SEC's job ring. 
    // GCC's builtins offers full memory barrier, use it here.

    atomic_synchronize();

    // On SEC 3.1 there is no hardware support for NULL-crypto and NULL-auth.
    // Need to simulate this with memcpy.
    // TODO: remove this 'if'statement on 9132(SEC 4.1 knows about NULL-algo).
#ifdef SEC_HW_VERSION_3_1
    if(sec_context->is_null_algo == FALSE)
    {
        hw_enqueue_packet_on_job_ring(job_ring, job->descr_phys_addr);
    }
    else
    {
        ret = sec_process_packet_with_null_algo(job);
        SEC_ASSERT(ret == SEC_SUCCESS, ret,
               "Failed to process packet with NULL crypto / "
               "NULL authentication algorithms");
    }
#else // SEC_HW_VERSION_3_1
    hw_enqueue_packet_on_job_ring(job_ring, job->descr_phys_addr);
#endif

    return SEC_SUCCESS;
}

uint32_t sec_get_last_error(void)
{
    void * ret = NULL;

    ret = pthread_getspecific(g_sec_errno);
    SEC_ASSERT(ret != NULL, 0, "Using non initialized pthread key for local errno!");
    return *(uint32_t*)ret;
}

const char* sec_get_status_message(sec_status_t status)
{
    SEC_ASSERT(status < SEC_STATUS_MAX_VALUE,
               g_status_string[SEC_STATUS_MAX_VALUE],
               "Provided invalid value for status argument");
    return g_status_string[status];
}

const char* sec_get_error_message(sec_return_code_t return_code)
{
    SEC_ASSERT(return_code < SEC_RETURN_CODE_MAX_VALUE,
               g_return_code_descriptions[SEC_RETURN_CODE_MAX_VALUE],
               "Provided invalid value for return_code argument");
    return g_return_code_descriptions[return_code];
}


/*================================================================================================*/

#ifdef __cplusplus
}
#endif
