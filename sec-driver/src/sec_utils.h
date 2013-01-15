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

#ifndef SEC_UTILS_H
#define SEC_UTILS_H

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/

#define TRUE    1
#define FALSE   0

#if SEC_DRIVER_LOGGING == OFF

/** No logging of info messages */
#define SEC_INFO(format, ...)
/** No logging of error messages */
#define SEC_ERROR(format, ...)
/** No logging of debug messages */
#define SEC_DEBUG(format, ...)

#elif SEC_DRIVER_LOGGING == ON

#if SEC_DRIVER_LOGGING_LEVEL == SEC_DRIVER_LOG_INFO

/** Log info messages to stdout */
#define SEC_INFO(format, ...) fprintf(stderr,"%s() (%s@%d): " format "\n", __FUNCTION__, __FILE__, __LINE__ , ##__VA_ARGS__) 
/** Logging of error messages to stdout*/
#define SEC_ERROR(format, ...) fprintf(stderr,"%s() (%s@%d): " format "\n", __FUNCTION__, __FILE__, __LINE__ , ##__VA_ARGS__) 
/** No logging of debug messages */
#define SEC_DEBUG(format, ...)

#elif SEC_DRIVER_LOGGING_LEVEL == SEC_DRIVER_LOG_ERROR

/** Log info messages to stdout */
#define SEC_INFO(format, ...)
/** Log error messages to stdout */
#define SEC_ERROR(format, ...) fprintf(stderr,"%s() (%s@%d): " format "\n", __FUNCTION__, __FILE__, __LINE__ , ##__VA_ARGS__) 
/** No logging of debug messages */
#define SEC_DEBUG(format, ...)

#elif SEC_DRIVER_LOGGING_LEVEL == SEC_DRIVER_LOG_DEBUG
/** Log info messages to stdout */
#define SEC_INFO(format, ...) fprintf(stderr,"%s() (%s@%d): " format "\n", __FUNCTION__, __FILE__, __LINE__ , ##__VA_ARGS__)
/** Logging of error messages to stdout*/
#define SEC_ERROR(format, ...) fprintf(stderr,"%s() (%s@%d): " format "\n", __FUNCTION__, __FILE__, __LINE__ , ##__VA_ARGS__)
/** Logging of debug messages to stdout*/
#define SEC_DEBUG(format, ...) fprintf(stderr,"%s() (%s@%d): " format "\n", __FUNCTION__, __FILE__, __LINE__ , ##__VA_ARGS__)

#else
#error "Invalid value for SEC_DRIVER_LOGGING_LEVEL!"
#endif // #if SEC_DRIVER_LOGGING_LEVEL == SEC_DRIVER_LOG_DEBUG

#else // #if SEC_DRIVER_LOGGING == OFF
#error "Invalid value for SEC_DRIVER_LOGGING!"
#endif

#ifdef DEBUG
/**
 * Assert that cond is true. If !cond is true, display str and the vararg list in a printf-like syntax.
 * also, if !cond is true, return altRet.
 *
 * \param cond          A boolean expression to be asserted true
 * \param altRet        The value to be returned if cond doesn't hold true
 * \param str           A quoted char string
 *
 * E.g.:
 *      SEC_ASSERT(ret > 0, 0, "ERROR initializing app: code = %d\n", ret);
 */
#define SEC_ASSERT(cond, altRet, str, ...) \
    if (unlikely(!(cond))) {\
        SEC_ERROR(str ,## __VA_ARGS__); \
        return (altRet); \
    }

/**
 * Assert that cond is true. If !cond is true, display str and the vararg list in a printf-like syntax.
 * also, if !cond is true, return.
 *
 * \param cond          A boolean expression to be asserted true
 * \param str           A quoted char string
 *
 * E.g.:
 *      SEC_ASSERT(ret > 0, "ERROR initializing app: code = %d\n", ret);
 */
#define SEC_ASSERT_RET_VOID(cond, str, ...) \
    if (unlikely(!(cond))) {\
        SEC_ERROR(str ,## __VA_ARGS__); \
        return ; \
    }

/**
 * Check condition (possibly print error message) and continue execution.
 * Same as ##SEC_ASSERT(), only without the return instruction.
 */
#define SEC_ASSERT_CONT(cond, str, ...) \
    if (unlikely(!(cond))) {\
        SEC_ERROR(str ,## __VA_ARGS__); \
    }

/**
 * Check condition (possibly print error message) and stop execution.
 * Same as ##SEC_ASSERT(), only without the return instruction.
 */
#define SEC_ASSERT_STOP(cond, str, ...) \
    if (unlikely(!(cond))) {\
        SEC_ERROR(str ,## __VA_ARGS__); \
        ASSERT(cond); \
    }

#else

#define SEC_ASSERT(cond, altRet, str, ...)
#define SEC_ASSERT_RET_VOID(cond, str, ...)
#define SEC_ASSERT_CONT(cond, str, ...)
#define SEC_ASSERT_STOP(cond, str, ...)

#endif

/** Shortcut for __attribute__ ((packed)) */
#define PACKED              __attribute__ ((packed))
/** Shortcut for aligned to #CACHE_LINE_SIZE attribute */
#define __CACHELINE_ALIGNED   __attribute__((aligned(CACHE_LINE_SIZE)))

/** compute offset for structure member B in structure A */
#ifndef offsetof
#ifdef __GNUC__
#define offsetof(a,b)           __builtin_offsetof(a,b)
#else
#define offsetof(type,member)   ((size_t) &((type*)0)->member)
#endif
#endif

/** Get a pointer to beginning of structure which contains <member> */
#define container_of(ptr, type, member) ({            \
 const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
  (type *)( (char *)__mptr - offsetof(type,member) );})

/** counts the number of elements in an array */
#ifndef countof
 #define countof(A) ((sizeof(A)/sizeof(A[0])))
#endif

/** Compile time assert.
 * This doesn't compile if a is false. Good to compile-time check things like
 * struct sizes
 */
#define CTASSERT(a) extern char __dummy[(a)?1:-1];

#ifdef DEBUG
/** ASSERT definition */
#define ASSERT(x)   assert(x)
#else
#define ASSERT(x)
#endif

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define __always_unused __attribute__((unused))

/** Size in bytes of a cacheline. */
#define CACHE_LINE_SIZE  32

/** Set indicated bits in register */
#define setbits32(_addr, _v) out_be32((_addr), in_be32(_addr) |  (_v))

/** Clear indicated bits in register */
#define clrbits32(_addr, _v) out_be32((_addr), in_be32(_addr) & ~(_v))

/** Expression that uses bitwise operations to evaluate if x == a AND y == b */
#define COND_EXPR1_EQ_AND_EXPR2_EQ(x, a, y, b) \
(!(((x) ^(a)) | ((y) ^ (b))))

/** Expression that uses bitwise operations to evaluate if x == a AND y != b */
#define COND_EXPR1_EQ_AND_EXPR2_NEQ(x, a, y, b) \
(!(((x) ^(a)) | !(((y) ^ (b)))))

/** Return higher 32 bits of physical address */
#define PHYS_ADDR_HI(phys_addr) \
    ((phys_addr) >> 32)

/** Return lower 32 bits of physical address */
#define PHYS_ADDR_LO(phys_addr) \
    ((phys_addr) & 0xFFFFFFFF)


// This is needed to prevent of lib definining its own implementation of
// in_be32 and out_be32 (otherwise unused)
#define REMOTE_IN_OUT_BE32


//////////////////////////////////////////////////////////////////////////////
// Precompilation that enable us to test SNOW F9 or AES CMAC only,
// without doing F8+F9, which is what PDCP control plane really does.
// When selecting PDCP_TEST_SNOW_F9_ONLY do not activate PDCP_TEST_AES_CMAC_ONLY,
// and viceversa.
//////////////////////////////////////////////////////////////////////////////
//#define PDCP_TEST_SNOW_F9_ONLY
//#define PDCP_TEST_AES_CMAC_ONLY

/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/

/*==============================================================================
                                 CONSTANTS
==============================================================================*/


/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/


/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/
/** Write 32bit values */
static inline void out_be32(volatile uint32_t *addr, uint32_t val) {
	*addr = val;
}

/** Read 32bit values */
static inline uint32_t in_be32(const volatile uint32_t *addr) {
	return *addr;
}

/*============================================================================*/


#endif  /* SEC_UTILS_H */
