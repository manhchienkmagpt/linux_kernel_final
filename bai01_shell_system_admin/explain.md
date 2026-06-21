# Explain - Bai 01: Linux System Admin Tool

## 1. Muc dich project

Project nay la mot ung dung desktop viet bang C va GTK4, dung de demo cac thao tac quan tri he thong Linux co ban tren Ubuntu. Thay vi bat nguoi dung go tung lenh trong terminal, chuong trinh tao giao dien gom nhieu trang: quan ly file, cron, thoi gian he thong, package apt va log.

Noi dung quan trong cua bai nay khong chi la giao dien GTK, ma la cach ket noi giao dien voi cac lenh Linux that. Nguoi dung bam nut tren UI, callback trong C xu ly input, sau do goi GLib, system call hoac shell script de thuc hien thao tac tren he thong.

## 2. Cau truc thu muc

```text
bai01_shell_system_admin/
|-- Makefile
|-- README.md
|-- CODE_EXPLANATION.md
|-- FUNCTION_BREAKDOWN.md
|-- explain.md
|-- scripts/
|   `-- system_ops.sh
`-- src/
    |-- main.c
    |-- ui_main_window.c
    |-- ui_main_window.h
    |-- ui_file_page.c
    |-- ui_file_page.h
    |-- ui_cron_page.c
    |-- ui_cron_page.h
    |-- ui_time_page.c
    |-- ui_time_page.h
    |-- ui_apt_page.c
    |-- ui_apt_page.h
    |-- ui_log_page.c
    |-- ui_log_page.h
    |-- system_commands.c
    `-- system_commands.h
```

Y nghia tung nhom file:

- `Makefile`: bien dich toan bo file C thanh chuong trinh chay duoc.
- `src/main.c`: diem bat dau cua chuong trinh GTK.
- `src/ui_main_window.c/.h`: tao cua so chinh, header, sidebar va vung noi dung.
- `src/ui_*_page.c/.h`: moi cap file tao mot trang chuc nang rieng.
- `src/system_commands.c/.h`: lop trung gian de goi script shell va lay output.
- `scripts/system_ops.sh`: tap hop cac lenh Linux that nhu `crontab`, `date`, `apt-get`, `dpkg`.
- `README.md`, `CODE_EXPLANATION.md`, `FUNCTION_BREAKDOWN.md`: tai lieu mo ta va phan ra chuc nang.

## 3. Luong khoi dong chuong trinh

Luong chay tong quat:

```text
main.c
  -> tao GtkApplication
  -> gan signal "activate"
  -> khi app duoc activate thi goi ui_main_window_new()
  -> ui_main_window_new() tao window, sidebar, stack, cac page
  -> gtk_window_present() hien cua so
```

Trong `main.c`, ham `gtk_application_new()` tao doi tuong ung dung GTK. Sau do `g_signal_connect()` noi signal `activate` voi callback `on_activate`. Khi chuong trinh bat dau vong lap GTK bang `g_application_run()`, GTK phat signal `activate`, callback se tao cua so chinh.

Sau khi cua so duoc tao, `gtk_window_present()` dua cua so len man hinh. Cuoi chuong trinh, `g_object_unref(app)` giam tham chieu den doi tuong application de giai phong tai nguyen.

## 4. Giao dien chinh trong `ui_main_window.c`

File `ui_main_window.c` tao bo khung cho toan bo app.

Cac thanh phan chinh:

- `GtkApplicationWindow`: cua so desktop chinh.
- `GtkHeaderBar`: thanh tieu de phia tren.
- `GtkBox` ngang: chia man hinh thanh cot trai va cot phai.
- Sidebar ben trai: gom cac nut `File Manager`, `Cron Scheduler`, `Time System`, `Apt Manager`, `Log`.
- `GtkStack` ben phai: vung hien thi page hien tai.

Moi page duoc them vao `GtkStack` bang ten noi bo:

```text
file_page
cron_page
time_page
apt_page
log_page
```

Khi nguoi dung bam mot nut sidebar, callback `on_sidebar_clicked()` duoc goi. Nut da duoc gan san hai du lieu:

- con tro den `GtkStack`
- ten page can hien

Callback lay hai du lieu nay bang `g_object_get_data()` roi goi:

```c
gtk_stack_set_visible_child_name(s, name);
```

Nghia la UI khong can tao lai page moi lan bam nut. Tat ca page da duoc tao san, `GtkStack` chi doi page nao dang hien.

## 5. Log Page va cach cac page ghi log

`ui_log_page.c` tao mot trang log dung chung. Cac page khac nhan con tro `LogPage *log_page`, nen khi co thao tac thanh cong hoac that bai, page co the goi:

```c
append_log(log_page, "INFO", "message");
append_log(log_page, "OK", "message");
append_log(log_page, "ERROR", "message");
```

Log duoc them vao `GtkTextView`, format dang:

```text
[INFO] Application started.
[OK] Created file demo.txt
[ERROR] Delete failed
```

Cach thiet ke nay giup log tap trung o mot noi. File Manager, Cron, Time va Apt khong can tu tao TextView rieng de ghi lich su.

Nut `Clear Log` xoa buffer cua TextView. Nut `Save Log` lay toan bo noi dung buffer va ghi ra file `system_admin_log.txt`.

## 6. Backend goi lenh he thong trong `system_commands.c`

`system_commands.c` la cau noi giua code C va script shell.

Ham quan trong nhat la:

```c
gboolean run_system_script(const char *args, char **output)
```

Ham nay tao command theo mau:

```text
bash scripts/system_ops.sh <args>
```

Sau do chay command bang:

```c
g_spawn_command_line_sync()
```

Day la ham cua GLib dung de chay mot lenh shell dong bo. "Dong bo" co nghia la UI se doi lenh chay xong moi nhan ket qua. Ham lay duoc:

- `stdout_text`: output binh thuong.
- `stderr_text`: output loi.
- `status`: ma thoat cua lenh.
- `error`: loi neu GLib khong chay duoc command.

Sau khi chay xong, ham ghep command va output vao `GString`, tra ve qua bien `output`. Gia tri return la `TRUE` neu command chay duoc va `status == 0`.

Ham:

```c
char *quote_arg(const char *value)
```

dung `g_shell_quote()` de boc input nguoi dung bang dau quote an toan hon. Vi du package name, cron command hoac thoi gian co dau cach thi khong lam hong command shell.

Voi thao tac can quyen cao, code dung:

```c
run_system_script_sudo(GTK_WINDOW(parent), args, &output)
```

Ham nay truoc het kiem tra app co dang chay bang root hay khong. Neu khong phai root, no thu `sudo -n true` de xem sudo da co phien xac thuc hop le chua. Neu sudo yeu cau mat khau, app mo dialog GTK de user nhap mat khau. Mat khau khong duoc dua vao command line; no duoc gui vao stdin cua `sudo -S`.

## 7. Script `scripts/system_ops.sh`

Script nay nhan tham so dau tien la ten lenh, sau do dung `case` de chon thao tac.

### Cron

- `cron-list`: goi `crontab -l`, neu chua co crontab thi khong coi la loi.
- `cron-add`: tao file tam bang `mktemp`, ghi crontab cu vao file, them dong moi, sau do cai lai crontab bang `crontab "$tmp"`.
- `cron-remove-line`: dung `awk` de bo qua dong can xoa theo so thu tu.

### Time

- `time-now`: goi `date`.
- `time-set`: kiem tra quyen root bang `id -u`. Neu khong phai root thi bao loi. Neu du quyen thi goi `date -s`.

### Apt

- `apt-install`: goi `apt-get install -y`.
- `apt-remove`: goi `apt-get remove -y`.
- `apt-check`: goi `dpkg -s` de kiem tra package da cai hay chua.

Script tach rieng giup code C gon hon. Neu muon doi lenh Linux, minh co the sua script ma khong can sua nhieu code giao dien.

## 8. File Manager Page

Trang File Manager nam trong `ui_file_page.c/.h`. Trang nay giong mot file explorer don gian.

Du lieu chinh cua page:

- thu muc hien tai
- tu khoa tim kiem
- file/directory dang duoc chon
- bang danh sach file
- con tro den Log Page

Khi bam `Choose Folder`, UI mo `GtkFileChooserNative` de chon thu muc. Sau khi co path, page goi ham refresh danh sach file.

Ham refresh thuong lam cac viec:

1. Xoa cac row cu trong `GtkListBox`.
2. Mo thu muc bang `g_dir_open()`.
3. Doc tung ten file bang `g_dir_read_name()`.
4. Ghep path day du.
5. Lay thong tin file bang `g_stat()`.
6. Tao row moi gom ten, loai, kich thuoc, quyen, thoi gian sua doi.
7. Them row vao list.

Cac thao tac file:

- Tao file: dung `g_file_set_contents()` de tao file rong.
- Tao thu muc: dung `g_mkdir()`.
- Xoa file: dung `g_remove()`.
- Xoa thu muc: dung `g_rmdir()`.
- Doi ten: dung `g_rename()`.
- Copy/move: dung `GFile`, la API file cua GLib/GIO.
- Xem thong tin: dung `stat`/`g_stat` roi hien dialog.

Diem dang chu y: moi thao tac nguy hiem nhu xoa deu co dialog xac nhan, tranh nguoi dung bam nham.

## 9. Cron Scheduler Page

Trang Cron Scheduler cho nguoi dung tao va xoa cron job.

Cron expression gom 5 truong:

```text
minute hour day_of_month month day_of_week
```

Vi du:

```text
* * * * *     chay moi phut
0 8 * * *     chay moi ngay luc 08:00
0 8 * * 1     chay moi thu Hai luc 08:00
```

Page co cac preset. Khi chon preset, code chi dien san gia tri vao cac entry. Neu chon custom, nguoi dung tu sua cac truong.

Khi bam them cron:

1. Lay lenh nguoi dung muon chay.
2. Lay 5 truong cron.
3. Kiem tra cac entry khong rong.
4. Ghep thanh cron expression.
5. Quote expression va command.
6. Goi `run_system_script("cron-add ...")`.
7. Refresh danh sach cron va ghi log.

Khi xoa cron, UI lay row dang chon trong list. Vi script xoa theo so dong, page chuyen row index thanh line number roi goi `cron-remove-line`.

## 10. Time System Page

Trang Time System co hai viec:

- hien thoi gian hien tai
- doi thoi gian he thong

Refresh thoi gian hien tai dung `GDateTime` cua GLib. Ham `get_current_time_string()` format theo dang:

```text
YYYY-MM-DD HH:MM:SS ZONE
```

Khi doi thoi gian, UI lay input ngay va gio, ghep thanh:

```text
YYYY-MM-DD HH:MM:SS
```

Sau do mo dialog xac nhan. Neu nguoi dung dong y, page goi script `time-set` qua `sudo`.

Thao tac doi gio can quyen root. Neu sudo can mat khau, app hien cua so nhap mat khau roi truyen cho `sudo -S`.

## 11. Apt Manager Page

Trang Apt Manager thao tac voi package Ubuntu.

Input chinh la ten package. Truoc khi chay, code kiem tra input khong rong.

Cac nut chuc nang:

- Check Package: goi `apt-check`, dung `dpkg -s`.
- Install Package: xac nhan roi goi `apt-install` qua `sudo`.
- Remove Package: xac nhan roi goi `apt-remove` qua `sudo`.

Output cua lenh apt/dpkg duoc hien trong TextView cua page, dong thoi ghi vao Log Page. Cai dat va go bo package thuong can quyen root, nen app se hien dialog mat khau neu sudo can xac thuc.

## 12. Cach build va chay

Cai thu vien can thiet tren Ubuntu:

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
./system_admin_gui
```

Neu can doi gio he thong hoac cai/go package:

```bash
sudo ./system_admin_gui
```

## 13. Tom tat kien thuc can nam

- GTK4 dung signal/callback: nguoi dung bam nut thi callback duoc goi.
- `GtkStack` dung de chua nhieu page va chuyen page bang ten.
- GLib co nhieu ham tien ich: `GString`, `GDateTime`, `g_spawn_command_line_sync`, `g_shell_quote`.
- File Manager ket hop GLib va Linux syscall.
- Cron, apt, date duoc dua ra script shell de de quan ly.
- Log Page la diem tap trung thong tin de nguoi dung theo doi thao tac.
