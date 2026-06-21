# Explain - Bai 02: Linux Process, File, Socket & Network Manager

## 1. Muc dich project

Project nay la ung dung GTK4 viet bang C, mo phong mot cong cu System Monitor ket hop File Manager va Socket Demo. Muc tieu la demo nhieu nhom kien thuc Linux:

- quan ly process bang `ps`, `kill`, `fork`
- doc/ghi file bang system call cap thap `open`, `read`, `write`, `close`
- giao tiep TCP socket bang `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`
- doc thong tin network bang `getifaddrs` va cac file trong `/sys/class/net`
- hien thi tat ca trong giao dien GTK4 de de demo

Khac voi bai 01, project nay co `AppContext`. Context giup cac page dung chung cua so chinh va Log Page.

## 2. Cau truc thu muc

```text
bai02_process_file_socket_network/
|-- Makefile
|-- README.md
|-- CODE_EXPLANATION.md
|-- FUNCTION_BREAKDOWN.md
|-- explain.md
`-- src/
    |-- main.c
    |-- app_context.h
    |-- ui_main_window.c
    |-- ui_main_window.h
    |-- ui_dashboard_page.c
    |-- ui_dashboard_page.h
    |-- ui_process_page.c
    |-- ui_process_page.h
    |-- ui_file_page.c
    |-- ui_file_page.h
    |-- ui_socket_page.c
    |-- ui_socket_page.h
    |-- ui_network_page.c
    |-- ui_network_page.h
    |-- ui_log_page.c
    |-- ui_log_page.h
    |-- process_utils.c
    |-- process_utils.h
    |-- file_syscall.c
    |-- file_syscall.h
    |-- socket_demo.c
    |-- socket_demo.h
    |-- network_info.c
    `-- network_info.h
```

Nhom file UI:

- `ui_main_window.*`: tao cua so chinh, sidebar va stack.
- `ui_dashboard_page.*`: trang tong quan CPU/RAM/process/network.
- `ui_process_page.*`: trang quan ly tien trinh.
- `ui_file_page.*`: trang thao tac file.
- `ui_socket_page.*`: trang server/client TCP.
- `ui_network_page.*`: trang xem interface network.
- `ui_log_page.*`: trang log dung chung.

Nhom file backend:

- `process_utils.*`: lay danh sach process, kill PID, tao child process.
- `file_syscall.*`: doc/ghi file bang Linux syscall.
- `socket_demo.*`: TCP server/client.
- `network_info.*`: lay thong tin card mang.

## 3. Luong khoi dong ung dung

Luong chinh:

```text
main.c
  -> tao GtkApplication
  -> khi activate, goi ui_main_window_new()
  -> ui_main_window_new() tao AppContext
  -> tao GtkPaned, GtkStackSidebar, GtkStack
  -> tao tung page va them vao stack
  -> hien Dashboard dau tien
```

`GtkStackSidebar` khac sidebar tu viet tay o bai 01. O day, moi page duoc them bang:

```c
gtk_stack_add_titled(stack, page, "id", "Title");
```

`GtkStackSidebar` tu doc cac title trong stack va tao menu ben trai. Vi vay code sidebar gon hon, de them page moi hon.

## 4. `AppContext` dung de lam gi

File `app_context.h` khai bao:

```c
typedef struct {
    GtkWidget *window;
    LogPage *log_page;
} AppContext;
```

Moi page nhan `AppContext *ctx`. Tu context, page co the:

- lay cua so cha de mo dialog
- ghi log vao Log Page chung

Neu khong co `AppContext`, moi page phai tu truyen rat nhieu tham so rieng, code se roi hon.

Trong `ui_main_window.c`, context duoc cap phat bang `g_new0`. Khi window destroy, callback `on_window_destroy()` se:

1. dung socket server neu dang chay
2. giai phong `ctx->log_page`
3. giai phong `ctx`

Day la diem quan trong vi socket server co thread rieng. Neu dong app ma khong stop server, thread/socket co the con dang ton tai.

## 5. Main Window

Giao dien chinh gom:

- `GtkApplicationWindow`: cua so app.
- `GtkHeaderBar`: tieu de.
- `GtkPaned`: chia hai vung trai/phai.
- `GtkStackSidebar`: menu trai tu dong.
- `GtkStack`: vung hien page ben phai.

Thu tu page:

```text
Dashboard
Processes
Files
Socket
Network
Log
```

Page dau tien la Dashboard:

```c
gtk_stack_set_visible_child_name(GTK_STACK(stack), "dashboard");
```

## 6. Dashboard Page

Dashboard hien thong tin tong quan:

- CPU Usage
- RAM Usage
- Total Processes
- Network Status
- IP Address

Du lieu lay tu nhieu backend:

- CPU: doc `/proc/stat`.
- RAM: dung `sysinfo`.
- Process count: lay output tu `get_process_list()` roi dem dong.
- Network/IP: lay thong tin tu `get_network_info()`.

Khi bam Refresh, page cap nhat label va ghi log. Dashboard khong truc tiep thuc hien thao tac nguy hiem, no chu yeu de nguoi xem thay trang thai he thong hien tai.

## 7. Process Page

Trang Processes co bang PID, Name, CPU %, RAM %, State.

Backend nam trong `process_utils.c`.

### Lay danh sach process

Ham:

```c
char *get_process_list(void)
```

chay:

```text
ps -eo pid,comm,pcpu,pmem --sort=pid
```

bang `g_spawn_command_line_sync()`. Output la text nhieu dong. UI parse tung dong de tao row trong `GtkListBox`.

`ps` tra ve:

- `pid`: ma tien trinh.
- `comm`: ten command.
- `pcpu`: phan tram CPU.
- `pmem`: phan tram RAM.

State cua process co the doc them tu `/proc/<pid>/stat`. Trang UI ghep cac thong tin nay thanh bang de nguoi dung xem.

### Search

Search loc theo PID hoac ten process. Thay vi goi lai backend moi lan, page co the refresh list va chi hien cac row phu hop tu khoa.

### Kill process

Khi nguoi dung chon mot process va bam Kill:

1. UI lay PID cua row dang chon.
2. Mo dialog xac nhan.
3. Neu PID la chinh app dang chay hoac parent process cua app, UI tu choi kill de tranh tat chuong trinh bai 2.
4. Neu dong y, goi `kill_process_by_pid(pid, &message)`.
5. Backend goi `kill(pid, SIGTERM)`.
6. UI hien ket qua va refresh list.

`SIGTERM` la tin hieu yeu cau process ket thuc mot cach lich su. Khong dung `SIGKILL` mac dinh vi `SIGKILL` qua manh va process khong co co hoi don dep.

### Create Child Process

Ham:

```c
char *fork_child_task(ChildTaskType task, const char *output_path, int interval_seconds)
```

goi `fork()` de tao process con chay nen. UI hien dialog theo thu tu:

- so luong process con can tao
- option cong viec moi process con se lam
- file output neu cong viec can ghi file
- thoi gian lap lai tinh bang giay

Co 3 option cong viec:

- `Write current date to file`: process con lap vo han, moi chu ky ghi PID, PPID va thoi gian hien tai vao file.
- `Write heartbeat counter to file`: process con lap vo han, moi chu ky ghi PID, PPID va counter tang dan vao file.
- `Idle only`: process con chi song nen va ngu theo chu ky, khong ghi file.

Ben trong child, code dat ten process bang `prctl(PR_SET_NAME, ...)`, nen trong bang Processes co the thay ten nhu `child_date`, `child_beat`, `child_idle`. Parent khong `waitpid()` ngay nua, vi neu wait thi UI se bi dung cho den khi child ket thuc. Thay vao do, child tiep tuc chay cho den khi user chon PID trong UI va bam `Kill Process`.

Sau khi tao child, o tim kiem cua Process Page tu dien `child_` de bang chi hien cac process con demo. Viec nay giup tranh chon nham PID cua app chinh.

Parent dat `SIGCHLD` thanh `SIG_IGN` de Linux tu thu gom child sau khi no bi kill, tranh tao zombie process.

## 8. File Page

Trang Files la file manager don gian. Backend doc/ghi file cap thap nam trong `file_syscall.c`.

### Liet ke file

UI cho chon folder bang `GtkFileChooserNative`. Sau do page doc danh sach file va hien:

- Name
- Type
- Size
- Permission
- Modified Time

### Open file

Open thuong dung `g_app_info_launch_default_for_uri()`, nghia la mo file bang ung dung mac dinh cua he thong.

### Read file bang syscall

Ham:

```c
char *read_file_syscall(const char *path)
```

lam cac buoc:

1. `open(path, O_RDONLY)` de mo file chi doc.
2. Tao `GString` de gom noi dung.
3. Lap `read(fd, buffer, sizeof(buffer))` cho den khi het file.
4. Moi lan doc duoc `n` byte thi append vao `GString`.
5. Neu `read` loi thi them message loi.
6. `close(fd)`.
7. Tra ve chuoi noi dung.

Day la cach doc file thap hon so voi `g_file_get_contents()`, phu hop muc tieu bai hoc system call.

### Write file bang syscall

Ham:

```c
int write_file_syscall(const char *path, const char *content, char **message)
```

goi:

```c
open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644)
```

Y nghia:

- `O_WRONLY`: mo de ghi.
- `O_CREAT`: neu file chua co thi tao.
- `O_TRUNC`: neu file da co thi xoa noi dung cu.
- `0644`: owner doc/ghi, group va others chi doc.

Sau do `write()` ghi toan bo chuoi vao file. Neu so byte ghi ra khac do dai input, ham bao loi.

### Delete

Delete co dialog xac nhan, sau do xoa file bang `g_remove()`. Xoa la thao tac mat du lieu nen can dialog.

## 9. Socket Page

Trang Socket la mini chat app bang TCP. Backend nam trong `socket_demo.c`, gom server, client va co che broadcast message.

### Bien trang thai server

```c
static int server_fd = -1;
static gboolean server_running = FALSE;
static pthread_t server_thread;
```

Server phai cho nhieu client ket noi, nen no chay trong thread rieng de khong lam treo giao dien GTK. Server cung giu danh sach socket client dang ket noi trong `server_clients`.

### Start server

Ham:

```c
int socket_server_start(int port, SocketLogFn log_fn, void *user_data, char **message)
```

lam cac buoc:

1. Kiem tra server da chay chua.
2. Tao socket bang `socket(AF_INET, SOCK_STREAM, 0)`.
3. Bat `SO_REUSEADDR` de port co the dung lai nhanh hon.
4. Gan dia chi `INADDR_ANY` va port bang `bind()`.
5. Cho ket noi bang `listen()`.
6. Dat `server_running = TRUE`.
7. Tao thread bang `pthread_create()`.

Thread chay ham `server_loop()`.

### Server loop

Trong vong lap:

1. Goi `accept()` de cho client ket noi.
2. Lay IP client bang `inet_ntop()`.
3. Them socket client vao danh sach `server_clients`.
4. Tao thread rieng de nhan message tu client do.

Thread cua moi client lap `recv()`. Khi nhan message, server format thanh:

```text
Client: noi dung
```

Sau do server hien dong nay tren log cua server va broadcast lai cho tat ca client dang ket noi. Vi broadcast ca ve client vua gui, nen client log se thay tin cua minh va tin cua cac client khac.

Neu server UI bam Send, app goi `socket_server_broadcast_message()` va format thanh:

```text
Server: noi dung
```

Dong nay cung hien tren log server va duoc gui den tat ca client.

### Stop server

`socket_server_stop()` dat `server_running = FALSE`, goi `shutdown()` va `close()` server socket de pha `accept()` dang cho. Sau do shutdown cac client socket de cac thread client tu thoat.

### Client connect va send

Khi chon Client mode va bam Start, UI goi:

```c
socket_client_connect(host, port, socket_log_callback, page, &message)
```

lam cac buoc:

1. Tao socket.
2. Chuyen IP text sang binary bang `inet_pton()`.
3. Ket noi server bang `connect()`.
4. Tao thread nhan du lieu tu server.

Khi bam Send o Client mode, UI goi `socket_client_send_message()`. Client chi gui noi dung goc len server. Server se gan tien to `Client:` va broadcast lai. Vi vay log tren client chi hien dang:

```text
Client: Xin chao
Server: Chao ban
```

### Cap nhat GTK tu thread

GTK khong nen duoc cap nhat truc tiep tu thread phu. Vi vay UI socket dua log tu thread ve main loop bang `g_idle_add()`. Day la diem quan trong de tranh loi race condition hoac crash giao dien.

## 10. Network Page

Trang Network gom danh sach interface ben trai va chi tiet ben phai.

Backend `network_info.c` dung:

```c
getifaddrs(&ifaddr)
```

de lay danh sach dia chi cua cac interface. Voi tung interface, code kiem tra `sa_family`:

- `AF_INET`: IPv4
- `AF_INET6`: IPv6

Sau do dung `getnameinfo()` de doi dia chi ve chuoi de doc.

Ngoai ra UI doc cac file trong `/sys/class/net/<iface>/`:

- `address`: MAC address.
- `operstate`: trang thai UP/DOWN.
- `statistics/tx_bytes`: byte da gui.
- `statistics/rx_bytes`: byte da nhan.

Day la cach Linux expose thong tin thiet bi network qua filesystem ao `/sys`.

## 11. Log Page

Log Page co `GtkTextView`, nhan log tu cac page qua:

```c
log_page_append(ctx->log_page, "INFO", "message");
```

Nut Save ghi ra file:

```text
process_file_socket_network_log.txt
```

Log giup khi demo co the thay moi thao tac da lam gi va ket qua ra sao.

## 12. Cach build va chay

Cai thu vien:

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-4-dev
```

Build:

```bash
make
```

Chay:

```bash
make run
```

Hoac:

```bash
./bin/process_file_socket_gui
```

## 13. Tom tat kien thuc can nam

- GTK4 chia UI thanh nhieu page bang `GtkStack`.
- `AppContext` giup truyen window va Log Page cho cac page.
- Process demo dung `ps`, `kill`, `fork`, child process chay nen va dung `SIGTERM` de ket thuc.
- File demo dung syscall cap thap `open/read/write/close`.
- Socket demo dung TCP server/client va thread.
- Network demo lay thong tin tu `getifaddrs` va `/sys/class/net`.
- Khi thread phu muon cap nhat UI GTK, nen dua viec cap nhat ve main loop bang `g_idle_add`.
