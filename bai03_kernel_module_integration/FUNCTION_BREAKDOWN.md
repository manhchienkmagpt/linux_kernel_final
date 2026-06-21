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

## GTK Files Page

- Giao dien duoc thiet ke giong Files page cua bai02.
- Choose Folder gui `SET_ROOT`.
- Refresh gui `LIST`, sau do goi `INFO` tung file de hien bang Name/Type/Size/Permission/Modified Time.
- Create gui `CREATE`.
- Read gui `READ` va hien dialog noi dung.
- Write gui `WRITE` voi noi dung tu dialog.
- Delete gui `DELETE` sau confirm dialog.

## GTK Pages

- Module Control: build/load/unload/status/clean.
- Files: giao dien bang file giong bai02, backend gui command xuong kernel module.
- Command Console: gui command thu cong.
- Kernel Log: doc va filter dmesg.
