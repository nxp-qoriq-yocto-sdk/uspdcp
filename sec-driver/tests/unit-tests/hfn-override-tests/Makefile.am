bin_PROGRAMS = hfn_override_tests

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

hfn_override_tests_LDFLAGS := -L$(IPC_LIB_DIR) -lmem
hfn_override_tests_LDADD := cgreen sec-driver of

hfn_override_tests_SOURCES := hfn_override_tests.c
