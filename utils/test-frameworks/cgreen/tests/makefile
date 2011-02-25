CC = gcc
CPP = g++
CFLAGS = -g



all_tests: cgreen ../cgreen.h *.o exceptions_tests.o
	g++ *.o  ../cgreen.a -o all_tests

*.o: all_tests.c breadcrumb_tests.c messaging_tests.c assertion_tests.c vector_tests.c constraint_tests.c parameters_test.c mocks_tests.c 
	$(CC) $(CFLAGS) -c $^

	
exceptions_tests.o: exceptions_tests.cpp
	$(CPP) $(CFLAGS) -c  $< 
	
	
	
test: all_tests
	./all_tests

cgreen:
	cd ..; make
