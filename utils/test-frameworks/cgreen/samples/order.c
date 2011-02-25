//testeaza eroarea cand functiile sunt apelate in alta ordine decat cea asteptata
#include "../cgreen.h"
#include <stdio.h>
#include <stdlib.h>


static void func1(int i)
    {
	mock(i);
    }
static void func2(int i)
    {
	mock(i);
    }
static void func3(int i)
    {
	mock(i);
    }
  
static void testing_order()
    {
	expect(func1,want(i,1));
	expect(func2,want(i,2));
	expect(func3,want(i,3));
	
	func3(3);
	func1(1);
	func2(2);
    }


int main(int argc,char **argv) {
    TestSuite *suite = create_test_suite();
    add_test(suite,testing_order);  
    return run_test_suite(suite,create_text_reporter());
    
}





    
    
    
