# Phan ra chuc nang - Bai 02

## Module process

- `get_process_list`: goi `ps -eo pid,comm,pcpu,pmem --sort=pid` de lay bang process.
- `kill_process_by_pid`: dung system call `kill(pid, SIGTERM)`.
- `fork_demo`: dung `fork()` tao child, child sleep ngan roi exit.
- Input: PID hoac nut lenh.
- Output: chuoi danh sach process, log PID, thong bao loi neu PID khong ton tai.

## Module file syscall

- `read_file_syscall`: dung `open`, lap `read`, dong file bang `close`.
- `write_file_syscall`: dung `open` voi `O_CREAT | O_TRUNC | O_WRONLY`, ghi bang `write`.
- Input: duong dan file, noi dung can ghi.
- Output: noi dung file hoac loi tu `errno`.

## Module socket

- `socket_server_start`: tao thread server, goi `socket`, `bind`, `listen`, `accept`, `recv`, `send`.
- `socket_server_stop`: dong server socket va dung thread.
- `socket_client_send`: tao client socket, `connect`, `send`, `recv`.
- Input: host, port, message.
- Output: log ket noi va phan hoi server.

## Module network

- `get_network_info`: dung `getifaddrs` va `getnameinfo` de liet ke interface IPv4/IPv6.
- Input: khong co.
- Output: chuoi thong tin interface.

## Module GTK

- Tao notebook gom Process, File, Socket, Network.
- Ket noi button voi cac ham backend.
- Tat ca output day vao `GtkTextView` log de de demo.

## Luong xu ly chinh

1. Nguoi dung bam nut tren GTK.
2. Callback doc input tu entry/text view.
3. Callback goi module process/file/socket/network.
4. Ket qua tra ve la chuoi hoac ma loi.
5. Giao dien hien thi log va hop thoai neu loi.
