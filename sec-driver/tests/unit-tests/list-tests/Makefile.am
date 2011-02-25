bin_PROGRAMS = test_lists

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

test_lists_SOURCES := list-tests.c ../../../../sec-driver/src/list.c
test_lists_LDADD := cgreen
