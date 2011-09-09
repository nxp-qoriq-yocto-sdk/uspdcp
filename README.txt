----------------------------------------------------------------------------------------------------
ABOUT THIS FILE
----------------------------------------------------------------------------------------------------
Date:	29 July 2011

This Readme file contains details on SEC user space driver package contents as well ass instructions
showing how to compile and install SEC user space driver related binaries: 
driver itself, unit-tests and system tests.
----------------------------------------------------------------------------------------------------
SEC USER SPACE DRIVER PACKAGE DETAILS
----------------------------------------------------------------------------------------------------
The package contains:
	- Kernel components, in folder <kernel-drivers>:
		-> Update for device tree specification (DTS)
		-> Freescale Shared Memory Driver, fsl_shmem, which implements
		   Freescale's custom memory management mechanism.
		-> Patch for enabling fsl_shmem support
		-> SEC kernel driver patch for UIO support
		
    - User space components, in folder <us-drivers>:
		* Utilitary libraries, in folder <utils>:
			-> DMA memory library. Enables user-space usage for Freescale's custom memory management mechanism
			-> OF library used for parsing DTS
			-> Cgreen test framework, used for implementing SEC user space driver unit-tests
		* SEC user space driver components:
			-> Public headers, in folder <include>:
				--> fsl_sec.h                 - contains SEC user space driver API
				--> fsl_sec_config.h 		  - contains configuration parameters for SEC user space driver
				--> external_mem_management.h - contains API definitions for external memory management mechanism.
											  - this API needs to be implemented when integrating SEC user space driver
											    into a software solution. Freescale delivers such a custom-and-not-supported
												memory mangement mechanism, for the purpose of validating driver's correctness.
			-> Source code, in folder <src>
			-> Tests, in folder <tests>:
				--> Unit-tests in folder <unit-tests>
					Different tests are created for validating a specific functionality like:
					PDCP context management, UIO interrupts, lists, API functions.
					
				--> Systems-tests in folder <system-tests>:
					---> <test-scenario-poll-irq-napi>
						 This is a two-thread test application that allows testing PDCP data plane and control 
						 plane with an option to select the ciphering and integrity check algorithms.
						 Where available, processing result is checked against reference test vectors.
						 Exercises all the available options for retrieving processed packets: poll, IRQ, NAPI.
						 See <test-scenario-poll-irq-napi>/readme.txt for more details.
						 
					---> <test-scenario-benchmark>
						 This is a two-thread test application that allows benchmarking PDCP data plane processing.
						 Benchmark results consist of:
							- core cycles/packet - spent in SEC user space driver APIs for sending and 
							                       retrieving a packet to/from SEC engine.
							- execution time
							- CPU load
						 See <test-scenario-benchmark>/readme.txt for more details.
						 
					---> <test-scenario-benchmark-single-th>
						 This is a single-thread test application that allows benchmarking PDCP data plane processing.
						 Benchmark results consist of:
							- core cycles/packet - spent in SEC user space driver APIs for sending and 
							                       retrieving a packet to/from SEC engine.
							- execution time
							- CPU load
						 See <test-scenario-benchmark-single-th>/readme.txt for more details.
						
	
----------------------------------------------------------------------------------------------------
HOW TO INSTALL SEC USER SPACE DRIVER
----------------------------------------------------------------------------------------------------
The instructions below assume you already have installed a P1010RDB Freescale SDK with ltib,
linux kernel tree and u-boot. The folder where Freescale P1010RDB SDK is installed will be
further reffered to as <p1010-sdk>.

* Extract SEC user space driver package in a directory <sec-us-driver-release>. Example:
   
	tar -xzvf <sec_us_driver.tgz>
	cd <sec-us-driver-release>

* Modify <sec-us-driver-release>/us-drivers/Makefile.config file with specific configuration options like:
   
	-> PowerPC Cross compiler path. The SEC driver was tested with gcc-4.5.55-eglibc-2.11.55, 
	  which is the GNU toolchain installed by default with ltib for Freescale SDK 1.0.

	-> SEC device version. Configured for SEC 4.4 now.
----------------------------------------------------------------------------------------------------
CONFIGURE LINUX KERNEL
----------------------------------------------------------------------------------------------------
* Sec user space driver requires that certain functionality is enabled in Linux kernel.

	cd <p1010-sdk>/ltib
    [ltib]$ ./ltib --preconfig p1010rdb_min
	[ltib]$ ./ltib -c

	-> Enable UIO package
	-> Enable udev
----------------------------------------------------------------------------------------------------
HOW TO INSTALL SEC KERNEL DRIVER CHANGES
----------------------------------------------------------------------------------------------------
UIO support was added to SEC kernel driver.
FSL-SHMEM kernel driver was added to implement external, custom and non-supported memory management
mechanism required for using the SEC user space driver.

* Install sources:

	cd <sec-us-driver-release>/kernel-drivers    
	[kernel-drivers]$ cp patches/* <p1010-sdk>/linux-2.6
    [kernel-drivers]$ cp dts/p1010.dts <p1010-sdk>/config/platform/SDK/dts
	[kernel-drivers]$ cd <p1010-sdk>/linux-2.6/
	[linux-2.6]$ patch -p1 < 0001-Latest-CAAM-driver-code-at-qoriq-dev-linux2.6.git-is.patch
	[linux-2.6]$ patch -p1 < 0002-crypto-caam-Added-UIO-support-in-CAAM-driver.patch
    [linux-2.6]$ patch -p1 < fsl_shmem.patch
   
* Compile and deploy. Recompile only kernel, assuming you already have the SDK compiled with ltib:

	[linux-2.6]$ cd <p1010-sdk>/ltib
	[ltib]$ ./ltib -m scbuild -p kernel
	[ltib]$ ./ltib --deploy

----------------------------------------------------------------------------------------------------
HOW TO COMPILE AND DEPLOY SEC USER SPACE DRIVER
----------------------------------------------------------------------------------------------------
* Compile:

	Go to folder <sec-us-driver-release>/us-drivers and run the following command: 
	[us-drivers]$ make

	The library for SEC user space driver as well as helper libraries are generated in folder: 
	us-drivers/lib-powerpc.

	The executables for test applications (unit-tests and system-tests) are generated in folder: 
	us-drivers/bin-powerpc.

* Clean

	Go to folder <sec-us-driver-release>/us-drivers and run: 
	[us-drivers]$ make distclean

* Install
	A. To install the sources in the PPC rootfs from the HOST machine,
	 go to folder <sec-us-driver-release>/us-drivers and run:

	[us-drivers]$ make install DESTDIR=<p1010-sdk>/ltib/rootfs

	where DESTDIR holds the path to the PPC root filesystem. 

	The executables will be installed in folder $DESTDIR/usr/bin.
	The libraries will be installed in folder $DESTDIR/usr/lib.

	Using any other directory path for DESTDIR will install the binaries
	in that provided location.

	B. To install the sources in the <sec-us-driver-release>/us-drivers/test-install folder, 
	go to folder <sec-us-driver-release>/us-drivers and run:

	[us-drivers]$ make install

	The executables will be installed in folder <sec-us-driver-release>/us-drivers/test-install/usr/bin.
	The libraries will be installed in folder <sec-us-driver-release>/us-drivers/test-install/usr/lib.

	NOTE: To deploy the SEC driver binaries on the P1010RDB Linux target use scp or nfs.

* Debug makefile

	[us-drivers]$ make debug

----------------------------------------------------------------------------------------------------
HOW TO RUN TESTS FOR SEC USER SPACE DRIVER
----------------------------------------------------------------------------------------------------
* Deploy the modified binaries on P1010RDB board: uImage, p1010rdb.dtb, rootfs.ext2.gz.uboot.

* Add kernel boot param "mem=768M" when booting kernel.
	This is required for using Freescale's custom memory management mechanism.	
	Example: setenv bootargs root=/dev/ram rw console=$consoledev,$baudrate $othbootargs mem=768M;

* Copy system tests from install folder to P1010RDB Linux. More options are available:
	A. Copy the binaries with scp or nfs.
	In this case, run tests using current path where binaries were installed on P1010RDB.
	B. Install binaries in ltib's rootfs directory and redeploy rootfs on P1010RDB board.
	In this case, run tests using test name.

* Run unit tests:
	[p1010rdb]./test_api
	[p1010rdb]./test_contexts_pool
	[p1010rdb]./test_dma_mem
	[p1010rdb]./test_lists
	[p1010rdb]./test_uio_notify	

* Run system tests:
	[p1010rdb]./test_sec_driver
	
* Run benchmarking tests:
	[p1010rdb]./test_sec_driver_benchmark
	[p1010rdb]./test_sec_driver_benchmark_single_th
	
	

