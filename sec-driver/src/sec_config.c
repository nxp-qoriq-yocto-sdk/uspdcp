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
#include "sec_config.h"
#include "sec_utils.h"
#include "fsl_sec_config.h"
#include "fsl_sec.h"
#include <dirent.h>
// For DTS parsing
#include <of.h>
#include <fcntl.h>

/*==================================================================================================
                                     LOCAL DEFINES
==================================================================================================*/

/** Prefix path to sysfs directory where UIO device attributes are exported.
 * Path for UIO device X is /sys/class/uio/uioX */
#define SEC_UIO_DEVICE_SYS_ATTR_PATH    "/sys/class/uio"

/** Name of UIO device file prefix. Each UIO device will have a device file /dev/uioX, 
 * where X is the minor device number. */
#define SEC_UIO_DEVICE_FILE_NAME    "/dev/uio"

/** Bit mask for each job ring in a bitfield of format: jr0 | jr1 | jr2 | jr3 */
#define JOB_RING_MASK(jr_id)        (1 << (MAX_SEC_JOB_RINGS - (jr_id) -1 ))

/** Maximum length for the name of an UIO device file.
 * Device file name format is: /dev/uioX. */
#define SEC_UIO_MAX_DEVICE_FILE_NAME_LENGTH 30

/** Maximum length for the name of an attribute file for an UIO device.
 * Attribute files are exported in sysfs and have the name formatted as:
 *      /sys/class/uio/uioX/<attribute_file_name>
 */
#define SEC_UIO_MAX_ATTR_FILE_NAME  100

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

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

    // substring <match> was found in <filename>
    // read number following <match> substring in <filename>
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

int sec_configure(int job_ring_number, sec_job_ring_t *job_rings)
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

        // Read from DTS job ring distribution map between user space and kernel space.
        // If bit is set, job ring belongs to kernel space.
        // Bitfield: jr0 | jr1 | jr2 | jr3
        kernel_usr_channel_map = of_get_property(dpa_node, "fsl,channel-kernel-user-space-map", &len);
        if(kernel_usr_channel_map == NULL)
        {
            SEC_ERROR("Error reading <fsl,channel-kernel-user-space-map> property from DTS!");
            return SEC_INVALID_INPUT_PARAM;
        }
        
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

int sec_config_uio_job_ring(sec_job_ring_t *job_ring)
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
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
