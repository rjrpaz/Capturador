#!/bin/sh
/usr/sbin/ntpdate -s 10.250.1.110
/sbin/hwclock --systohc
