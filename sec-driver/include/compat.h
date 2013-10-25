/* Copyright 2013 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
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


#ifndef SEC_COMPAT_H
#define SEC_COMPAT_H

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <assert.h>
#include <dirent.h>
#include <inttypes.h>
#include <error.h>
#include <errno.h>

struct list_s;

/** Size in bytes of a cacheline. */
#define L1_CACHE_BYTES  32

/** Shortcut for __attribute__ ((packed)) */
#define __packed	__attribute__((__packed__))
/** Shortcut for aligned to #L1_CACHE_BYTES attribute */
#define ____cacheline_aligned __attribute__((aligned(L1_CACHE_BYTES)))

#if defined(__powerpc64__) && defined(CONFIG_PHYS_64BIT)
/** Physical address on 36 bits or more. MUST be kept in synch with same define from kernel! */
typedef uint64_t dma_addr_t;
#else
/** Physical address on 32 bits. MUST be kept in synch with same define from kernel!*/
typedef uint32_t dma_addr_t;
#endif

/* Required types */
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

/*
 * I/O operations
 */
/** Write 32bit values */
static inline void out_be32(volatile void *addr, uint32_t val) {
	volatile uint32_t *paddr = addr;
	*paddr = val;
}

/** Read 32bit values */
static inline uint32_t in_be32(const volatile void *addr) {
	const volatile uint32_t *paddr = addr;
	return *paddr;
}

/*
 * List stuff
 */
/** A list node.
 *
 *  The list_node_t must be declared inside the data structure
 *  that will be included in a list.
 *
 */
struct list_head
{
	/** Pointer to the previous node in the list. */
	struct list_head *prev;
	/** Pointer to the next node in the list. */
	struct list_head *next;
};

#define LIST_HEAD(n) \
struct list_head n = { \
	.prev = &n, \
	.next = &n \
}
#define INIT_LIST_HEAD(p) \
do { \
	struct list_head *__p298 = (p); \
	__p298->prev = __p298->next =__p298; \
} while(0)
#define list_entry(node, type, member) \
	(type *)((void *)node - offsetof(type, member))
#define list_empty(p) \
({ \
	const struct list_head *__p298 = (p); \
	((__p298->next == __p298) && (__p298->prev == __p298)); \
})
#define list_add_tail(p,l) \
do { \
	struct list_head *__p298 = (p); \
	struct list_head *__l298 = (l); \
	__p298->prev = __l298->prev; \
	__p298->next = __l298; \
	__l298->prev->next = __p298; \
	__l298->prev = __p298; \
} while(0)
#define list_for_each_entry(i, l, name) \
	for (i = list_entry((l)->next, typeof(*i), name); &i->name != (l); \
		i = list_entry(i->name.next, typeof(*i), name))
#define list_for_each_entry_safe(i, j, l, name) \
	for (i = list_entry((l)->next, typeof(*i), name), \
		j = list_entry(i->name.next, typeof(*j), name); \
		&i->name != (l); \
		i = j, j = list_entry(j->name.next, typeof(*j), name))
#define list_del(i) \
do { \
	(i)->next->prev = (i)->prev; \
	(i)->prev->next = (i)->next; \
} while(0)

/* Compiler/type stuff */
typedef uint32_t	phandle;

/* Required compiler attributes */
#define container_of(p, t, f) (t *)((void *)p - offsetof(t, f))

#endif /* SEC_COMPAT_H */
