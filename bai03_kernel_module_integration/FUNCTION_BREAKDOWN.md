# Phan ra chuc nang - Bai 03

## Kernel Module `mouse_monitor`

- File: `src/mouse_monitor.c`
- Module name: `mouse_monitor`
- User-space interface: `/proc/mouse_monitor`
- Dang ky `input_handler` de nghe su kien tu chuot/touchpad hien co tren Ubuntu.

## Module init / exit

- `mouse_monitor_init`:
  - Tao proc entry `/proc/mouse_monitor`.
  - Dang ky input handler bang `input_register_handler`.
  - Log `mouse_monitor loaded`.

- `mouse_monitor_exit`:
  - Huy input handler bang `input_unregister_handler`.
  - Xoa proc entry.
  - Log `mouse_monitor unloaded`.

## Input connect / disconnect

- `mouse_connect`:
  - Duoc goi khi input subsystem tim thay device match chuot `EV_REL` hoac touchpad `EV_ABS`.
  - Tao `input_handle`.
  - Goi `input_register_handle`.
  - Goi `input_open_device` de bat dau nhan event.
  - Tang so device dang ket noi va cap nhat `connected=1`.

- `mouse_disconnect`:
  - Goi `input_close_device`.
  - Goi `input_unregister_handle`.
  - Giai phong handle.
  - Giam so device dang ket noi va cap nhat `connected`.

## Input event handler

- `mouse_event`:
  - Nhan event tu input subsystem.
  - Xu ly `EV_KEY` cho nut chuot:
    - `BTN_LEFT`
    - `BTN_RIGHT`
    - `BTN_MIDDLE`
  - Xu ly `EV_REL` cho dich chuyen:
    - `REL_X` -> `dx`
    - `REL_Y` -> `dy`
    - `REL_WHEEL` -> `wheel`
  - Xu ly `EV_ABS` cho touchpad:
    - `ABS_X` -> tinh delta ngang
    - `ABS_Y` -> tinh delta doc
  - Khi nhan `SYN_REPORT`, cap nhat frame dich chuyen gan nhat va ghi log.

Log format:

```text
[mouse_monitor] left=1 right=0 middle=0 dx=5 dy=-2 wheel=0
```

## Proc status

Doc:

```bash
cat /proc/mouse_monitor
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

- Refresh Status doc `/proc/mouse_monitor`.
- Hien Connected, Left, Right, Middle, dx, dy, Wheel.
- TextView hien raw status.

## GUI Event Log

- Refresh dmesg.
- Filter `mouse_monitor`.
- Clear View.
