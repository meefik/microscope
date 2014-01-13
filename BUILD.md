Build kernel module for Android
===============================

1) Get kernel source (kernel version should match version of the device, e.g. 2.6.29):

    $ git clone https://android.googlesource.com/kernel/common android
    $ git branch -a
    $ git checkout android-2.6.29

2) Use Sourcery G++ Lite for ARM EABI to build:

    $ cd /path/to/kernel
    $ make ARCH=arm CROSS_COMPILE=/path/to/arm-2011.03/bin/arm-none-eabi- pxa168_defconfig
    $ make ARCH=arm CROSS_COMPILE=/path/to/arm-2011.03/bin/arm-none-eabi- menuconfig
    Device Drivers -> USB support -> Support for Host-side USB
    Device Drivers -> Multimedia support -> Video For Linux
    Device Drivers -> Multimedia support -> Video capture adapters -> V4L USB devices -> GSPCA based webcams -> ZC3XX USB Camera Drivers
      or add in .config file:
    CONFIG_V4L_USB_DRIVERS=y
    CONFIG_USB_GSPCA=m
    CONFIG_USB_GSPCA_ZC3XX=m
    $ make ARCH=arm CROSS_COMPILE=/path/to/arm-2011.03/bin/arm-none-eabi- modules

3) Copy files /path/to/kernel/drivers/media/video/gspca/gspca_main.ko and /path/to/kernel/drivers/media/video/gspca/gspca_zc3xx.ko to Android device in /system/lib/modules directory.

4) On Android device:

    # insmod /system/lib/modules/gspca_main.ko
    # insmod /system/lib/modules/gspca_zc3xx.ko


Build kernel for Android
========================

For Nexus 7 (2013):

1) Get kernel source:

    git clone https://github.com/meefik/tinykernel-flo.git
    cd tinykernel-flo
    
2) Use Android NDK for build kernel (http://source.android.com/source/building-kernels.html):

    export ARCH=arm
    export SUBARCH=arm
    export CROSS_COMPILE=arm-linux-androideabi-
    export PATH=$HOME/android-ndk-r8d/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin:$PATH
    make flo_defconfig
    make menuconfig
    make

3) After make change image compression to XZ.

4) Create update.zip and upgrade the device.
