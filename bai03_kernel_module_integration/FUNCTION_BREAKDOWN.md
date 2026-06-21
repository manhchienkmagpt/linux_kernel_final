# Function Breakdown - Bai 03

## Character Device

- Module: `kfile_manager`
- Device: `/dev/kfile_manager`
- File: `src/kfile_manager.c`
- Dang ky bang `alloc_chrdev_region`, `cdev_add`, `class_create`, `device_create`.

## file_operations

- `open`: mo device.
- `write`: nhan command tu user-space bang `copy_from_user`.
- `read`: tra result buffer bang `simple_read_from_buffer`.
- `release`: dong device.

## Command Parser

`execute_command` tach command va dispatch:

- `SET_ROOT`
- `CREATE`
- `WRITE`
- `APPEND`
- `READ`
- `DELETE`
- `RENAME`
- `COPY`
- `INFO`
- `LIST`
- `STATUS`
- `HELP`

## Safe Path Manager

- `safe_root_path`: chi cho root trong `/tmp` hoac `/home`.
- `validate_filename`: chan path traversal, absolute path va ky tu nguy hiem.
- `build_safe_path`: ghep root hien tai voi filename hop le.

## File Operations

- `file_create`: tao file bang `filp_open`.
- `file_write_content`: ghi/append bang `kernel_write`.
- `file_read_content`: doc bang `kernel_read`.
- `file_delete`: xoa bang `vfs_unlink`.
- `file_copy`: doc source va ghi destination.
- `RENAME`: copy file moi roi delete file cu.
- `file_info`: lay metadata qua `kern_path` va inode.
- `file_list`: liet ke thu muc bang `iterate_dir`.

## Result Buffer

- `result_buffer` luu output moi nhat.
- `read()` cua device tra ve buffer nay.
- `state_lock` bao ve root, result va counters.

## Logging

Moi command printk:

```text
[kfile_manager] PID=... COMM=... CMD=CREATE FILE=file1.txt RESULT=OK
```

## GTK File Manager

- Choose Root / Set Root gui `SET_ROOT`.
- Refresh gui `LIST` va do file vao danh sach ben trai.
- Khi chon mot file, filename duoc dua vao form thao tac.
- Read nap noi dung file vao editor.
- Write/Append ghi noi dung editor xuong file.
- Delete co dialog xac nhan.
- Rename/Copy dung target filename.

## GTK Pages

- Module Control: build/load/unload/status/clean.
- File Manager: gui command theo form.
- Command Console: gui command thu cong.
- Kernel Log: doc va filter dmesg.
