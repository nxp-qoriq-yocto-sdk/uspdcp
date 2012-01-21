bin_PROGRAMS = mixed_descs_tests

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

mixed_descs_tests_SOURCES := mixed_descs_tests.c
mixed_descs_tests_LDADD := cgreen sec-driver of dma_mem
