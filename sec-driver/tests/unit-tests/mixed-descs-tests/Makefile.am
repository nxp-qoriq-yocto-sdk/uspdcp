bin_PROGRAMS = mixed_descs_tests

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen
AM_CFLAGS += -g -O0 -DDEBUG
ifneq (,$(findstring USDPAA, $(EXTRA_DEFINE)))
mixed_descs_tests_LDFLAGS := -lusdpaa_dma_mem -lusdpaa_process
mixed_descs_tests_LDADD := cgreen sec-driver-dbg
else
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include


AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

mixed_descs_tests_LDFLAGS := -L$(IPC_LIB_DIR) -lmem

mixed_descs_tests_LDADD := cgreen sec-driver-dbg of
endif
mixed_descs_tests_SOURCES := mixed_descs_tests.c
