# Explain - Bai 03: Linux File Access Monitor

## 1. Project nay lam gi

Bai 03 xay dung kernel module `access_monitor` de giam sat truy cap file/thu muc nhay cam. Module theo doi thao tac WRITE va DELETE trong protected path, mac dinh:

```text
/tmp/protected
```

Khi co process ghi hoac xoa file trong path nay, module ghi log vao kernel:

```text
[access_monitor] PID=... COMM=... ACTION=WRITE PATH=...
[access_monitor] PID=... COMM=... ACTION=DELETE PATH=...
```

App GTK co ten **Linux File Access Monitor** dung de build/load/unload module, cau hinh protected path, tao thao tac test va xem event log.

## 2. Cau truc chinh

```text
src/access_monitor.c              kernel module
src/kernel_module_commands.c/.h   backend GUI goi lenh, sudo, dmesg, device
src/ui_dashboard_page.c           Dashboard
src/ui_module_control_page.c      Module Control
src/ui_device_io_page.c           Monitor Config
src/ui_kernel_log_page.c          Event Log
src/ui_help_page.c                Help
scripts/module_control.sh         load/unload/status/dmesg
```

## 3. Kernel module `access_monitor`

Module dung kprobe, khong hook syscall table truc tiep.

- `vfs_write`: bat thao tac ghi file.
- `vfs_unlink`: bat thao tac xoa file.

Khi handler chay, module lay PID, process name `current->comm`, action va path neu lay duoc. Neu path nam trong protected path thi log bang `pr_info`.

Module cung tao character device:

```text
/dev/access_monitor
```

Device nay dung de cau hinh:

- `read`: doc protected path hien tai.
- `write`: doi protected path.

## 4. GUI Pages

### Dashboard

Hien:

- Module Status
- Protected Path
- Last Event
- Total Events

### Module Control

Co cac nut:

- Build Module
- Load Module
- Unload Module
- Check Status
- Clean Build

Load/unload dung sudo neu can va app se hien dialog nhap mat khau.

### Monitor Config

Cho phep:

- Nhap protected path.
- Set Protected Path.
- Read Current Path.
- Create Test File.
- Write Test File.
- Delete Test File.

Write/Delete test file se tao event trong kernel log neu file nam trong protected path.

### Event Log

Doc `dmesg`, loc `access_monitor`, parse thanh cot:

```text
Time | PID | Process | Action | Path
```

### Help

Hien cac buoc demo bang terminal:

```bash
make
sudo insmod src/access_monitor.ko protected_path=/tmp/protected
mkdir -p /tmp/protected
echo "hello" > /tmp/protected/test.txt
rm /tmp/protected/test.txt
dmesg | grep access_monitor
sudo rmmod access_monitor
```

## 5. Cach demo nhanh

1. `make`
2. Chay GUI bang `make run`.
3. Vao Module Control, bam Build Module.
4. Bam Load Module.
5. Vao Monitor Config, set `/tmp/protected`.
6. Bam Create Test File.
7. Bam Write Test File.
8. Bam Delete Test File.
9. Vao Event Log, bam Filter access_monitor.
