#!/bin/sh

insmod driver.ko
mknod /dev/cam_ctrl c 251 0