# Explain - Bai 03: Ubuntu Mouse Monitor

## 1. Project nay lam gi

Bai 03 xay dung Linux Kernel Module `mouse_monitor` va app GTK4 de giam sat chuot hien co tren Ubuntu.

Module dang ky `input_handler` voi Linux input subsystem. Nho vay module co the nghe su kien tu chuot thuong, touchpad hoac thiet bi input dang duoc Ubuntu quan ly.

Khi co su kien click, di chuyen hoac cuon chuot, module ghi log vao kernel:

```text
[mouse_monitor] left=1 right=0 middle=0 dx=5 dy=-2 wheel=0
```

App GTK co ten **Ubuntu Mouse Monitor** dung de build/load/unload module, doc trang thai chuot tu `/proc/mouse_monitor`, va xem log kernel bang `dmesg`.

## 2. Cau truc chinh

```text
src/mouse_monitor.c               kernel module input mouse monitor
src/kernel_module_commands.c/.h   backend GUI goi lenh, sudo, procfs, dmesg
src/ui_dashboard_page.c           Dashboard
src/ui_module_control_page.c      Module Control
src/ui_device_io_page.c           Mouse Status
src/ui_kernel_log_page.c          Event Log
src/ui_help_page.c                Help
scripts/module_control.sh         load/unload/status/dmesg
```

## 3. Kernel module `mouse_monitor`

Module khong hook syscall table va khong can thiep vao driver chuot goc. No lam viec theo huong input subsystem:

- `input_register_handler`: dang ky handler voi input subsystem.
- `mouse_connect`: duoc goi khi co input device giong chuot match voi handler.
- `mouse_disconnect`: cleanup khi device roi khoi handler.
- `mouse_event`: nhan event chuot va cap nhat status.

Module match device co cac kha nang:

```text
EV_KEY + BTN_LEFT
EV_REL + REL_X + REL_Y
hoac
EV_ABS + ABS_X + ABS_Y
```

Day la dau hieu co ban cua chuot/touchpad.

## 4. Event chuot duoc doc nhu nao

Input subsystem gui event theo dang:

```text
EV_KEY BTN_LEFT   1   -> nhan nut trai
EV_KEY BTN_LEFT   0   -> tha nut trai
EV_REL REL_X      5   -> dich ngang 5 don vi
EV_REL REL_Y     -2   -> dich doc -2 don vi
EV_REL REL_WHEEL  1   -> cuon chuot
EV_ABS ABS_X   1200   -> toa do X touchpad
EV_ABS ABS_Y    800   -> toa do Y touchpad
EV_SYN SYN_REPORT 0   -> ket thuc mot frame event
```

Module luu:

- `left`, `right`, `middle`: trang thai nut chuot hien tai.
- `dx`, `dy`: do dich chuyen tuong doi gan nhat.
- `wheel`: gia tri cuon gan nhat.

`dx` va `dy` khong phai toa do tuyet doi tren man hinh. Chung chi la do dich chuyen so voi event truoc.

## 5. Interface `/proc/mouse_monitor`

Module tao file:

```text
/proc/mouse_monitor
```

Khi user-space doc file nay:

```bash
cat /proc/mouse_monitor
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

## 6. GUI Pages

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

TextView ben duoi hien output raw tu `/proc/mouse_monitor`.

### Event Log

Doc `dmesg`, loc log co chuoi `mouse_monitor`, va hien log kernel lien quan den ket noi chuot hoac su kien chuot.

### Help

Hien lenh demo:

```bash
make
sudo insmod src/mouse_monitor.ko
dmesg | grep mouse_monitor
cat /proc/mouse_monitor
sudo rmmod mouse_monitor
```

## 7. Cach demo nhanh

1. `make`
2. Chay GUI bang `make run`.
3. Vao Module Control, bam Build Module.
4. Bam Load Module.
5. Di chuyen/click/cuon chuot dang dung tren Ubuntu.
6. Vao Mouse Status, bam Refresh Status.
7. Vao Event Log, bam Filter mouse_monitor.
8. Khi xong, bam Unload Module.
