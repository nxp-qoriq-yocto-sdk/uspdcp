bin_PROGRAMS = test_sec_driver_b2b

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/tests/system-tests/common
ifneq (,$(findstring USDPAA, $(EXTRA_DEFINE)))
test_sec_driver_b2b_LDFLAGS := -lusdpaa_dma_mem -lusdpaa_process
test_sec_driver_b2b_LDADD := sec-driver
else
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_sec_driver_b2b_LDFLAGS := -L$(IPC_LIB_DIR) -lmem

test_sec_driver_b2b_LDADD := sec-driver of
endif

AM_CFLAGS += -O0 -pthread -D_GNU_SOURCE -g

test_sec_driver_b2b_SOURCES := test_sec_driver_b2b.c

