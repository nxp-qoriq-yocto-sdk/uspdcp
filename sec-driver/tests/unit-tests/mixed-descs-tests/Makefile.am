bin_PROGRAMS = mixed_descs_tests

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

ifeq (DMA_MEM,$(MEM_MANAGER))
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include

mixed_descs_tests_LDADD := cgreen sec-driver of dma_mem

else

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

mixed_descs_tests_LDFLAGS := -L$(IPC_DIR)/ipc -L$(IPC_DIR)/fsl_shm -l:libmem.a -l:liblg_shm.a

mixed_descs_tests_LDADD := cgreen sec-driver of

endif

mixed_descs_tests_SOURCES := mixed_descs_tests.c