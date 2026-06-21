# Phan ra chuc nang - Bai 03

## Kernel module character device

- `simple_init`: dang ky major/minor bang `alloc_chrdev_region`, khoi tao `cdev`, tao class va device `/dev/simple_kmod`.
- `simple_exit`: huy device, class, cdev va unregister device number.
- `simple_open`: log khi user-space mo device.
- `simple_read`: copy du lieu tu kernel buffer sang user buffer bang `copy_to_user`.
- `simple_write`: copy du lieu tu user buffer vao kernel buffer bang `copy_from_user`.
- `simple_release`: log khi dong device.

## Main Window

- `main.c`: tao `GtkApplication`.
- `ui_main_window.c`: tao `GtkApplicationWindow`, HeaderBar, `GtkStackSidebar` va `GtkStack`.
- Sidebar gom Dashboard, Module Control, Device I/O, Kernel Log, Help.

## Dashboard Page

- Hien Module Status: Loaded / Not Loaded.
- Hien Device File: `/dev/simple_kmod`.
- Hien Device Status: Exists / Missing.
- Hien Kernel Version.
- Hien Last Action.
- Nut Refresh cap nhat lai thong tin.

## Module Control Page

- Build Module: chay `make module`.
- Load Module: xac nhan roi chay `scripts/module_control.sh load`.
- Unload Module: xac nhan roi chay `scripts/module_control.sh unload`.
- Check Status: chay `scripts/module_control.sh status`.
- Clean Build: chay `make clean`.
- Command chay bang thread nen UI khong bi treo.
- Output hien trong `GtkTextView`.

## Device I/O Page

- Hien device path `/dev/simple_kmod`.
- Write to Device goi backend user-space `write_device_data`.
- Read from Device goi `read_device_data`.
- Neu device missing thi hien loi ro rang.
- Clear Input xoa entry.
- Refresh Device Status cap nhat trang thai device file.

## Kernel Log Page

- Refresh dmesg: doc `dmesg | tail -200`.
- Filter Module Log: chi hien dong co `simple_kmod`.
- Search/Filter loc theo tu khoa.
- Clear View xoa man hinh log.
- Label hien so dong log dang xem.

## Help Page

- Hien cac buoc demo.
- Hien cac lenh Linux tuong ung: make, insmod, lsmod, echo/cat device, dmesg, rmmod.

## Backend user-space

- `kernel_module_commands.c/.h`: gom cac ham check module, check device, lay kernel version, chay command sync/async, doc/ghi device, doc kernel log.
- `scripts/module_control.sh`: load/unload/status/dmesg module.
