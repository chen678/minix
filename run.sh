#!/bin/bash
IMAG_PATH=$(realpath minix_x86.img)
cd ../obj.i386/destdir.i386/boot/minix/.temp/
qemu-system-x86_64 \
    -enable-kvm \
    -cpu host \
    -serial mon:stdio \
    -kernel kernel \
    -append "console=tty00 rootdevname=c0d0p0" \
    -initrd "mod01_ds,mod02_rs,mod03_pm,mod04_sched,mod05_vfs,mod06_memory,mod07_log,mod08_tty,mod09_mfs,mod10_vm,mod11_pfs,mod12_init" \
    -drive file=$IMAG_PATH,index=0,media=disk,format=raw \
    -net user \
    -net nic \
    -m 1024M \
    -nographic
