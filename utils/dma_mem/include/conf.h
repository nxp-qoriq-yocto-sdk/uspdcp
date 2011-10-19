/* Copyright (c) 2008-2011 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CONF_H
#define __CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The contiguous memory map for 'dma_mem' uses the DMA_MEM_*** constants, the
 * _PHYS and _SIZE values *must* agree with the "mem=<...>" kernel boot
 * parameter as well as the device-tree's "fsl-shmem" node.
 *
 * Also, the first part of that memory map is used for sec driver internal memory, as
 * indicated by DMA_MEM_SEC_DRIVER. The second part can be used for startup/runtime allocations
 * using dma_mem_memalign/dma_mem_free.
 *
 * A TLB hack is done in kernel to optimize the conversion between virtual and physical memory.
 * Here is some background:
 * - e500mc MMU has two levels of TLB:
 *    -> TLB0 can store 2048 mappings between virt and phys memory
 *       with support for 4K pages only
 *    -> TLB1 can store 64 mappings between virt and phys memory
 *       with support for variable sized pages
 *   (See e500mc_RM_RevF.pdf - Chapter 6.1.1 MMU Features (Level2 MMU features)
 * - Linux kernel handles only TLB0 updates
 * - The reserved physical memory of 256 MB has a number of 65,536 4K-pages and
 *   TLB0 could not store all the mappings for this reserved memory. So the page faults
 *   occurred for the missing mappings would be solved in software. To avoid software
 *   interference an entry in TLB1 can be added.
 * - The TLB hack done in kernel (see arch/powerpc/mm/mem.c update_mmu_cache() function definition)
 *   is actually adding a mapping in TLB1 between __dma_virt and DMA_MEM_PHYS of size 256MB
 *   when the first page fault occurs. This guarantees that no page fault would occur when
 *   accessing the reserved memory area from user space.
 *
 *   Note: When changing the start physical address (DMA_MEM_PHYS) or size of the reserved
 *   memory area (DMA_MEM_SIZE) remember to update the TLB kernel hack also!!!
 *   Update function update_mmu_cache() from file arch/powerpc/mm/mem.c accordingly.
 */

#define DMA_MEM_PATH	"/dev/fsl-shmem"
#ifdef SEC_HW_VERSION_3_1
#define DMA_MEM_PHYS	    0x30000000 /* 0.75G - The start address of the physical memory reserved
                                          for user space application.*/
#define DMA_MEM_SIZE	    0x10000000 /* 256M - The size of the physical memory area reserved for
                                          user space application. */
#define DMA_MEM_SEC_DRIVER	0x08000000 /* First 128M reserved for SEC driver internal memory.
                                          The rest of 128M are used for buffer allocation. */
#else // SEC_HW_VERSION_3_1
#define DMA_MEM_PHYS        (__dma_phys)        /* For PSC 9131, this is retrieved at runtime.*/
#define DMA_MEM_SIZE        (64UL*1024*1024)    /* 64M - The size of the physical memory area reserved for
                                                   user space application. */
#define DMA_MEM_SEC_DRIVER  (1UL*1024*1024)   /* First MB reserved for SEC driver internal memory.
                                                 The rest are used for buffer allocation. */

#define DMA_MEM_KEY         0x2                 /* Key for shmget */

#endif // SEC_HW_VERSION_3_1
/* support for BUG_ON()s, might_sleep()s, etc */
#undef CONFIG_BUGON

/* When copying aligned words or shorts, try to avoid memcpy() */
#define CONFIG_TRY_BETTER_MEMCPY

#ifdef __cplusplus
}
#endif
#endif
