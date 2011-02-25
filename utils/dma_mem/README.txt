About:

dma_mem library provides an interface for:
 - mapping a physical memory area into user space (see dma_mem_setup())
 - conversion functions: virt-to-phys and viceversa
 - runtime allocations of buffers from a zone of the mapped memory area 
   (see dma_mem_memalign(), dma_mem_free())

The configuration of the library is done in file us-drivers/utils/dma_mem/include/conf.h :
 - the start address of the physical memory
 - the size of the phyical memory
Note: Read the comments in the file about the required changes needed in case the
default values are changed.

The library depends on some kernel changes:
 - a driver that maps the virtual memory to physical memory
   (linux-2.6/drivers/misc/fsl-shmem.c)
 - a TLB1 hack (described in file us-drivers/utils/dma_mem/include/conf.h)
 - changes in platform DTS (p2020rdb.dts)

Running the test application:

1. Add kernel boot param "mem=768M" when booting kernel.
   This makes sure that the kernel will use only the first 768M of the 1G available physical
   memory, the rest of the 256M being reserved for mapping from user space.

   Example: setenv bootargs root=/dev/ram rw console=$consoledev,$baudrate $othbootargs mem=768M;

2. Make sure that linux kernel is compiled with:
   - changes from b00553_sec_driver_1 branch from linux-2.6 git repository
   - CONFIG_FSL_SHMEM=y (enables fsl-shmem driver at startup)

3. Make sure the udev package is enabled in ltib.
   ./ltib -c
   Go to "Package List", then go to "device nodes" and select "udev".

4. Compile test_dma_mem 
   Go to us-drivers folder and run make.

5. Run test_dma_mem on target.
   ./test_dma_mem

