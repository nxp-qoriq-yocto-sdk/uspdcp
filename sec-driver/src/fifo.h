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

#ifndef FIFO_H
#define FIFO_H

#ifdef __cplusplus
/* *INDENT-OFF* */

extern "C"{
/* *INDENT-ON* */
#endif

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

/** Fifo capacity. Must be power of 2 so that we can use 
    bitfield operations to reset producer/consumer indexes/ */
#define FIFO_CAPACITY   32

/** Update circular counter when maximum value of counter is a power of 2.
 * Use bitwise operations to roll over. */
#define FIFO_CIRCULAR_COUNTER(x, max)   (((x) + 1) & (max - 1))

 /** The number of jobs in a JOB RING */
#define FIFO_NUMBER_OF_ITEMS(ring_max_size, pi, ci) (((pi) < (ci)) ? \
                                                    ((ring_max_size) + (pi) - (ci)) : ((pi) - (ci)))

/** Test if FIFO ring is full. Used ring capacity to be 32 = a power of 2. */
#define FIFO_IS_FULL(pi, ci, ring_max_size, ring_threshold) \
                      (((pi) + 1 + ((ring_max_size) - (ring_threshold))) & (ring_max_size - 1))  == ((ci))

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** Simple FIFO array that can be used with a single 
 *  producer thread and a single consumer thread */
struct fifo_t
{
    volatile uint32_t cidx;                 /*< Consumer index for fifo */
    void* items[FIFO_CAPACITY];             /*< Array of items */
    volatile uint32_t pidx;                 /*< Producer index for fifo */
}____cacheline_aligned;

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/*================================================================================================*/

/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //FIFO_H
