# Code Explanation - Bai 03

## module_init / module_exit

`kfile_manager_init` khoi tao root mac dinh `/tmp/kfile_manager_root`, dang ky character device, tao class/device `/dev/kfile_manager`, va printk `kfile_manager loaded`.

`kfile_manager_exit` huy device/class/cdev/dev_t va printk `kfile_manager unloaded`.

## struct file_operations

Module cung cap:

- `.open`
- `.read`
- `.write`
- `.release`

User-space ghi command vao device. Kernel xu ly command va luu output vao result buffer. User-space doc device de lay output.

## copy_from_user / copy_to_user

`write()` dung `copy_from_user` de copy command tu user-space vao kernel buffer.

`read()` dung `simple_read_from_buffer`, ben trong thuc hien copy result ve user-space an toan.

## filp_open / kernel_read / kernel_write

- Tao file: `filp_open(path, O_CREAT | O_EXCL | O_WRONLY, 0644)`.
- Ghi file: `kernel_write`.
- Doc file: `kernel_read`.
- Dong file: `filp_close`.

## validate_filename / build_safe_path

`validate_filename` chan:

- filename rong
- filename bat dau bang `/`
- filename chua `..`
- ky tu ngoai `[a-zA-Z0-9._-]`

`build_safe_path` ghep root directory hien tai voi filename da validate.

## mutex

`state_lock` bao ve:

- `root_dir`
- `result_buffer`
- `last_command`
- `last_result`
- `total_commands`

## User-space command flow

Vi du:

```bash
echo "WRITE test.txt hello" | sudo tee /dev/kfile_manager
cat /dev/kfile_manager
```

Kernel parser nhan `WRITE`, tao safe path trong root, mo file bang VFS API, ghi noi dung, luu result va printk log.

## GUI flow

GTK backend trong `kernel_module_commands.c` gui command vao `/dev/kfile_manager`. Neu permission denied, GUI hien dialog nhap sudo password va chay command bang sudo.
