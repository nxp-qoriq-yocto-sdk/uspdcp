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
#include "sec_pdcp.h"
#include "sec_rlc.h"
#include "sec_hw_specific.h"
#if (SEC_ENABLE_SCATTER_GATHER == ON)
#include "sec_sg_utils.h"
#endif // (SEC_ENABLE_SCATTER_GATHER == ON)
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
            int __temp_sec_driver_c_86 = pthread_key_delete(g_sec_errno); \
            __temp_sec_driver_c_86 = __temp_sec_driver_c_86; \
            SEC_ASSERT(__temp_sec_driver_c_86 == 0, \
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

/** Forward structure declaration */
typedef struct sec_job_t sec_job_t;

/** Forward structure declaration */
typedef struct sec_descriptor_t sec_descriptor_t;
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
    {"SEC_JR_IS_FULL"},
    {"SEC_DRIVER_RELEASE_IN_PROGRESS"},
    {"SEC_DRIVER_ALREADY_INITIALIZED"},
    {"SEC_DRIVER_NOT_INITIALIZED"},
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

/** Global function for virtual to physical translation for the internal
 * SEC driver structures
 */
sec_vtop g_sec_vtop = NULL;

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
 * @param [in]  job_ring            The job ring to poll.
 * @param [in]  limit               The maximum number of jobs to notify.
 *                                  If set to negative value, all available jobs are notified.
 * @param [out] packets_no          No of jobs notified to UA.
 * @param [out] stop_processing     Is set to #TRUE if User App callback returned #SEC_RETURN_STOP
 *
 * @retval 0 for success
 * @retval other value for error
 */
static uint32_t hw_poll_job_ring(sec_job_ring_t *job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no,
                                 int *stop_processing);

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

/** @brief Updates a SEC job descriptor for each packet with pointers to
 * input packet, output packet and crypto information.
 *
 * @param [in]     ctx          SEC context
 * @param [in,out] job          The job structure
 * @param [in,out] descriptor   SEC descriptor
 *
 * @retval SEC_SUCCESS for success
 * @retval other for error
 */
static int sec_update_job_descriptor(sec_context_t *ctx,
                                     sec_job_t *job,
                                     sec_descriptor_t *descriptor);
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
    dma_addr_t  current_desc = 0;

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Flushing jr id %d.packet status[%d]."
              "SEC error code[0x%x]. notify packet=[%d]",
              job_ring, job_ring->pidx, job_ring->cidx,
              job_ring->jr_id, status, error_code, do_notify);

    number_of_jobs_available = hw_get_no_finished_jobs(job_ring);

    // Discard all jobs
    jobs_no_to_discard = number_of_jobs_available;

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Discarding %d packets",
              job_ring, job_ring->pidx, job_ring->cidx, jobs_no_to_discard);

    // If function was called because an error happened on the job ring,
    // then most likely the job ring is halted and it will not accept any more jobs.
    // Still, used a job ring state to reflect that job ring is in flush/reset process.
    // This way the UA will not be able to submit new jobs until reset/flush is over.
    while(jobs_no_to_discard > discarded_packets_no)
    {
        // get the first un-notified job from the job ring

        current_desc = job_ring->output_ring[job_ring->cidx].desc;
        
        /* Since the memory is continous, then P2V translation is a mere addition to
           the base descriptor physical address */
        current_desc = (current_desc - job_ring->jobs[0].descr_phys_addr) >> 6;

        job = (job_ring->descriptors + current_desc)->job_ptr;
        SEC_ASSERT_RET_VOID( job != NULL,"Job ring retrieved from descriptor is NULL");

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

        hw_remove_one_entry(job_ring);

        if(do_notify == TRUE)
        {
            new_status = status;
            // If context is retiring, set a suggestive status for the packets notified to UA.
            // Doesn't matter if the status is overwritten here, if UA deleted the context,
            // it does not care about any other packet status.
            if(sec_context->state == SEC_CONTEXT_RETIRING)
            {
                SEC_DEBUG("Retiring context: 0x%x, pkts: %d",(uint32_t)job->sec_context,CONTEXT_GET_PACKETS_NO(sec_context));
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
                                 uint32_t *packets_no,
                                 int *stop_processing)
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
    uint32_t do_driver_shutdown = FALSE;

    dma_addr_t current_desc;

    /* check here if any JR error that cannot be written
    * in the output status word has occurred
    */
    sec_error_code = hw_job_ring_error(job_ring);
    if (unlikely(sec_error_code))
    {
        // Set errno value to the error code returned by SEC engine.
        // Errno value is thread-local.
        SEC_ERRNO_SET(sec_error_code);
        return SEC_PROCESSING_ERROR;
    }

    // Compute the number of notifications that need to be raised to UA
    // If limit < 0 -> notify all done jobs
    // If limit > total number of done jobs -> notify all done jobs
    // If limit = 0 -> error
    // If limit > 0 && limit < total number of done jobs -> notify a number of done jobs equal with limit

    // compute the number of jobs available in the job ring based on the
    // producer and consumer index values.
    number_of_jobs_available = hw_get_no_finished_jobs(job_ring);

    jobs_no_to_notify = (limit < 0 || limit > number_of_jobs_available) ? number_of_jobs_available : limit;

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Jobs submitted %d.Jobs to notify %d",
              job_ring, job_ring->pidx, job_ring->cidx,
              number_of_jobs_available, jobs_no_to_notify);

    while(jobs_no_to_notify > notified_packets_no)
    {
        status = SEC_STATUS_SUCCESS;

        /* Get job status here */
        sec_error_code = job_ring->output_ring[job_ring->cidx].status;

        /* Get completed descriptor */
        /* Since the memory is contigous, then P2V translation is a mere addition to
           the base descriptor physical address */
        current_desc = (job_ring->output_ring[job_ring->cidx].desc - 
                        job_ring->descriptors_base_addr) >> 6;

        job = (job_ring->descriptors + current_desc)->job_ptr;

        SEC_ASSERT (job != NULL, SEC_PROCESSING_ERROR,
                    "Job ring retrieved from descriptor is NULL");

        // Test here for HFN gte HFN treshold
        // If so, this is not an error, signal to upper layer
        if( HFN_THRESHOLD_MATCH(sec_error_code) )
        {
            sec_error_code = 0;
            status = SEC_STATUS_HFN_THRESHOLD_REACHED;
            SEC_INFO("SEC context[%p] in pkt[%p] out pkt[%p]."
                          "HFN threshold reached.",
                          job->sec_context, job->in_packet, job->out_packet);
        }

        /* 
         * Test here for ICV check fail
         * If so, this is not an error, signal to upper layer
         */
        if( ICV_CHECK_FAIL(sec_error_code) )
        {
            sec_error_code = 0;
            status = SEC_STATUS_MAC_I_CHECK_FAILED;
            SEC_ERROR("SEC context[%p] in pkt[%p] out pkt[%p]."
                          "Integrity check FAILED!.",
                          job->sec_context, job->in_packet, job->out_packet);
        }

        {
            if (sec_error_code)
            {
                // Set errno value to the error code returned by SEC engine.
                // Errno value is thread-local.
                SEC_ERRNO_SET(sec_error_code);

                SEC_ERROR("Packet at cidx %d generated error 0x%x on job ring with id %d\n",
                          job_ring->cidx, sec_error_code, job_ring->jr_id);

                // flush jobs from JR, with error code set to callback
                sec_handle_packet_error(job_ring, sec_error_code, &error_packets_no, &do_driver_shutdown);

                notified_packets_no += error_packets_no;
                *packets_no = notified_packets_no;

                return ((do_driver_shutdown == TRUE) ? SEC_PROCESSING_ERROR : SEC_PACKET_PROCESSING_ERROR);
            }
        }

        sec_context = job->sec_context;

        // copy into a temporary job the fields from the job we need to raise callback
        // this is done to free the slot before the callback is called,
        // which we cannot control in terms of how much processing it will do.
        saved_job.in_packet = job->in_packet;
        saved_job.out_packet = job->out_packet;
        saved_job.ua_handle = job->ua_handle;

        // now increment the consumer index for the current job ring,
        // AFTER saving job in temporary location!
        job_ring->cidx = SEC_CIRCULAR_COUNTER(job_ring->cidx, SEC_JOB_RING_SIZE);

        /* Signal that the job has been processed and the slot is free */
        hw_remove_one_entry(job_ring);

        // If context is retiring, set a suggestive status for the packets notified to UA.
        // Doesn't matter if the status is overwritten here, if UA deleted the context,
        // it does not care about any other packet status.
        if(sec_context->state == SEC_CONTEXT_RETIRING)
        {
            SEC_DEBUG("Retiring context: 0x%x, pkts: %d",(uint32_t)job->sec_context,CONTEXT_GET_PACKETS_NO(sec_context));
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
            *stop_processing = TRUE;
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
        // with producer index, then we have packets to flush.
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

static void sec_handle_packet_error(sec_job_ring_t *job_ring,
                                    uint32_t sec_error_code,
                                    uint32_t *notified_packets,
                                    uint32_t *do_driver_shutdown)
{
    uint32_t reset_cont_job_ring = FALSE;

    ASSERT(notified_packets != NULL);

    // Job ring will be restarted, change its state so that
    // no new packets can be submitted by UA on this job ring.
    job_ring->jr_state = SEC_JOB_RING_STATE_RESET;
    *do_driver_shutdown = FALSE;

    // Analyze the SEC error on this job ring
    // and see if a job ring reset is required.
    hw_handle_job_ring_error(job_ring, sec_error_code, &reset_cont_job_ring);

    // Flush the channel no matter the error type.
    // Notify all submitted jobs to UA, setting corresponding error cause
    hw_flush_job_ring(job_ring,
                      TRUE, // notify packets to UA
                      SEC_STATUS_ERROR, // the status to be set for each packet when notified to UA
                      sec_error_code,
                      notified_packets);
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
               SEC_DRIVER_ALREADY_INITIALIZED,
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
    SEC_ASSERT ((dma_addr_t)sec_config_data->memory_area % L1_CACHE_BYTES == 0,
                SEC_INVALID_INPUT_PARAM,
                "Configured memory is not cacheline aligned");

    // Must check the user passed a valid function for V2P translation.
    SEC_ASSERT (sec_config_data->sec_drv_vtop != NULL,
                SEC_INVALID_INPUT_PARAM,
                "A valid V2P function is required for internal memory.");

    // Update V2P function
    g_sec_vtop = sec_config_data->sec_drv_vtop;

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
    SEC_INFO("Using DMA memory area with start address = %p\n", g_dma_mem_start);

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
                                 MAX_SEC_CONTEXTS_PER_POOL,
                                 &g_dma_mem_free,
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
        ret = init_job_ring(&g_job_rings[i], &g_dma_mem_free, sec_config_data->work_mode
                , sec_config_data->irq_coalescing_timer & 0xFFFF
                , sec_config_data->irq_coalescing_count & 0xFF
                );
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
                             MAX_SEC_CONTEXTS_PER_POOL,
                             &g_dma_mem_free,
                             THREAD_SAFE_POOL);
    if (ret != SEC_SUCCESS)
    {
        SEC_ERROR("Failed to initialize global pool of SEC contexts. "
                "Starting sec_release()");
        sec_release();
        g_driver_state = SEC_DRIVER_STATE_IDLE;
        return ret;
    }

    /* Now check if I've overrun the memory 'segment' allocated by the UA
     * TODO: Add some tests for this
     */
    if(((dma_addr_t)g_dma_mem_free - (dma_addr_t)g_dma_mem_start) < SEC_DMA_MEMORY_SIZE)
    {
        SEC_INFO("Allocated %u KB for SEC driver, remaining free %u KB",
                ((dma_addr_t)g_dma_mem_free - (dma_addr_t)g_dma_mem_start)/1024,
                (SEC_DMA_MEMORY_SIZE - ((dma_addr_t)(dma_addr_t)g_dma_mem_free - (dma_addr_t)g_dma_mem_start))/1024);
    }
    else
    {
        SEC_ERROR("Overrun the memory allocated for SEC driver! (requested: %u KB, configured %u KB)",
                  ((dma_addr_t)g_dma_mem_free - (dma_addr_t)g_dma_mem_start)/1024,
                  SEC_DMA_MEMORY_SIZE/1024);

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
    SEC_ASSERT(!(g_driver_state == SEC_DRIVER_STATE_RELEASE),
               SEC_DRIVER_RELEASE_IN_PROGRESS,
               "Driver release is already in progress");

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

static inline sec_return_code_t create_context(sec_context_t **ctx,
                                               sec_job_ring_handle_t job_ring_handle)
{
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;

    // Make sure a valid pointer is given
    ASSERT(ctx != NULL);

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ?
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITIALIZED,
               "Driver release is in progress or driver not initialized");

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
    if((*ctx = get_free_context(&job_ring->ctx_pool)) == NULL)
    {
        // get free context from the global pool of contexts (with lock)
        if((*ctx = get_free_context(&g_ctx_pool)) == NULL)
        {
                // no free contexts in the global pool
                 return SEC_DRIVER_NO_FREE_CONTEXTS;
        }
    }

    ASSERT(*ctx != NULL);
    ASSERT((*ctx)->pool != NULL);

    // Set the JR handle.
    (*ctx)->jr_handle = (sec_job_ring_handle_t)job_ring;

    return SEC_SUCCESS;
}


sec_return_code_t sec_create_rlc_context(sec_job_ring_handle_t job_ring_handle,
                                         const sec_rlc_context_info_t *rlc_ctx_nfo,
                                         sec_context_handle_t *sec_ctx_handle)
{
    int ret = SEC_SUCCESS;
    sec_context_t *ctx = NULL;
    
    // Validate input arguments
    SEC_ASSERT(rlc_ctx_nfo != NULL, SEC_INVALID_INPUT_PARAM, "rlc_ctx_nfo is NULL");
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");
    SEC_ASSERT(rlc_ctx_nfo->notify_packet != NULL,
               SEC_INVALID_INPUT_PARAM,
               "rlc_ctx_nfo has NULL notify_packet function pointer");
    SEC_ASSERT(rlc_ctx_nfo->cipher_key != NULL,
               SEC_INVALID_INPUT_PARAM,
               "rlc_ctx_nfo->cipher_key is NULL");

    // Crypto keys must come from DMA memory area and must be cacheline aligned
    SEC_ASSERT((dma_addr_t)rlc_ctx_nfo->cipher_key % L1_CACHE_BYTES == 0,
               SEC_INVALID_INPUT_PARAM,
               "Configured crypto key is not cacheline aligned");
#ifdef SEC_RRC_PROCESSING
    if(rlc_ctx_nfo->integrity_key != NULL)
    {
        // Authentication keys must come from DMA memory area and must be cacheline aligned
        SEC_ASSERT((dma_addr_t)rlc_ctx_nfo->integrity_key % L1_CACHE_BYTES == 0,
                   SEC_INVALID_INPUT_PARAM,
                   "Configured integrity key is not cacheline aligned");
    }
#endif // SEC_RRC_PROCESSING
    ret = create_context(&ctx, job_ring_handle);
    if(ret != SEC_SUCCESS)
    {
        // create_context will return either SEC_SUCCESS or SEC_DRIVER_NO_FREE_CONTEXTS
        return ret;
    }
    
    // Set the crypto info
    ret = sec_rlc_context_set_crypto_info(ctx, rlc_ctx_nfo);
    if(ret != SEC_SUCCESS)
    {
        SEC_ERROR("rlc_ctx_nfo contains invalid data");
        free_or_retire_context(ctx->pool, ctx);
        return SEC_INVALID_INPUT_PARAM;
    }

    if (rlc_ctx_nfo->hfn_ov_en == TRUE)
    {
        ctx->dpovrd_en = TRUE;
        SEC_DEBUG("Jr[%p].Context %p configured for HFN override",
                  job_ring_handle, ctx);
    }

    // set the notification callback per context
    ctx->notify_packet_cbk = rlc_ctx_nfo->notify_packet;

    // provide to UA a SEC ctx handle
    *sec_ctx_handle = (sec_context_handle_t)ctx;

    return SEC_SUCCESS;
}

sec_return_code_t sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                                           const sec_pdcp_context_info_t *pdcp_ctx_info,
                                           sec_context_handle_t *sec_ctx_handle)
{
    int ret = SEC_SUCCESS;
    sec_context_t *ctx = NULL;
    
    // Validate input arguments
    SEC_ASSERT(pdcp_ctx_info != NULL, SEC_INVALID_INPUT_PARAM, "pdcp_ctx_info is NULL");
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");
    SEC_ASSERT(pdcp_ctx_info->notify_packet != NULL,
               SEC_INVALID_INPUT_PARAM,
               "pdcp_ctx_info has NULL notify_packet function pointer");
    SEC_ASSERT(pdcp_ctx_info->cipher_key != NULL,
               SEC_INVALID_INPUT_PARAM,
               "pdcp_ctx_info->cipher_key is NULL");

    // Crypto keys must come from DMA memory area and must be cacheline aligned
    SEC_ASSERT((dma_addr_t)pdcp_ctx_info->cipher_key % L1_CACHE_BYTES == 0,
               SEC_INVALID_INPUT_PARAM,
               "Configured crypto key is not cacheline aligned");

    if(pdcp_ctx_info->integrity_key != NULL)
    {
        // Authentication keys must come from DMA memory area and must be cacheline aligned
        SEC_ASSERT((dma_addr_t)pdcp_ctx_info->integrity_key % L1_CACHE_BYTES == 0,
                   SEC_INVALID_INPUT_PARAM,
                   "Configured integrity key is not cacheline aligned");
    }

    ret = create_context(&ctx, job_ring_handle);
    if(ret != SEC_SUCCESS)
    {
        // create_context will return either SEC_SUCCESS or SEC_DRIVER_NO_FREE_CONTEXTS
        return ret;
    }
    
    // Set the crypto info
    ret = sec_pdcp_context_set_crypto_info(ctx, pdcp_ctx_info);
    if(ret != SEC_SUCCESS)
    {
        SEC_ERROR("pdcp_ctx_info contains invalid data");
        free_or_retire_context(ctx->pool, ctx);
        return SEC_INVALID_INPUT_PARAM;
    }

    if (pdcp_ctx_info->hfn_ov_en == TRUE)
    {
        ctx->dpovrd_en = TRUE;
        SEC_DEBUG("Jr[%p].Context %p configured for HFN override",
                  job_ring_handle, ctx);
    }

    // set the notification callback per context
    ctx->notify_packet_cbk = pdcp_ctx_info->notify_packet;

    // provide to UA a SEC ctx handle
    *sec_ctx_handle = (sec_context_handle_t)ctx;

    return SEC_SUCCESS;
}

sec_return_code_t sec_delete_rlc_context(sec_context_handle_t sec_ctx_handle)
{
    return sec_delete_pdcp_context (sec_ctx_handle);
}

sec_return_code_t sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle)
{
    sec_contexts_pool_t * pool = NULL;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ?
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITIALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");

    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;
    SEC_ASSERT(sec_context != NULL, SEC_INVALID_INPUT_PARAM, "sec_context is NULL");

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
    int stop_processing = FALSE;
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
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITIALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments

    // - A limit equal with zero is considered invalid because it makes no sense to call
    // sec_poll and request for no notification.
    // - A weight of zero is considered invalid because the weighted round robing algorithm
    // used for polling the JRs needs a weight != 0.
    // To skip the round robin algorithm, UA can call sec_poll_per_jr for each JR and thus
    // implement its own algorithm.
    // - A limit smaller or equal than weight is considered invalid.
    SEC_ASSERT(!(limit == 0 || weight == 0 || (limit <= weight) || (weight > SEC_JOB_RING_SIZE)),
               SEC_INVALID_INPUT_PARAM,
               "Invalid limit/weight parameter configuration");

    SEC_DEBUG("Polling. weight [%d] limit[%d]", weight, limit);

    // Poll job rings
    // If limit < 0 -> poll JRs in round robin fashion until no more notifications are available.
    // If limit > 0 -> poll JRs in round robin fashion until limit is reached.

    // Exit from while if one of the following is true:
    //  - the required number of notifications were raised to UA.
    //  - there are no more done jobs on either of the available JRs.

    do
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
            ret = hw_poll_job_ring(job_ring,
                                   jr_limit,
                                   &notified_packets_no_per_jr,
                                   &stop_processing);
            SEC_ASSERT(ret == SEC_SUCCESS, ret, "Error polling SEC engine job ring with id %d", job_ring->jr_id);

            SEC_DEBUG("Jr[%p].Jobs notified[%d]. UA cbk ret STOP[%d]",
                      job_ring, notified_packets_no_per_jr, stop_processing);



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

            if (stop_processing == TRUE)
            {
                // In case User App returned #SEC_RETURN_STOP from packet-handler callback,
                // we must stop polling for packets. Break will end for loop.
                // Flag no_more_packets_on_jrs set to 0 will end while loop.
                no_more_packets_on_jrs = 0;
                break;
            }

        }
    }while (notified_packets_no != limit &&
            no_more_packets_on_jrs != 0);

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
    int stop_processing = FALSE;
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ?
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITIALIZED,
               "Driver release is in progress or driver not initialized");

    // Validate input arguments
    SEC_ASSERT(job_ring != NULL, SEC_INVALID_INPUT_PARAM, "job_ring_handle is NULL");
    SEC_ASSERT(!((limit == 0) || (limit > SEC_JOB_RING_SIZE)),
                   SEC_INVALID_INPUT_PARAM,
                   "Invalid limit parameter configuration");

    SEC_DEBUG("Jr[%p]Polling. limit[%d]", job_ring, limit);

    // Poll job ring
    // If limit < 0 -> poll JR until no more notifications are available.
    // If limit > 0 -> poll JR until limit is reached.

    // Run hw poll job ring
    ret = hw_poll_job_ring(job_ring, limit, &notified_packets_no, &stop_processing);
    SEC_ASSERT(ret == SEC_SUCCESS, ret, "Error polling SEC engine job ring with id %d", job_ring->jr_id);

    SEC_DEBUG("Jr[%p].Jobs notified[%d]. UA cbk ret STOP[%d]",
              job_ring, notified_packets_no, stop_processing);

    if (packets_no != NULL)
    {
        *packets_no = notified_packets_no;
    }

#if (SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_NAPI)
    if (limit < 0)
    {
        uio_job_ring_enable_irqs(job_ring);
    }
    else if (notified_packets_no < limit)
    {
        uio_job_ring_enable_irqs(job_ring);
    }
#endif

#if (SEC_NOTIFICATION_TYPE == SEC_NOTIFICATION_TYPE_IRQ)
    // Always enable IRQ generation when in pure IRQ mode
    uio_job_ring_enable_irqs(job_ring);

#endif
    return SEC_SUCCESS;
}

int sec_update_job_descriptor(sec_context_t *ctx,
                              sec_job_t *job,
                              sec_descriptor_t *descriptor)
{
    dma_addr_t  phys_addr;
    uint32_t    offset = 0;
    uint32_t    length = 0;
    int ret = SEC_SUCCESS;

    SEC_JD_INIT(descriptor);
    SEC_JD_SET_SD(descriptor,
                   job->sec_context->sh_desc_phys,
                   SEC_GET_DESC_LEN(job->sec_context->sh_desc));

    SEC_JD_SET_JOB_PTR(descriptor,job);

#if (SEC_ENABLE_SCATTER_GATHER == ON)
    if( SG_CONTEXT_OUT_TBL_EN(job->sg_ctx ))
    {
        SEC_JD_SET_SG_OUT(descriptor);
        phys_addr = SG_CONTEXT_GET_TBL_OUT_PHY(job->sg_ctx);
        offset = 0;
        length = SG_CONTEXT_GET_LEN_OUT(job->sg_ctx);
    }
    else
#endif // SEC_ENABLE_SCATTER_GATHER == ON
    {
        phys_addr = job->out_packet->address;
        offset = job->out_packet->offset;
        length = job->out_packet->length;
    }

    SEC_JD_SET_OUT_PTR(descriptor,
                        phys_addr,
                        offset,
                        length);

#if (SEC_ENABLE_SCATTER_GATHER == ON)
    if( SG_CONTEXT_IN_TBL_EN(job->sg_ctx ))
    {
        SEC_JD_SET_SG_IN(descriptor);
        phys_addr = SG_CONTEXT_GET_TBL_IN_PHY(job->sg_ctx);
        offset = 0;
        length = SG_CONTEXT_GET_LEN_IN(job->sg_ctx);
    }
    else
#endif // (SEC_ENABLE_SCATTER_GATHER == ON)
    {
        phys_addr = job->in_packet->address;
        offset = job->in_packet->offset;
        length = job->in_packet->length;
    }

    SEC_JD_SET_IN_PTR(descriptor,
                       phys_addr,
                       offset,
                       length);

    if( job->sec_context->dpovrd_en == TRUE)
    {
        descriptor->dpovrd = CMD_DPOVRD_EN | job->dpovrd_value;
    }

    SEC_DUMP_DESC(descriptor);

    SEC_INFO("Update job descriptor descriptor return code = %d", ret);

    return ret;
}
sec_return_code_t sec_process_packet(sec_context_handle_t sec_ctx_handle,
                                     const sec_packet_t *in_packet,
                                     const sec_packet_t *out_packet,
                                     ua_context_handle_t ua_ctx_handle)
{
    return sec_process_packet_hfn_ov(sec_ctx_handle,
                                     in_packet,
                                     out_packet,
                                     0x00,
                                     ua_ctx_handle);
}

sec_return_code_t sec_process_packet_hfn_ov(sec_context_handle_t sec_ctx_handle,
                                            const sec_packet_t *in_packet,
                                            const sec_packet_t *out_packet,
                                            uint32_t hfn_ov_val,
                                            ua_context_handle_t ua_ctx_handle)
{
    int ret = SEC_SUCCESS;
    sec_job_t *job = NULL;
    sec_job_ring_t *job_ring = NULL;

    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ?
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITIALIZED,
               "Driver release is in progress or driver not initialized");


    // Validate input arguments
    SEC_ASSERT(sec_ctx_handle != NULL, SEC_INVALID_INPUT_PARAM, "sec_ctx_handle is NULL");
    SEC_ASSERT(in_packet != NULL, SEC_INVALID_INPUT_PARAM, "in_packet is NULL");
    SEC_ASSERT(out_packet != NULL, SEC_INVALID_INPUT_PARAM, "out_packet is NULL");
    SEC_ASSERT(in_packet->address != 0, SEC_INVALID_INPUT_PARAM, "in_packet->address is 0");
    SEC_ASSERT(out_packet->address != 0, SEC_INVALID_INPUT_PARAM, "out_packet->address is 0");
    SEC_ASSERT(in_packet->length != 0, SEC_INVALID_INPUT_PARAM, "in_packet->length is 0");
    SEC_ASSERT(out_packet->length != 0, SEC_INVALID_INPUT_PARAM, "out_packet->length is 0");
    SEC_ASSERT((in_packet->offset & 0x1FFFFFFF) == in_packet->offset, SEC_INVALID_INPUT_PARAM, "in_packet->offset is invalid")
    SEC_ASSERT((out_packet->offset & 0x1FFFFFFF) == out_packet->offset, SEC_INVALID_INPUT_PARAM, "out_packet->offset is invalid")
#warning "There should be a validation for maximum packet length"
#if (SEC_ENABLE_SCATTER_GATHER == ON)
#warning "Add some more validation here"
    SEC_ASSERT(in_packet->num_fragments < SEC_MAX_SG_TBL_ENTRIES, SEC_INVALID_INPUT_PARAM, "in_packet->num_fragments too large");
    SEC_ASSERT(out_packet->num_fragments < SEC_MAX_SG_TBL_ENTRIES, SEC_INVALID_INPUT_PARAM, "out_packet->num_fragments too large");
#else // (SEC_ENABLE_SCATTER_GATHER == ON)
    SEC_ASSERT(in_packet->num_fragments == 0, SEC_INVALID_INPUT_PARAM, "Please enable Scatter Gather support");
    SEC_ASSERT(out_packet->num_fragments == 0, SEC_INVALID_INPUT_PARAM, "Please enable Scatter Gather support");
#endif // SEC_ENABLE_SCATTER_GATHER == ON

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
               "Do polling until all in-fligh packets are processed.");
    ASSERT(sec_context->state != SEC_CONTEXT_UNUSED);

    job_ring = (sec_job_ring_t *)sec_context->jr_handle;
    ASSERT(job_ring != NULL);

    // Check job ring state
    SEC_ASSERT(job_ring->jr_state == SEC_JOB_RING_STATE_STARTED,
               SEC_JOB_RING_RESET_IN_PROGRESS,
               "Job ring with id %d is currently resetting. "
               "Can use it again after reset is over(when sec_poll function/s return)", job_ring->jr_id);

    if( SEC_JOB_RING_IS_FULL(job_ring->pidx, job_ring->cidx,
                              SEC_JOB_RING_SIZE,SEC_JOB_RING_SIZE ) )
    {
        SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Job Ring is full.",
                          job_ring, job_ring->pidx, job_ring->cidx);
                return SEC_JR_IS_FULL;
    }

    SEC_DEBUG("Jr[%p] pi[%d] ci[%d].Before sending packet",
              job_ring, job_ring->pidx, job_ring->cidx);

    // get first available job from job ring
    job = &job_ring->jobs[job_ring->pidx];
#if (SEC_ENABLE_SCATTER_GATHER == ON)

    ret = build_sg_context(job->sg_ctx,in_packet,SEC_SG_CONTEXT_TYPE_IN,in_packet->num_fragments);
    if( ret != SEC_SUCCESS )
    {
        SEC_ERROR("Error creating Scatter-Gather table for input packet: %s",sec_get_error_message(ret));
        return ret;
    }

    ret = build_sg_context(job->sg_ctx,out_packet,SEC_SG_CONTEXT_TYPE_OUT,out_packet->num_fragments);
    if( ret != SEC_SUCCESS )
    {
        SEC_ERROR("Error creating Scatter-Gather table for output packet: %s",sec_get_error_message(ret));
        return ret;
    }

#endif // (SEC_ENABLE_SCATTER_GATHER == ON)

    // update job with crypto context and in/out packet data
    job->in_packet = in_packet;
    job->out_packet = out_packet;

    job->sec_context = sec_context;
    job->ua_handle = ua_ctx_handle;
    job->dpovrd_value = hfn_ov_val;

    // update descriptor with pointers to input/output data and pointers to crypto information
    ASSERT(job->descr != NULL);

    /* Update SEC Job Descriptor for this packet with the relevant pointers
     * (i.e. Shared Descriptor pointer (per context), in packet address,
     * out packet address, etc.)
     */
    ret = sec_update_job_descriptor(sec_context, job, job->descr);
    SEC_ASSERT(ret == SEC_SUCCESS, ret,
               "sec_update_job_descriptor returned error code %d", ret);

    // keep count of submitted packets for this sec context
    CONTEXT_ADD_PACKET(sec_context);

    // Set ptr in input ring to current descriptor
    job_ring->input_ring[job_ring->pidx] = job->descr_phys_addr;

    // Notify HW that a new job is enqueued
    hw_enqueue_packet_on_job_ring(job_ring);

    // increment the producer index for the current job ring
    job_ring->pidx = SEC_CIRCULAR_COUNTER(job_ring->pidx, SEC_JOB_RING_SIZE);

    return SEC_SUCCESS;
}

int32_t sec_get_last_error(void)
{
    void * ret = NULL;

    ret = pthread_getspecific(g_sec_errno);
    SEC_ASSERT(ret != NULL, -1, "Using non initialized pthread key for local errno!");
    if (ret == NULL)
        return -1;

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


sec_return_code_t sec_get_stats(sec_job_ring_handle_t job_ring_handle,sec_statistics_t* sec_stat)
{
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;
    // Validate driver state
    SEC_ASSERT(g_driver_state == SEC_DRIVER_STATE_STARTED,
               (g_driver_state == SEC_DRIVER_STATE_RELEASE) ?
               SEC_DRIVER_RELEASE_IN_PROGRESS : SEC_DRIVER_NOT_INITIALIZED,
               "Driver release is in progress or driver not initialized");

    SEC_ASSERT(job_ring != NULL, SEC_INVALID_INPUT_PARAM, "job_ring_handle is NULL");
    SEC_ASSERT(sec_stat != NULL, SEC_INVALID_INPUT_PARAM, "sec_stat is NULL");
    
    // Check job ring state
    SEC_ASSERT(job_ring->jr_state == SEC_JOB_RING_STATE_STARTED,
               SEC_JOB_RING_RESET_IN_PROGRESS,
               "Job ring with id %d is currently resetting. "
               "Can use it again after reset is over(when sec_poll function/s return)", job_ring->jr_id);
    
    sec_stat->consumer_index = job_ring->cidx;
    sec_stat->producer_index = job_ring->pidx;
    sec_stat->slots_available = SEC_JOB_RING_NUMBER_OF_ITEMS(SEC_JOB_RING_SIZE,
                                    job_ring->pidx,
                                    job_ring->cidx);
    sec_stat->jobs_waiting_dequeue = (SEC_JOB_RING_SIZE - sec_stat->slots_available) % SEC_JOB_RING_SIZE;

    return SEC_SUCCESS;
}
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
