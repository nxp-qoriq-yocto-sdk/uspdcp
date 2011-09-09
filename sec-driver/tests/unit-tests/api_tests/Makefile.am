bin_PROGRAMS = test_api

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

test_api_SOURCES := api_tests.c
test_api_LDADD := cgreen sec-driver of dma_mem
