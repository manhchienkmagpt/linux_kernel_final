# Bai 03 - Linux Kernel File Manager

## Muc tieu

Project xay dung Linux Kernel Module `kfile_manager`, cung cap character device `/dev/kfile_manager` de user-space gui lenh quan ly file xuong kernel-space.

Module khong phai demo read/write chuoi don gian. No co command parser va thuc hien thao tac file that bang VFS/kernel file API nhu `filp_open`, `kernel_read`, `kernel_write`, `vfs_unlink`, `iterate_dir`.

## Cau truc

```text
src/kfile_manager.c              kernel module
src/kernel_module_commands.c/.h  backend GTK gui command/read result
src/ui_dashboard_page.c          Dashboard
src/ui_module_control_page.c     Build/load/unload module
src/ui_device_io_page.c          File Manager
src/ui_command_console_page.c    Command Console
src/ui_kernel_log_page.c         Kernel Log
src/ui_help_page.c               Help
```

## Build

```bash
sudo apt install build-essential pkg-config libgtk-4-dev linux-headers-$(uname -r) kmod
make
```

## Load / Unload

```bash
sudo insmod src/kfile_manager.ko
lsmod | grep kfile_manager
ls -l /dev/kfile_manager
sudo rmmod kfile_manager
```

## Test terminal

```bash
sudo mkdir -p /tmp/kfile_manager_root
echo "SET_ROOT /tmp/kfile_manager_root" | sudo tee /dev/kfile_manager
echo "CREATE test.txt" | sudo tee /dev/kfile_manager
echo "WRITE test.txt hello world" | sudo tee /dev/kfile_manager
echo "READ test.txt" | sudo tee /dev/kfile_manager
cat /dev/kfile_manager
echo "INFO test.txt" | sudo tee /dev/kfile_manager
cat /dev/kfile_manager
echo "LIST" | sudo tee /dev/kfile_manager
cat /dev/kfile_manager
dmesg | grep kfile_manager
```

## Commands

- `SET_ROOT path`
- `CREATE filename`
- `WRITE filename content`
- `APPEND filename content`
- `READ filename`
- `DELETE filename`
- `RENAME oldname newname`
- `COPY src dst`
- `INFO filename`
- `LIST`
- `STATUS`
- `HELP`

## GUI

```bash
make run
```

Pages:

- Dashboard: module status, device file, root directory, last command/result, total commands.
- Module Control: build/load/unload/status/clean.
- File Manager: chon root directory, hien danh sach file trong root, chon file va thao tac create/read/write/append/delete/rename/copy/info.
- Command Console: gui command thu cong va doc result.
- Kernel Log: refresh/filter `kfile_manager`.
- Help: huong dan demo.

## Bao mat path

- Root directory chi duoc nam trong `/tmp` hoac `/home`.
- Chan root nguy hiem nhu `/`, `/etc`, `/bin`, `/usr`, `/boot`.
- Filename khong duoc bat dau bang `/`.
- Filename khong duoc chua `..`.
- Filename chi duoc dung chu cai, so, `.`, `_`, `-`.
- Moi full path deu duoc tao bang `build_safe_path(root, filename)`.
