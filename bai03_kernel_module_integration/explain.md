# Explain - Bai 03: Linux Kernel File Manager

## 1. Project nay lam gi

Bai 03 xay dung Linux Kernel Module `kfile_manager`. Module tao character device:

```text
/dev/kfile_manager
```

User-space ghi command vao device, kernel parser se thuc hien thao tac file that trong kernel-space bang VFS/kernel file API.

Vi du:

```bash
echo "CREATE test.txt" | sudo tee /dev/kfile_manager
echo "WRITE test.txt hello world" | sudo tee /dev/kfile_manager
echo "READ test.txt" | sudo tee /dev/kfile_manager
cat /dev/kfile_manager
```

## 2. Cau truc

```text
src/kfile_manager.c              kernel module
src/kernel_module_commands.c/.h  backend GTK gui command/read result
src/ui_dashboard_page.c          Dashboard
src/ui_module_control_page.c     Module Control
src/ui_device_io_page.c          Files
src/ui_command_console_page.c    Command Console
src/ui_kernel_log_page.c         Kernel Log
src/ui_help_page.c               Help
```

## 3. Kernel module

Module init:

- Tao root mac dinh `/tmp/kfile_manager_root`.
- Dang ky character device.
- Tao class/device `/dev/kfile_manager`.
- Printk `kfile_manager loaded`.

Module exit:

- Huy device/class.
- Huy cdev/dev_t.
- Printk `kfile_manager unloaded`.

## 4. Command parser

`write()` cua device nhan command tu user-space bang `copy_from_user`, sau do `execute_command` dispatch:

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

Ket qua moi nhat duoc luu trong `result_buffer`. Khi user chay `cat /dev/kfile_manager`, ham `read()` tra buffer nay ve user-space.

## 5. File operations

- Tao/mo file bang `filp_open`.
- Doc file bang `kernel_read`.
- Ghi file bang `kernel_write`.
- Xoa file bang `vfs_unlink`.
- List thu muc bang `iterate_dir`.
- Lay metadata bang `kern_path` va inode.

`RENAME` duoc cai dat theo huong copy sang file moi roi delete file cu de tranh phu thuoc qua nhieu vao signature `vfs_rename` giua cac kernel version.

## 6. Bao mat path

Module chi cho root directory nam trong:

```text
/tmp
/home
```

Filename bi chan neu:

- bat dau bang `/`
- chua `..`
- qua dai
- chua ky tu ngoai `a-z A-Z 0-9 . _ -`

Moi thao tac deu build full path bang `build_safe_path`, nen command khong the thao tac truc tiep ngoai root directory dang quan ly.

## 7. GUI

App GTK ten **Linux Kernel File Manager** gom:

- Dashboard
- Module Control
- Files
- Command Console
- Kernel Log
- Help

Files page duoc lam giong bai02: chon folder, hien bang file, Open/Read/Write/Delete/Create. Khac voi bai02, cac thao tac file trong bai03 duoc gui xuong kernel module `kfile_manager`. Command Console cho phep nhap command thu cong nhu:

```text
CREATE a.txt
WRITE a.txt hello
READ a.txt
```

Neu thao tac can sudo, GUI se hien dialog nhap mat khau.
