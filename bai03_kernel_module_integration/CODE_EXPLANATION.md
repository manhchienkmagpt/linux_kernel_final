# Giai thich code - Bai 03

## Tong quan

Bai 03 da duoc nang cap thanh **Linux File Access Monitor**. Kernel module `access_monitor` theo doi thao tac ghi/xoa file trong protected path va ghi event vao kernel log.

## Kprobe la gi

Kprobe la co che cua Linux kernel cho phep gan handler vao mot ham kernel de quan sat khi ham do duoc goi. Trong bai nay module dung:

- `vfs_write`: theo doi thao tac ghi file.
- `vfs_unlink`: theo doi thao tac xoa file.

Khi cac ham nay duoc kernel goi, pre-handler cua module chay va lay thong tin process/path.

## Vi sao khong hook syscall table

Hook syscall table truc tiep nguy hiem va khong phu hop kernel hien dai:

- syscall table thuong duoc bao ve read-only.
- de gay crash kernel neu sai dia chi/hook sai.
- bi xem la ky thuat rootkit.
- kem tuong thich giua cac ban kernel.

Kprobe la co che duoc kernel ho tro san, phu hop hon cho quan sat va demo.

## `src/access_monitor.c`

### Init / Exit

- `access_monitor_init` tao `/dev/access_monitor`, dang ky kprobe `vfs_write` va `vfs_unlink`, log `access_monitor loaded`.
- `access_monitor_exit` huy kprobe, huy device, log `access_monitor unloaded`.

### Protected path

Protected path luu trong buffer `protected_path`, mac dinh `/tmp/protected`.

Co the truyen khi load module:

```bash
sudo insmod src/access_monitor.ko protected_path=/tmp/protected
```

Co the doi khi module dang chay bang:

```bash
echo "/path/can/bao/ve" | sudo tee /dev/access_monitor
```

### Phat hien WRITE

Kprobe tren `vfs_write` lay `struct file *`, sau do dung `d_path(&file->f_path, ...)` de lay duong dan file. Neu path bat dau bang protected path thi log:

```text
[access_monitor] PID=... COMM=... ACTION=WRITE PATH=...
```

### Phat hien DELETE

Kprobe tren `vfs_unlink` lay `struct dentry *`, sau do dung `dentry_path_raw` de lay path. Neu path thuoc protected path thi log:

```text
[access_monitor] PID=... COMM=... ACTION=DELETE PATH=...
```

### Printk va dmesg

Module dung `pr_info` de ghi log kernel. User-space xem log bang:

```bash
dmesg | grep access_monitor
```

## Character device `/dev/access_monitor`

Device nay khong phai noi luu data demo nua. No la kenh cau hinh:

- `read`: doc protected path hien tai.
- `write`: doi protected path.
- `open/release`: ghi log debug.

## GUI goi lenh nhu the nao

File `kernel_module_commands.c` la backend user-space:

- `run_command_async`: chay lenh trong thread de UI khong treo.
- `run_command_async_sudo`: hien dialog mat khau sudo neu can.
- `module_is_loaded`: doc `/proc/modules`.
- `device_exists`: kiem tra `/dev/access_monitor`.
- `read_protected_path`: doc `/dev/access_monitor`.
- `set_protected_path`: ghi path vao `/dev/access_monitor`.
- `create_test_file`, `write_test_file`, `delete_test_file`: tao event de demo.
- `read_kernel_log_sudo`: doc `dmesg`, co fallback sudo.

## GUI Pages

- Dashboard: hien module status, protected path, last event, total events.
- Module Control: build/load/unload/status/clean.
- Monitor Config: set/read protected path va thao tac file test.
- Event Log: parse log access_monitor thanh cot Time/PID/Process/Action/Path.
- Help: huong dan demo bang terminal va GUI.
