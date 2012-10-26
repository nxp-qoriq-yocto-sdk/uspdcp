bin_PROGRAMS = mixed_descs_tests

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include
AM_CFLAGS += -DDEBUG

mixed_descs_tests_LDFLAGS := -L$(IPC_LIB_DIR) -lmem

mixed_descs_tests_LDADD := cgreen sec-driver-dbg of

mixed_descs_tests_SOURCES := mixed_descs_tests.c