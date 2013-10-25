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
#include "sec_utils.h"
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
/** Unsynchronized implementation of a function of type ::is_empty_func .
 *
 * @see Description of type ::is_empty_func
 * */
static uint8_t list_empty_no_lock(list_t * list);
/** Synchronized implementation of a function of type ::is_empty_func .
 *
 * @see Description of type ::is_empty_func
 * */
static uint8_t list_empty_with_lock(list_t * list);

/** Unsynchronized implementation of a function of type ::remove_first_func .
 *
 * @see Description of type ::remove_first_func
 * */
static struct list_head* list_remove_first_no_lock(list_t *list);
/** Synchronized implementation of a function of type ::remove_first_func .
 *
 * @see Description of type ::remove_first_func
 * */
static struct list_head* list_remove_first_with_lock(list_t *list);

/** Unsynchronized implementation of a function of type ::add_tail_func .
 *
 * @see Description of type ::add_tail_func
 * */
static void list_add_tail_no_lock(list_t *list, struct list_head *node);

/** Synchronized implementation of a function of type ::add_tail_func .
 *
 * @see Description of type ::add_tail_func
 * */
static void list_add_tail_with_lock(list_t *list, struct list_head* node);

/** Unsynchronized implementation of a function of type ::delete_node_func .
 *
 * @see Description of type ::delete_node_func
 * */
static void list_delete_node_no_lock(list_t * list, struct list_head *node);
/** Synchronized implementation of a function of type ::delete_node_func .
 *
 * @see Description of type ::delete_node_func
 * */
static void list_delete_node_with_lock(list_t * list, struct list_head *node);

/** Unsynchronized implementation of a function of type ::attach_list_to_tail_func .
 *
 * @see Description of type ::attach_list_to_tail_func
 * */
static void list_attach_list_to_tail_no_lock(list_t *list, list_t *new_list);
/** Synchronized implementation of a function of type ::attach_list_to_tail_func .
 *
 * @see Description of type ::attach_list_to_tail_func
 * */
static void list_attach_list_to_tail_with_lock(list_t *list, list_t *new_list);

/** Unsynchronized implementation of a function of type ::delete_matching_nodes_func .
 *
 * @see Description of type ::delete_matching_nodes_func
 * */
static void list_delete_matching_nodes_no_lock(list_t *list,
		                        list_t *deleted_nodes_list,
		                        node_match_func is_match,
                                node_modify_after_delete_func node_modify_after_delete);
/** Synchronized implementation of a function of type ::delete_matching_nodes_func .
 *
 * @see Description of type ::delete_matching_nodes_func
 * */
static void list_delete_matching_nodes_with_lock(list_t *list,
		                        list_t *deleted_nodes_list,
		                        node_match_func is_match,
                                node_modify_after_delete_func node_modify_after_delete);

/** @brief Get a pointer to first node from the list. Do no remove the node.
 *
 * @pre The list must not be empty.
 *
 * @note Unsynchronized implementation.
 *
 * @param [in] list                       The list.
 *
 * @return Pointer to the first node in the list.
 * */
static struct list_head* get_first(list_t * list);

/** @brief Get a pointer to the next node starting from a specific node in the list.
 *
 * @note Unsynchronized implementation.
 *
 * @note If specified node is the last node in the list, this function returns the dummy
 *       node of the list. The list_end() function should be called to identify the
 *       end of the list.
 *
 * @param [in] node                The node.
 *
 * @return Pointer to the first node following the specified node.
 * */
static struct list_head* get_next(struct list_head * node);

/** @brief Checks if a specified node is the end of the list (the dummy node)
 *
 * @note Unsynchronized implementation.
 *
 * @param [in] list                The list.
 * @param [in] node                The node.
 *
 * @return 0 if specified node is not the dummy node of the list.
 * @return 1 if specified node is the dummy node of the list meaning that the end of the list
 *           was reached.
 * */
static uint8_t list_end(list_t * list, struct list_head *node);
/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static uint8_t list_empty_no_lock(list_t *list)
{
	return list_empty(&list->head);
}

static uint8_t list_empty_with_lock(list_t * list)
{
	uint8_t ret;
#ifdef DEBUG
    int mutex_ret;
#endif

	ASSERT(list != NULL);
#ifdef DEBUG
    mutex_ret = 
#endif
    pthread_mutex_lock(&list->mutex);
    ASSERT(mutex_ret == 0);

	ret = list_empty(&list->head);
#ifdef DEBUG
    mutex_ret =
#endif
	pthread_mutex_unlock(&list->mutex);
    
    ASSERT(mutex_ret == 0);


	return ret;
}

static uint8_t list_end(list_t * list, struct list_head *node)
{
	ASSERT(list != NULL);
	ASSERT(node != NULL);

	return (node == &list->head);
}

static struct list_head* list_remove_first_no_lock(list_t *list)
{

	struct list_head *node = list->head.next;

	ASSERT(node != NULL);
	ASSERT(node->next != NULL);
	ASSERT(node->next->prev != NULL);

	list->head.next = node->next;
	node->next->prev = &list->head;

	node->next = node;
	node->prev = node;

	return node;
}

static struct list_head* list_remove_first_with_lock(list_t *list)
{
	struct list_head *node;

	ASSERT(list != NULL);
	if( pthread_mutex_lock(&list->mutex) != 0 )
        return NULL;

	node = list_remove_first_no_lock(list);

	if( pthread_mutex_unlock(&list->mutex) != 0 )
        return NULL;

	return node;
}

static void list_add_tail_no_lock(list_t *list, struct list_head *node)
{
	ASSERT(node != NULL);
    ASSERT( list->head.prev != NULL );
    ASSERT( &list->head != NULL );

    list_add_tail(node, &list->head);

}

static void list_add_tail_with_lock(list_t *list, struct list_head *node)
{
#ifdef DEBUG
    int ret;
#endif
	ASSERT(list != NULL);

#ifdef DEBUG
    ret =
#endif
    pthread_mutex_lock(&list->mutex);

    ASSERT(ret == 0);

    list_add_tail(node, &list->head);

#ifdef DEBUG
    ret =
#endif
    pthread_mutex_unlock(&list->mutex);
    
    ASSERT(ret == 0);
}

static void list_delete_node_no_lock(list_t * list, struct list_head *node)
{
	ASSERT(node != NULL);
	ASSERT(node->next != NULL);
	ASSERT(node->prev != NULL);
	ASSERT(node->prev->next != NULL);
	ASSERT(node->next->prev != NULL);

	node->prev->next = node->next;
	node->next->prev = node->prev;

	node->next = node;
	node->prev = node;
}

static void list_delete_node_with_lock(list_t * list, struct list_head *node)
{
#ifdef DEBUG
    int ret;
#endif
    ASSERT(list != NULL);

#ifdef DEBUG
    ret =
#endif
    pthread_mutex_lock(&list->mutex);
    
    ASSERT(ret == 0);
	list_delete_node_no_lock(list, node);

#ifdef DEBUG
    ret =
#endif
	pthread_mutex_unlock(&list->mutex);
    
    ASSERT(ret == 0);
}

static struct list_head* get_first(list_t * list)
{
	ASSERT(list != NULL);
	ASSERT(list->head.next != NULL);

	return list->head.next;
}

static struct list_head* get_next(struct list_head *node)
{
	ASSERT(node != NULL);
	ASSERT(node->next != NULL);

	return node->next;
}

static void list_attach_list_to_tail_no_lock(list_t *list, list_t *new_list)
{
	ASSERT(new_list != NULL);
	ASSERT(list_empty_no_lock(new_list) == 0);

	// connect first node from the new list with the last node from the first list
	list->head.prev->next = new_list->head.next;
	new_list->head.next->prev = list->head.prev;

	// connect the last node from the new list with the head of the first list
	list->head.prev = new_list->head.prev;
	new_list->head.prev->next = &list->head;

	// mark the new list empty
	new_list->head.next = new_list->head.prev = &new_list->head;
}

static void list_attach_list_to_tail_with_lock(list_t *list, list_t *new_list)
{
#ifdef DEBUG
    int ret;
#endif
	ASSERT(list != NULL);
#ifdef DEBUG
    ret =
#endif
	pthread_mutex_lock(&list->mutex);
    
    ASSERT(ret == 0);

    list_attach_list_to_tail_no_lock(list, new_list);

#ifdef DEBUG
    ret =
#endif
    pthread_mutex_unlock(&list->mutex);
    
    ASSERT(ret == 0);
}

static void list_delete_matching_nodes_no_lock(list_t *list,
		                        list_t *deleted_nodes_list,
		                        node_match_func is_match,
                                node_modify_after_delete_func node_modify_after_delete)
{
	struct list_head *node = NULL;
	struct list_head *node_to_delete = NULL;

	ASSERT(deleted_nodes_list != NULL);
	ASSERT(is_match != NULL);
	ASSERT(node_modify_after_delete != NULL);

	node = get_first(list);
	do{
		if(is_match(node) == 1)
		{
			// remember the node to be removed from retire list
			node_to_delete = node;
			// get the next node from the retire list
			node = get_next(node);

			// remove saved node from retire list
			list_delete_node_no_lock(list, node_to_delete);

			// modify the deleted node
			node_modify_after_delete(node_to_delete);

			// add deleted node to list of deleted nodes
			list_add_tail(node_to_delete, &deleted_nodes_list->head);
		}
		else
		{
			node = get_next(node);
		}
	}while(!list_end(list, node));
}

static void list_delete_matching_nodes_with_lock(list_t *list,
		                                         list_t *deleted_nodes_list,
		                                         node_match_func is_match,
		                                         node_modify_after_delete_func node_modify_after_delete)
{
#ifdef DEBUG
    int ret;
#endif
	ASSERT(list != NULL);
#ifdef DEBUG
    ret =
#endif
    pthread_mutex_lock(&list->mutex);
    
    ASSERT(ret == 0);

    list_delete_matching_nodes_no_lock(list, deleted_nodes_list, is_match, node_modify_after_delete);

#ifdef DEBUG
    ret =
#endif
    pthread_mutex_unlock(&list->mutex);
    
    ASSERT(ret == 0);
}

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

void list_init(list_t * list, uint8_t thread_safe)
{
	ASSERT(list != NULL);

	list->head.next = list->head.prev = &list->head;

	list->thread_safe = thread_safe;
	ASSERT(thread_safe == THREAD_SAFE_LIST || thread_safe == THREAD_UNSAFE_LIST);

	// if list needs to be thread safe initialize
	// the pointers to the functions with synchronized functions
	if (list->thread_safe == THREAD_SAFE_LIST)
	{
		pthread_mutex_init(&list->mutex, NULL);

		list->is_empty = &list_empty_with_lock;
		list->add_tail = &list_add_tail_with_lock;
		list->remove_first = &list_remove_first_with_lock;
		list->delete_node = &list_delete_node_with_lock;
		list->attach_list_to_tail = &list_attach_list_to_tail_with_lock;
		list->delete_matching_nodes = &list_delete_matching_nodes_with_lock;
	}
	else
	{
		list->is_empty = &list_empty_no_lock;
		list->add_tail = &list_add_tail_no_lock;
		list->remove_first = &list_remove_first_no_lock;
		list->delete_node = &list_delete_node_no_lock;
		list->attach_list_to_tail = &list_attach_list_to_tail_no_lock;
		list->delete_matching_nodes = &list_delete_matching_nodes_no_lock;
	}
}

void list_destroy(list_t *list)
{
#ifdef DEBUG
    int ret;
#endif
	if (list->thread_safe == THREAD_SAFE_LIST)
	{
#ifdef DEBUG
        ret =
#endif
        pthread_mutex_destroy(&list->mutex);

        ASSERT(ret == 0);
	}
	memset(list, 0, sizeof(list));
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
