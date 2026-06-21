# Giai thich code - Bai 01

## Tong quan

Chuong trinh duoc viet bang C va GTK4. Code tach theo page de de mo rong: moi page co file `.c/.h` rieng, backend lenh he thong nam trong `system_commands.c`.

## `main.c`

`main.c` tao `GtkApplication` voi application id `vn.edu.linux.system_admin_tool`. Khi app activate, ham `on_activate` tao cua so chinh bang `ui_main_window_new`.

## `ui_main_window.c`

File nay tao layout chinh:

- `GtkApplicationWindow`: cua so app.
- `GtkHeaderBar`: hien tieu de.
- `GtkBox` ngang: chia sidebar va noi dung.
- Sidebar la `GtkBox` doc gom cac nut.
- Noi dung la `GtkStack`, moi page duoc add bang ten: `file_page`, `cron_page`, `time_page`, `apt_page`, `log_page`.

Nut sidebar luu con tro `GtkStack` va ten page bang `g_object_set_data`. Khi click, callback goi `gtk_stack_set_visible_child_name`.

## `ui_file_page.c`

File Manager dung `GtkListBox` de tao bang nhieu cot. Moi row la mot `GtkListBoxRow` chua `GtkBox` ngang gom cac label: ten, loai, kich thuoc, quyen, thoi gian sua doi.

Ham quan trong:

- `refresh_file_list`: xoa row cu, doc thu muc hien tai va them row moi.
- `on_choose_folder`: mo dialog chon folder.
- `on_create`: mo dialog nhap ten file/thu muc.
- `on_delete`: hoi xac nhan truoc khi xoa.
- `on_rename`: mo dialog nhap ten moi.
- `choose_destination`: mo dialog chon thu muc dich cho copy/move.
- `on_info`: doc `stat` va hien thong tin file trong dialog.

Backend file dung GLib va syscall boc san cua Linux: `g_stat`, `g_mkdir`, `g_remove`, `g_rmdir`, `g_rename`, `GFile`.

## `ui_cron_page.c`

Cron page co entry lenh va 5 entry cron. Preset chi dien san cac entry. Khi them cron, code kiem tra khong de trong, ghep 5 field thanh cron expression va goi:

```bash
bash scripts/system_ops.sh cron-add '<expr>' '<command>'
```

Danh sach cron lay bang `cron-list`, sau do tach tung dong va dua vao `GtkListBox`. Xoa cron dung index cua row dang chon va goi `cron-remove-line`.

## `ui_time_page.c`

Refresh hien thoi gian local bang `GDateTime`. Doi thoi gian ghep ngay va gio thanh chuoi `YYYY-MM-DD HH:MM:SS`, hoi xac nhan, sau do goi script `time-set`. Lenh nay can sudo/root.

## `ui_apt_page.c`

Page nay co entry package, 3 nut va TextView output. Truoc khi chay lenh, code kiem tra package khong rong. Cai/go package co dialog xac nhan. Output cua apt/dpkg vua hien trong TextView vua ghi vao Log Page.

## `ui_log_page.c`

Log Page giu `GtkTextView` dung chung. Ham `append_log` them dong log theo format:

```text
[INFO] ...
[OK] ...
[ERROR] ...
```

Nut Save Log lay toan bo buffer va ghi ra `system_admin_log.txt`.

## `system_commands.c`

`run_system_script` dung `g_spawn_command_line_sync` de chay script shell mot cach dong bo, lay stdout/stderr va status. `quote_arg` dung `g_shell_quote` de han che loi khi input co dau cach hoac ky tu dac biet.

## `scripts/system_ops.sh`

Script gom cac lenh he thong:

- `cron-list`, `cron-add`, `cron-remove-line`
- `time-now`, `time-set`
- `apt-install`, `apt-remove`, `apt-check`

Trong tuong lai co the thay backend script bang API rieng ma khong can sua nhieu code giao dien.
