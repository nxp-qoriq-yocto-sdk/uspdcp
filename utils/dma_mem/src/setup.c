/* Copyright (c) 2010-2011 Freescale Semiconductor, Inc.
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

#define LOCAL_DMA_ADDR_TYPE

#define test_printf(format, ...)
//#define test_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__)

#ifdef SEC_HW_VERSION_4_4
#include "compat.h"

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <sys/mman.h>
#include <sys/shm.h>

#include "fsl_het_mgr.h"
#include "fsl_types.h"

#include "fsl_usmmgr.h"

/* This global is exported for use in ptov/vtop inlines. It is the result of the
 * mmap(), but pre-cast to dma_addr_t, so if we have 64-bit physical addresses
 * the ptov/vtop inlines will have less conversion to do. */
dma_addr_t      __dma_virt;
dma_addr_t      __dma_phys;

/* This is the same value, but it's the pointer type */
static void *virt;

/** US memory manager. One per each instance of an application */
fsl_usmmgr_t    mmgr;

/** Value indicating if the memory manager was instantiated. MemMgr must
  * be instantiated only once per application
  */
uint32_t    dma_mem_init = 0;

int dma_mem_setup(uint32_t sec_driver_size, uint32_t align)
{
    range_t r;
    int ret;

    if( dma_mem_init == 0 )
    {
        mmgr = fsl_usmmgr_init();
        dma_mem_init = 1;
    }

    r.size = sec_driver_size;

    test_printf("Allocating %u for SEC driver internal use\n", sec_driver_size);

    ret = fsl_usmmgr_memalign(&r,align,mmgr);

    if(ret)
    {
        test_printf("Error allocating SEC driver memory: %d\n",ret);
        return -1;
    }

    virt = r.vaddr;
    __dma_virt = (dma_addr_t)r.vaddr;
    __dma_phys = r.phys_addr;

    return 0;
}

int dma_mem_release()
{
    test_printf("Releasing DMA mem from address: %p\n",(void*)__dma_virt);
    dma_mem_free((void*)__dma_virt,0);

    return 0;
}
#else // SEC_HW_VERSION_4_4
#include "private.h"
/* For an efficient conversion between user-space virtual address map(s) and bus
 * addresses required by hardware for DMA, we use a single contiguous mmap() on
 * the /dev/fsl-shmem device, a pre-arranged physical base address (and
 * similarly reserved from regular linux use by a "mem=<...>" kernel boot
 * parameter). See conf.h for the hard-coded constants that are used. */

static int fd;

/* This global is exported for use in ptov/vtop inlines. It is the result of the
 * mmap(), but pre-cast to dma_addr_t, so if we have 64-bit physical addresses
 * the ptov/vtop inlines will have less conversion to do. */
dma_addr_t __dma_virt;

/* This is the same value, but it's the pointer type */
static void *virt;

int dma_mem_setup(void)
{
	void *trial;
	int ret = -ENODEV;
	fd = open(DMA_MEM_PATH, O_RDWR);
	if (fd < 0) {
		perror("can't open dma_mem device");
		return ret;
	}
	/* TODO: this needs to be improved but may not be possible until
	 * hugetlbfs is available. If we let the kernel choose the virt address,
	 * it will only guarantee page-alignment, yet our TLB1 hack in the
	 * kernel requires that this mapping be *size*-aligned. With this in
	 * mind, we'll do a trial-and-error proposing addresses to the kernel
	 * until we find one that works or give up. */
	for (trial = (void *)0x30000000; (unsigned long)trial < 0xc0000000;
				trial += DMA_MEM_SIZE) {
		test_printf("trial=%p\n", trial);
		virt = mmap(trial, DMA_MEM_SIZE, PROT_READ | PROT_WRITE,
				MAP_SHARED | MAP_FIXED, fd, DMA_MEM_PHYS);
		test_printf("  -> %p\n", virt);
		if (virt != MAP_FAILED)
			break;
		pr_info("nah, try again\n");
	}
	if (virt == MAP_FAILED) {
		perror("can't mmap() dma_mem device");
		goto err;
	}
	/* dma_mem is used for ad-hoc allocations. */
	ret = dma_mem_alloc_init(virt + DMA_MEM_SEC_DRIVER, DMA_MEM_SIZE - DMA_MEM_SEC_DRIVER);
	if (ret)
		goto err;

	__dma_virt = (dma_addr_t)virt;

	test_printf("FSL dma_mem device mapped (phys=0x%x,virt=%p,sz=0x%x)\n",
		DMA_MEM_PHYS, virt, DMA_MEM_SIZE);

	// close file DMA_MEM_PATH
	close(fd);

	return 0;
err:
	fprintf(stderr, "ERROR: dma_mem setup failed, ret = %d\n", ret);
	close(fd);
	return ret;
}

int dma_mem_release(void)
{
	int ret = 0;

	ret = munmap(virt, DMA_MEM_SIZE);
	if (ret != 0)
	{
		test_printf("dma_mem_release: failed to unmap the virtual address 0x%x", (dma_addr_t)virt);
		return ret;
	}
	return 0;
}

#endif // SEC_HW_VERSION_3_1
