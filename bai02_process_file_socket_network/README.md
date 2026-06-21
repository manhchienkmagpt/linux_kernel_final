# Bai 02 - Process, File, Socket, Network

## Muc tieu

Xay dung ung dung GTK tren Ubuntu de demo lap trinh tien trinh, thao tac file bang system call cap thap, TCP socket va thong tin network.

## Chuc nang chinh

- Xem danh sach process dang chay: PID, ten, CPU/RAM neu `ps` ho tro.
- Kill process theo PID bang `kill`.
- Tao tien trinh con demo bang `fork()`.
- Doc va ghi file bang `open`, `read`, `write`, `close`.
- Tao TCP server don gian, nhan message tu client.
- Tao TCP client gui tin nhan den server.
- Hien thi log socket tren giao dien.
- Hien thi interface, IP va trang thai network bang `getifaddrs`.

## Cau truc thu muc

```text
bai02_process_file_socket_network/
├── src/
│   ├── main.c
│   ├── process_utils.c
│   ├── process_utils.h
│   ├── file_syscall.c
│   ├── file_syscall.h
│   ├── socket_demo.c
│   ├── socket_demo.h
│   ├── network_info.c
│   └── network_info.h
├── docs/
├── Makefile
├── README.md
├── FUNCTION_BREAKDOWN.md
└── CODE_EXPLANATION.md
```

## Cai thu vien

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-3-dev
```

## Build

```bash
make
```

## Chay

```bash
make run
```

## Demo

1. Bam **Refresh Process** de xem process. Nhap PID va bam **Kill PID** de thu kill mot process demo.
2. Bam **Fork Demo** de tao child process. Log se hien PID cua parent va child.
3. Chon file, bam **Read File** de doc bang syscall. Nhap noi dung va bam **Write File** de ghi.
4. Bam **Start Server** voi port 9090, sau do dung client host `127.0.0.1`, port `9090`, gui message.
5. Bam **Network Info** de xem IP/interface hien co.
