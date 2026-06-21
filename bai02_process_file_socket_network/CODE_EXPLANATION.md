# Giai thich code - Bai 02

## Tong quan

UI da duoc chuyen sang GTK4 va tach module theo tung page. Backend Linux co san van duoc giu nguyen: process, file syscall, socket va network.

## `src/main.c`

Tao `GtkApplication`, bat signal `activate` va goi `ui_main_window_new`.

## `src/ui_main_window.c`

Tao cua so chinh:

- `GtkApplicationWindow`
- `GtkHeaderBar` voi tieu de "Linux Process, File, Socket & Network Manager"
- `GtkPaned` ngang
- `GtkStackSidebar` ben trai
- `GtkStack` ben phai

Moi page duoc them vao stack bang `gtk_stack_add_titled`, nen sidebar tu dong hien dung ten page.

## `src/ui_dashboard_page.c`

Dashboard hien cac card thong tin nhanh:

- CPU: doc `/proc/stat`.
- RAM: dung `sysinfo`.
- Process count: dem output cua `get_process_list`.
- Network/IP: dung `get_network_info`.

Nut Refresh cap nhat label va ghi log.

## `src/ui_process_page.c`

Process Page giong Task Manager:

- `get_process_list` tra ve output `ps`.
- UI parse tung dong de tao row PID/Name/CPU/RAM.
- State doc tu `/proc/<pid>/stat`.
- Search loc theo PID hoac name.
- Kill Process mo dialog xac nhan roi goi `kill_process_by_pid`.
- Kill Process tu choi kill PID cua chinh app va PID parent cua app de tranh tat chuong trinh dang demo.
- Create Child Process mo dialog nhap so luong, chon task, file output, interval va goi `fork_child_task`.

## `src/ui_file_page.c`

File Page giong File Explorer don gian:

- Choose Folder dung `GtkFileChooserNative`.
- Bang file dung `GtkListBox`, moi row gom Name, Type, Size, Permission, Modified Time.
- Read dung backend `read_file_syscall` va hien trong dialog.
- Write dung backend `write_file_syscall`.
- Delete co dialog xac nhan.

Backend file van dung system call cap thap `open/read/write/close` trong `file_syscall.c`.

## `src/ui_socket_page.c`

Socket Page la mini chat app:

- Server mode: Start tao TCP server bang `socket_server_start`.
- Stop goi `socket_server_stop`.
- Send tao TCP client bang `socket_client_send`.
- Server log tu thread socket duoc dua ve GTK main loop bang `g_idle_add`.

## `src/ui_network_page.c`

Network Page gom list interface va panel chi tiet:

- Interface list doc tu `/sys/class/net`.
- IPv4 lay bang `getifaddrs`.
- MAC doc tu `/sys/class/net/<iface>/address`.
- State doc tu `/sys/class/net/<iface>/operstate`.
- Bytes sent/received doc tu `/sys/class/net/<iface>/statistics`.

## `src/ui_log_page.c`

Log Page cung cap `log_page_append`. Moi page nhan `AppContext`, tu do ghi vao Log Page chung. Save Log ghi file `process_file_socket_network_log.txt`.

## Backend cu

- `process_utils.c`: chay `ps`, goi `kill`, goi `fork`; child co the ghi date/heartbeat vao file va chay den khi user kill.
- `file_syscall.c`: doc/ghi file bang syscall cap thap.
- `socket_demo.c`: TCP server/client su dung `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`.
- `network_info.c`: liet ke interface bang `getifaddrs`.
