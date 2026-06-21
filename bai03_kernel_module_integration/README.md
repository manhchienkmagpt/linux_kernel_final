# Bai 03 - Linux File Access Monitor

## Muc tieu

Project nay xay dung Linux Kernel Module `access_monitor` va ung dung GTK4 de demo mot mini File Access Monitor / Security Monitor.

Module theo doi thao tac ghi va xoa file trong mot protected path, mac dinh:

```text
/tmp/protected
```

Khi co process ghi hoac xoa file trong protected path, module ghi log vao kernel bang `printk/pr_info`:

```text
[access_monitor] PID=... COMM=... ACTION=WRITE PATH=...
[access_monitor] PID=... COMM=... ACTION=DELETE PATH=...
```

## Y nghia thuc te

Day la module co tinh ung dung bao mat he thong: theo doi cac thu muc nhay cam, phat hien process nao dang ghi/xoa file, va cho phep xem su kien qua `dmesg` hoac GUI.

Module khong hook syscall table truc tiep. Thay vao do no dung kprobe tren cac ham kernel nhu `vfs_write` va `vfs_unlink`, an toan va phu hop hon cho demo tren Ubuntu kernel pho bien.

## Giao dien

- Dashboard: Module Status, Protected Path, Last Event, Total Events.
- Module Control: Build, Load, Unload, Check Status, Clean Build.
- Monitor Config: set/read protected path, tao/ghi/xoa file test.
- Event Log: refresh dmesg, filter access_monitor, search, clear.
- Help: cac buoc demo va lenh Linux tuong ung.

## Cau truc source

```text
bai03_kernel_module_integration/
|-- src/
|   |-- access_monitor.c
|   |-- kernel_module_commands.c/.h
|   |-- ui_dashboard_page.c/.h
|   |-- ui_module_control_page.c/.h
|   |-- ui_device_io_page.c/.h
|   |-- ui_kernel_log_page.c/.h
|   |-- ui_help_page.c/.h
|   |-- ui_main_window.c/.h
|   |-- main.c
|   `-- Makefile
|-- scripts/module_control.sh
|-- Makefile
|-- README.md
|-- FUNCTION_BREAKDOWN.md
`-- CODE_EXPLANATION.md
```

## Cai thu vien

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-4-dev linux-headers-$(uname -r) kmod
```

## Build

```bash
make
```

Lenh nay build ca GTK app va kernel module:

```text
src/access_monitor.ko
```

## Load / Unload bang terminal

```bash
sudo insmod src/access_monitor.ko protected_path=/tmp/protected
lsmod | grep access_monitor
sudo rmmod access_monitor
```

Hoac dung script:

```bash
bash scripts/module_control.sh status
sudo bash scripts/module_control.sh load /tmp/protected
sudo bash scripts/module_control.sh unload
```

## Test bang terminal

```bash
sudo insmod src/access_monitor.ko protected_path=/tmp/protected
mkdir -p /tmp/protected
echo "hello" > /tmp/protected/test.txt
rm /tmp/protected/test.txt
dmesg | grep access_monitor
sudo rmmod access_monitor
```

## Test bang GUI

```bash
make run
```

Trong GUI:

1. Vao Module Control, bam Build Module.
2. Bam Load Module.
3. Vao Monitor Config, nhap `/tmp/protected`.
4. Bam Set Protected Path.
5. Bam Create Test File.
6. Bam Write Test File.
7. Bam Delete Test File.
8. Vao Event Log, bam Filter access_monitor.
