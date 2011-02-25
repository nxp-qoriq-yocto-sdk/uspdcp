#include"../cgreen.h"
#include<exception>
#include <string.h>



using namespace std;

class myexception: public exception
{  
private:
	int code;
	const char *description;
	
public:
	myexception() {
		code = 0;
		description = NULL;
	}
	
	myexception(int c,const char *d) {
		code = c;
		description = d;
		
	}
	int get_code() const {
	return code;
	}
	
	const char *get_description() {
		return description;	
	}

}

extern "C" TestSuite *exceptions_tests();

void throws_expected() {

	myexception ex(4,"abcd");
	throw ex;	 
}

void throws_basic_exception() {

exception e;
throw e;
}

void throws_int() {

	throw 20;
}

void throws_nothing() {

	int i=0;
}

void should_catch_expected_and_values_are_equal() {
	
	assert_throws_equal_string(throws_expected(),myexception e,"ok","ok");	
	assert_throws_equal_value(throws_expected(),myexception e,1,1);
}

void should_catch_expected_and_values_are_equal_using_getters() {

	assert_throws_equal_string(throws_expected(),myexception e,e.get_description(),"Exception");
	assert_throws_equal_value(throws_expected(),myexception e,e.get_code(),12);
}

void should_catch_expected_and_values_are_not_equal() {

	assert_throws_equal_string(throws_expected(),myexception e,"ok","not ok");
	assert_throws_equal_value(throws_expected(),myexception e,1,5);
}

void no_exception() {

	assert_throws_equal_string(throws_nothing(),myexception e,"ok","not ok");
	assert_throws_equal_value(throws_nothing(),myexception e,1,5);

	assert_throws_equal_string(throws_nothing(),myexception e,"ok","ok");
	assert_throws_equal_value(throws_nothing(),myexception e,1,1);

}

void should_catch_other_exceptions() {  

	assert_throws_equal_string(throws_basic_exception(),myexception e,"ok","not ok");
	assert_throws_equal_value(throws_basic_exception(),myexception e,1,5);
	
	assert_throws_equal_string(throws_int(),myexception e,"ok","ok");
	assert_throws_equal_value(throws_int(),myexception e,1,1);
	
}

void should_not_throw_exception() {

	assert_throws_nothing(throws_nothing());
}

void unexpected_throw() {
	
	assert_throws_nothing(throws_int());	
}

extern "C" TestSuite *exceptions_tests() {
	TestSuite *suite = create_test_suite();
	add_test(suite,should_catch_expected_and_values_are_equal);
	add_test(suite,should_catch_expected_and_values_are_equal_using_getters);
	add_test(suite,should_catch_expected_and_values_are_not_equal);
	add_test(suite,no_exception);
	add_test(suite,should_catch_other_exceptions);
	add_test(suite,should_not_throw_exception);
	add_test(suite,unexpected_throw);
	return suite;
}
	



