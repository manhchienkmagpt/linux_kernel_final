# Giai thich code - Bai 02

## `src/main.c`

Tao giao dien GTK gom 4 tab. Moi tab co entry, button va text view de hien output. File nay chi xu ly giao dien va dieu phoi, phan logic Linux nam trong cac file `.c` rieng.

## `src/process_utils.c`

- `get_process_list` dung `g_spawn_command_line_sync` chay lenh `ps`. Day la cach gon de lay CPU/RAM phuc vu demo.
- `kill_process_by_pid` goi truc tiep `kill(pid, SIGTERM)`.
- `fork_demo` goi `fork()`. Child process ghi log co PID rieng, sleep 2 giay roi thoat. Parent tra ve PID child.

## `src/file_syscall.c`

File nay dung system call cap thap:

- `open`: mo file.
- `read`: doc tung block byte.
- `write`: ghi byte vao file.
- `close`: dong file descriptor.

Khac voi `fopen/fread`, cac ham nay lam viec truc tiep voi file descriptor cua Linux.

## `src/socket_demo.c`

Server chay trong thread rieng de giao dien GTK khong bi treo. Server tao TCP socket, bind port, listen va accept client. Khi client gui message, server log noi dung va tra ve chuoi `ACK from server`.

Client tao socket, connect den host/port, gui message va doc phan hoi.

## `src/network_info.c`

Dung `getifaddrs` de duyet danh sach network interface, `getnameinfo` de chuyen dia chi IP sang chuoi. Ket qua gom ten interface, IPv4/IPv6 va trang thai UP/DOWN.

## GTK goi backend

Moi nut bam duoc gan voi callback qua `g_signal_connect`. Callback lay gia tri tu `GtkEntry`, goi ham backend, sau do hien ket qua trong `GtkTextView`. Loi tu backend duoc tra ve bang chuoi `GError` hoac `errno`.
