bin_PROGRAMS = test_uio_notify

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_uio_notify_LDFLAGS := -L$(IPC_LIB_DIR) -lmem

test_uio_notify_LDADD := cgreen sec-driver of

test_uio_notify_SOURCES := uio-notify-tests.c