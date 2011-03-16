bin_PROGRAMS = test_uio_notify

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/src
AM_CFLAGS += -I$(TOP_LEVEL)/sec-driver/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/of/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/dma_mem/include
AM_CFLAGS += -I$(TOP_LEVEL)/utils/test-frameworks/cgreen

test_uio_notify_SOURCES := uio-notify-tests.c
test_uio_notify_LDADD := cgreen sec-driver of dma_mem
