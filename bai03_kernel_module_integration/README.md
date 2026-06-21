# Bai 03 - Linux Kernel Module Control Center

## Muc tieu

Xay dung Linux Kernel Module dang character device va ung dung GTK4 de demo load/unload module, doc/ghi `/dev/simple_kmod`, xem log kernel va kiem tra trang thai module.

## Giao dien

- `GtkApplicationWindow`
- HeaderBar: **Linux Kernel Module Control Center**
- Sidebar ben trai bang `GtkStackSidebar`
- Noi dung ben phai bang `GtkStack`
- Cac trang:
  - Dashboard
  - Module Control
  - Device I/O
  - Kernel Log
  - Help

## Chuc nang chinh

- Dashboard hien Module Status, Device File, Device Status, Kernel Version, Last Action.
- Module Control co Build Module, Load Module, Unload Module, Check Status, Clean Build.
- Load/Unload co dialog xac nhan vi can quyen sudo/root.
- Device I/O cho ghi/doc `/dev/simple_kmod`, bao loi ro neu device missing.
- Kernel Log co Refresh dmesg, Filter Module Log, Search, Clear View.
- Help hien cac buoc demo va lenh Linux tuong ung.

## Cau truc source

```text
bai03_kernel_module_integration/
├── src/
│   ├── main.c
│   ├── ui_main_window.c/.h
│   ├── ui_dashboard_page.c/.h
│   ├── ui_module_control_page.c/.h
│   ├── ui_device_io_page.c/.h
│   ├── ui_kernel_log_page.c/.h
│   ├── ui_help_page.c/.h
│   ├── kernel_module_commands.c/.h
│   ├── simple_kmod.c
│   └── Makefile
├── scripts/
├── docs/
├── Makefile
├── README.md
├── FUNCTION_BREAKDOWN.md
└── CODE_EXPLANATION.md
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

Lenh nay build ca GTK app va kernel module `src/simple_kmod.ko`.

## Chay

```bash
make run
```

Hoac:

```bash
./bin/kernel_module_gtk
```

Load/unload module can quyen root. App se goi `sudo bash scripts/module_control.sh ...` khi can, hoac co the chay app bang sudo:

```bash
sudo ./bin/kernel_module_gtk
```

## Demo

1. Mo Dashboard va bam Refresh.
2. Vao Module Control, bam Build Module.
3. Bam Load Module va xac nhan.
4. Bam Check Status.
5. Vao Device I/O, ghi data vao `/dev/simple_kmod`.
6. Bam Read from Device de doc lai.
7. Vao Kernel Log, bam Filter Module Log.
8. Quay lai Module Control va Unload Module.
