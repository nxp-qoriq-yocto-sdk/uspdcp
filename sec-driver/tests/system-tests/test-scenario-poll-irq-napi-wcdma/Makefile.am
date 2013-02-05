bin_PROGRAMS = test_sec_driver_wcdma

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_sec_driver_wcdma_LDFLAGS := -L$(IPC_LIB_DIR) -lmem
test_sec_driver_wcdma_LDADD := sec-driver of

test_sec_driver_wcdma_SOURCES := test_sec_driver_wcdma.c

