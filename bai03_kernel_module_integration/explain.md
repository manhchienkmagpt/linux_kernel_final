# Explain - Bai 03: USB Mouse Monitor

## 1. Project nay lam gi

Bai 03 xay dung Linux Kernel Module `usb_mouse_monitor` va app GTK4 de giam sat chuot USB HID o muc kernel.

Module dang ky mot USB driver cho thiet bi HID boot mouse. Khi module bind duoc vao chuot USB, no doc du lieu tu interrupt endpoint, parse HID mouse report co ban, roi cap nhat trang thai hien tai:

```text
left=0
right=0
middle=0
dx=0
dy=0
wheel=0
```

Khi co su kien click, di chuyen hoac cuon chuot, module ghi log vao kernel:

```text
[usb_mouse_monitor] left=1 right=0 middle=0 dx=5 dy=-2 wheel=0
```

App GTK co ten **USB Mouse Monitor** dung de build/load/unload module, doc trang thai chuot tu `/proc/usb_mouse_monitor`, va xem log kernel bang `dmesg`.

## 2. Cau truc chinh

```text
src/usb_mouse_monitor.c           kernel module USB mouse monitor
src/kernel_module_commands.c/.h   backend GUI goi lenh, sudo, procfs, dmesg
src/ui_dashboard_page.c           Dashboard
src/ui_module_control_page.c      Module Control
src/ui_device_io_page.c           Mouse Status
src/ui_kernel_log_page.c          Event Log
src/ui_help_page.c                Help
scripts/module_control.sh         build/load/unload/status/dmesg
```

## 3. Kernel module `usb_mouse_monitor`

Module khong hook syscall table va khong can thiep vao he thong file. No lam viec theo huong USB driver:

- `usb_register`: dang ky driver voi USB core.
- `mouse_probe`: duoc goi khi co interface USB HID boot mouse phu hop bind vao module.
- `mouse_disconnect`: cleanup khi chuot bi rut hoac driver bi unbind.
- `mouse_irq_callback`: duoc goi moi khi URB interrupt IN nhan du lieu tu chuot.
- `parse_mouse_report`: parse report thanh nut trai/phai/giua, `dx`, `dy`, `wheel`.

Report HID mouse co ban thuong co dang:

```text
byte 0: bit button
byte 1: dx
byte 2: dy
byte 3: wheel neu co
```

`dx` va `dy` la do dich chuyen tuong doi so voi lan report truoc, khong phai toa do tuyet doi `x/y` tren man hinh.

## 4. Interface `/proc/usb_mouse_monitor`

Module tao file:

```text
/proc/usb_mouse_monitor
```

Khi user-space doc file nay:

```bash
cat /proc/usb_mouse_monitor
```

Kernel tra ve trang thai hien tai:

```text
connected=1
left=0
right=0
middle=0
dx=0
dy=0
wheel=0
```

GUI Mouse Status cung doc file proc nay de hien label va raw status.

## 5. GUI Pages

### Dashboard

Hien:

- Module Status
- Mouse Connected
- Device Interface
- Last Event

### Module Control

Co cac nut:

- Build Module
- Load Module
- Unload Module
- Check Status
- Clean Build

Load/unload can quyen sudo. Neu sudo can mat khau, GUI se hien dialog de user nhap mat khau.

### Mouse Status

Co nut Refresh Status va cac label:

- Connected
- Left Button
- Right Button
- Middle Button
- dx
- dy
- Wheel

TextView ben duoi hien output raw tu `/proc/usb_mouse_monitor`.

### Event Log

Doc `dmesg`, loc log co chuoi `usb_mouse_monitor`, va hien log kernel lien quan den ket noi chuot hoac su kien chuot.

### Help

Hien lenh demo:

```bash
make
sudo insmod src/usb_mouse_monitor.ko
dmesg | grep usb_mouse_monitor
cat /proc/usb_mouse_monitor
sudo rmmod usb_mouse_monitor
```

## 6. Luu y khi demo

Tren Ubuntu, chuot USB thuong da duoc driver `usbhid` quan ly. Neu `usbhid` dang bind vao chuot, module `usb_mouse_monitor` co the khong probe duoc chuot do.

Khi can demo URB interrupt callback, co the phai unbind interface chuot khoi `usbhid` roi bind sang `usb_mouse_monitor`. Viec nay co the lam chuot do tam thoi khong dieu khien con tro cho den khi bind lai driver ban dau, nen nen demo voi chuot USB phu.

Vi du interface chuot la `1-2:1.0`:

```bash
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usbhid/unbind'
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usb_mouse_monitor/bind'
cat /proc/usb_mouse_monitor
dmesg | grep usb_mouse_monitor
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usb_mouse_monitor/unbind'
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usbhid/bind'
```

Thay `1-2:1.0` bang interface thuc te trong `/sys/bus/usb/drivers/usbhid/`.

## 7. Cach demo nhanh

1. `make`
2. Chay GUI bang `make run`.
3. Vao Module Control, bam Build Module.
4. Bam Load Module.
5. Cam chuot USB phu neu can.
6. Vao Mouse Status, bam Refresh Status.
7. Click/di chuyen/cuon chuot.
8. Vao Event Log, bam Filter usb_mouse_monitor.
9. Khi xong, bam Unload Module.
