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

#include <fsl_sec.h>
#include <sec_utils.h>
#include <sec_contexts.h>
// For DTS parsing
#include <of.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/
/** Max sec contexts per pool computed based on the number of Job Rings assigned.
 *  All the available contexts will be split evenly between all the per-job-ring pools. 
 *  There is one additional global pool besides the per-job-ring pools. */
#define MAX_SEC_CONTEXTS_PER_POOL   (SEC_MAX_PDCP_CONTEXTS / (g_job_rings_no))

/** Bit mask for each job ring in a bitfield of format: jr0 | jr1 | jr2 | jr3 */
#define JOB_RING_MASK(jr_id)        (1 << (MAX_SEC_JOB_RINGS - (jr_id) -1 ))

/** Name of UIO device file prefix. Each UIO device will have a device file /dev/uioX, 
 * where X is the minor device number. */
#define SEC_UIO_DEVICE_FILE_NAME    "/dev/uio"

/** Prefix path to sysfs directory where UIO device attributes are exported.
 * Path for UIO device X is /sys/class/uio/uioX */
#define SEC_UIO_DEVICE_SYS_ATTR_PATH    "/sys/class/uio"

/** Maximum length for the name of an UIO device file.
 * Device file name format is: /dev/uioX. */
#define SEC_UIO_MAX_DEVICE_FILE_NAME_LENGTH 30

/** Maximum length for the name of an attribute file for an UIO device.
 * Attribute files are exported in sysfs and have the name formatted as:
 * /sys/class/uio/uioX/<attribute_file_name>
 */
#define SEC_UIO_MAX_ATTR_FILE_NAME  100


/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/* SEC job */
typedef struct sec_job_s
{
    sec_packet_t in_packet;
    sec_packet_t out_packet;
    ua_context_handle_t ua_handle;
    sec_context_t * sec_context;
}sec_job_t;

/* SEC Job Ring */
typedef struct sec_job_ring_s
{
	sec_contexts_pool_t ctx_pool;       /*< Pool of SEC contexts */
    sec_job_t jobs[SEC_JOB_RING_SIZE];  /*< Ring of jobs. In this stub the same ring is used for
                                            input jobs and output jobs. */
    int cidx;                           /*< Consumer index for job ring (jobs array) */
    int pidx;                           /*< Producer index for job ring (jobs array) */
    uint32_t uio_fd;                    /*< The file descriptor used for polling from user space
                                            for interrupts notifications */
    uint32_t jr_id;                     /*< Job ring id */
}sec_job_ring_t;
/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

#define SEC_CIRCULAR_COUNTER(x, max)   ((x) + 1) * ((x) != (max - 1))

// The number of jobs in a JOB RING
#define SEC_JOB_RING_DIFF(ring_max_size, pi, ci) (((pi) < (ci)) ? ((ring_max_size) + (pi) - (ci)) : ((pi) - (ci)))

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/
/* Job rings used for communication with SEC HW */
static sec_job_ring_t g_job_rings[MAX_SEC_JOB_RINGS];
static int g_job_rings_no = 0;

/* Array of job ring handles provided to UA from sec_init() function. */
static sec_job_ring_descriptor_t g_job_ring_handles[MAX_SEC_JOB_RINGS];

static int sec_work_mode = 0;

/* Last JR assigned to a context by the SEC driver using a round robin algorithm.
 * Not used if UA associates the contexts created to a certain JR.*/
static int last_jr_assigned = 0;

// global context pool
static sec_contexts_pool_t g_ctx_pool;
/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/** @brief Initialize the data stored in a ::sec_job_ring_t structure.
 *
 **/
static void reset_job_ring(sec_job_ring_t *job_ring);

/** @brief Enable IRQ generation for all SEC's job rings.
 *
 * @retval 0 for success
 * @retval other value for error
 */
static int enable_irq();

/** @brief Enable IRQ generation for all SEC's job rings.
 *
 * @param [in]  job_ring    The job ring to enable IRQs for.
 * @retval 0 for success
 * @retval other value for error
 */
static int enable_irq_per_job_ring(sec_job_ring_t *job_ring);

/** @brief Reads configuration data from DTS
 *
 * @retval 0 for success
 * @retval other value for error
 */
static int sec_configure(int job_ring_number, sec_job_ring_t *job_rings);

/** @brief Obtains file handle to access UIO device for a specific job ring.
 *  The file handle can be used to: 
 *  - receive job done interrupts(read number of IRQs)
 *  - request SEC engine reset (write #value TODO)
 *
 * @retval 0 for success
 * @retval other value for error
 */
static int sec_config_uio_job_ring(sec_job_ring_t *job_rings);
/** @brief Poll the HW for already processed jobs in the JR
 * and notify the available jobs to UA.
 *
 * @param [in]  job_ring    The job ring to poll.
 * @param [in]  limit       The maximum number of jobs to notify.
 *                          If set to -1, all available jobs are notified.
 * @param [out] packets_no  No of jobs notified to UA.
 *
 * @retval 0 for success
 * @retval other value for error
 */
static uint32_t hw_poll_job_ring(sec_job_ring_t *job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no);

/** @brief Checks if a file name contains a certain substring.
 * If so, it extracts the number following the substring.
 * This function assumes a filename format of: [text][number].
 * @param [in]  filename    File name
 * @param [in]  match       String to match in file name
 * @param [out] number      The number extracted from filename
 *
 * @retval true if file name matches the criteria
 * @retval false if file name does not match the criteria
 */
static bool file_name_match_extract(char filename[], char match[], int *number);
/** @brief Reads first line from a file.
 * Composes file name as: root/subdir/filename
 *
 * @param [in]  root     Root path
 * @param [in]  subdir   Subdirectory name
 * @param [in]  filename File name
 * @param [out] line     The first line read from file.
 *
 * @retval 0 for succes
 * @retval other value for error
 */
static int file_read_first_line(char root[], char subdir[], char filename[], char* line);
/** @brief Finds the UIO device for a certain job ring
 *
 * @param [in]  jr_id           Job ring id
 * @param [out] device_file     UIO device file name
 * @retval true if UIO device found
 * @retval false if UIO device not found
 */
static bool uio_find_device_file(int jr_id, char *device_file);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

static void reset_job_ring(sec_job_ring_t * job_ring)
{
    ASSERT(job_ring != NULL);
    SEC_INFO("job ring %d UIO fd = %d", job_ring->jr_id, job_ring->uio_fd);
    //memset(job_ring, 0, sizeof(sec_job_ring_t));
    close(job_ring->uio_fd);
}

static int enable_irq()
{
    // to be implemented
    return 0;
}

static int enable_irq_per_job_ring(sec_job_ring_t *job_ring)
{
    // to be implemented
    return 0;
}

static uint32_t hw_poll_job_ring(sec_job_ring_t * job_ring,
                                 int32_t limit,
                                 uint32_t *packets_no)
{
    /* Stub Implementation - START */
    sec_context_t * sec_context;
    sec_job_t *job;
    int jobs_no_to_notify = 0; // the number of done jobs to notify to UA
    sec_status_t status = SEC_STATUS_SUCCESS;
    int notified_packets_no = 0;
    int number_of_jobs_available = 0;

    // Compute the number of notifications that need to be raised to UA
    // If limit < 0 -> notify all done jobs
    // If limit > total number of done jobs -> notify all done jobs
    // If limit = 0 -> error
    // If limit > 0 && limit < total number of done jobs -> notify a number of done jobs equal with limit

    // compute the number of jobs available in the job ring based on the
    // producer and consumer index values.
    number_of_jobs_available = SEC_JOB_RING_DIFF(SEC_JOB_RING_SIZE, job_ring->pidx, job_ring->cidx);

    jobs_no_to_notify = (limit < 0 || limit > number_of_jobs_available) ? number_of_jobs_available : limit;

    while(jobs_no_to_notify > notified_packets_no)
    {
        // get the first un-notified job from the job ring
        job = &job_ring->jobs[job_ring->cidx];

        sec_context = job->sec_context;
        ASSERT (sec_context->notify_packet_cbk != NULL);

        pthread_mutex_lock(&sec_context->mutex);

        status = SEC_STATUS_SUCCESS;
        // if context is retiring, set a suggestive status for the
        // packets notified to UA
        if (sec_context->usage == SEC_CONTEXT_RETIRING)
        {
            ASSERT(sec_context->packets_no >= 1);
            status = (sec_context->packets_no > 1) ? SEC_STATUS_OVERDUE : SEC_STATUS_LAST_OVERDUE;
        }
        // call the calback
        sec_context->notify_packet_cbk(&job->in_packet,
                                       &job->out_packet,
                                       job->ua_handle,
                                       status,
                                       0);

        // decrement packet reference count in sec context
        sec_context->packets_no--;

        pthread_mutex_unlock(&sec_context->mutex);

        notified_packets_no++;

        // increment the consumer index for the current job ring
        job_ring->cidx = SEC_CIRCULAR_COUNTER(job_ring->cidx, SEC_JOB_RING_SIZE);
    }

    *packets_no = notified_packets_no;

    return SEC_SUCCESS;
}

static int sec_configure(int job_ring_number, sec_job_ring_t *job_rings)
{
    struct device_node *dpa_node = NULL;
    uint32_t *kernel_usr_channel_map = NULL;
    uint32_t usr_channel_map = 0;
    uint32_t len = 0;
    uint8_t config_jr_no = 0;
    int jr_idx = 0, jr_no = 0;
 
#ifdef SEC_HW_VERSION_3_1
    // Get device node for SEC 3.1
    for_each_compatible_node(dpa_node, NULL, "fsl,sec3.1")
    {
        // If device is disabled from DTS, skip
        if(of_device_is_available(dpa_node) == false)
        {
            continue;
        }

        SEC_INFO("Found SEC 3.1 device node %s", dpa_node->full_name);

        // Read from DTS job ring distribution map between user space and kernel space.
        // If bit is set, job ring belongs to kernel space.
        // Bitfield: jr0 | jr1 | jr2 | jr3
        kernel_usr_channel_map = of_get_property(dpa_node, "fsl,channel-kernel-user-space-map", &len);
        if(kernel_usr_channel_map == NULL)
        {
            SEC_ERROR("Error reading <fsl,channel-kernel-user-space-map> property from DTS!");
            return SEC_INVALID_INPUT_PARAM;
        }
        SEC_INFO("Success reading <fsl,channel-kernel-user-space-map> = %x", *kernel_usr_channel_map);
        usr_channel_map = ~(*kernel_usr_channel_map);
      
#endif // SEC_HW_VERSION_3_1

        // Calculate the number of available job rings for user space usage.
        config_jr_no = SEC_NUMBER_JOB_RINGS(usr_channel_map);
        if(config_jr_no == 0)
        {
            SEC_ERROR("Configuration ERROR! No SEC Job Rings assigned for userspace usage!");
            return SEC_INVALID_INPUT_PARAM;
        }

        if(config_jr_no < job_ring_number)
        {
            SEC_ERROR("Number of available job rings configured from DTS (%d) "
                    "is less than requested number (%d)",
                    config_jr_no, job_ring_number);
            return SEC_INVALID_INPUT_PARAM;
        }

        // Use and configure requested number of job rings from the total
        // user space available job rings.
        jr_idx = 0;
        jr_no = 0;
        do
        {
            if(usr_channel_map & JOB_RING_MASK(jr_idx))
            {
                SEC_INFO("Using Job Ring number %d", jr_idx);
                job_rings[jr_no].jr_id = jr_idx;
                jr_no++;
                if (jr_no == job_ring_number)
                {
                    break;
                }
            }
            jr_idx++;

        }while(jr_idx < MAX_SEC_JOB_RINGS);
    }
    return SEC_SUCCESS; 
}

static int sec_config_uio_job_ring(sec_job_ring_t *job_ring)
{
    bool uio_device_found = false;
    char uio_device_file_name[SEC_UIO_MAX_DEVICE_FILE_NAME_LENGTH];

    // Find UIO device created by SEC kernel driver for this job ring.
    memset(uio_device_file_name,  0, sizeof(uio_device_file_name));
    uio_device_found = uio_find_device_file(job_ring->jr_id, uio_device_file_name);

    SEC_ASSERT(uio_device_found == true, 
            SEC_INVALID_INPUT_PARAM, 
            "UIO device for job ring %d not found!", 
            job_ring->jr_id);

    // Open device file
    job_ring->uio_fd = open(uio_device_file_name, O_RDWR);
    SEC_ASSERT(job_ring->uio_fd > 0, 
            SEC_INVALID_INPUT_PARAM, 
            "Failed to open UIO device file for job ring %d", 
            job_ring->jr_id);

    SEC_INFO("Opened device file for job ring %d , fd = %d", job_ring->jr_id, job_ring->uio_fd);


    return SEC_SUCCESS; 
}

static bool uio_find_device_file(int jr_id, char *device_file)
{
    bool device_file_found = false;
    int uio_minor_number = -1;
    int jr_number = -1;
    char uio_name[SEC_UIO_MAX_DEVICE_NAME_LENGTH];
    DIR *uio_root_dir = NULL;
    struct dirent *uio_dp = NULL;
    int ret = 0;

    uio_root_dir = opendir(SEC_UIO_DEVICE_SYS_ATTR_PATH);
    SEC_ASSERT(uio_root_dir != NULL, 
            SEC_INVALID_INPUT_PARAM, 
            "Failed to open dir %s", 
            SEC_UIO_DEVICE_SYS_ATTR_PATH);

    // Iterate through all subdirs
    while((uio_dp=readdir(uio_root_dir)) != NULL)
    {
        // This subdirectory is for an uio device if it contains substring 'uio'.
        // If so, extract X = minor number. The subdir names are of form: 
        // uioX, where X is the minor number for the UIO char driver.
        if(file_name_match_extract(uio_dp->d_name, "uio", &uio_minor_number))
        {
            // Open file uioX/name and read first line which contains the name for the
            // device. Based on the name check if this UIO device is UIO device for 
            // job ring with id jr_id.
            memset(uio_name,  0, sizeof(uio_name));
            ret = file_read_first_line(SEC_UIO_DEVICE_SYS_ATTR_PATH, 
                    uio_dp->d_name, 
                    "name", 
                    uio_name);
            SEC_ASSERT(ret == 0, SEC_INVALID_INPUT_PARAM, "file_read_first_line() failed");

            // Checks if UIO device name matches the name assigned for sec job rings(channel).
            // If so, the function will extract the job ring number this UIO device services.
            if(file_name_match_extract(uio_name, SEC_UIO_DEVICE_NAME, &jr_number))
            {
                if(jr_number == jr_id)
                {
                    sprintf(device_file, "%s%d", SEC_UIO_DEVICE_FILE_NAME, uio_minor_number);
                    SEC_INFO("Found UIO device %s for job ring id %d\n", 
                            device_file,
                            jr_id);
                    device_file_found = true;
                    break;
                }
            }
        }
    }
    return device_file_found;
}

static bool file_name_match_extract(char filename[], char match[], int *number)
{
    char* substr = NULL;

    substr = strstr(filename, match);
    if(substr == NULL)
    {
        return false;
    }

    sscanf(filename + strlen(match), "%d", number);
    return true;
}

static int file_read_first_line(char root[], char subdir[], char filename[], char* line)
{
    char file_name[SEC_UIO_MAX_ATTR_FILE_NAME];
    int fd = 0, ret = 0;

    // compose the file name: root/subdir/filename
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "%s/%s/%s", root, subdir, filename);

    fd = open(file_name, O_RDONLY );
    SEC_ASSERT(fd > 0, -1, "Error opening file %s", file_name);

    // read UIO device name from first line in file
    ret = read(fd, line, SEC_UIO_MAX_DEVICE_FILE_NAME_LENGTH);
    SEC_ASSERT(ret != 0, -1, "Error reading from file %s", file_name);
    
    close(fd);
    return 0;
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

sec_return_code_t sec_init(sec_config_t *sec_config_data,
                           uint8_t job_rings_no,
                           sec_job_ring_descriptor_t **job_ring_descriptors)
{
    int i;
    int ret;

    if(job_rings_no > MAX_SEC_JOB_RINGS)
    {
        SEC_ERROR("Requested number of job rings(%d) is greater than maximum hw supported(%d)",
                job_rings_no, MAX_SEC_JOB_RINGS);
        return SEC_INVALID_INPUT_PARAM;
    }

    g_job_rings_no = job_rings_no;
    memset(g_job_rings, 0, sizeof(g_job_rings));
    SEC_INFO("Configuring %d SEC job rings", g_job_rings_no);
    
    // Read configuration data from DTS (Device Tree Specification).
    ret = sec_configure(g_job_rings_no, g_job_rings);
    SEC_ASSERT(ret == SEC_SUCCESS, SEC_INVALID_INPUT_PARAM, "Failed to configure SEC driver");

    // Per-job ring initialization
    for (i = 0; i < g_job_rings_no; i++)
    {
        reset_job_ring(&g_job_rings[i]);

        // Initialize the context pool per JR
        // No need for thread synchronizations mechanisms for this pool because
        // one of the assumptions for this API is that only one thread will
        // create/delete contexts for a certain JR (also known as the producer of the JR).
        ret = init_contexts_pool(&(g_job_rings[i].ctx_pool), MAX_SEC_CONTEXTS_PER_POOL, THREAD_UNSAFE_POOL);
        ASSERT(ret == 0);

        // Obtain UIO device file descriptor for each owned job ring      
        sec_config_uio_job_ring(&g_job_rings[i]);

        g_job_ring_handles[i].job_ring_handle = (sec_job_ring_handle_t)&g_job_rings[i];
        g_job_ring_handles[i].job_ring_irq_fd = g_job_rings[i].uio_fd;
    }

    // initialize the global pool of contexts also
    // we need for thread synchronizations mechanisms for this pool
    ret = init_contexts_pool(&g_ctx_pool, MAX_SEC_CONTEXTS_PER_POOL, THREAD_SAFE_POOL);
    ASSERT(ret == 0);

    // Remember initial work mode
    sec_work_mode = sec_config_data->work_mode;
    if (sec_work_mode == SEC_INTERRUPT_MODE )
    {
        enable_irq();
    }

    *job_ring_descriptors =  &g_job_ring_handles[0];

    return SEC_SUCCESS;
}

uint32_t sec_release()
{
    /* Stub Implementation - START */
    int i;

    for (i = 0; i < g_job_rings_no; i++)
    {
        // destroy the contexts pool per JR
        destroy_contexts_pool(&(g_job_rings[i].ctx_pool));

    	reset_job_ring(&g_job_rings[i]);
    }
    g_job_rings_no = 0;

    // destroy the global context pool also
    destroy_contexts_pool(&g_ctx_pool);

    return SEC_SUCCESS;

    /* Stub Implementation - END */
}

uint32_t sec_create_pdcp_context (sec_job_ring_handle_t job_ring_handle,
                                  sec_pdcp_context_info_t *sec_ctx_info, 
                                  sec_context_handle_t *sec_ctx_handle)
{
    /* Stub Implementation - START */
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;
    sec_context_t * ctx = NULL;

    if (sec_ctx_handle == NULL || sec_ctx_info == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    if (sec_ctx_info->notify_packet == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    *sec_ctx_handle = NULL;

    if(job_ring == NULL)
    {
        /* Implement a round-robin assignment of JRs to this context */
        last_jr_assigned = SEC_CIRCULAR_COUNTER(last_jr_assigned, g_job_rings_no);
        ASSERT(last_jr_assigned >= 0 && last_jr_assigned < g_job_rings_no);
        job_ring = &g_job_rings[last_jr_assigned];
    }

    // Try to get a free context from the JR's pool
    // The advantage of the JR's pool is that it needs NO synchronization mechanisms.
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

	// provide to UA a SEC ctx handle
	*sec_ctx_handle = (sec_context_handle_t)ctx;

    return SEC_SUCCESS;

    /* Stub Implementation - END */


    // 1. ret = get_free_context();

    // No free contexts available in free list. 
    // Maybe there are some retired contexts that can be recycled.
    // Run context garbage collector routine.
    //if (ret != 0)
    //{
    //    ret = collect_retired_contexts();
    //}

    //if (ret != 0 )
    //{
    //    return error, there are no free contexts
    //}

    // 2. Associate context with job ring
    // 3. Update context with PDCP info from UA, create shared descriptor
    // 4. return context handle to UA
    // 5. Run context garbage collector routine
}

uint32_t sec_delete_pdcp_context (sec_context_handle_t sec_ctx_handle)
{
    /* Stub Implementation - START */
	sec_contexts_pool_t * pool = NULL;

    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;
    if (sec_context == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    pool = sec_context->pool;
    ASSERT (pool != NULL);

    // Now try to free the current context. If there are packets
    // in flight the context will be retired (not freed). The context
    // will be freed in the next garbage collector call.
    return free_or_retire_context(pool, sec_context);

    /* Stub Implementation - END */

    // 1. Mark context as retiring
    // 2. If context.packet_count == 0 then move context from in-use list to free list. 
    //    Else move to retiring list
    // 3. Run context garbage collector routine
}

uint32_t sec_poll(int32_t limit, uint32_t weight, uint32_t *packets_no)
{
    /* Stub Implementation - START */
    sec_job_ring_t * job_ring = NULL;
    uint32_t notified_packets_no = 0;
    uint32_t notified_packets_no_per_jr = 0;
    int i = 0;
    int ret = SEC_SUCCESS;
    int no_more_packets_on_jrs = 0;
    int jr_limit = 0; // limit computed per JR

    if (packets_no == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    /* - A limit equal with zero is considered invalid because it makes no sense to call
       sec_poll and request for no notification.
       - A weight of zero is considered invalid because the weighted round robing algorithm
       used for polling the JRs needs a weight != 0.
       To skip the round robin algorithm, UA can call sec_poll_per_jr for each JR and thus
       implement its own algorithm.
       - A limit smaller or equal than weight is considered invalid.
    */
    if (limit == 0 || weight == 0 || (limit <= weight) || (weight > SEC_JOB_RING_SIZE))
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    /* Poll job rings
       If limit < 0 && weight > 0 -> poll JRs in round robin fashion until
                                     no more notifications are available.
       If limit > 0 && weight > 0 -> poll JRs in round robin fashion until
                                     limit is reached.
     */

    ASSERT (g_job_rings_no != 0);
    /* Exit from while if one of the following is true:
     * - the required number of notifications were raised to UA.
     * - there are no more done jobs on either of the available JRs.
     * */
    while ((notified_packets_no == limit) ||
            no_more_packets_on_jrs == 0)
    {
        no_more_packets_on_jrs = 0;
        for (i = 0; i < g_job_rings_no; i++)
        {
            job_ring = &g_job_rings[i];

            /* Compute the limit for this JR
               If there are less packets to notify then configured weight,
               then notify the smaller number of packets.
            */
            int packets_left_to_notify = limit - notified_packets_no;
            jr_limit = (packets_left_to_notify < weight) ? packets_left_to_notify  : weight ;

            /* Poll one JR */
            ret = hw_poll_job_ring((sec_job_ring_handle_t)job_ring, jr_limit, &notified_packets_no_per_jr);
            if (ret != SEC_SUCCESS)
            {
                return ret;
            }
            /* Update total number of packets notified to UA */
            notified_packets_no += notified_packets_no_per_jr;

            /* Update flag used to identify if there are no more notifications
             * in either of the available JRs.*/
            no_more_packets_on_jrs |= notified_packets_no_per_jr;
        }
    }
    *packets_no = notified_packets_no;

    if (limit < 0)// and no more ready packets in SEC
    {
        enable_irq();
    }
    else if (notified_packets_no < limit)// and no more ready packets in SEC
    {
        enable_irq();
    }

    return SEC_SUCCESS;

    /* Stub Implementation - END */

    // 1. call sec_hw_poll() to check directly SEC's Job Rings for ready packets.
    //
    // sec_hw_poll() will do:
    // a. SEC 3.1 specific:
    //      - poll for errors on /dev/sec_uio_x. Raise error notification to UA if this is the case.
    // b. for all owned job rings:
    //      - check and notify ready packets.
    //      - decrement packet reference count per context
    //      - other
}

uint32_t sec_poll_job_ring(sec_job_ring_handle_t job_ring_handle, int32_t limit, uint32_t *packets_no)
{
    /* Stub Implementation - START */
    int ret = SEC_SUCCESS;
    sec_job_ring_t * job_ring =  (sec_job_ring_t *)job_ring_handle;

    if(job_ring == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    if (packets_no == NULL || limit == 0)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    *packets_no = 0;

    // run hw poll job ring
    ret = hw_poll_job_ring(job_ring, limit, packets_no);
    if (ret != SEC_SUCCESS)
    {
        return ret;
    }

    if (limit < 0)// and no more ready packets  in SEC
    {
        enable_irq_per_job_ring(job_ring);
    }
    else if (*packets_no < limit)// and no more ready packets  in SEC
    {
        enable_irq_per_job_ring(job_ring);
    }

    return SEC_SUCCESS;
    /* Stub Implementation - END */

    // 1. call hw_poll_job_ring() to check directly SEC's Job Ring for ready packets.
}

uint32_t sec_process_packet(sec_context_handle_t sec_ctx_handle,
                            sec_packet_t *in_packet,
                            sec_packet_t *out_packet,
                            ua_context_handle_t ua_ctx_handle)
{
    /* Stub Implementation - START */

#if FSL_SEC_ENABLE_SCATTER_GATHER == ON
#error "Scatter/Gather support is not implemented!"
#endif
    sec_context_t * sec_context = (sec_context_t *)sec_ctx_handle;

    if (sec_context == NULL ||
        sec_context->usage == SEC_CONTEXT_UNUSED)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    if (sec_context->usage == SEC_CONTEXT_RETIRING)
    {
        return SEC_CONTEXT_MARKED_FOR_DELETION;
    }

    sec_job_ring_t * job_ring = (sec_job_ring_t *)sec_context->jr_handle;
    if(job_ring == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }
    if (in_packet == NULL || out_packet == NULL)
    {
        return SEC_INVALID_INPUT_PARAM;
    }

    // check of the Job Ring is full (the difference between PI and CI is equal with the JR SIZE - 1)
    if(SEC_JOB_RING_DIFF(SEC_JOB_RING_SIZE, job_ring->pidx, job_ring->cidx) == (SEC_JOB_RING_SIZE - 1))
    {
        return SEC_JR_IS_FULL;
    }

    // add new job in job ring
    job_ring->jobs[job_ring->pidx].in_packet.address = in_packet->address;
    job_ring->jobs[job_ring->pidx].in_packet.offset = in_packet->offset;
    job_ring->jobs[job_ring->pidx].in_packet.length = in_packet->length;

    job_ring->jobs[job_ring->pidx].out_packet.address = out_packet->address;
    job_ring->jobs[job_ring->pidx].out_packet.offset = out_packet->offset;
    job_ring->jobs[job_ring->pidx].out_packet.length = out_packet->length;

    job_ring->jobs[job_ring->pidx].sec_context = sec_context;
    job_ring->jobs[job_ring->pidx].ua_handle = ua_ctx_handle;

    // increment packet reference count in sec context
    pthread_mutex_lock(&sec_context->mutex);
    sec_context->packets_no++;
    pthread_mutex_unlock(&sec_context->mutex);

    // increment the producer index for the current job ring
    job_ring->pidx = SEC_CIRCULAR_COUNTER(job_ring->pidx, SEC_JOB_RING_SIZE);

    return SEC_SUCCESS;
    /* Stub Implementation - END */
}

uint32_t sec_get_last_error(void)
{
    // stub function
    return 0;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
