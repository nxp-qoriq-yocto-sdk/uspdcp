----------------------------------------------------------------------------------------------------
ABOUT THIS FILE
----------------------------------------------------------------------------------------------------
Date:    14 January 2013

This Readme file contains details on SEC user space driver package contents as well as instructions
showing how to compile and install SEC user space driver related binaries:
driver itself, unit-tests and system tests.
----------------------------------------------------------------------------------------------------
SEC USER SPACE DRIVER PACKAGE DETAILS
----------------------------------------------------------------------------------------------------
The package contains:
    - User space components, in folder <us-drivers>:
        * Utilitary libraries, in folder <utils>:
            -> OF library used for parsing DTS
            -> Cgreen test framework, used for implementing SEC user space driver unit-tests
        * SEC user space driver components:
            -> Public headers, in folder <include>:
                --> fsl_sec.h                 - contains SEC user space driver API
                --> fsl_sec_config.h           - contains configuration parameters for SEC user space driver
            -> Source code, in folder <src>
            -> Tests, in folder <tests>:
                --> Unit-tests in folder <unit-tests>
                    Different tests are created for validating a specific functionality like:
                    PDCP context management, UIO interrupts, lists, API functions, HFN override,
                    mixed descriptors, etc.

                --> Systems-tests in folder <system-tests>:
                    ---> <test-scenario-poll-irq-napi>
                         This is a two-thread test application that allows testing PDCP data plane and control
                         plane which testes all available combinations of protocol direction (Uplink/Downlink),
                         protocol direction (Encapsulate/Decapsulate), with an option of selecting a random subset
                         of these. All the obtained results are checked againsted pre-made reference testt vectors.
                         Exercises all the available options for retrieving processed packets: poll, IRQ, NAPI.
                         See <test-scenario-poll-irq-napi>/readme.txt for more details.

                    ---> <test-scenario-benchmark-sg>
                         This is a two-thread test application that allows benchmarking PDCP data plane and control plane
                         processing, using Scatter-Gather tables
                         Benchmark results consist of:
                            - core cycles/packet - spent in SEC user space driver APIs for sending and
                                                   retrieving a packet to/from SEC engine.
                            - execution time
                            - CPU load
                         See <test-scenario-benchmark-sg>/readme.txt for more details.

                    ---> <test-scenario-benchmark-single-th-sg>
                         This is a single-thread test application that allows benchmarking PDCP data plane and control plane
                         processing, using Scatter-Gather tables
                         Benchmark results consist of:
                            - core cycles/packet - spent in SEC user space driver APIs for sending and
                                                   retrieving a packet to/from SEC engine.
                            - execution time
                            - CPU load
                         See <test-scenario-benchmark-single-th-sg>/readme.txt for more details.

                    ---> <test-scenario-b2b>
                         This is a two-thread application that performs loopback encapsulation/decapsulation of PDCP
                         data plane and control plane, using Scatter-Gather tables.
                         See <test-scenario-b2b>/readme.txt for more details

----------------------------------------------------------------------------------------------------
HOW TO INSTALL SEC USER SPACE DRIVER
----------------------------------------------------------------------------------------------------
The instructions below assume you already have installed a PSC9131 RDB Freescale SDK with ltib,
linux kernel tree and u-boot. The folder where Freescale PSC9131 RDB SDK is installed will be
further reffered to as <psc9131-sdk>.

* Extract SEC user space driver package in a directory <sec-us-driver-release>. Example:

    tar -xzvf <sec_us_driver.tgz>
    cd <sec-us-driver-release>

* Modify <sec-us-driver-release>/us-drivers/Makefile.config file with specific configuration options like:

    -> PowerPC Cross compiler path. The SEC driver was tested with gcc-4.5.55-eglibc-2.11.55,
      which is the GNU toolchain installed by default with ltib for Freescale SDK 1.0.

    -> WUSDK Kernel source path. Usually found under <WUSDK BSP release path>/src/linux-2.6

    -> WUSDK IPC source path. Usually found under <WUSDK BSP release path>/src/ipc

    -> WUSDK IPC libraries path. Usually found under <WUSDK BSP release path>/src/ipc/ipc
       Note: Before compiling SEC US driver, you MUST compile IPC by issuing the following commands:
       [<WUSDK BSP release path>/src/ipc] make

       In order to confirm that the required objects have been built, issue the following command:
       [<WUSDK BSP release path>/src/ipc] ls -la ipc/*.so

       The output should list the following two files
       ipc/libipc.so  ipc/libmem.so

----------------------------------------------------------------------------------------------------
CONFIGURE LINUX KERNEL
----------------------------------------------------------------------------------------------------
* By default PSC 9131 Kernel has the required functionalities for PDCP SEC Driver enabled by default:
    -> UIO package
    -> udev

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

    [us-drivers]$ make install DESTDIR=<psc9131-sdk>/ltib/rootfs

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

    NOTE: To deploy the SEC driver binaries on the PSC9131 RDB Linux target use scp or nfs.

* Debug makefile

    [us-drivers]$ make debug

----------------------------------------------------------------------------------------------------
HOW TO RUN TESTS FOR SEC USER SPACE DRIVER
----------------------------------------------------------------------------------------------------
* Deploy the modified binaries on PSC9131 RDB board: uImage, psc9131rdb.dtb, rootfs.ext2.gz.uboot.

* Copy system tests from install folder to PSC9131RDB Linux. More options are available:
    A. Copy the binaries with scp or nfs.
    In this case, run tests using current path where binaries were installed on PSC9131RDB.
    B. Install binaries in ltib's rootfs directory and redeploy rootfs on PSC9131RDB board.
    In this case, run tests using test name.

* Run unit tests:
    [psc9131rdb]~ ./test_api
    [psc9131rdb]~ ./test_contexts_pool
    [psc9131rdb]~ ./test_lists
    [psc9131rdb]~ ./test_uio_notify
    [psc9131rdb]~ ./mixed_descs_tests
    [psc9131rdb]~ ./hfn_override_tests

* Run system tests:
    [psc9131rdb]~ ./test_sec_driver

* Run benchmarking tests:
    [psc9131rdb]~ ./test_sec_driver_benchmark_sg
    [psc9131rdb]~ ./test_sec_driver_benchmark_single_th_sg
    [psc9131rdb]~ ./test_sec_driver_b2b



