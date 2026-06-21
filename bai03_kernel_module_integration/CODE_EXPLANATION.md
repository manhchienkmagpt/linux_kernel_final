# Giai thich code - Bai 03

## Tong quan

Bai 03 la **USB Mouse Monitor**. Kernel module `usb_mouse_monitor` theo doi USB HID mouse report va GUI GTK hien trang thai chuot.

## Vi sao dung usb_driver

Module dang ky `usb_driver` thay vi hook syscall. USB core se goi `probe` khi co interface phu hop. Cach nay dung API kernel hop le, co cleanup ro rang va khong can can thiep syscall table.

Luu y: chuot USB tren Ubuntu thuong da do `usbhid` quan ly. Neu `usbhid` da bind thiet bi, module demo co the khong nhan probe cho den khi unbind/bind thu cong.

Vi the nen demo voi chuot USB phu. Interface co the tim trong `/sys/bus/usb/drivers/usbhid/`, vi du `1-2:1.0`. Sau khi load module, co the unbind khoi `usbhid` va bind sang driver moi:

```bash
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usbhid/unbind'
sudo sh -c 'echo 1-2:1.0 > /sys/bus/usb/drivers/usb_mouse_monitor/bind'
```

Khi xong, bind lai `usbhid` de chuot hoat dong binh thuong.

## `src/usb_mouse_monitor.c`

### Device id

Module match USB HID boot mouse:

```c
USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
                   USB_INTERFACE_SUBCLASS_BOOT,
                   USB_INTERFACE_PROTOCOL_MOUSE)
```

### Probe

`mouse_probe`:

- Lay `usb_device` tu interface.
- Tim interrupt IN endpoint.
- Cap phat `usb_alloc_coherent`.
- Cap phat `usb_alloc_urb`.
- Goi `usb_fill_int_urb`.
- Submit URB bang `usb_submit_urb`.

### Interrupt callback

`mouse_irq_callback` nhan data tu chuot. Neu URB thanh cong, code parse report:

- `data[0]`: nut left/right/middle.
- `data[1]`: dx.
- `data[2]`: dy.
- `data[3]`: wheel neu co.

Sau khi parse xong, callback submit lai URB de tiep tuc nhan report.

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
