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
#include "sec_contexts.h"
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

static void test_contexts_pool_init_destroy(void)
{
	sec_contexts_pool_t pool;
	int ret = 0;

#define NO_OF_CONTEXTS 10

	ret = init_contexts_pool(&pool, NO_OF_CONTEXTS, THREAD_UNSAFE_POOL);

	assert_equal_with_message(ret, 0,
			  "ERROR on init_contexts_pool: ret = %d!", ret);

	destroy_contexts_pool(&pool);
}

static void test_contexts_pool_get_free_contexts(void)
{
	sec_contexts_pool_t pool;
	int ret = 0, i = 0;
#define NO_OF_CONTEXTS 10
	sec_context_t* sec_ctxs[NO_OF_CONTEXTS];

	ret = init_contexts_pool(&pool, NO_OF_CONTEXTS, THREAD_UNSAFE_POOL);

	assert_equal_with_message(ret, 0,
			  "ERROR on init_contexts_pool: ret = %d!", ret);

	// get all the contexts in the pool
	for (i = 0; i < NO_OF_CONTEXTS; i++)
	{
		sec_ctxs[i] = get_free_context(&pool);
		assert_not_equal_with_message(sec_ctxs[i], 0,
				  "ERROR on get_free_context: no more contexts available and there should be (%d)", i);
		assert_equal_with_message(CONTEXT_GET_STATE(sec_ctxs[i]->state_packets_no), SEC_CONTEXT_USED,
				  "ERROR on get_free_context: invalid state of context!");
		assert_equal_with_message(sec_ctxs[i]->pool, &pool,
				  "ERROR on get_free_context: invalid pool pointer in context!");
		assert_equal_with_message(CONTEXT_GET_PACKETS_NO(sec_ctxs[i]->state_packets_no), 0,
				  "ERROR on get_free_context: invalid packets_no in context!");
	}
	// try and get another context -> we should receive none
	assert_equal_with_message(get_free_context(&pool), 0,
			  "ERROR on get_free_context: free contexts available and there should be node!");

	destroy_contexts_pool(&pool);
}

static void test_contexts_pool_free_contexts_with_no_packets_in_flight(void)
{
	sec_contexts_pool_t pool;
	int ret = 0, i = 0;
#define NO_OF_CONTEXTS 10
	sec_context_t* sec_ctxs[NO_OF_CONTEXTS];

	ret = init_contexts_pool(&pool, NO_OF_CONTEXTS, THREAD_UNSAFE_POOL);

	assert_equal_with_message(ret, 0,
			  "ERROR on init_contexts_pool: ret = %d!", ret);

	// get all the contexts in the pool
	for (i = 0; i < NO_OF_CONTEXTS; i++)
	{
		sec_ctxs[i] = get_free_context(&pool);
		assert_not_equal_with_message(sec_ctxs[i], 0,
				  "ERROR on get_free_context: no more contexts available and there should be (%d)", i);
		assert_equal_with_message(CONTEXT_GET_STATE(sec_ctxs[i]->state_packets_no), SEC_CONTEXT_USED,
				  "ERROR on get_free_context: invalid state of context!");
		assert_equal_with_message(sec_ctxs[i]->pool, &pool,
				  "ERROR on get_free_context: invalid pool pointer in context!");
		assert_equal_with_message(CONTEXT_GET_PACKETS_NO(sec_ctxs[i]->state_packets_no), 0,
				  "ERROR on get_free_context: invalid packets_no in context!");
	}
	// try and get another context -> we should receive none
	assert_equal_with_message(get_free_context(&pool), 0,
			  "ERROR on get_free_context: free contexts available and there should be node!");

	// free all the contexts -> the contexts have no packets in flight
	for (i = 0; i < NO_OF_CONTEXTS; i++)
	{
		ret = free_or_retire_context(&pool, sec_ctxs[i]);
		assert_equal_with_message(ret, SEC_SUCCESS,
				  "ERROR on free_or_retire_context: ret = (%d)", ret);
		assert_equal_with_message(CONTEXT_GET_STATE(sec_ctxs[i]->state_packets_no), SEC_CONTEXT_UNUSED,
				  "ERROR on free_or_retire_context: invalid state of context!");
		assert_equal_with_message(sec_ctxs[i]->pool, &pool,
				  "ERROR on free_or_retire_context: invalid pool pointer in context!");
		assert_equal_with_message(CONTEXT_GET_PACKETS_NO(sec_ctxs[i]->state_packets_no), 0,
				  "ERROR on free_or_retire_context: invalid packets_no in context!");
	}

	// try and get one context -> it should work ok because we just freed some
	sec_ctxs[0] = get_free_context(&pool);
	assert_not_equal_with_message(sec_ctxs[0], 0,
		  "ERROR on get_free_context: free contexts not available and there should be!");

	destroy_contexts_pool(&pool);
}

static void test_contexts_pool_free_contexts_with_packets_in_flight(void)
{
	sec_contexts_pool_t pool;
	int ret = 0, i = 0;
#define NO_OF_CONTEXTS 10
	sec_context_t* sec_ctxs[NO_OF_CONTEXTS];
	sec_context_t* sec_ctx = NULL;

	ret = init_contexts_pool(&pool, NO_OF_CONTEXTS, THREAD_UNSAFE_POOL);

	assert_equal_with_message(ret, 0,
			  "ERROR on init_contexts_pool: ret = %d!", ret);

	// get all the contexts in the pool
	for (i = 0; i < NO_OF_CONTEXTS; i++)
	{
		sec_ctxs[i] = get_free_context(&pool);
		assert_not_equal_with_message(sec_ctxs[i], 0,
				  "ERROR on get_free_context: no more contexts available and there should be (%d)", i);
		assert_equal_with_message(CONTEXT_GET_STATE(sec_ctxs[i]->state_packets_no), SEC_CONTEXT_USED,
				  "ERROR on get_free_context: invalid state of context!");
		assert_equal_with_message(sec_ctxs[i]->pool, &pool,
				  "ERROR on get_free_context: invalid pool pointer in context!");
		assert_equal_with_message(CONTEXT_GET_PACKETS_NO(sec_ctxs[i]->state_packets_no), 0,
				  "ERROR on get_free_context: invalid packets_no in context!");
	}
	// try and get another context -> we should receive none
	assert_equal_with_message(get_free_context(&pool), 0,
			  "ERROR on get_free_context: free contexts available and there should be node!");

	// add packets in flight for all the contexts with an even number
	for (i = 0; i < NO_OF_CONTEXTS; i+=2)
	{
		CONTEXT_SET_PACKETS_NO(sec_ctxs[i]->state_packets_no, i+1);
	}

	// free all the contexts -> half of them have packets in flight and the other half no
	// packets in flight
	for (i = 0; i < NO_OF_CONTEXTS; i++)
	{
		ret = free_or_retire_context(&pool, sec_ctxs[i]);
		if (i%2 == 0)
		{
			assert_equal_with_message(ret, SEC_PACKETS_IN_FLIGHT,
					"ERROR on free_or_retire_context: should have returned SEC_PACKETS_IN_FLIGHT ret = (%d)", ret);
			assert_equal_with_message(CONTEXT_GET_STATE(sec_ctxs[i]->state_packets_no), SEC_CONTEXT_RETIRING,
					"ERROR on free_or_retire_context: invalid state of context!");
			assert_equal_with_message(CONTEXT_GET_PACKETS_NO(sec_ctxs[i]->state_packets_no), i+1,
					"ERROR on free_or_retire_context: invalid packets_no in context (%d)!", 
                    CONTEXT_GET_PACKETS_NO(sec_ctxs[i]->state_packets_no));
		}
		else
		{
			assert_equal_with_message(ret, SEC_SUCCESS,
					"ERROR on free_or_retire_context: should have returned SEC_SUCCESS ret = (%d)", ret);
			assert_equal_with_message(CONTEXT_GET_STATE(sec_ctxs[i]->state_packets_no), SEC_CONTEXT_UNUSED,
					"ERROR on free_or_retire_context: invalid state of context!");
			assert_equal_with_message(CONTEXT_GET_PACKETS_NO(sec_ctxs[i]->state_packets_no), 0,
					"ERROR on free_or_retire_context: invalid packets_no in context (%d)!", 
                    CONTEXT_GET_PACKETS_NO(sec_ctxs[i]->state_packets_no));
		}

		assert_equal_with_message(sec_ctxs[i]->pool, &pool,
				  "ERROR on free_or_retire_context: invalid pool pointer in context!");

	}

	// now remove the packets in flight for all the contexts with an even number
	for (i = 0; i < NO_OF_CONTEXTS; i+=2)
	{
		CONTEXT_SET_PACKETS_NO(sec_ctxs[i]->state_packets_no, 0);
	}

	// the contexts should be freed at the next call of get_free_context() or free_or_retire_context()
	// try and get one context -> it should work ok because we just freed some
	sec_ctx = get_free_context(&pool);
	assert_not_equal_with_message(sec_ctx, 0,
		  "ERROR on get_free_context: free contexts not available and there should be!");

	ret = free_or_retire_context(&pool, sec_ctx);
	assert_equal_with_message(ret, SEC_SUCCESS,
			  "ERROR on free_or_retire_context: should have returned SEC_SUCCESS ret = (%d)", ret);

	// now check that all the contexts are unused
	for (i = 0; i < NO_OF_CONTEXTS; i++)
	{
		assert_equal_with_message(CONTEXT_GET_STATE(sec_ctxs[i]->state_packets_no), SEC_CONTEXT_UNUSED,
						"ERROR on free_or_retire_context: invalid state of context %d: %d!", i, 
                        CONTEXT_GET_STATE(sec_ctxs[i]->state_packets_no));
	}

	destroy_contexts_pool(&pool);
}

static TestSuite * contexts_pool_tests()
{
    /* create test suite */
    TestSuite * suite = create_test_suite();

    /* setup/teardown functions to be called before/after each unit test */
//    setup(suite, tests_setup);
//    teardown(suite, tests_teardown);

    /* start adding unit tests */
    add_test(suite, test_contexts_pool_init_destroy);
    add_test(suite, test_contexts_pool_get_free_contexts);
    add_test(suite, test_contexts_pool_free_contexts_with_no_packets_in_flight);
    add_test(suite, test_contexts_pool_free_contexts_with_packets_in_flight);



    return suite;
} /* contexts_pool_tests() */
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
    TestSuite * suite = contexts_pool_tests();
    TestReporter * reporter = create_text_reporter();

    /* Run tests */
    run_single_test(suite, "test_contexts_pool_init_destroy", reporter);
    run_single_test(suite, "test_contexts_pool_get_free_contexts", reporter);
    run_single_test(suite, "test_contexts_pool_free_contexts_with_no_packets_in_flight", reporter);
    run_single_test(suite, "test_contexts_pool_free_contexts_with_packets_in_flight", reporter);


    destroy_test_suite(suite);
    (*reporter->destroy)(reporter);

    return 0;
} /* main() */

/*================================================================================================*/

#ifdef __cplusplus
}
#endif
