# Explain - Bai 03: Linux Kernel Module Control Center

## 1. Muc dich project

Project nay gom hai phan ket hop voi nhau:

- kernel-space: mot Linux kernel module tao character device `/dev/simple_kmod`
- user-space: mot ung dung GTK4 de build, load, unload module, doc/ghi device va xem kernel log

Day la bai co muc do he thong sau hon bai 01 va bai 02. App GTK chay o user-space, con `simple_kmod.c` chay trong kernel-space sau khi duoc load bang `insmod`. Hai phan giao tiep voi nhau qua file device `/dev/simple_kmod`.

## 2. Cau truc thu muc

```text
bai03_kernel_module_integration/
|-- Makefile
|-- README.md
|-- CODE_EXPLANATION.md
|-- FUNCTION_BREAKDOWN.md
|-- explain.md
|-- scripts/
|   |-- module_control.sh
|   `-- setup_device_permission.sh
`-- src/
    |-- Makefile
    |-- main.c
    |-- app_context.h
    |-- ui_main_window.c
    |-- ui_main_window.h
    |-- ui_dashboard_page.c
    |-- ui_dashboard_page.h
    |-- ui_module_control_page.c
    |-- ui_module_control_page.h
    |-- ui_device_io_page.c
    |-- ui_device_io_page.h
    |-- ui_kernel_log_page.c
    |-- ui_kernel_log_page.h
    |-- ui_help_page.c
    |-- ui_help_page.h
    |-- kernel_module_commands.c
    |-- kernel_module_commands.h
    `-- simple_kmod.c
```

Y nghia nhom file:

- `src/simple_kmod.c`: kernel module tao character device.
- `src/kernel_module_commands.c/.h`: backend user-space de UI kiem tra module, chay command, doc/ghi device, doc dmesg.
- `src/ui_*_page.c/.h`: cac trang GTK.
- `scripts/module_control.sh`: script load/unload/status module.
- `scripts/setup_device_permission.sh`: script ho tro phan quyen device neu can.
- `src/Makefile`: build kernel module.
- `Makefile` o thu muc project: build app GTK va module.

## 3. Kien thuc nen nam truoc

### User-space va kernel-space

User-space la noi ung dung binh thuong chay, vi du app GTK. Code user-space khong duoc truy cap truc tiep bo nho kernel.

Kernel-space la noi Linux kernel va kernel module chay. Code o day co quyen cao, neu loi co the lam treo he thong. Vi vay giao tiep giua user-space va kernel-space phai qua API an toan nhu system call, device file, procfs, sysfs.

### Character device

Character device la thiet bi doc/ghi theo dong byte. Tu user-space, no nhin nhu file:

```text
/dev/simple_kmod
```

Nguoi dung co the:

```bash
echo "hello" > /dev/simple_kmod
cat /dev/simple_kmod
```

Ben trong kernel module, cac thao tac tren duoc anh xa vao callback:

- `open` -> `kmod_open`
- `read` -> `kmod_read`
- `write` -> `kmod_write`
- `close` -> `kmod_release`

## 4. Luong khoi dong app GTK

Luong user-space:

```text
main.c
  -> tao GtkApplication
  -> activate
  -> ui_main_window_new()
  -> tao AppContext
  -> tao GtkPaned, GtkStackSidebar, GtkStack
  -> them Dashboard, Module Control, Device I/O, Kernel Log, Help
  -> hien Dashboard
```

`app_context.h` cua bai nay chi giu:

```c
typedef struct {
    GtkWidget *window;
} AppContext;
```

Khac bai 02, bai 03 khong co Log Page dung chung. Cac page tu hien output rieng, dac biet Module Control va Kernel Log.

## 5. Main Window

`ui_main_window.c` tao:

- `GtkApplicationWindow`
- `GtkHeaderBar`
- `GtkPaned`
- `GtkStackSidebar`
- `GtkStack`

Cac page:

```text
Dashboard
Module Control
Device I/O
Kernel Log
Help
```

`GtkStackSidebar` tu tao danh sach trang dua tren cac title khi them page bang `gtk_stack_add_titled()`.

## 6. Kernel module `simple_kmod.c`

Day la phan quan trong nhat cua project.

### Bien toan cuc

```c
static dev_t dev_number;
static struct cdev simple_cdev;
static struct class *simple_class;
static char device_buffer[BUFFER_SIZE];
static size_t buffer_len;
```

Y nghia:

- `dev_number`: chua major/minor number cua device.
- `simple_cdev`: cau truc character device cua kernel.
- `simple_class`: class de kernel/udev tao node trong `/dev`.
- `device_buffer`: buffer trong kernel luu du lieu nguoi dung ghi vao.
- `buffer_len`: so byte hop le trong buffer.

### File operations

Kernel module khai bao:

```c
static const struct file_operations simple_fops = {
    .owner = THIS_MODULE,
    .open = kmod_open,
    .read = kmod_read,
    .write = kmod_write,
    .release = kmod_release,
};
```

`struct file_operations` la bang callback. Khi user-space thao tac voi `/dev/simple_kmod`, kernel goi ham tuong ung trong bang nay.

### `simple_init`

Ham nay chay khi module duoc load.

Cac buoc:

1. `alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME)`
   - xin kernel cap mot major/minor number.

2. `cdev_init(&simple_cdev, &simple_fops)`
   - khoi tao character device va gan bang callback.

3. `cdev_add(&simple_cdev, dev_number, 1)`
   - dang ky character device voi kernel.

4. `class_create(...)`
   - tao device class.
   - Code co `#if` de tuong thich kernel truoc va sau version 6.4, vi signature cua `class_create` thay doi.

5. `device_create(simple_class, NULL, dev_number, NULL, DEVICE_NAME)`
   - tao device node `/dev/simple_kmod`.

6. Gan du lieu mac dinh:

```c
snprintf(device_buffer, sizeof(device_buffer), "Hello from simple_kmod\n");
buffer_len = strlen(device_buffer);
```

7. Ghi log kernel bang `pr_info`.

Neu buoc nao loi, code don dep tai nguyen da tao truoc do roi tra ve ma loi.

### `kmod_open`

Ham nay chay khi user-space mo device:

```bash
cat /dev/simple_kmod
```

hoac app GTK goi `open(DEVICE_PATH, O_RDONLY)`.

Ham chi ghi log:

```c
pr_info("simple_kmod: device opened\n");
```

### `kmod_read`

Ham nay chay khi user-space doc device.

Logic:

1. Neu `*offset >= buffer_len`, nghia la da doc het du lieu, tra ve `0`. Trong Linux, `read()` tra ve 0 thuong co nghia la EOF.
2. Tinh so byte co the doc:

```c
bytes_to_read = min(count, buffer_len - (size_t)*offset);
```

3. Copy du lieu tu kernel buffer sang user buffer:

```c
copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read)
```

Khong duoc dung `memcpy` truc tiep tu kernel sang user-space. `copy_to_user` kiem tra dia chi user-space va xu ly an toan hon.

4. Tang offset:

```c
*offset += bytes_to_read;
```

5. Ghi log va tra ve so byte da doc.

### `kmod_write`

Ham nay chay khi user-space ghi vao device.

Logic:

1. Gioi han so byte ghi toi da la `BUFFER_SIZE - 1` de con cho ky tu ket thuc chuoi `\0`.
2. Xoa buffer cu bang `memset`.
3. Copy du lieu tu user-space vao kernel-space:

```c
copy_from_user(device_buffer, user_buffer, bytes_to_write)
```

4. Them `\0` de buffer co the in nhu chuoi C.
5. Cap nhat `buffer_len`.
6. Ghi log noi dung vua ghi.
7. Tra ve so byte da ghi.

### `kmod_release`

Ham nay chay khi user-space dong device. No ghi log `device released`.

### `simple_exit`

Ham nay chay khi module bi unload.

Thu tu don dep:

1. `device_destroy`
2. `class_destroy`
3. `cdev_del`
4. `unregister_chrdev_region`
5. `pr_info("simple_kmod: unloaded")`

Thu tu nay nguoc voi luc khoi tao, giup khong de lai device/class/major number rac.

## 7. Backend user-space `kernel_module_commands.c`

File nay la cau noi giua app GTK va he thong Linux/kernel module.

### Kiem tra module da load chua

```c
gboolean module_is_loaded(void)
```

Ham doc file:

```text
/proc/modules
```

roi tim chuoi `"simple_kmod "`. Neu thay, module dang loaded.

### Kiem tra device co ton tai chua

```c
gboolean device_exists(void)
```

goi:

```c
g_file_test(DEVICE_PATH, G_FILE_TEST_EXISTS)
```

`DEVICE_PATH` thuong la `/dev/simple_kmod`.

### Lay kernel version

```c
char *kernel_version_string(void)
```

dung `uname()` va tra ve `uts.release`, vi du `6.x.x-generic`.

### Chay command dong bo

```c
char *run_command_sync(const char *command, gboolean *ok)
```

Ham chay command bang `g_spawn_command_line_sync()`, gom stdout/stderr vao mot chuoi, va gan `ok = TRUE` neu status bang 0.

### Chay command bat dong bo

```c
void run_command_async(const char *command, CommandDoneCallback callback, gpointer user_data)
```

Dung cho cac lenh co the lau nhu build module, load/unload. Neu chay dong bo tren UI thread, giao dien co the bi dung.

Luong async:

```text
run_command_async()
  -> cap phat AsyncCommand
  -> tao GThread
  -> command_worker() chay trong thread phu
  -> command_worker() goi run_command_sync()
  -> tao AsyncResult
  -> g_idle_add(command_done_idle, result)
  -> command_done_idle() chay lai tren GTK main loop
  -> goi callback cua UI
```

Diem quan trong: callback cap nhat UI duoc goi qua `g_idle_add`, nen no quay ve main thread cua GTK.

Voi lenh can quyen root, backend co them:

```c
run_command_async_sudo(GTK_WINDOW(parent), command, callback, user_data)
```

Ham nay kiem tra app co dang chay bang root hay sudo da co phien xac thuc hop le chua. Neu sudo can mat khau, app mo dialog GTK de user nhap mat khau. Mat khau khong dua vao command line; no duoc gui vao stdin cua `sudo -S`, sau do command van chay trong thread phu de UI khong bi treo.

### Doc device

```c
char *read_device_data(gboolean *ok)
```

User-space mo `/dev/simple_kmod` bang:

```c
open(DEVICE_PATH, O_RDONLY)
```

roi `read()` toi da 1024 byte. Ben kernel, thao tac nay se kich hoat `kmod_open`, `kmod_read`, `kmod_release`.

### Ghi device

```c
char *write_device_data(const char *text, gboolean *ok)
```

User-space mo device bang:

```c
open(DEVICE_PATH, O_WRONLY)
```

roi `write()` chuoi nguoi dung nhap. Ben kernel, thao tac nay kich hoat `kmod_write`.

### Doc kernel log

```c
char *read_kernel_log(gboolean module_only, const char *filter, int *line_count)
```

Ham chay:

```text
dmesg | tail -200
```

Sau do tach tung dong:

- bo dong rong
- neu `module_only == TRUE` thi chi giu dong co `simple_kmod`
- neu co `filter` thi chi giu dong chua filter
- dem so dong ket qua

Trang Kernel Log dung bien the co ho tro sudo. App thu doc `dmesg` binh thuong truoc; neu he thong tu choi quyen va sudo can mat khau, app hien dialog nhap mat khau roi chay lai `dmesg` qua `sudo -S`.

## 8. Script `module_control.sh`

Script nay nhan lenh:

```text
load
unload
status
dmesg
```

### `need_root`

Load/unload kernel module can quyen root. Ham `need_root` kiem tra:

```bash
id -u
```

Neu khong phai root va co `sudo`, script tu chay lai chinh no bang `sudo`. Neu khong co sudo, script bao nguoi dung chay app bang root.

### Load

Kiem tra module da load chua bang:

```bash
lsmod | grep -q "^simple_kmod"
```

Neu chua, goi:

```bash
insmod src/simple_kmod.ko
```

### Unload

Neu module dang loaded, goi:

```bash
rmmod simple_kmod
```

### Status

In dong `lsmod` cua module va thong tin `/dev/simple_kmod` neu device ton tai.

### Dmesg

Loc kernel log:

```bash
dmesg | grep simple_kmod | tail -40
```

## 9. Dashboard Page

Dashboard hien:

- Module Status: Loaded / Not Loaded.
- Device File: `/dev/simple_kmod`.
- Device Status: Exists / Missing.
- Kernel Version.
- Last Action.

Khi bam Refresh, page goi cac ham backend:

- `module_is_loaded()`
- `device_exists()`
- `kernel_version_string()`

Day la trang de xem nhanh module da san sang demo hay chua.

## 10. Module Control Page

Trang nay co cac nut:

- Build Module
- Load Module
- Unload Module
- Check Status
- Clean Build

Build Module chay `make module` hoac lenh tuong ung trong Makefile. Load/Unload goi `scripts/module_control.sh`.

Load va Unload co dialog xac nhan vi day la thao tac can quyen cao va anh huong kernel. Sau khi nguoi dung dong y, command duoc chay bang `run_command_async_sudo`. Neu sudo yeu cau mat khau, app hien them dialog nhap mat khau.

Output command hien trong `GtkTextView`. Vi chay async, giao dien khong bi treo trong luc build/load/unload.

## 11. Device I/O Page

Trang Device I/O la noi user-space giao tiep voi kernel module qua `/dev/simple_kmod`.

### Write to Device

Luong chay:

```text
nguoi dung nhap text
  -> bam Write
  -> UI kiem tra device co ton tai khong
  -> goi write_device_data(text, &ok)
  -> user-space open/write/close /dev/simple_kmod
  -> kernel goi kmod_write
  -> buffer trong kernel duoc cap nhat
  -> UI hien ket qua
```

Neu input rong, page hoi xac nhan de tranh ghi rong ngoai y muon.

### Read from Device

Luong chay:

```text
bam Read
  -> UI kiem tra device
  -> goi read_device_data(&ok)
  -> user-space open/read/close /dev/simple_kmod
  -> kernel goi kmod_read
  -> du lieu trong device_buffer tra ve app
  -> UI hien noi dung
```

Neu module chua load, `/dev/simple_kmod` se missing, page hien loi ro rang.

## 12. Kernel Log Page

Trang nay giup xem log tu `dmesg`.

Nut chuc nang:

- Refresh dmesg: doc 200 dong cuoi, tu dong hoi mat khau sudo neu he thong chan doc `dmesg`.
- Filter Module Log: chi hien dong co `simple_kmod`.
- Search/Filter: loc theo tu khoa nguoi dung nhap.
- Clear View: xoa TextView.

Kernel module dung `pr_info()` de ghi log. Cac log nay khong hien trong stdout cua app GTK, ma nam trong kernel ring buffer, xem bang `dmesg`.

## 13. Help Page

Help Page hien quy trinh demo va lenh Linux tuong ung:

```bash
make
sudo insmod src/simple_kmod.ko
lsmod | grep simple_kmod
echo "hello" | sudo tee /dev/simple_kmod
cat /dev/simple_kmod
dmesg | grep simple_kmod
sudo rmmod simple_kmod
```

Trang nay huu ich khi thuyet trinh vi nguoi xem co the thay UI dang tu dong hoa cac lenh Linux nao.

## 14. Cach build va chay

Cai thu vien va header:

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-4-dev linux-headers-$(uname -r) kmod
```

Build:

```bash
make
```

Lenh nay build ca app GTK va kernel module.

Chay:

```bash
make run
```

Hoac:

```bash
./bin/kernel_module_gtk
```

Load/unload module can sudo/root. Co the chay app bang:

```bash
sudo ./bin/kernel_module_gtk
```

## 15. Tom tat kien thuc can nam

- Kernel module co `module_init` va `module_exit`.
- Character device can `alloc_chrdev_region`, `cdev_init`, `cdev_add`, `class_create`, `device_create`.
- User-space doc/ghi `/dev/simple_kmod` bang `open/read/write/close`.
- Kernel-space copy du lieu bang `copy_to_user` va `copy_from_user`, khong copy truc tiep.
- Load/unload module dung `insmod` va `rmmod`.
- Log kernel xem bang `dmesg`.
- App GTK chay command lau bang thread phu va dua ket qua ve main loop bang `g_idle_add`.
