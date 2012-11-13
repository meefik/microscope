Build kernel module for Android
===============================

1) Get kernel source (kernel version should match the version of the device, e.g. 2.6.29):

    $ git clone https://android.googlesource.com/kernel/common android-2.6.29

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

