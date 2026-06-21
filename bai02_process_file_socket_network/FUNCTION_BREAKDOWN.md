# Phan ra chuc nang - Bai 02

## Main Window

- `main.c`: tao `GtkApplication`.
- `ui_main_window.c`: tao `GtkApplicationWindow`, HeaderBar, `GtkPaned`, `GtkStackSidebar` va `GtkStack`.
- `app_context.h`: giu con tro window va Log Page de cac page ghi log chung.

## Dashboard Page

- File: `ui_dashboard_page.c/.h`
- Hien thi card CPU, RAM, Total Processes, Network Status, IP Address.
- Input: nut Refresh.
- Output: label dashboard va Log Page.
- Du lieu lay tu `/proc/stat`, `sysinfo`, `get_process_list`, `get_network_info`.

## Process Page

- File: `ui_process_page.c/.h`
- Bang process gom PID, Name, CPU %, RAM %, State.
- Search loc theo PID hoac ten process.
- Refresh goi `get_process_list`.
- Kill Process doc PID dang chon, mo dialog xac nhan, sau do goi `kill_process_by_pid`.
- Create Child Process mo dialog nhap so luong, chon task, file output, interval, sau do goi `fork_child_task` nhieu lan.

## File Page

- File: `ui_file_page.c/.h`
- File Explorer don gian: current path, Choose Folder, Refresh, bang file.
- Open dung `g_app_info_launch_default_for_uri`.
- Read goi `read_file_syscall`, hien noi dung trong dialog.
- Write mo dialog nhap noi dung va goi `write_file_syscall`.
- Delete mo dialog xac nhan va xoa file bang `g_remove`.

## Socket Page

- File: `ui_socket_page.c/.h`
- Mini chat app co Server/Client mode.
- Server mode: Start goi `socket_server_start`, Stop goi `socket_server_stop`.
- Client mode: Send goi `socket_client_send`.
- Connection log hien trong TextView rieng va dong thoi ghi Log Page.

## Network Page

- File: `ui_network_page.c/.h`
- List interface ben trai lay tu `/sys/class/net`.
- Panel chi tiet ben phai:
  - Interface Name
  - IPv4
  - MAC
  - State
  - Bytes Sent
  - Bytes Received
- IPv4 lay bang `getifaddrs`; MAC/state/bytes lay tu `/sys/class/net/<iface>`.

## Log Page

- File: `ui_log_page.c/.h`
- `log_page_append` ghi log theo dang `[INFO]`, `[OK]`, `[ERROR]`.
- Clear Log xoa buffer.
- Save Log luu ra `process_file_socket_network_log.txt`.

## Backend giu nguyen

- `process_utils.c/.h`: `ps`, `kill`, `fork`, child task chay nen den khi bi kill.
- `file_syscall.c/.h`: `open`, `read`, `write`, `close`.
- `socket_demo.c/.h`: TCP server/client.
- `network_info.c/.h`: liet ke interface bang `getifaddrs`.
