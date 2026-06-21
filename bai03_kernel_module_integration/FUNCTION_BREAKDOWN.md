# Phan ra chuc nang - Bai 03

## Kernel Module `usb_mouse_monitor`

- File: `src/usb_mouse_monitor.c`
- Module name: `usb_mouse_monitor`
- User-space interface: `/proc/usb_mouse_monitor`
- Dang ky `usb_driver` cho USB HID boot mouse.

## Module init / exit

- `usb_mouse_monitor_init`:
  - Tao proc entry `/proc/usb_mouse_monitor`.
  - Dang ky USB driver.
  - Log `usb_mouse_monitor loaded`.

- `usb_mouse_monitor_exit`:
  - Huy USB driver.
  - Xoa proc entry.
  - Log `usb_mouse_monitor unloaded`.

## USB probe / disconnect

- `mouse_probe`:
  - Duoc goi khi USB core bind duoc interface HID mouse vao module.
  - Tim interrupt IN endpoint.
  - Cap phat buffer DMA va URB.
  - Submit interrupt URB de nhan mouse report.
  - Cap nhat `connected=1`.

- `mouse_disconnect`:
  - Kill URB.
  - Giai phong buffer/URB/device.
  - Cap nhat `connected=0`.

## Luu y bind driver

- Chuot USB tren Ubuntu thuong dang duoc `usbhid` quan ly.
- Neu module khong probe duoc, demo co the can unbind interface khoi `usbhid` roi bind sang `usb_mouse_monitor`.
- Nen dung chuot USB phu vi chuot bi bind sang module monitor co the tam thoi khong dieu khien con tro.

## URB interrupt callback

- `mouse_irq_callback`:
  - Nhan mouse report tu interrupt endpoint.
  - Goi parser report.
  - Submit lai URB de tiep tuc nhan report.

## Parse HID mouse report

Report co ban:

- Byte 0: button bits.
  - bit 0: left
  - bit 1: right
  - bit 2: middle
- Byte 1: `dx`
- Byte 2: `dy`
- Byte 3 neu co: `wheel`

Log format:

```text
[usb_mouse_monitor] left=1 right=0 middle=0 dx=5 dy=-2 wheel=0
```

## Proc status

Doc:

```bash
cat /proc/usb_mouse_monitor
```

Tra ve:

```text
connected=1
left=0
right=0
middle=0
dx=0
dy=0
wheel=0
```

## GUI Module Control

- Build Module: `make module`.
- Load Module: `scripts/module_control.sh load`.
- Unload Module: `scripts/module_control.sh unload`.
- Check Status: `scripts/module_control.sh status`.
- Clean Build: `make clean`.

## GUI Mouse Status

- Refresh Status doc `/proc/usb_mouse_monitor`.
- Hien Connected, Left, Right, Middle, dx, dy, Wheel.
- TextView hien raw status.

## GUI Event Log

- Refresh dmesg.
- Filter `usb_mouse_monitor`.
- Clear View.
