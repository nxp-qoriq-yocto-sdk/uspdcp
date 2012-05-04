bin_PROGRAMS = test_uio_notify

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

ifeq (DMA_MEM,$(MEM_MANAGER))
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include

test_uio_notify_LDADD := cgreen sec-driver of dma_mem

else

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_uio_notify_LDFLAGS := -L$(IPC_DIR)/ipc -L$(IPC_DIR)/fsl_shm -l:libmem.a -l:liblg_shm.a

test_uio_notify_LDADD := cgreen sec-driver of

endif

test_uio_notify_SOURCES := uio-notify-tests.c