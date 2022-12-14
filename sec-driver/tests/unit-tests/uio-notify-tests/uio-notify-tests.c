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
#include "cgreen.h"
#ifdef USDPAA
#include <usdpaa/dma_mem.h>
#include <flib/protoshared.h>
#else
// For shared memory allocator
#include "fsl_usmmgr.h"
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/
// Number of SEC JRs used by this test application
// @note: Currently this test application supports only 2 JRs (not less, not more).
//        The DTS must have configured SEC device with at least 2 user space owned job rings!
#define JOB_RING_NUMBER 2

// Code sent to SEC kernel driver that indicates an IRQ is to be simulated for a job ring
#define SIMULATE_IRQ    2

// Maximum number of IRQs to be generated in a test
#define MAX_IRQ     50

// For keeping the code relatively the same between HW versions
#define dma_mem_memalign  test_memalign
#define dma_mem_free      test_free
#ifdef USDPAA
/* The size of the DMA-able memory zone to be allocated through the test */
#define DMAMEM_SIZE	0x1000000
#endif
/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
// configuration data for SEC driver
static sec_config_t sec_config_data;

// job ring handles provided by SEC driver
static const sec_job_ring_descriptor_t *job_ring_descriptors = NULL;

// Job ring used for testing, in tests using a single job ring.
static int job_ring_id;

#ifdef USDPAA
enum rta_sec_era rta_sec_era = RTA_SEC_ERA_2;
#else
// FSL Userspace Memory Manager structure
fsl_usmmgr_t g_usmmgr;
#endif

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
/* Returns the physical address corresponding to the virtual
 * address passed as a parameter. 
 */
#ifdef USDPAA
dma_addr_t test_vtop(void *v);
#define test_vtop __dma_mem_vtop

void *test_ptov(dma_addr_t p);
#define test_ptov __dma_mem_ptov

void *test_memalign(size_t align, size_t size);
#define test_memalign __dma_mem_memalign

void test_free(void *ptr, size_t size);
#define test_free(ptr, size) __dma_mem_free(ptr)
#else
static inline dma_addr_t test_vtop(void *v)
{
    return fsl_usmmgr_v2p(v,g_usmmgr);
}

/* Allocates an aligned memory area from the FSL USMMGR pool */
static void * test_memalign(size_t align, size_t size);

/* Frees a previously allocated FSL USMMGR memory region */
static void test_free(void *ptr, size_t size);
#endif
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
#ifndef USDPAA
static void * test_memalign(size_t align, size_t size)
{
    int ret;
    range_t r = {0,0,size};
    
    ret = fsl_usmmgr_memalign(&r,align,g_usmmgr);    
    assert_equal_with_message(ret, 0, "FSL USMMGR memalign failed: %d",ret);
    return r.vaddr;
}

static void test_free(void *ptr, size_t size)
{
   range_t r = {0,ptr,size};   
   fsl_usmmgr_free(&r,g_usmmgr);
}
#endif
static void test_setup(void)
{
    int ret = 0;
#ifndef USDPAA
    // Init FSL USMMGR
    g_usmmgr = fsl_usmmgr_init();
    assert_not_equal_with_message(g_usmmgr, NULL, "ERROR on fsl_usmmgr_init");
#endif
    // Fill SEC driver configuration data
    sec_config_data.memory_area = dma_mem_memalign(L1_CACHE_BYTES,SEC_DMA_MEMORY_SIZE);
    sec_config_data.sec_drv_vtop = test_vtop;

    sec_config_data.work_mode = SEC_STARTUP_POLLING_MODE;

    // Init sec driver
    ret = sec_init(&sec_config_data, JOB_RING_NUMBER, &job_ring_descriptors);
    assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_init: ret = %d", ret);
}

static void test_teardown()
{
    int __attribute__((unused)) ret = 0;

    // release sec driver
    ret = sec_release();
	assert_equal_with_message(ret, SEC_SUCCESS, "ERROR on sec_release: ret = %d", ret);

    dma_mem_free(sec_config_data.memory_area,SEC_DMA_MEMORY_SIZE);
#ifndef USDPAA
    /* Destroy FSL USMMGR object. */
    ret = fsl_usmmgr_exit(g_usmmgr);
    assert_equal_with_message(ret,0,"Failure to destroy the FSL USMMGR: %d",ret);
#endif
}


static void test_one_jr_generate_all_irq_read_all_irq(void)
{
    int ret = 0;
    int irq_count, old_irq_count;
    int counter = 0;
    int irq_control = SIMULATE_IRQ;


    printf("Running test %s on job ring %d\n", __FUNCTION__, job_ring_id);
    test_setup();

        // Generate one IRQ, then read the IRQ counter.
    // The aim is to read the initial IRQ counter, as it may not be 0 because
    // other IRQ tests were previously executed.
    ret = write(job_ring_descriptors[job_ring_id].job_ring_irq_fd, // UIO device file descriptor
                &irq_control, // IRQ control word
                4); // number of bytes to write = sizeof(int)
    assert_equal_with_message(ret, 4, "Error on writing IRQ control word in UIO device file");

    // Now read the initial IRQ counter
    ret = read(job_ring_descriptors[job_ring_id].job_ring_irq_fd,
               &old_irq_count,
               4); // size of irq counter = sizeof(int)
    assert_equal_with_message(ret, 4, "Error on reading IRQ counter word from UIO device file");

    // Generate all IRQs
    for(counter = 0; counter < MAX_IRQ; counter++)
    {
        ret = write(job_ring_descriptors[job_ring_id].job_ring_irq_fd, // UIO device file descriptor
                &irq_control, // IRQ control word
                4); // number of bytes to write = sizeof(int)
        assert_equal_with_message(ret, 4, "Error on writing IRQ control word in UIO device file");
    }
    assert_equal_with_message(counter, MAX_IRQ, "Error generating all %d IRQs", MAX_IRQ);

    // Read number of IRQs generated so far for this job ring
    ret = read(job_ring_descriptors[job_ring_id].job_ring_irq_fd,
               &irq_count,
               4); // size of irq counter = sizeof(int)
    assert_equal_with_message(ret, 4, "Error on reading IRQ counter word from UIO device file");
    assert_equal_with_message(irq_count - old_irq_count,
                              MAX_IRQ, "Error. Number of received IRQs(%d) does not match number "
                              "of generated IRQs(%d)",
                              irq_count - old_irq_count,
                              MAX_IRQ);
    test_teardown();
}

static void test_one_jr_alternate_irq_gen_irq_read(void)
{
    int ret = 0;
    int irq_count;
    int old_irq_count = 0;
    int new_irq_count = 0;
    int counter = 0;
    int irq_control = SIMULATE_IRQ;
    int first_run = 1;



    printf("Running test %s on job ring %d\n", __FUNCTION__, job_ring_id);
    test_setup();

    do
    {
        // Generate one IRQ
        ret = write(job_ring_descriptors[job_ring_id].job_ring_irq_fd, // UIO device file descriptor
                &irq_control, // IRQ control word
                4); // number of bytes to write = sizeof(int)
        assert_equal_with_message(ret, 4, "Error on writing IRQ control word in UIO device file");

        // Now read the IRQ counter
        ret = read(job_ring_descriptors[job_ring_id].job_ring_irq_fd,
                &irq_count,
                4); // size of irq counter = sizeof(int)
        assert_equal_with_message(ret, 4, "Error on reading IRQ counter word from UIO device file");
        if(first_run)
        {
            first_run = 0;
            old_irq_count = irq_count;
            new_irq_count++;
        }
        else
        {
            assert_equal_with_message(irq_count - old_irq_count,
                                      1,
                                      "Error: number of not handled IRQs should be 1");
            new_irq_count++;
            old_irq_count = irq_count;
        }
        counter++;
    }while(counter < MAX_IRQ);
    assert_equal_with_message(counter, MAX_IRQ, "Error generating all %d IRQs", MAX_IRQ);
    assert_equal_with_message(new_irq_count, MAX_IRQ, "Error receiving all %d IRQs", MAX_IRQ);

    test_teardown();
}
static TestSuite * uio_tests()
{
    /* create test suite */
    TestSuite * suite = create_test_suite();

    /* setup/teardown functions to be called before/after each unit test */
//    setup(suite, tests_setup);
//    teardown(suite, tests_teardown);

    /* start adding unit tests */
    add_test(suite, test_one_jr_generate_all_irq_read_all_irq);
    add_test(suite, test_one_jr_alternate_irq_gen_irq_read);

    return suite;
} /* list_tests() */
/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int main(int argc, char *argv[])
{
    /* *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** */
    /* Be aware that by using run_test_suite() instead of run_single_test(), CGreen will execute
     * each test case in a separate UNIX process, so:
     * (1) unit tests' thread safety might need to be ensured by defining critical regions
     *     (beware, CGreen error messages are not explanatory and intuitive enough)
     *
     * Although it is more difficult to maintain synchronization manually,
     * it is recommended to run_single_test() for each test case.
     */

    /* create test suite */
    TestSuite * suite = uio_tests();
    TestReporter * reporter = create_text_reporter();

#ifdef USDPAA
    dma_mem_generic = dma_mem_create(DMA_MAP_FLAG_ALLOC, NULL, DMAMEM_SIZE);
    assert_not_equal_with_message(dma_mem_generic, NULL, "ERROR on dma_mem_create");
#endif
    /* Run tests */

    job_ring_id = 0;
    run_single_test(suite, "test_one_jr_generate_all_irq_read_all_irq", reporter);
    run_single_test(suite, "test_one_jr_alternate_irq_gen_irq_read", reporter);

    job_ring_id = 1;
    run_single_test(suite, "test_one_jr_generate_all_irq_read_all_irq", reporter);
    run_single_test(suite, "test_one_jr_alternate_irq_gen_irq_read", reporter);

    destroy_test_suite(suite);
    (*reporter->destroy)(reporter);

#ifdef USDPAA
    dma_mem_destroy(dma_mem_generic);
#endif
    return 0;
} /* main() */

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
