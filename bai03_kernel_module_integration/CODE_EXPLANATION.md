# Giai thich code - Bai 03

## Kernel module la gi

Kernel module la doan code co the nap vao Linux kernel khi he thong dang chay. Trong bai nay module tao character device `/dev/simple_kmod` de user-space doc/ghi du lieu.

## Character device la gi

Character device trao doi du lieu theo dong byte. User-space dung `open`, `read`, `write`, `close`; kernel module cung cap cac callback tuong ung trong `struct file_operations`.

## `src/simple_kmod.c`

- `simple_init`: dang ky device number, add `cdev`, tao class/device `/dev/simple_kmod`.
- `simple_exit`: huy device va giai phong tai nguyen.
- `simple_open`: ghi log khi device duoc mo.
- `simple_read`: dung `copy_to_user`.
- `simple_write`: dung `copy_from_user`.
- `simple_release`: ghi log khi dong device.

## `src/main.c`

Tao `GtkApplication` va goi `ui_main_window_new` khi app activate.

## `src/ui_main_window.c`

Tao giao dien chinh GTK4:

- `GtkApplicationWindow`
- `GtkHeaderBar`
- `GtkPaned`
- `GtkStackSidebar`
- `GtkStack`

Moi page duoc them vao stack bang `gtk_stack_add_titled`.

## `src/ui_dashboard_page.c`

Dashboard doc trang thai tu `kernel_module_commands.c`:

- `module_is_loaded`
- `device_exists`
- `kernel_version_string`

Ket qua hien trong cac card de giang vien nhin nhanh trang thai module.

## `src/ui_module_control_page.c`

Trang dieu khien module co cac nut Build, Load, Unload, Status, Clean. Load/Unload mo dialog xac nhan. Command duoc chay nen bang `run_command_async`, sau khi xong callback cap nhat TextView output.

## `src/ui_device_io_page.c`

Trang nay thao tac character device:

- Write goi `write_device_data`.
- Read goi `read_device_data`.
- Neu `/dev/simple_kmod` chua ton tai thi hien loi.
- Neu input rong khi write thi hoi xac nhan.

## `src/ui_kernel_log_page.c`

Trang log doc `dmesg | tail -200`, co nut filter theo `simple_kmod`, search keyword va clear view. TextView dung monospace de de doc log kernel.

## `src/ui_help_page.c`

Hien quy trinh demo va cac lenh Linux tuong ung: `make`, `insmod`, `lsmod`, ghi/doc device, `dmesg`, `rmmod`.

## `src/kernel_module_commands.c`

Day la lop backend user-space cho UI:

- `run_command_sync`: chay command va gom stdout/stderr.
- `run_command_async`: chay command trong thread de UI khong bi treo.
- `module_is_loaded`: doc `/proc/modules`.
- `device_exists`: kiem tra `/dev/simple_kmod`.
- `read_device_data`, `write_device_data`: dung `open/read/write/close`.
- `read_kernel_log`: doc va loc `dmesg`.

## `scripts/module_control.sh`

Script van giu vai tro load/unload/status module:

- `insmod src/simple_kmod.ko`
- `rmmod simple_kmod`
- `lsmod`
- `dmesg`
