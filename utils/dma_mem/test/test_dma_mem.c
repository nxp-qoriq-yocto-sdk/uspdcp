#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "compat.h"

#define BUFFER_ALIGNEMENT 64
#define BUFFER_SIZE       128

int main(void)
{
	int ret;
	char * test_string = "Mary has an apple!";
	dma_addr_t test_phys_addr;
	void * test_virt_addr;
	void * buffer = NULL;

	// map the physical memory
	ret = dma_mem_setup();
	if (ret != 0)
	{
		printf("dma_mem_setup failed with ret = %d\n", ret);
		return ret;
	}
	printf("dma_mem_setup: mapped virtual mem 0x%x to physical mem 0x%x\n", __dma_virt, DMA_MEM_PHYS);

	// TEST access to memory area reserved for SEC driver
	printf ("\nTest access to memory area reserved for SEC driver (first part)\n");

	// __dma_virt is the start virtual address for the mapped memory area
	test_virt_addr = (void*)__dma_virt;

	test_phys_addr = dma_mem_vtop(test_virt_addr);
	assert(test_phys_addr == DMA_MEM_PHYS);

	memcpy(test_virt_addr, test_string, strlen(test_string));
	((char *)test_virt_addr)[strlen(test_string)] = '\0';

	printf("Wrote at address 0x%x the string: %s\n", (dma_addr_t)test_virt_addr, (char*)test_virt_addr);


	// TEST access to memory area reserved for dynamic allocation
	printf ("\nTest access to memory area reserved for buffers (second part)\n");

	buffer = dma_mem_memalign(BUFFER_ALIGNEMENT, BUFFER_SIZE);
	// validate that the address of the freshly allocated buffer falls in the second memory
	// are.
	assert (buffer != NULL);
	assert((dma_addr_t)buffer >= DMA_MEM_SEC_DRIVER);

	// copy a string to the buffer
	assert (strlen(test_string) < BUFFER_SIZE);
	memcpy(buffer, test_string, strlen(test_string));
	((char*)buffer)[strlen(test_string)] = '\0';

	// read the string from the buffer
	printf("Wrote at address 0x%x the string: %s\n", (dma_addr_t)buffer, (char*)buffer);

	// free the buffer
	dma_mem_free(buffer, BUFFER_SIZE);


	printf("\nUnmap dma-mem.");
	// unmap the physical memory
	ret = dma_mem_release();
	if (ret != 0)
	{
		return ret;
	}

	return 0;
}