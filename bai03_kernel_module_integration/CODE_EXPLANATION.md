# Giai thich code - Bai 03

## Kernel module la gi

Kernel module la mot doan code co the nap vao Linux kernel khi he thong dang chay. Module giup them driver, filesystem, network feature hoac device moi ma khong can bien dich lai toan bo kernel.

## Character device la gi

Character device la loai device trao doi du lieu theo dong byte, thuong duoc truy cap qua file trong `/dev`. User-space dung cac system call quen thuoc nhu `open`, `read`, `write`, `close`; kernel module cung cap cac ham xu ly tuong ung.

## `src/simple_kmod.c`

- `MODULE_LICENSE`, `MODULE_AUTHOR`, `MODULE_DESCRIPTION`: metadata cua module.
- `DEVICE_NAME`: ten device la `simple_kmod`.
- `device_buffer`: buffer trong kernel de luu du lieu user ghi vao.
- `simple_open`: duoc goi khi user-space `open("/dev/simple_kmod")`.
- `simple_read`: dung `copy_to_user` de dua du lieu tu kernel sang user-space.
- `simple_write`: dung `copy_from_user` de lay du lieu tu user-space vao kernel.
- `simple_release`: duoc goi khi file descriptor dong.
- `simple_init`: dang ky char device, add `cdev`, tao class va `/dev/simple_kmod`.
- `simple_exit`: don dep nguoc lai khi `rmmod`.

## Luong open/read/write/release

1. App goi `open("/dev/simple_kmod", O_RDWR)`.
2. Kernel chay `simple_open`.
3. Khi app ghi, kernel chay `simple_write` va luu data vao `device_buffer`.
4. Khi app doc, kernel chay `simple_read` va copy data ve user-space.
5. Khi app dong fd, kernel chay `simple_release`.

## `src/main.c`

Day la ung dung GTK user-space:

- `run_script`: chay script load/unload/status/dmesg.
- `write_device`: mo `/dev/simple_kmod`, ghi text bang `write`.
- `read_device`: mo `/dev/simple_kmod`, doc bang `read`.
- Cac callback GTK ket noi button voi backend.

## `scripts/module_control.sh`

Script gom cac lenh he thong:

- `insmod src/simple_kmod.ko`
- `rmmod simple_kmod`
- `lsmod`
- `dmesg`

Neu app khong chay bang root, script tu goi lai qua `pkexec` cho lenh can quyen.

## Cach tich hop vao he thong

Sau khi `insmod`, module duoc kernel nap vao bo nho, dang ky major/minor number va tao device node `/dev/simple_kmod`. User-space app giao tiep voi module nhu mot file binh thuong.

## Cach kiem tra

- `lsmod | grep simple_kmod`: kiem tra module da load.
- `dmesg | grep simple_kmod`: xem log `printk`.
- `ls -l /dev/simple_kmod`: kiem tra device file.
- `cat /dev/simple_kmod`: doc data tu character device.
