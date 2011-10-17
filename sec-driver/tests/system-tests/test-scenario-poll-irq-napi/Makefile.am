bin_PROGRAMS = test_sec_driver

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include

test_sec_driver_SOURCES := test_sec_driver.c
test_sec_driver_LDADD := sec-driver of dma_mem
