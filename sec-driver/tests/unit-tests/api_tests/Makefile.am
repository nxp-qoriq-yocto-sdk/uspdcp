bin_PROGRAMS = test_api

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen
ifneq (,$(findstring USDPAA, $(EXTRA_DEFINE)))
test_api_LDFLAGS := -lusdpaa_dma_mem -lusdpaa_process
test_api_LDADD := cgreen sec-driver-dbg
else
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_api_LDFLAGS := -L$(IPC_LIB_DIR) -lmem

test_api_LDADD := cgreen sec-driver-dbg of
endif
test_api_SOURCES := api_tests.c

