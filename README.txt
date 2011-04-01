This Readme file contains instructions showing how to compile and install SEC user space driver.

1. Extract SEC user space driver package in a directory <us-drivers>. Example:
   
    tar -xzvf <sec_us_driver.tgz>
    cd us-drivers

2. Modify <us-drivers>/Makefile.config file with specific configuration options like:

   -> PowerPC Cross compiler path. The SEC driver was tested with gcc-4.3.74-eglibc-2.8.74-dp-2, 
      which is the cross compiler installed by default with ltib for P2020RDB Freescale SDK.

   -> SEC device version (now only SEC 3.1 is supported; SEC driver will be later ported for SEC 4.4)  

3. How to compile and deploy kernel space sec driver (rough instructions):

   -> Install sources:

    cd kernel-drivers
    [kernel-drivers]$ cp dts/p2020rdb.dts <p2020-sdk>/linux-2.6/arch/powerpc/boot/dts/
    [kernel-drivers]$ cp talitos/* <p2020-sdk>/linux-2.6/drivers/crypto
    [kernel-drivers]$ cp misc/fsl_shmem.c <p2020-sdk>/linux-2.6/drivers/misc
    [kernel-drivers]$ cd <p2020-sdk>/linux-2.6/
    [linux-2.6]$ patch -p1 < patches/fsl_shmem.patch
   
   -> Compile and deploy. Recompile only kernel:

   cd <p2020-sdk>/ltib
   [ltib]$ ./ltib -m scbuild -p kernel
   [ltib]$ ./ltib --deploy

4. How to compile and deploy user space sec driver:

   -> Compile:

      Go to folder <us-drivers> and run the following command: 
      [us-drivers]$ make

      The library for sec user space driver as well as helper libraries are generated in folder: 
      <us-drivers>/lib-powerpc.

      The executables for test applications are generated in folder: 
      <us-drivers>/bin-powerpc.

   -> Clean

      Go to folder <us-drivers> and run: 
      [us-drivers]$ make distclean

   -> Install
      A. To install the sources in the PPC rootfs from the HOST machine,
         go to folder <us-drivers> and run:

      [us-drivers]$ make install DESTDIR=<p2020-sdk>/ltib/rootfs

      where DESTDIR holds the path to the PPC root filesystem. 

      The executables will be installed in folder $DESTDIR/usr/bin.
      The libraries will be installed in folder $DESTDIR/usr/lib.

      Using any other directory path for DESTDIR will install the binaries
      in that provided location.

      B. To install the sources in the <us-drivers>/test-install folder, 
      go to folder <us-drivers> and run:

      [us-drivers]$ make install

      The executables will be installed in folder <us-drivers>/test-install/usr/bin.
      The libraries will be installed in folder <us-drivers>/test-install/usr/lib.

      NOTE: To deploy the binaries on the P2020 Linux target use scp or nfs.

   -> Debug makefile

      [us-drivers]$ make debug

5. How to run system tests for sec driver:

   -> Copy system tests from install folder to P2020RDB Linux. More options are available:
   A. Copy the binaries with scp or nfs.
   B. Install binaries in ltib's rootfs directory and redeploy rootfs on P2020 board.

   -> Run system test:
   A. If binaries deployed with scp/nfs:

   [p2020rdb]./<test_sec_driver_poll>

   B. If binaries deployed with ltib generated rootfs:

   [p2020rdb]<test_sec_driver_poll>


