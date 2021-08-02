#!/bin/bash
mknod /dev/so2_cdev c 42 0 
insmod so2_cdev.ko
