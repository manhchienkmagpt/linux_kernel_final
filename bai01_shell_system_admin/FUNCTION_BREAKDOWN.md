# Phan ra chuc nang - Bai 01

## `main.c`

- Tao `GtkApplication`.
- Xu ly signal `activate`.
- Goi `ui_main_window_new()` de tao cua so chinh.

## `ui_main_window.c/.h`

- Tao `GtkApplicationWindow`.
- Tao HeaderBar voi tieu de "Linux System Admin Tool".
- Tao layout ngang: sidebar trai va `GtkStack` phai.
- Sidebar co 5 nut: File Manager, Cron Scheduler, Time System, Apt Manager, Log.
- Khi bam nut sidebar, doi page hien thi trong `GtkStack`.

## `ui_file_page.c/.h`

- Input: thu muc hien tai, tu khoa tim kiem, file dang chon, ten moi hoac thu muc dich.
- Output: bang file va log.
- Chuc nang:
  - Chon thu muc bang `GtkFileChooserNative`.
  - Liet ke file bang `g_dir_open`, `g_dir_read_name`, `g_stat`.
  - Tim file theo chuoi trong ten.
  - Tao file bang `g_file_set_contents`, tao thu muc bang `g_mkdir`.
  - Xoa bang `g_remove` hoac `g_rmdir`.
  - Doi ten bang `g_rename`.
  - Copy/move bang `GFile`.
  - Xem thong tin file bang dialog.

## `ui_cron_page.c/.h`

- Input: lenh can chay va 5 truong cron: phut, gio, ngay, thang, thu.
- Output: danh sach cron job va log.
- Preset:
  - Moi phut: `* * * * *`
  - Moi ngay luc 8h: `0 8 * * *`
  - Moi tuan: `0 8 * * 1`
  - Custom: nguoi dung tu sua.
- Them/xem/xoa cron thong qua `system_commands.c` va `scripts/system_ops.sh`.

## `ui_time_page.c/.h`

- Input: ngay `YYYY-MM-DD`, gio `HH:MM:SS`.
- Output: label thoi gian hien tai va log.
- Refresh lay thoi gian local bang GLib.
- Doi thoi gian goi script `time-set`, co dialog xac nhan va can sudo.

## `ui_apt_page.c/.h`

- Input: ten package.
- Output: TextView apt/dpkg output va Log Page.
- Kiem tra package bang `apt-check`.
- Cai dat bang `apt-install`.
- Go bo bang `apt-remove`.
- Cai/go package co dialog xac nhan.

## `ui_log_page.c/.h`

- Cung cap `append_log(LogPage*, level, message)` cho cac page khac.
- Log format: `[INFO] ...`, `[OK] ...`, `[ERROR] ...`.
- Clear Log xoa TextView.
- Save Log luu ra `system_admin_log.txt`.

## `system_commands.c/.h`

- `run_system_script`: goi `bash scripts/system_ops.sh ...` bang `g_spawn_command_line_sync`.
- `quote_arg`: escape input nguoi dung bang `g_shell_quote`.
- `get_current_time_string`: lay thoi gian hien tai.

## Luong xu ly chinh

1. App khoi dong va tao main window.
2. User chon page tu sidebar.
3. User bam nut tren page.
4. Event handler kiem tra input, mo dialog neu can.
5. Backend GLib/script thuc hien thao tac.
6. Ket qua hien tren page lien quan va ghi vao Log Page.
