# Bai 03 - Ubuntu Mouse Monitor

## Muc tieu

Project nay xay dung Linux Kernel Module `mouse_monitor` va ung dung GTK4 de giam sat chuot hien co tren Ubuntu.

Module dang ky `input_handler` voi Linux input subsystem de nghe su kien tu chuot/touchpad dang duoc Ubuntu quan ly.

Khi user click, di chuyen hoac cuon chuot, module ghi log kernel:

```text
[mouse_monitor] left=1 right=0 middle=0 dx=5 dy=-2 wheel=0
```

Module cung tao interface user-space:

```text
/proc/mouse_monitor
```

Doc status:

```bash
cat /proc/mouse_monitor
```

Output:

```text
connected=1
left=0
right=0
middle=0
dx=0
dy=0
wheel=0
```

## Cach module doc chuot

- Khong chiem quyen dieu khien chuot khoi driver goc.
- Doc su kien chuot/touchpad hien co neu device xuat `EV_REL`, `EV_ABS` hoac multi-touch position.
- Van khong hook syscall va khong can thiep syscall table.

## Giao dien

- Dashboard: Module Status, Mouse Connected, Last Event, Device Interface.
- Module Control: Build, Load, Unload, Check Status, Clean Build.
- Mouse Status: doc `/proc/mouse_monitor` va hien nut/dx/dy/wheel.
- Event Log: refresh dmesg, filter `mouse_monitor`, clear.
- Help: huong dan demo va giai thich dx/dy.

## Build

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-4-dev linux-headers-$(uname -r) kmod
make
```

File module:

```text
src/mouse_monitor.ko
```

## Load / Unload

```bash
sudo insmod src/mouse_monitor.ko
lsmod | grep mouse_monitor
cat /proc/mouse_monitor
dmesg | grep mouse_monitor
sudo rmmod mouse_monitor
```

Hoac dung GUI:

```bash
make run
```

## Demo bang GUI

1. Vao Module Control.
2. Bam Build Module.
3. Bam Load Module.
4. Di chuyen/click/cuon chuot dang dung tren Ubuntu.
5. Vao Mouse Status, bam Refresh Status.
6. Vao Event Log, bam Filter mouse_monitor.

## dx/dy la gi

`dx` va `dy` la do dich chuyen tuong doi cua chuot trong report gan nhat, khong phai toa do tuyet doi tren man hinh.
