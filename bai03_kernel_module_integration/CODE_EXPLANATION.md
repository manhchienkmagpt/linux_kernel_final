# Giai thich code - Bai 03

## Tong quan

Bai 03 la **Ubuntu Mouse Monitor**. Kernel module `usb_mouse_monitor` theo doi su kien chuot hien co tren Ubuntu thong qua Linux input subsystem va GUI GTK hien trang thai chuot.

## Vi sao dung input_handler

Ban truoc dung `usb_driver`, nen module chi doc duoc chuot USB khi bind truc tiep vao thiet bi. Tren Ubuntu, chuot USB thuong da do `usbhid` quan ly, vi vay phai unbind/bind thu cong va co the lam chuot tam thoi mat tac dung.

Ban hien tai dung `input_handler`. Input subsystem cho phep module dang ky mot handler de nghe event tu device da ton tai, vi vay:

- Khong can chiem quyen cua `usbhid`.
- Khong can hook syscall.
- Co the nghe chuot USB, touchpad hoac thiet bi input tuong thich.
- Cleanup ro rang bang API kernel chuan.

## `src/usb_mouse_monitor.c`

### Match input device

Module match device co:

- `EV_KEY`
- `BTN_LEFT`
- `EV_REL + REL_X + REL_Y` voi chuot tuong doi
- hoac `EV_ABS + ABS_X + ABS_Y` voi touchpad

Day la dau hieu cua mot thiet bi co hanh vi giong chuot/touchpad.

### Connect

`mouse_connect`:

- Cap phat `input_handle`.
- Gan `handle->dev`, `handle->handler`, `handle->name`.
- Goi `input_register_handle`.
- Goi `input_open_device`.
- Cap nhat `connected=1`.

### Event

`mouse_event` nhan event tu input subsystem:

- `EV_KEY + BTN_LEFT`: nut trai.
- `EV_KEY + BTN_RIGHT`: nut phai.
- `EV_KEY + BTN_MIDDLE`: nut giua.
- `EV_REL + REL_X`: dich chuyen ngang `dx`.
- `EV_REL + REL_Y`: dich chuyen doc `dy`.
- `EV_REL + REL_WHEEL`: cuon chuot.
- `EV_ABS + ABS_X/ABS_Y`: touchpad gui toa do tuyet doi, module lay chenh lech voi gia tri truoc de tinh `dx/dy`.
- `EV_SYN + SYN_REPORT`: ket thuc mot frame event, module ghi log neu co thay doi.

### Proc interface

Module tao:

```text
/proc/usb_mouse_monitor
```

`proc_status_read` tra ve status hien tai theo dang key=value.

## GUI backend

`kernel_module_commands.c`:

- `module_is_loaded`: doc `/proc/modules`.
- `device_exists`: kiem tra `/proc/usb_mouse_monitor`.
- `read_mouse_status`: doc proc status va parse connected/left/right/middle/dx/dy/wheel.
- `last_mouse_event`: doc `dmesg | grep usb_mouse_monitor`.
- `run_command_async_sudo`: load/unload module co dialog sudo.

## GUI pages

- Dashboard: module status, mouse connected, last event, device interface.
- Module Control: build/load/unload/status/clean.
- Mouse Status: refresh status va hien raw proc output.
- Event Log: refresh/filter dmesg.
- Help: huong dan demo, giai thich dx/dy.
