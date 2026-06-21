# Linux Kernel Programming Project

Project gom 3 bai lon cho mon Lap trinh nhan Linux. Moi bai co giao dien GTK rieng, source code C/C++, Makefile, script ho tro va tai lieu giai thich.

## Cau truc

```text
linux_kernel_programming_project/
├── bai01_shell_system_admin/
├── bai02_process_file_socket_network/
└── bai03_kernel_module_integration/
```

## Cai dat chung tren Ubuntu

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-3-dev libgtk-4-dev
```

Voi bai 3 can them header kernel:

```bash
sudo apt install linux-headers-$(uname -r) kmod
```

## Build nhanh tung bai

```bash
cd bai01_shell_system_admin && make
cd ../bai02_process_file_socket_network && make
cd ../bai03_kernel_module_integration && make
```

Doc README.md trong tung thu muc de biet cach demo chi tiet.
