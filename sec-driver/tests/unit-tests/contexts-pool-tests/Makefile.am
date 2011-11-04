bin_PROGRAMS = test_contexts_pool

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include

test_contexts_pool_SOURCES :=  contexts-pool-tests.c ../../../../sec-driver/src/list.c ../../../../sec-driver/src/sec_contexts.c
test_contexts_pool_LDADD := cgreen dma_mem
