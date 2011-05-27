bin_PROGRAMS = test_sec_driver_benchmark_single_th

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include -I$(TOP_LEVEL)/utils/of/include -I$(TOP_LEVEL)/utils/dma_mem/include -pthread -D_GNU_SOURCE -g

test_sec_driver_benchmark_single_th_SOURCES := test_sec_driver_benchmark_single_th.c
test_sec_driver_benchmark_single_th_LDADD := sec-driver of dma_mem
