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
#include "cgreen.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
typedef struct data_for_test_s
{
	list_node_t node;
	int packets_no;
	int val_to_modify;
}data_for_test_t;
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

static void test_list_init_destroy(void)
{
	list_t list;

	list_init(&list, THREAD_UNSAFE_LIST);

	assert_equal_with_message(list.is_empty(&list), 1,
	                              "ERROR on list_is_empty: list not empty!");
	list_destroy(&list);
}

static void test_list_order_of_nodes_addition(void)
{
	list_t list;
#define LIST_NODES 10
	list_node_t nodes[LIST_NODES];
	list_node_t *node = NULL;
	int i = 0;

	memset(nodes, 0, sizeof(nodes));

	list_init(&list, THREAD_UNSAFE_LIST);

	// add a number of nodes to the list
	for (i = 0; i < LIST_NODES; i++)
	{
		list.add_tail(&list, &nodes[i]);
	}

	assert_equal_with_message(list.is_empty(&list), 0,
	                              "ERROR on list_is_empty: list is empty and shouldn't be!");

	// remove all the nodes from the list and validate the order in which they
	// were added to the list
	i = 0;
	while(!list.is_empty(&list))
	{
		node = list.remove_first(&list);
		assert_equal_with_message(node, &nodes[i],
			     "ERROR on list_add_tail: incorrect order of nodes inserted in the list!");
		i++;
	}
	assert_equal_with_message(i, LIST_NODES,
			     "ERROR on list_add_tail: incorrect number of nodes removed from the list: %d!", i);

	assert_equal_with_message(list.is_empty(&list), 1,
		                              "ERROR on list_is_empty: list not empty and it should be!");

	list_destroy(&list);
}

static void test_list_delete_nodes(void)
{
	list_t list;
#define LIST_NODES 10
	list_node_t nodes[LIST_NODES];
	int i = 0;

	memset(nodes, 0, sizeof(nodes));

	list_init(&list, THREAD_UNSAFE_LIST);

	// add a number of nodes to the list
	for (i = 0; i < LIST_NODES; i++)
	{
		list.add_tail(&list, &nodes[i]);
	}

	assert_equal_with_message(list.is_empty(&list), 0,
	                              "ERROR on list_is_empty: list is empty and shouldn't be!");

	// remove all the nodes from the list
	for (i = 0; i < LIST_NODES; i++)
	{
		list.delete_node(&list, &nodes[i]);
	}
	assert_equal_with_message(list.is_empty(&list), 1,
		                              "ERROR on list_is_empty: list not empty and it should be!");

	list_destroy(&list);
}

static void test_list_mixed_adds_and_deletes(void)
{
	list_t list;
#define LIST_NODES 10
	list_node_t nodes[LIST_NODES];
	list_node_t *node = NULL;
	int i = 0;

	memset(nodes, 0, sizeof(nodes));

	list_init(&list, THREAD_UNSAFE_LIST);

	// add half of the available nodes to the list
	for (i = 0; i < LIST_NODES/2; i++)
	{
		list.add_tail(&list, &nodes[i]);
	}

	assert_equal_with_message(list.is_empty(&list), 0,
	                              "ERROR on list_is_empty: list is empty and shouldn't be!");

	// remove the second half of the nodes previously added to the list
	for (i = LIST_NODES/4; i < LIST_NODES/2; i++)
	{
		list.delete_node(&list, &nodes[i]);
	}
	assert_equal_with_message(list.is_empty(&list), 0,
		                              "ERROR on list_is_empty: list is empty and it shouldn't be!");

	// add the other half of nodes to the list
	for (i = LIST_NODES/2; i < LIST_NODES; i++)
	{
		list.add_tail(&list, &nodes[i]);
	}

	assert_equal_with_message(list.is_empty(&list), 0,
								  "ERROR on list_is_empty: list is empty and shouldn't be!");

	// No remove the nodes one by one and validate the order
	// First remove the first quarter
	i = 0;
	while(i < LIST_NODES/4)
	{
		node = list.remove_first(&list);
		assert_equal_with_message(node, &nodes[i],
				 "ERROR on list_remove_first: incorrect order of nodes inserted in the list!");
		i++;
	}

	// Next remove the second half
	i = 0;
	while(!list.is_empty(&list))
	{
		node = list.remove_first(&list);
		assert_equal_with_message(node, &nodes[i + LIST_NODES/2],
				 "ERROR on list_remove_first: incorrect order of nodes inserted in the list!");
		i++;
	}

	assert_equal_with_message(i, LIST_NODES/2,
				 "ERROR: incorrect number of nodes removed from the list: %d!", i);

	assert_equal_with_message(list.is_empty(&list), 1,
				"ERROR on list_is_empty: list not empty and it should be!");

	list_destroy(&list);
}

static void test_list_attach_list_to_tail(void)
{
	list_t list_a;
	list_t list_b;
#define LIST_NODES 10
	list_node_t nodes[LIST_NODES];
	list_node_t *node;
	int i = 0;

	memset(nodes, 0, sizeof(nodes));

	list_init(&list_a, THREAD_UNSAFE_LIST);
	list_init(&list_b, THREAD_UNSAFE_LIST);

	// add a number of nodes to the first list
	for (i = 0; i < LIST_NODES/2; i++)
	{
		list_a.add_tail(&list_a, &nodes[i]);
	}

	assert_equal_with_message(list_a.is_empty(&list_a), 0,
	                              "ERROR on list_is_empty: list_a is empty and shouldn't be!");

	// add a number of nodes to the second list
	for (i = LIST_NODES/2; i < LIST_NODES; i++)
	{
		list_a.add_tail(&list_b, &nodes[i]);
	}

	assert_equal_with_message(list_b.is_empty(&list_b), 0,
								  "ERROR on list_is_empty: list_a is empty and shouldn't be!");

	// attach the second list to the first list
	list_a.attach_list_to_tail(&list_a, &list_b);

	assert_equal_with_message(list_b.is_empty(&list_b), 1,
						  "ERROR on list_is_empty: list_b not empty and it should be!");


	// now remove all the nodes from the first list and check the order
	i = 0;
	while(!list_a.is_empty(&list_a))
	{
		node = list_a.remove_first(&list_a);
		assert_equal_with_message(node, &nodes[i],
				 "ERROR on list_remove_first: incorrect order of nodes inserted in the list!");
		i++;
	}
	assert_equal_with_message(i, LIST_NODES,
				 "ERROR: incorrect number of nodes removed from the list_a: %d!", i);

	assert_equal_with_message(list_a.is_empty(&list_a), 1,
				  "ERROR on list_is_empty: list_a not empty and it should be!");

	list_destroy(&list_a);
	list_destroy(&list_b);
}

static uint8_t node_match_1(list_node_t *node)
{
	data_for_test_t * data = NULL;

	data = (data_for_test_t *)node;

	if(data->packets_no % 2 == 0)
	{
		return 1;
	}

	return 0;
}
#define OLD_TEST_VALUE 5
#define NEW_TEST_VALUE 7

static void node_modify_after_delete_1(list_node_t *node)
{
	data_for_test_t * data = NULL;

	assert(node != NULL);

	data = (data_for_test_t *)node;

	data->val_to_modify = NEW_TEST_VALUE;
}

static void test_list_delete_matching_nodes(void)
{
	list_t list;
	list_t list_of_del_nodes;
#define LIST_NODES 10
	data_for_test_t nodes[LIST_NODES];
	list_node_t * node;
	data_for_test_t * data;

	int i = 0;

	memset(nodes, 0, sizeof(nodes));

	list_init(&list, THREAD_UNSAFE_LIST);
	list_init(&list_of_del_nodes, THREAD_UNSAFE_LIST);

	assert_equal_with_message(list.is_empty(&list), 1,
			"ERROR on list_is_empty: list is not empty and it should be!");
	assert_equal_with_message(list_of_del_nodes.is_empty(&list_of_del_nodes), 1,
			"ERROR on list_is_empty: list is not empty and it should be!");

	// add a number of nodes to the first list
	for (i = 0; i < LIST_NODES; i++)
	{
		list.add_tail(&list, &nodes[i].node);
	}

	assert_equal_with_message(list.is_empty(&list), 0,
			  "ERROR on list_is_empty: list is empty and shouldn't be!");

	// initialize the packets_no field of nodes structures
	for (i = 0; i < LIST_NODES; i++)
	{
		nodes[i].packets_no = i;
		nodes[i].val_to_modify = OLD_TEST_VALUE;
	}

	// remove from the list all the nodes that have packets_no even
	list.delete_matching_nodes(&list, &list_of_del_nodes, &node_match_1, &node_modify_after_delete_1);

	assert_equal_with_message(list_of_del_nodes.is_empty(&list_of_del_nodes), 0,
			  "ERROR on list_is_empty: list_of_del_nodes is empty and it shouldn't be!");


	// now remove all the nodes from the list_of_del_nodes and validate
	// that they have packets_no value even and the member val_to_modify was changed
	// accordingly
	i = 0;
	while(!list_of_del_nodes.is_empty(&list_of_del_nodes))
	{
		node = list_of_del_nodes.remove_first(&list_of_del_nodes);
		data = (data_for_test_t*)node;
		assert_equal_with_message(data->packets_no % 2, 0,
				 "ERROR on list_remove_first: invalid packets_no in the data node!");
		assert_equal_with_message(data->val_to_modify, NEW_TEST_VALUE,
						 "ERROR on list_remove_first: invalid val_to_modify in the data node!");
		i++;
	}
	assert_equal_with_message(i, LIST_NODES/2,
				 "ERROR: incorrect number of nodes removed from the list_of_del_nodes: %d!", i);

	assert_equal_with_message(list_of_del_nodes.is_empty(&list_of_del_nodes), 1,
				  "ERROR: list_of_del_nodes not empty and it should be!");

	// now remove all the nodes from the list and validate
	// that they have packets_no value odd and the member val_to_modify was not changed
	i = 0;
	while(!list.is_empty(&list))
	{
		node = list.remove_first(&list);
		data = (data_for_test_t*)node;
		assert_equal_with_message(data->packets_no % 2, 1,
				 "ERROR on list_remove_first: invalid packets_no in the data node!");
		assert_equal_with_message(data->val_to_modify, OLD_TEST_VALUE,
						 "ERROR on list_remove_first: invalid val_to_modify in the data node!");
		i++;
	}
	assert_equal_with_message(i, LIST_NODES/2,
				 "ERROR: incorrect number of nodes removed from the list: %d!", i);

	assert_equal_with_message(list.is_empty(&list), 1,
				  "ERROR: list not empty and it should be!");

	list_destroy(&list_of_del_nodes);
	list_destroy(&list);
}

static uint8_t node_match_2(list_node_t *node)
{
	assert(node != NULL);

	// delete all nodes
	return 1;
}

static void node_modify_after_delete_2(list_node_t *node)
{
	data_for_test_t * data = NULL;

	assert(node != NULL);

	data = (data_for_test_t *)node;

	data->val_to_modify = NEW_TEST_VALUE;
}

static void test_list_delete_matching_nodes_all(void)
{
	list_t list;
	list_t list_of_del_nodes;
#define LIST_NODES 10
	data_for_test_t nodes[LIST_NODES];
	list_node_t * node;
	data_for_test_t * data;

	int i = 0;

	memset(nodes, 0, sizeof(nodes));

	list_init(&list, THREAD_UNSAFE_LIST);
	list_init(&list_of_del_nodes, THREAD_UNSAFE_LIST);

	assert_equal_with_message(list.is_empty(&list), 1,
			"ERROR on list_is_empty: list is not empty and it should be!");
	assert_equal_with_message(list_of_del_nodes.is_empty(&list_of_del_nodes), 1,
			"ERROR on list_is_empty: list is not empty and it should be!");

	// add a number of nodes to the first list
	for (i = 0; i < LIST_NODES; i++)
	{
		list.add_tail(&list, &nodes[i].node);
	}

	assert_equal_with_message(list.is_empty(&list), 0,
			  "ERROR on list_is_empty: list is empty and shouldn't be!");

	// initialize the packets_no field of nodes structures
	for (i = 0; i < LIST_NODES; i++)
	{
		nodes[i].packets_no = i;
		nodes[i].val_to_modify = OLD_TEST_VALUE;
	}

	// remove from the list all the nodes
	// node_match_2 function always returns true
	list.delete_matching_nodes(&list, &list_of_del_nodes, &node_match_2, &node_modify_after_delete_2);

	assert_equal_with_message(list_of_del_nodes.is_empty(&list_of_del_nodes), 0,
						  "ERROR on list_is_empty: list_of_del_nodes is empty and it shouldn't be!");
	assert_equal_with_message(list.is_empty(&list), 1,
				  "ERROR on list_is_empty: list is not empty and it should be!");

	// now remove all the nodes from the list_of_del_nodes and validate
	// that they have packets_no in order and the member val_to_modify was changed
	// accordingly
	i = 0;
	while(!list_of_del_nodes.is_empty(&list_of_del_nodes))
	{
		node = list_of_del_nodes.remove_first(&list_of_del_nodes);
		data = (data_for_test_t*)node;
		assert_equal_with_message(data->packets_no, i,
				 "ERROR on list_remove_first: invalid packets_no in the data node!");
		assert_equal_with_message(data->val_to_modify, NEW_TEST_VALUE,
						 "ERROR on list_remove_first: invalid val_to_modify in the data node!");
		i++;
	}
	assert_equal_with_message(i, LIST_NODES,
				 "ERROR: incorrect number of nodes removed from the list_of_del_nodes: %d!", i);

	assert_equal_with_message(list_of_del_nodes.is_empty(&list_of_del_nodes), 1,
				  "ERROR: list_of_del_nodes not empty and it should be!");

	list_destroy(&list_of_del_nodes);
	list_destroy(&list);
}

static TestSuite * list_tests()
{
    /* create test suite */
    TestSuite * suite = create_test_suite();

    /* setup/teardown functions to be called before/after each unit test */
//    setup(suite, tests_setup);
//    teardown(suite, tests_teardown);

    /* start adding unit tests */
    add_test(suite, test_list_init_destroy);
    add_test(suite, test_list_order_of_nodes_addition);
    add_test(suite, test_list_delete_nodes);
    add_test(suite, test_list_mixed_adds_and_deletes);
    add_test(suite, test_list_attach_list_to_tail);
    add_test(suite, test_list_delete_matching_nodes);
    add_test(suite, test_list_delete_matching_nodes_all);


    return suite;
} /* list_tests() */
/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/

int main(int argc, char *argv[])
{
    /* *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** */
    /* Be aware that by using run_test_suite() instead of run_single_test(), CGreen will execute
     * each test case in a separate UNIX process, so:
     * (1) unit tests' thread safety might need to be ensured by defining critical regions
     *     (beware, CGreen error messages are not explanatory and intuitive enough)
     *
     * Although it is more difficult to maintain synchronization manually,
     * it is recommended to run_single_test() for each test case.
     */
    //run_test_suite(host_api_tests(), create_text_reporter());

    /* create test suite */
    TestSuite * suite = list_tests();
    TestReporter * reporter = create_text_reporter();

    /* Run tests */
    run_single_test(suite, "test_list_init_destroy", reporter);
    run_single_test(suite, "test_list_order_of_nodes_addition", reporter);
    run_single_test(suite, "test_list_delete_nodes", reporter);
    run_single_test(suite, "test_list_mixed_adds_and_deletes", reporter);
    run_single_test(suite, "test_list_attach_list_to_tail", reporter);
    run_single_test(suite, "test_list_delete_matching_nodes", reporter);
    run_single_test(suite, "test_list_delete_matching_nodes_all", reporter);


    destroy_test_suite(suite);
    (*reporter->destroy)(reporter);

    return 0;
} /* main() */

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
