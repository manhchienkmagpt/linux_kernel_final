# Bai 03 - Ubuntu Mouse Monitor

## Muc tieu

Project nay xay dung Linux Kernel Module `usb_mouse_monitor` va ung dung GTK4 de giam sat chuot hien co tren Ubuntu.

Phien ban nay khong doc truc tiep USB interrupt endpoint nua. Thay vao do, module dang ky `input_handler` voi Linux input subsystem de nghe su kien tu cac thiet bi chuot/touchpad dang duoc Ubuntu quan ly.

Khi user click, di chuyen hoac cuon chuot, module ghi log kernel:

```text
[usb_mouse_monitor] left=1 right=0 middle=0 dx=5 dy=-2 wheel=0
```

Module cung tao interface user-space:

```text
/proc/usb_mouse_monitor
```

Doc status:

```bash
cat /proc/usb_mouse_monitor
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

## Diem khac so voi ban USB driver

- Khong can unbind/bind `usbhid`.
- Khong chiem quyen dieu khien chuot khoi driver goc.
- Co the doc su kien chuot/touchpad hien co neu device xuat `BTN_LEFT` kem `EV_REL` hoac `EV_ABS`.
- Van khong hook syscall va khong can thiep syscall table.

## Giao dien

- Dashboard: Module Status, Mouse Connected, Last Event, Device Interface.
- Module Control: Build, Load, Unload, Check Status, Clean Build.
- Mouse Status: doc `/proc/usb_mouse_monitor` va hien nut/dx/dy/wheel.
- Event Log: refresh dmesg, filter `usb_mouse_monitor`, clear.
- Help: huong dan demo va giai thich dx/dy.

## Build

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-4-dev linux-headers-$(uname -r) kmod
make
```

File module:

```text
src/usb_mouse_monitor.ko
```

## Load / Unload

```bash
sudo insmod src/usb_mouse_monitor.ko
lsmod | grep usb_mouse_monitor
cat /proc/usb_mouse_monitor
dmesg | grep usb_mouse_monitor
sudo rmmod usb_mouse_monitor
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
6. Vao Event Log, bam Filter usb_mouse_monitor.

## dx/dy la gi

`dx` va `dy` la do dich chuyen tuong doi cua chuot trong report gan nhat, khong phai toa do tuyet doi tren man hinh.
