bin_PROGRAMS = test_sec_driver

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include

ifeq (DMA_MEM,$(MEM_MANAGER))
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include
test_sec_driver_LDADD := sec-driver of dma_mem

else

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_sec_driver_LDFLAGS := -L$(IPC_LIB_DIR) -lmem
test_sec_driver_LDADD := sec-driver of

endif

test_sec_driver_SOURCES := test_sec_driver.c

