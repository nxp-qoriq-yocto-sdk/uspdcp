CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

CFLAGS = -g -O0 -Wall

SOURCES =			\
	unit.c 			\
	messaging.c		\
	breadcrumb.c	\
	reporter.c 		\
	assertions.c 	\
	vector.c 		\
	mocks.c 		\
	constraint.c 	\
	parameters.c 	\
	text_reporter.c


##################################################################################

cgreen.a: *.o
	$(AR) -rs cgreen.a $^


*.o: $(SOURCES)
	$(CC) $(CFLAGS) -c $^


# (bhamciu1) added target for compliance with the Host Makefiles convention
clean:
	-rm -f *.o cgreen.a

