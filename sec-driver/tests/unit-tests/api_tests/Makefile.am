bin_PROGRAMS = test_api

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

ifeq (DMA_MEM,$(MEM_MANAGER))
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include

test_api_LDADD := cgreen sec-driver of dma_mem

else

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_api_LDFLAGS := -L$(IPC_LIB_DIR) -lmem

test_api_LDADD := cgreen sec-driver of

endif

test_api_SOURCES := api_tests.c