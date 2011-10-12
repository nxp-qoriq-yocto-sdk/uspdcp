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

#include "private.h"
#ifdef SEC_HW_VERSION_4_4
#include <sys/ipc.h>    // _IOXXX macros
#include <sys/shm.h>    // shmXXX functions
#include <sys/ioctl.h>  // ioctl function
#endif //SEC_HW_VERSION_4_4

#define test_printf(format, ...)
//#define test_printf(format, ...) printf("%s(): " format "\n", __FUNCTION__,  ##__VA_ARGS__)

/* For an efficient conversion between user-space virtual address map(s) and bus
 * addresses required by hardware for DMA, we use a single contiguous mmap() on
 * the /dev/fsl-shmem device, a pre-arranged physical base address (and
 * similarly reserved from regular linux use by a "mem=<...>" kernel boot
 * parameter). See conf.h for the hard-coded constants that are used. */

static int fd;
#ifdef SEC_HW_VERSION_4_4
int     shmid;
#endif // SEC_HW_VERSION_4_4

/* This global is exported for use in ptov/vtop inlines. It is the result of the
 * mmap(), but pre-cast to dma_addr_t, so if we have 64-bit physical addresses
 * the ptov/vtop inlines will have less conversion to do. */
dma_addr_t __dma_virt;
#ifdef SEC_HW_VERSION_4_4
dma_addr_t __dma_phys;
#endif //SEC_HW_VERSION_4_4

/* This is the same value, but it's the pointer type */
static void *virt;
#ifdef SEC_HW_VERSION_3_1
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
#else // SEC_HW_VERSION_3_1
int dma_mem_setup(void)
{
        int ret = -ENODEV;
        range_t r;

        test_printf("Trying to open %s\n",DMA_MEM_PATH);

        /* query ranges from /dev/het_mgr */
        fd = open(DMA_MEM_PATH, O_RDWR);
        if (fd < 0)
        {
                perror("Error: Cannot open DMA_MEM_PATH");
                return ret;
        }

        /* Try to get memory range */
        shmid = shmget(DMA_MEM_KEY, DMA_MEM_SIZE, SHM_HUGETLB| IPC_CREAT | SHM_R | SHM_W);
        if ( shmid < 0 ) {
                perror("shmget failed");
                return -1;
        }

        test_printf("HugeTLB shmid: 0x%x\n", shmid);
        r.vaddr = shmat(shmid, 0, 0);

        if (r.vaddr == (char *)-1) {
                perror("Shared memory attach failure");
                shmctl(shmid, IPC_RMID, NULL);
                return -1;
        }

        test_printf("Clearing the memory");
        memset(r.vaddr, 0, 4); //try with 4 bytes

        // Get phyisical address
        ret = ioctl(fd, IOCTL_HET_MGR_V2P, &r);
        test_printf("Ret ioctl = %d\n",ret);
        test_printf("V2P %x %x \n", (uint32_t)r.vaddr, r.phys_addr);


        virt = r.vaddr;
        __dma_virt = (dma_addr_t)virt;

        __dma_phys = (dma_addr_t)r.phys_addr;

        /* dma_mem is used for ad-hoc allocations. */
        ret = dma_mem_alloc_init(virt + DMA_MEM_SEC_DRIVER, DMA_MEM_SIZE - DMA_MEM_SEC_DRIVER);
        if (ret)
            goto err;

        return 0;

err:
    fprintf(stderr, "ERROR: dma_mem setup failed, ret = %d\n", ret);
    shmdt(virt);
    close(fd);
    shmctl(shmid, IPC_RMID, NULL);
    return ret;
}

int dma_mem_release(void)
{
    if (shmdt(virt) != 0) {
        perror("Detach failure");
        shmctl(shmid, IPC_RMID, NULL);
        return -1;
    }

    close(fd);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}

#endif // SEC_HW_VERSION_3_1
