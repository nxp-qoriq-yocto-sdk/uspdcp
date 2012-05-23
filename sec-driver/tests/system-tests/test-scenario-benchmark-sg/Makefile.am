bin_PROGRAMS = test_sec_driver_benchmark_sg

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include

AM_CFLAGS += -I$(KERNEL_DIR)/drivers/misc
AM_CFLAGS += -I$(IPC_DIR)/ipc/include
AM_CFLAGS += -I$(IPC_DIR)/fsl_shm/include

test_sec_driver_benchmark_sg_LDFLAGS := -L$(IPC_LIB_DIR) -lmem
test_sec_driver_benchmark_sg_LDADD := sec-driver of

AM_CFLAGS += -pthread -D_GNU_SOURCE

test_sec_driver_benchmark_sg_SOURCES := test_sec_driver_benchmark_sg.c

