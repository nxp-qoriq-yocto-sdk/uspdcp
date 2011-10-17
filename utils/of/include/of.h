/* Copyright (c) 2010-2011 Freescale Semiconductor, Inc.
 * All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __OF_H
#define	__OF_H

#ifdef __cplusplus
/* *INDENT-OFF* */

extern "C"{
/* *INDENT-ON* */
#endif

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
#include <compat_of.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>	/* PATH_MAX */
#include <string.h>	/* strcmp(), strcasecmp() */

#ifndef OF_INIT_DEFAULT_PATH
#define OF_INIT_DEFAULT_PATH "/proc/device-tree"
#endif

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define __always_unused	__attribute__((unused))

#define for_each_compatible_node(dev_node, type, compatible)			\
    for (dev_node = of_find_compatible_node(NULL, type, compatible);	\
            dev_node != NULL;							                \
            dev_node = of_find_compatible_node(NULL, type, compatible))

#define for_each_child_node(parent, child) \
    for (child = of_get_next_child(parent, NULL); child != NULL; \
            child = of_get_next_child(parent, child))

//#define WARN_ON(cond,msg)   (cond) ? test_printf(msg) : 0
#define WARN_ON(cond,msg)   (cond) ? printf(msg) : 0

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

struct device_node
{
    char	name[NAME_MAX];
    char	 full_name[PATH_MAX];

    uint8_t	 _property[64];
};

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

const struct device_node *of_find_compatible_node(
                    const struct device_node *from,
                    const char *type __always_unused,
                    const char *compatible)
    __attribute__((nonnull(3)));

const void *of_get_property(const struct device_node *from, const char *name,
                size_t *lenp) __attribute__((nonnull(2)));
bool of_device_is_available(const struct device_node *dev_node);

const struct device_node *of_find_node_by_phandle(phandle ph);

const struct device_node *of_get_parent(const struct device_node *dev_node);

const struct device_node *of_get_next_child(const struct device_node *dev_node,
                        const struct device_node *prev);

uint32_t of_n_addr_cells(const struct device_node *dev_node);
uint32_t of_n_size_cells(const struct device_node *dev_node);

const uint32_t *of_get_address(const struct device_node *dev_node, size_t idx,
                uint64_t *size, uint32_t *flags);

uint64_t of_translate_address(const struct device_node *dev_node,
                const u32 *addr) __attribute__((nonnull));

bool of_device_is_compatible(const struct device_node *dev_node,
                const char *compatible);

/* of_init() must be called prior to initialisation or use of any driver
 * subsystem that is device-tree-dependent. Eg. Qman/Bman, config layers, etc.
 * The path is should usually be "/proc/device-tree". */
int of_init_path(const char *dt_path);

/* Use of this wrapper is recommended. */
static inline int of_init(void)
{
    return of_init_path(OF_INIT_DEFAULT_PATH);
}

/* of_finish() allows a controlled tear-down of the device-tree layer, eg. if a
 * full USDPAA reload is desired without a process exit. */
void of_finish(void);

#endif	/*  __OF_H */
