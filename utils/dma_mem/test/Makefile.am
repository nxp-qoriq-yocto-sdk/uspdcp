ifeq (DMA_MEM,$(MEM_MANAGER))

bin_PROGRAMS = test_dma_mem

AM_CFLAGS := -I$(TOP_LEVEL)/utils/dma_mem/include

test_dma_mem_SOURCES := test_dma_mem.c
test_dma_mem_LDADD := dma_mem

endif