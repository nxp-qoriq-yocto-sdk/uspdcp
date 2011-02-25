
#include<exception>
using namespace std;
extern "C" {
#include "assertions.h"
} 
static int isException = 0;
#define assert_throws_equal_string(func,expected,expresion1,expresion2)     \
	isException = 0;												        \
	try																		\
		{               						         				    \
			func;  						             			    	    \
		}                					      				            \
    catch(expected)													        \
		{						    	 				                    \
			assert_string_equal(expresion1,expresion2);			            \
			isException = 1;										        \
		} 					   										        \
    catch(...)																\
		{														      		\
			assert_false_with_message(1,"Not expected exception type");	    \
			isException = 1;               							        \
		}							    						            \
	assert_equal_with_message(isException,1,"Exception not thrown");        
      
     
#define assert_throws_equal_value(func,expected,expresion1,expresion2)  	\
    isException = 0;												        \
	try       																\
		{					                  				     	        \
			func;							               			    	\
	    }					                 				    	        \
	catch(expected)    														\
		{													    	        \
			assert_equal(expresion1,expresion2);			                \
			isException = 1;	 										    \
		}												        			\
    catch(...)																\
		{														       		\
			assert_false_with_message(1,"Not expected exception type");     \
			isException = 1;												\
		}														 	    	\
  	assert_equal_with_message(isException,1,"Exception not thrown");  

#define assert_throws_nothing(func)							            	\
	isException = 0;														\
	try                     												\
		{																	\
			func;															\
		}																	\
	catch(...)  															\
		{  																	\
			isException = 1;  												\
		}  																	\
	assert_equal_with_message(isException,0,"Not expected exception was thrown");
		  
