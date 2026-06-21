# Bai 02 - Linux Process, File, Socket & Network Manager

## Muc tieu

Ung dung GTK4 kieu System Monitor de demo quan ly tien trinh, thao tac file bang system call cap thap, TCP socket va thong tin network tren Ubuntu.

## Giao dien

- `GtkApplicationWindow`
- HeaderBar: **Linux Process, File, Socket & Network Manager**
- `GtkPaned` ngang:
  - `GtkStackSidebar` ben trai
  - `GtkStack` ben phai
- Sidebar gom:
  - Dashboard
  - Processes
  - Files
  - Socket
  - Network
  - Log

## Chuc nang

- Dashboard: CPU Usage, RAM Usage, Total Processes, Network Status, IP Address.
- Processes: bang PID, Name, CPU %, RAM %, State; search; refresh; kill process co dialog xac nhan; tao child process co dialog nhap so luong.
- Files: chon folder, refresh, bang file Name/Type/Size/Permission/Modified Time; Open, Read, Write, Delete. Read/Write mo dialog rieng, Delete co xac nhan.
- Socket: mini chat app co Server/Client mode, IP, Port, Start, Stop, Connection Log, Message va Send.
- Network: danh sach interface ben trai, panel chi tiet ben phai gom IPv4, MAC, state, bytes sent/received.
- Log: TextView log toan chuong trinh, Clear Log, Save Log.

## Cau truc source

```text
bai02_process_file_socket_network/
├── src/
│   ├── main.c
│   ├── app_context.h
│   ├── ui_main_window.c/.h
│   ├── ui_dashboard_page.c/.h
│   ├── ui_process_page.c/.h
│   ├── ui_file_page.c/.h
│   ├── ui_socket_page.c/.h
│   ├── ui_network_page.c/.h
│   ├── ui_log_page.c/.h
│   ├── process_utils.c/.h
│   ├── file_syscall.c/.h
│   ├── socket_demo.c/.h
│   └── network_info.c/.h
├── docs/
├── Makefile
├── README.md
├── FUNCTION_BREAKDOWN.md
└── CODE_EXPLANATION.md
```

## Cai thu vien

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
make run
```

Hoac:

```bash
./bin/process_file_socket_gui
```

## Demo

1. Mo Dashboard va bam Refresh de xem CPU/RAM/process/network.
2. Vao Processes, search process, chon mot process demo va bam Kill Process de thay dialog xac nhan.
3. Bam Create Child Process, nhap so luong child process can tao.
4. Vao Files, chon folder, doc/ghi file bang dialog.
5. Vao Socket, Start server port 9090, chuyen Client hoac giu IP `127.0.0.1`, gui message.
6. Vao Network, chon interface de xem IPv4/MAC/state/bytes.
7. Vao Log de Clear hoac Save log.
