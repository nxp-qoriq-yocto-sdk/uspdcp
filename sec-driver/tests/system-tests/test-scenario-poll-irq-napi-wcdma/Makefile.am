bin_PROGRAMS = test_sec_driver_wcdma

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include

ifneq (,$(findstring USDPAA, $(EXTRA_DEFINE)))
test_sec_driver_wcdma_LDFLAGS := -lusdpaa_dma_mem -lusdpaa_process
test_sec_driver_wcdma_LDADD := sec-driver
else
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_sec_driver_wcdma_LDFLAGS := -L$(IPC_LIB_DIR) -lmem
test_sec_driver_wcdma_LDADD := sec-driver of
endif



test_sec_driver_wcdma_SOURCES := test_sec_driver_wcdma.c

