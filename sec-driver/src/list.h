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

#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
/* *INDENT-OFF* */

extern "C"{
/* *INDENT-ON* */
#endif

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
#ifdef USDPAA
#include <usdpaa/compat.h>
#else
#include <compat.h>
#endif
#include <stdint.h>
#include <pthread.h>
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define THREAD_SAFE_LIST        0
#define THREAD_UNSAFE_LIST      1
/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
struct list_s;

/** @brief Check if list is empty.
 *
 * @param [in] list              Pointer to the list.
 *
 * @return 1 if list is empty
 * @return 0 if list is not empty
 * */
typedef uint8_t (*is_empty_func)(struct list_s * list);

/** @brief Add a node to the tail of the list.
 *
 * @param [in] list              Pointer to the list.
 * @param [in] node              Pointer to the node that must be added to the list.
 * */
typedef void (*add_tail_func)(struct list_s *list, struct list_head *node);

/** @brief Remove the node from the head of the list.
 *
 * @note This function should be called on a non-empty list.
 *
 * @pre The list is not empty.
 *
 * @param [in] list              Pointer to the list.
 *
 * @return      The node removed from the head of the list.
 * */
typedef struct list_head* (*remove_first_func)(struct list_s *list);

/** @brief Remove a specified node from the list (the node can be
 * located anywhere in the list).
 *
 * @note This function should be called on a non-empty list.
 *
 * @pre The list is not empty.
 *
 * @param [in] list              Pointer to the list.
 * @param [in] node              Pointer to the node that must be deleted from the list.
 * */
typedef void (*delete_node_func)(struct list_s * list, struct list_head *node);

/** @brief Add the nodes from one list to the tail of another list.
 *
 * @note This function should be called for a non-empty new list.
 *
 * @pre The new_list is not empty.
 *
 * @param [in] list              The list to which the new nodes will be attached.
 * @param [in] new_list          The list containing the nodes to be attached to another list.
 * */
typedef void (*attach_list_to_tail_func)(struct list_s *list, struct list_s *new_list);

/** @brief Function that identifies a matching node against the user's criteria.
 *
 * @note This function is implemented by the user of the list and is called from the
 * delete_matching_nodes_func() function.
 *
 * @param [in] node              Pointer to the node that is checked if it matches the user criteria.
 *
 * @return 1 if node matches
 * @return 0 if node does not match
 * */
typedef uint8_t (*node_match_func)(struct list_head *node);

/** @brief Function that modifies a node after being deleted from a list.
 *
 * @note This function is implemented by the user of the list and is called from the
 * delete_matching_nodes_func() function.
 *
 * @param [in] node              Pointer to the node that is checked if it matches the user criteria.
 *
 * */
typedef void (*node_modify_after_delete_func)(struct list_head *node);

/** @brief Function that deletes all the nodes that match a user criteria.
 * After deleting the nodes, they are modified according to user's desires and placed
 * in a temporary list. The list of deleted nodes is provided to user for further
 * processing like attaching them to another list.
 *
 * @param [in] list                       The list from which all matching nodes are deleted.
 * @param [in] deleted_nodes_list         The list in which the deleted nodes are added.
 *                                        A valid (initialized) list must be provided by the user.
 * @param [in] is_match                   Pointer to a function of type ::node_match_func .
 * @param [in] node_modify_after_delete   Pointer to a function of type ::node_modify_after_delete_func .
 *
 * */
typedef void (*delete_matching_nodes_func)(struct list_s *list,
	                                       struct list_s *deleted_nodes_list,
		                                   node_match_func is_match,
		                                   node_modify_after_delete_func node_modify_after_delete);

/** Declaration of a double-linked circular list. */
typedef struct list_s
{
	/** Pointer to a function that checks if the list is empty.
	 *
	 * @note If the list was configured to be thread safe then this function CAN
	 * be called from multiple threads simultaneously.
	 * @note If the list was configured to be thread safe then this function CANNOT
	 * be called from multiple threads simultaneously.
	 *
	 * @see For more details see the type definition of the function pointer.
	 * */
	is_empty_func is_empty;
	/** Pointer to a function that adds a node to the tail of the list (before
	 * the head of the list).
	 *
	 * @note If the list was configured to be thread safe then this function CAN
	 * be called from multiple threads simultaneously.
	 * @note If the list was configured to be thread safe then this function CANNOT
	 * be called from multiple threads simultaneously.
	 *
	 * @see For more details see the type definition of the function pointer.
	 * */
	add_tail_func add_tail;
	/** Pointer to a function that removes the first node in the list (the first node
	 * after the head of the list).
	 *
	 * @note If the list was configured to be thread safe then this function CAN
	 * be called from multiple threads simultaneously.
	 * @note If the list was configured to be thread safe then this function CANNOT
	 * be called from multiple threads simultaneously.
	 *
	 * @see For more details see the type definition of the function pointer.
	 * */
	remove_first_func remove_first;
	/** Pointer to a function that removes a specified node from the middle of
	 * the list.
	 *
	 * @note If the list was configured to be thread safe then this function CAN
	 * be called from multiple threads simultaneously.
	 * @note If the list was configured to be thread safe then this function CANNOT
	 * be called from multiple threads simultaneously.
	 *
	 * @see For more details see the type definition of the function pointer.
	 * */
	delete_node_func delete_node;
	/** Pointer to a function that attaches at the end of the list another list.
	 *
	 * @note If the list was configured to be thread safe then this function CAN
	 * be called from multiple threads simultaneously.
	 * @note If the list was configured to be thread safe then this function CANNOT
	 * be called from multiple threads simultaneously.
	 *
	 * @see For more details see the type definition of the function pointer.
	 * */
	attach_list_to_tail_func attach_list_to_tail;
	/** Pointer to a function that deletes all matching nodes from the list.
	 *
	 * @note If the list was configured to be thread safe then this function CAN
	 * be called from multiple threads simultaneously.
	 * @note If the list was configured to be thread safe then this function CANNOT
	 * be called from multiple threads simultaneously.
	 *
	 * @see For more details see the type definition of the function pointer.
	 * */
	delete_matching_nodes_func delete_matching_nodes;

	/** Dummy node of the list. Can be considered as both
	 * the head and the tail of the list, because this is a
	 * circular list. */
	struct list_head head;

	/** Access to list can be synchronized, if requested so.
	 * The synchronization is done using a mutex. */
	pthread_mutex_t mutex;

	/** The list's thread safeness configured by the user of this list. */
	uint8_t thread_safe;
}list_t;
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

/** @brief Initialize a double-linked circular list.
 *
 * @note The list operations are accessible via function pointers
 * located in the list_t structure. Depending on the thread_safe
 * value, the function pointers point to synchronized functions or not.
 * For more details on the available list operations @see ::list_t .
 *
 * @param [in] list              Pointer to the list.
 * @param [in] thread_safe       Configure the thread safeness.
 *                               Valid values: #THREAD_SAFE_LIST, #THREAD_UNSAFE_LIST
 * */
inline void list_init(list_t * list, uint8_t thread_safe);

/** @brief Destroy a double-linked circular list.
 *
 * Destroys the mutex if needed and memsets the list with 0.
 *
 * @param [in] list              Pointer to the list.
 * */
inline void list_destroy(list_t *list);

/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //LIST_H
