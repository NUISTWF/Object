#!/bin/sh

insmod driver.ko
mknod /dev/cam_ctrl c 251 0
cp -r demo /tmp
cd /tmp/demo/driver
chmod 777 load_modules_FH8862.sh
./load_modules_FH8862.sh
cd ../lib
cp imx415_mipi_attr.hex /home
cd ../
chmod 777 demo
./demo 192.168.1.1 &
cd /mnt
./ftpserver
