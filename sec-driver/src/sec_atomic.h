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

#ifndef SEC_ATOMIC_H
#define SEC_ATOMIC_H

/*==============================================================================
                                INCLUDE FILES
==============================================================================*/
#include <stdint.h>

/*==============================================================================
                              DEFINES AND MACROS
==============================================================================*/

/** Wrappers over GCC's builtins for atomic variable access. */

 /** Perform the operation suggested by the name, and return the value that had previously been in memory.*/
#define atomic_load_sub(ptr, value, ...) \
    __sync_fetch_and_sub (ptr, value , ##__VA_ARGS__)

 /** Perform the operation suggested by the name, and return the value that had previously been in memory.*/
#define atomic_load_add(ptr, value, ...) \
    __sync_fetch_and_add (ptr, value , ##__VA_ARGS__)

 /** Perform the operation suggested by the name, and return the value that had previously been in memory.*/
#define atomic_load_or(ptr, value, ...) \
    __sync_fetch_and_or (ptr, value , ##__VA_ARGS__)

 /** Perform the operation suggested by the name, and return the value that had previously been in memory.*/
#define atomic_load_and(ptr, value, ...) \
    __sync_fetch_and_and (ptr, value , ##__VA_ARGS__)

 /** Perform the operation suggested by the name, and return the value that had previously been in memory.*/
#define atomic_load_xor(ptr, value, ...) \
    __sync_fetch_and_xor (ptr, value , ##__VA_ARGS__)

#define atomic_load_nand(ptr, value, ...) \
    __sync_fetch_and_nand (ptr, value , ##__VA_ARGS__)

/** Perform the operation suggested by the name, and return the new value.*/
#define atomic_sub_load(ptr, value, ...) \
    __sync_sub_and_fetch (ptr, value , ##__VA_ARGS__)

/** Perform the operation suggested by the name, and return the new value.*/
#define atomic_add_load(ptr, value, ...) \
    __sync_add_and_fetch (ptr, value , ##__VA_ARGS__)

/** Perform the operation suggested by the name, and return the new value.*/
#define atomic_or_load(ptr, value, ...) \
    __sync_or_and_fetch (ptr, value , ##__VA_ARGS__)
    
/** Perform the operation suggested by the name, and return the new value.*/
#define atomic_and_load(ptr, value, ...) \
    __sync_and_and_fetch (ptr, value , ##__VA_ARGS__)
    
/** Perform the operation suggested by the name, and return the new value.*/
#define atomic_xor_load(ptr, value, ...) \
    __sync_xor_and_fetch (ptr, value , ##__VA_ARGS__)

/** Perform the operation suggested by the name, and return the new value.*/
#define atomic_nand_load(ptr, value, ...) \
    __sync_nand_and_fetch (ptr, value , ##__VA_ARGS__)

/** Perform an atomic compare and swap. 
 * If the current value of *ptr is oldval, then write newval  into *ptr.
 * Return true if the comparison is successful and newval was written.*/
#define atomic_bool_compare_and_swap(ptr, oldval, newval, ...) \
    __sync_bool_compare_and_swap (ptr, oldval, newval , ##__VA_ARGS__)

/** Perform an atomic compare and swap. 
 * If the current value of *ptr is oldval, then write newval  into *ptr.
 * Return the contents of *ptr before the operation.*/
#define atomic_val_compare_and_swap(ptr, oldval, newval, ...) \
    __sync_val_compare_and_swap (ptr, oldval, newval , ##__VA_ARGS__)

/** Issue a full memory barrier. */
#define atomic_synchronize(...) \
    __sync_synchronize ( , ##__VA_ARGS__)

/** Write value into *ptr, and return the previous contents of *ptr.
 * This builtin is not a full barrier, but rather an acquire barrier.*/
#define atomic_lock_test_and_set(ptr, value, ...) \
    __sync_lock_test_and_set (ptr, value , ##__VA_ARGS__)

/** Release the lock acquired by __sync_lock_test_and_set. 
 * Normally this means writing the constant 0 to *ptr.
 * This builtin is not a full barrier, but rather a release barrier.*/
#define atomic_lock_release(ptr, ...) \
    __sync_lock_release (ptr , ##__VA_ARGS__)

// TODO: remove this once external memory management mechanism is used:
// ptov + vtop functions provided by external higher layer application.
// This is required in the mean time because it conflicts with same
// function defined in compat.h from dma_mem library.
#define REMOTE_ATOMIC_TYPE

/*==============================================================================
                                    ENUMS
==============================================================================*/

/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/

typedef uint32_t    atomic_t;

/*==============================================================================
                                 CONSTANTS
==============================================================================*/


/*==============================================================================
                         GLOBAL VARIABLE DECLARATIONS
==============================================================================*/


/*==============================================================================
                            FUNCTION PROTOTYPES
==============================================================================*/

/*============================================================================*/


#endif  /* SEC_ATOMIC_H */
