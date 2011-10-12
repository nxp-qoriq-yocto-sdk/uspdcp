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

#ifndef SEC_JOB_RING_H
#define SEC_JOB_RING_H

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include "fsl_sec.h"
#include "sec_contexts.h"
#include "sec_hw_specific.h"
#include "fifo.h"
#ifdef SEC_HW_VERSION_4_4
#if (SEC_ENABLE_SCATTER_GATHER == ON)
#include "sec_sg_utils.h"
#endif // (SEC_ENABLE_SCATTER_GATHER == ON)
#endif // SEC_HW_VERSION_4_4
/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/

/** Command that is used by SEC user space driver and SEC kernel driver
 *  to signal a request from the former to the later to disable job DONE
 *  and error IRQs on a certain job ring.
 *  The configuration is done at SEC Controller's level.
 *  @note   Need to be kept in synch with #SEC_UIO_DISABLE_IRQ_CMD from
 *          linux-2.6/drivers/crypto/talitos.c ! */
#define SEC_UIO_DISABLE_IRQ_CMD     0

/** Command that is used by SEC user space driver and SEC kernel driver
 *  to signal a request from the former to the later to enable job DONE
 *  and error IRQs on a certain job ring.
 *  The configuration is done at SEC Controller's level.
 *  @note   Need to be kept in synch with #SEC_UIO_ENABLE_IRQ_CMD from
 *          linux-2.6/drivers/crypto/talitos.c ! */
#define SEC_UIO_ENABLE_IRQ_CMD      1

/** Command that is used by SEC user space driver and SEC kernel driver
 *  to signal a request from the former to the later to do a SEC engine reset.
 *  @note   Need to be kept in synch with #SEC_UIO_RESET_SEC_ENGINE_CMD from
 *          linux-2.6/drivers/crypto/talitos.c ! */
#define SEC_UIO_RESET_SEC_ENGINE_CMD    3

/** Update circular counter when maximum value of counter is a power of 2.
 * Use bitwise operations to roll over. */
#define SEC_CIRCULAR_COUNTER            FIFO_CIRCULAR_COUNTER

/** The number of jobs in a JOB RING */
#define SEC_JOB_RING_NUMBER_OF_ITEMS    FIFO_NUMBER_OF_ITEMS

/** Test if job ring is full. Used job ring capacity to be 32 = a power of 2.
 * A job ring is full when there are 24 entries, which is the maximum
 * capacity of SEC's hardware FIFO. */
#define SEC_JOB_RING_IS_FULL            FIFO_IS_FULL

/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
#ifdef SEC_HW_VERSION_3_1
/** MAC-I, encapsulated in a structure to ensure cacheline-alignment.*/
typedef struct sec_mac_i_s
{
    volatile uint8_t code[SEC_3_1_MAC_I_REQUIRED_LEN];
}__CACHELINE_ALIGNED sec_mac_i_t;
#endif

/** SEC job */
struct sec_job_t
{
    sec_context_t *sec_context;         /*< SEC context this packet belongs to */
#if defined(SEC_HW_VERSION_4_4) && (SEC_ENABLE_SCATTER_GATHER == ON)
    sec_sg_context_t *sg_ctx;           /*< SEC Scatter Gather context this packet belongs to */
#endif // defined(SEC_HW_VERSION_4_4) && (SEC_ENABLE_SCATTER_GATHER == ON)
    struct sec_descriptor_t *descr;     /*< SEC descriptor sent to SEC engine(virtual address)*/
    dma_addr_t descr_phys_addr;         /*< SEC descriptor sent to SEC engine(physical address) */
    const sec_packet_t *in_packet;      /*< Input packet */
    const sec_packet_t *out_packet;     /*< Output packet */
    ua_context_handle_t ua_handle;      /*< UA handle for the context this packet belongs to */
#ifdef SEC_HW_VERSION_3_1
    sec_mac_i_t *mac_i;                 /*< SEC 3.1 generates only 8 bytes MAC-I. SEC generates here the MAC-I.
                                            Later the driver will copy it to packet, as necessary. */
    sec_status_t job_status;            /*< Processing status for the packet indicated by this job.
                                            Is required for indication that HFN reached threshold.
                                            TODO: remove this field when migrating on 9132!*/

    volatile uint8_t is_integrity_algo; /*< Is set to value #TRUE for jobs with integrity algorithm configured.
                                            Set to #FALSE for crypto algorithm. */
#endif // SEC_HW_VERSION_3_1
}__CACHELINE_ALIGNED;

#ifdef SEC_HW_VERSION_4_4
struct sec_outring_entry {
    dma_addr_t  desc;                   /*< Pointer to completed descriptor */
    uint32_t    status;                 /*< Status for completed descriptor */
} PACKED;

#endif // SEC_HW_VERSION_4_4

/** Lists the possible states for a job ring. */
typedef enum sec_job_ring_state_e
{
    SEC_JOB_RING_STATE_STARTED, /*< Job ring is initialized */
    SEC_JOB_RING_STATE_RESET,   /*< Job ring reset is in progress */
}sec_job_ring_state_t;

/** SEC Job Ring */
struct sec_job_ring_t
{
    /* TODO: Add wrapper macro to make it obvious this is the consumer index on the output ring */
    volatile uint32_t cidx;                     /*< Consumer index for job ring (jobs array).
                                                    @note: cidx and pidx are accessed from different threads.
                                                    Place the cidx and pidx inside the structure so that
                                                    they lay on different cachelines, to avoid false
                                                    sharing between threads when the threads run on different cores! */
#ifdef SEC_HW_VERSION_4_4
    volatile uint32_t   hw_cidx;                /*< Index showing the last entry processed by
                                                    the SEC 4.4 HW. */
#endif // SEC_HW_VERSION_4_1
    struct sec_job_t jobs[SEC_JOB_RING_SIZE];   /*< Ring of jobs. SEC 3.1 ONLY: The same ring is used for
                                                    input jobs and output jobs because SEC engine writes
                                                    back output indication in input job.
                                                    Size of array is power of 2 to allow fast update of
                                                    producer/consumer indexes with bitwise operations. */
    struct sec_descriptor_t *descriptors;       /*< Ring of descriptors sent to SEC engine for processing */
#ifdef SEC_HW_VERSION_3_1
    struct fifo_t pdcp_c_plane_fifo;            /*< Ring of PDCP control plane packets that must be sent
                                                    a second time to SEC for processing */
#endif
    // TODO: Add wrapper macro to make it obvious this is the producer index on the input ring
    volatile uint32_t pidx;                     /*< Producer index for job ring (jobs array) */
#ifdef SEC_HW_VERSION_4_4
    volatile uint32_t   hw_pidx;                /*< Index showing the currently enqueued entry for
                                                    processing by the user to the SEC 4.4 HW.*/

    dma_addr_t *input_ring;                     /*< Ring of output descriptors received from SEC.
                                                    Size of array is power of 2 to allow fast update of
                                                    producer/consumer indexes with bitwise operations. */

    struct sec_outring_entry *output_ring;      /*< Ring of output descriptors received from SEC.
                                                    Size of array is power of 2 to allow fast update of
                                                    producer/consumer indexes with bitwise operations. */
#endif // SEC_HW_VERSION_4_4
    uint32_t uio_fd;                            /*< The file descriptor used for polling from user space
                                                    for interrupts notifications */
    uint32_t jr_id;                             /*< Job ring id */
#ifdef SEC_HW_VERSION_3_1
    uint32_t alternate_register_range;          /*< Can be #TRUE or #FALSE. Indicates if the registers for
                                                    this job ring are mapped to an alternate 4k page.*/
#endif // SEC_HW_VERSION_3_1
    volatile void *register_base_addr;          /*< Base address for SEC's register memory for this job ring.
                                                    @note On SEC 3.1 all channels share the same register address space,
                                                    so this member will have the exact same value for all of them. */
    volatile sec_job_ring_state_t jr_state;     /*< The state of this job ring */
    sec_contexts_pool_t ctx_pool;               /*< Pool of SEC contexts */
#if defined(SEC_HW_VERSION_4_4) && (SEC_ENABLE_SCATTER_GATHER == ON)
    sec_sg_context_t sg_ctxs[SEC_JOB_RING_SIZE]; /*< Scatter Gather contexts for this jobring */
#endif // defined(SEC_HW_VERSION_4_4) && (SEC_ENABLE_SCATTER_GATHER == ON)
}__CACHELINE_ALIGNED;
/*==============================================================================
                                 CONSTANTS
==============================================================================*/


/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/
/* Job rings used for communication with SEC HW */
extern struct sec_job_ring_t g_job_rings[MAX_SEC_JOB_RINGS];


/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/
/** @brief Initialize the software and hardware resources tied to a job ring.
 * @param [in,out] job_ring             The job ring
 * @param [in,out] dma_mem              DMA-capable memory area from where to
 *                                      allocate SEC descriptors.
 * @param [in]     startup_work_mode    The work mode to configure a job ring at startup.
 *                                      Used only when #SEC_NOTIFICATION_TYPE is set to
 *                                      #SEC_NOTIFICATION_TYPE_NAPI.
 * @param [in]     irq_coalescing_timer Only for SEC 4.4
 * @param [in]     irq_coalescing_count Only for SEC 4.4
 * @retval  SEC_SUCCESS for success
 * @retval  other for error
 *
 */
int init_job_ring(struct sec_job_ring_t *job_ring, void **dma_mem,int startup_work_mode
#ifdef SEC_HW_VERSION_4_4
        ,uint8_t irq_coalescing_timer, uint8_t irq_coalescing_count
#endif
    );

/** @brief Release the software and hardware resources tied to a job ring.
 * @param [in] job_ring The job ring
 *
 * @retval  SEC_SUCCESS for success
 * @retval  other for error
 */
int shutdown_job_ring(struct sec_job_ring_t *job_ring);

/** @brief Request to SEC kernel driver to reset SEC engine.
 *  Use UIO to communicate with SEC kernel driver: write command
 *  value that indicates a reset action into UIO file descriptor
 *  of this job ring.
 *  After SEC device reset, SEC user space driver will need a shutdown.
 *
 * @param [in]  job_ring     Job ring
 */
void uio_reset_sec_engine(sec_job_ring_t *job_ring);

/** @brief SEC 3.1: Request to SEC kernel driver to enable job DONE and
 *  error interrupts on this job ring.
 *         SEC 4.4: Request to SEC kernel driver to enable interrupts for
 *         descriptor finished processing
 *  Use UIO to communicate with SEC kernel driver: write command
 *  value that indicates an IRQ enable action into UIO file descriptor
 *  of this job ring.
 *
 * @param [in]  job_ring     Job ring
 */
void uio_job_ring_enable_irqs(sec_job_ring_t *job_ring);

#ifdef SEC_HW_VERSION_4_4
/** @brief Request to SEC kernel driver to disable interrupts for descriptor
 * finished processing
 *  Use UIO to communicate with SEC kernel driver: write command
 *  value that indicates an IRQ enable action into UIO file descriptor
 *  of this job ring.
 *
 * @param [in]  job_ring     Job ring
 */
void uio_job_ring_disable_irqs(sec_job_ring_t *job_ring);

#endif // SEC_HW_VERSION_4_4
/*============================================================================*/


#endif  /* SEC_JOB_RING_H */
