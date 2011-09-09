This folder contains documentation generated with Doxygen from public header
files of SEC user space driver.
To generate a new .chm file, run the following command on a Windows PC with
Microsoft HTML Help Compiler installed(hhc.exe), from current folder sec-driver/docs,
taking care not to run it on a mounted filesystem!:

> doxygen sec_doxy_file

The compiled .chm file will be located in folder sec-driver/docs.
