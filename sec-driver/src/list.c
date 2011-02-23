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
#include "list.h"
#include "sec_internal.h"

#include <assert.h>
/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

uint32_t list_init(list_t * list, uint8_t thread_safe)
{
	assert(list != NULL);

	list->head.next = list->head.prev = &list->head;

	// TODO: how to handle synchronization???
	list->thread_safe = thread_safe;
	if (list->thread_safe == THREAD_SAFE)
	{
		pthread_mutex_init(&list->mutex, NULL);
	}
	return 0;
}

uint32_t list_destroy(list_t *list)
{
	list->head.next = list->head.prev = NULL;

	if (list->thread_safe == THREAD_SAFE)
	{
		pthread_mutex_destroy(&list->mutex);
	}
	return 0;
}

uint8_t list_empty(list_t * list)
{
	assert(list != NULL);

	return ((list->head.next == &list->head) && (list->head.prev == &list->head));
}

uint8_t list_end(list_t * list, list_node_t * node)
{
	assert(list != NULL);
	assert(node != NULL);

	return (node->next == &list->head);
}

list_node_t* list_remove_first(list_t *list)
{
	assert(list != NULL);
	list_node_t * node = list->head.next;

	assert(node != NULL);
	assert(node->next != NULL);
	assert(node->next->prev != NULL);

	list->head.next = node->next;
	node->next->prev = &list->head;

	node->next = node;
	node->prev = node;

	return node;
}

list_node_t* list_remove_first_with_lock(list_t *list)
{
	return NULL;
}

void list_add_tail(list_t *list, list_node_t* node)
{
	assert(list != NULL);
	assert(node != NULL);

	list->head.prev->next = node;
	node->prev = list->head.prev;
	list->head.prev = node;
	node->next = &list->head;

	assert(node->next != NULL);
	assert(node->prev != NULL);
}

void list_add_tail_with_lock(list_t *list, list_node_t* node)
{
}

void list_delete(list_node_t *node)
{
	assert(node != NULL);
	assert(node->next != NULL);
	assert(node->prev != NULL);
	assert(node->prev->next != NULL);
	assert(node->next->prev != NULL);

	node->prev->next = node->next;
	node->next->prev = node->prev;

	node->next = node;
	node->prev = node;
}

void list_delete_with_lock(list_node_t *node)
{
	assert(node != NULL);
	assert(node->next != NULL);
	assert(node->prev != NULL);
}

list_node_t* get_first(list_t * list)
{
	assert(list != NULL);
	assert(list->head.next != NULL);

	return list->head.next;
}

list_node_t* get_next(list_node_t * node)
{
	assert(node != NULL);
	assert(node->next != NULL);

	return node->next;
}

/*
int main(void)
{
	list_t list;
	list_init(&list);

	list_node_t node[10];

	list_add_tail(&node[0]);
	list_add_tail(&node[1]);
	list_add_tail(&node[2]);
}
*/
/*================================================================================================*/

#ifdef __cplusplus
}
#endif
