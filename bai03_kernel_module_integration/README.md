# Bai 03 - USB Mouse Monitor

## Muc tieu

Project nay xay dung Linux Kernel Module `usb_mouse_monitor` va ung dung GTK4 de giam sat chuot USB HID.

Module doc interrupt endpoint cua USB mouse, parse HID mouse report co ban va ghi log kernel khi co su kien chuot:

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

## Ghi chu quan trong

Module dang ky mot `usb_driver` cho USB HID boot mouse. Tren Ubuntu, chuot USB thuong da duoc driver `usbhid` quan ly. Neu `usbhid` da bind vao chuot, module demo co the khong probe duoc thiet bi do cho den khi ban unbind khoi `usbhid` va bind sang `usb_mouse_monitor`.

Module khong hook syscall va khong can thiep syscall table.

Nen demo bang chuot USB phu. Khi bind chuot sang module nay, chuot do co the tam thoi khong dieu khien con tro cho den khi bind lai `usbhid`.

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
4. Cam chuot USB hoac rebind chuot sang module neu can.
5. Di chuyen/click chuot.
6. Vao Mouse Status, bam Refresh Status.
7. Vao Event Log, bam Filter usb_mouse_monitor.

## Demo khi usbhid dang giu chuot

Tim interface chuot USB:

```bash
ls /sys/bus/usb/drivers/usbhid/
```

Interface thuong co dang `1-2:1.0`, `2-1:1.0`. Sau khi load module:

```bash
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usbhid/unbind'
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usb_mouse_monitor/bind'
cat /proc/usb_mouse_monitor
dmesg | grep usb_mouse_monitor
```

Khi demo xong, tra chuot ve driver ban dau:

```bash
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usb_mouse_monitor/unbind'
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usbhid/bind'
```

Thay `1-2:1.0` bang interface thuc te tren may cua ban.

## dx/dy la gi

`dx` va `dy` la do dich chuyen tuong doi cua chuot trong report gan nhat, khong phai toa do tuyet doi tren man hinh.
