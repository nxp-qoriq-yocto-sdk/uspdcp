1. How to bring sec-driver sources from GIT (available only on FSR git server):
   
   -> Create folder p2020-sdk
   -> Clone the following git repository in folder p2020-sdk

      [/]$ cd p2020-sdk
      [p2020-sdk]$ git ssh://fsrcode.ea.freescale.net/space/git_repos/p2020/us-drivers.git us-drivers
      [p2020-sdk]$ cd us-drivers
 
      Note: The us-drivers.git repository is located only on
      fsrcode.ea.freescale.net server!

2. How to compile and deploy user space sec driver:

   -> Compile:

      Go to folder us-drivers and run the following command: 
      [us-drivers]$ make

      The library for sec user space driver is generated in folder: 
      us-drivers/lib-powerpc.

      The executables for test applications are generated in folder: 
      us-drivers/bin-powerpc.

   -> Clean

      Go to folder us-drivers and run: 
      [us-drivers]$ make distclean

   -> Install
      A. To install the sources in the PPC rootfs from the HOST machine,
         go to folder us-drivers and run:

      [us-drivers]$ make install DESTDIR=/work/p2020-sdk/ltib/rootfs

      where DESTDIR holds the path to the PPC root filesystem. 

      The executables will be installed in folder $DESTDIR/usr/bin.
      The libraries will be installed in folder $DESTDIR/usr/lib.

      B. To install the sources in the us-drivers/test-install folder, 
      go to folder us-drivers and run:

      [us-drivers]$ make install

      The executables will be installed in folder us-drivers/test-install/usr/bin.
      The libraries will be installed in folder us-drivers/test-install/usr/lib.

      NOTE: To install the binaries on the target use scp or nfs.

   -> Debug makefile

      [us-drivers]$ make debug

   -> Add new file to sec driver:
      Add source files (.c) in folder us-drivers/sec-driver/src.
      Add header files (.h) in either folder us-drivers/sec-driver/include or
      us-drivers/sec-driver/src.
      Specify each source file (.c) added in us-drivers/sec-driver/Makefile.am

   -> Add new system test application for sec-driver:
      Create a new folder in us-drivers/sec-driver/tests/system-tests/ .
      Place the source files under the newly created folder.
      Create a new Makefile.am file and configure the name of the 
      executable, the libraries to link to and the sources to be compiled
      (see example from folder
       us-drivers/sec-driver/tests/system-tests/test-scenario-1/).

3. How to setup the P2020 SDK git sources

   -> Create folder p2020-sdk
   -> Clone the following git repositories in folder p2020-sdk

      Note: The branches listed below are associated to release P2020_P1020_Beta_20100901 .

      Note: The git repositories are also available on fsrcode (TBD).

      [/]$ cd p2020-sdk
 
      A. Kernel sources:
      [p2020-sdk]$ git clone git://git.am.freescale.net/p20x0/linux-2.6.git linux-2.6
      [p2020-sdk]$ cd linux-2.6
      [p2020-sdk/linux-2.6]$ git checkout origin/P2020_P1020_Beta_20100901

      B. Ltib sources:
      [p2020-sdk]$ git clone git://git.am.freescale.net/ltib.git ltib
      [p2020-sdk]$ cd ltib
      [p2020-sdk/ltib]$ git checkout origin/branch-p1020rdb-p2020rdb-10-3-1

      C. U-boot sources:
      [p2020-sdk]$ git clone git://git.am.freescale.net/p20x0/u-boot.git u-boot
      [p2020-sdk]$ cd u-boot
      [p2020-sdk/u-boot]$ git checkout origin/P2020_P1020_Beta_20100901

      
4. How to (re)compile the kernel from the GIT sources

   -> Configure ltib to use the kernel from a local folder

      Go to folder p2020-sdk/ltib and run:

      [p2020-sdk]$ cd ltib
      [p2020-sdk/ltib]$ ./ltib -c

      Go to "Choose your kernel" section. Select kernel -> "Local Linux directory build"
      Go to "Enter your Linux source directory" and set the folder "${ABSOLUTE_PATH}/p2020-sdk/linux-2.6". Press enter. 
      Exit the configuration meniu.

      Now the kernel will be compiled from local sources.

      The compiled binaries are located in folder p2020-sdk/ltib/rootfs/boot/ .
   
   -> Recompile/redeploy the kernel

      After changing the kernel sources in folder p2020-sdk/linux-2.6, 
      run the following command to recompile and redeploy the kernel:

      [p2020-sdk/ltib]$ ./ltib -m scbuild -p kernel
      [p2020-sdk/ltib]$ ./ltib -m scdeploy -p kernel	

  
5. How to recompile the u-boot from the GIT sources

   TBD
