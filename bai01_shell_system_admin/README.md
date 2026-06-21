# Bai 01 - Linux System Admin Tool

## Mo ta

Ung dung quan tri he thong Ubuntu bang GTK4. Giao dien gom mot cua so chinh, HeaderBar, sidebar ben trai va vung noi dung ben phai dung `GtkStack`.

## Giao dien

- HeaderBar: **Linux System Admin Tool**
- Sidebar:
  - File Manager
  - Cron Scheduler
  - Time System
  - Apt Manager
  - Log
- Moi chuc nang nam tren mot page rieng.
- Cac thao tac nho nhu tao file, xoa, doi ten, copy/move, doi gio, cai/go package deu mo dialog xac nhan hoac dialog nhap du lieu.
- Log Page ghi log theo dang `[INFO]`, `[OK]`, `[ERROR]`.

## Chuc nang

- File Manager: chon thu muc, refresh, tim file, bang danh sach file, tao/xoa/doi ten/copy/move/xem thong tin.
- Cron Scheduler: nhap lenh, nhap 5 truong cron, preset moi phut/moi ngay 8h/moi tuan/custom, them/xem/xoa cron.
- Time System: xem thoi gian hien tai, refresh, nhap ngay/gio va doi thoi gian he thong.
- Apt Manager: cai dat, go bo, kiem tra package bang apt/dpkg.
- Log: xem log toan chuong trinh, clear log, save log ra `system_admin_log.txt`.

## Cau truc source

```text
bai01_shell_system_admin/
├── src/
│   ├── main.c
│   ├── ui_main_window.c/.h
│   ├── ui_file_page.c/.h
│   ├── ui_cron_page.c/.h
│   ├── ui_time_page.c/.h
│   ├── ui_apt_page.c/.h
│   ├── ui_log_page.c/.h
│   └── system_commands.c/.h
├── scripts/system_ops.sh
├── screenshots/
├── Makefile
├── README.md
├── FUNCTION_BREAKDOWN.md
└── CODE_EXPLANATION.md
```

## Cai thu vien tren Ubuntu

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-4-dev
```

## Build

```bash
make
```

## Chay

```bash
./system_admin_gui
```

Mot so chuc nang can quyen root, vi du doi thoi gian he thong va cai/go package:

```bash
sudo ./system_admin_gui
```

## Demo nhanh

1. Mo File Manager, chon thu muc, thu tim file va xem thong tin.
2. Tao file demo, doi ten, copy/move sang thu muc khac, sau do xoa.
3. Mo Cron Scheduler, chon preset "Moi phut", nhap lenh va them cron job.
4. Mo Time System, bam Refresh, thu doi thoi gian khi chay bang sudo.
5. Mo Apt Manager, nhap `htop`, kiem tra package, cai dat hoac go bo neu can.
6. Mo Log, xem output, Clear Log hoac Save Log.
