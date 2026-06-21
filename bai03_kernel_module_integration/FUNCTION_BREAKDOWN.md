# Phan ra chuc nang - Bai 03

## Kernel module character device

- `simple_init`: dang ky major/minor bang `alloc_chrdev_region`, khoi tao `cdev`, tao class va device `/dev/simple_kmod`.
- `simple_exit`: huy device, class, cdev va unregister device number.
- `simple_open`: ghi log khi user-space mo device.
- `simple_read`: copy du lieu tu kernel buffer sang user buffer bang `copy_to_user`.
- `simple_write`: copy du lieu tu user buffer vao kernel buffer bang `copy_from_user`.
- `simple_release`: ghi log khi dong device.
- Input: read/write tu `/dev/simple_kmod`.
- Output: du lieu trong buffer va log kernel.

## Script dieu khien module

- `module_control.sh load`: goi `insmod src/simple_kmod.ko`.
- `module_control.sh unload`: goi `rmmod simple_kmod`.
- `module_control.sh status`: goi `lsmod | grep simple_kmod`.
- `module_control.sh dmesg`: loc log kernel co tu khoa `simple_kmod`.
- `setup_device_permission.sh`: chmod `/dev/simple_kmod`.

## User-space GTK app

- Tao cua so co cac nut load/unload/status/log/read/write.
- Goi script module bang `g_spawn_command_line_sync`.
- Doc/ghi device bang system call `open`, `read`, `write`, `close`.
- Input: text nguoi dung nhap va thao tac button.
- Output: log/output tren `GtkTextView`.

## Luong xu ly chinh

1. Build module tao file `.ko`.
2. App GTK goi script load module.
3. Kernel tao `/dev/simple_kmod`.
4. App mo device, ghi buffer, doc buffer.
5. Kernel module ghi log bang `printk`.
6. App hien thi `dmesg` lien quan.
7. App goi unload module khi demo xong.
