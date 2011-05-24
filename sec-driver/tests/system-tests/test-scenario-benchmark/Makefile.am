bin_PROGRAMS = test_sec_driver_benchmark

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include -I$(TOP_LEVEL)/utils/of/include -I$(TOP_LEVEL)/utils/dma_mem/include -pthread -D_GNU_SOURCE -g

test_sec_driver_benchmark_SOURCES := test_sec_driver_benchmark.c
test_sec_driver_benchmark_LDADD := sec-driver of dma_mem
