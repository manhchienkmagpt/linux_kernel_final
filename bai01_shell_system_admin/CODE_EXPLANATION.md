# Giai thich code - Bai 01

## `src/main.c`

File nay chua toan bo ung dung GTK. Chuong trinh dung GTK3 de tao cua so, `GtkNotebook` de chia tab va `GtkTreeView` de hien thi danh sach file.

### Ham quan trong

- `append_log`: them noi dung vao vung log/output.
- `show_message`: hien hop thoai bao loi hoac thanh cong.
- `run_script`: goi script shell bang `g_spawn_command_line_sync`, thu stdout/stderr va ma thoat.
- `refresh_file_list`: liet ke file trong thu muc hien tai.
- `show_file_info`: dung `g_stat` de lay kich thuoc, permission va thoi gian sua doi.
- `on_create_clicked`, `on_delete_clicked`, `on_rename_clicked`, `on_copy_clicked`, `on_move_clicked`: cac callback quan ly file.
- `on_cron_add_clicked`, `on_cron_list_clicked`, `on_cron_remove_clicked`: callback quan ly crontab thong qua shell script.
- `on_time_now_clicked`, `on_time_set_clicked`, `on_apt_install_clicked`, `on_apt_remove_clicked`: callback he thong.

## `scripts/system_ops.sh`

Script nay gom cac thao tac he thong can shell:

- `cron-list`: chay `crontab -l`.
- `cron-add`: lay crontab hien tai, them dong moi, nap lai bang `crontab`.
- `cron-remove-line`: xoa dong cron theo so thu tu.
- `time-now`: chay `date`.
- `time-set`: chay `date -s`, can root.
- `apt-install`: chay `apt-get install -y`.
- `apt-remove`: chay `apt-get remove -y`.

## System call va lenh Linux su dung

- Quan ly file trong app dung API GLib boc quanh syscall Linux nhu stat, rename, remove.
- Cron dung lenh `crontab`.
- Thoi gian dung lenh `date`.
- Package dung `apt-get`.

## GTK goi backend nhu the nao

Moi nut trong giao dien duoc gan voi mot callback. Callback doc gia tri tu `GtkEntry`, kiem tra input, sau do thuc hien lenh C hoac goi `scripts/system_ops.sh`. Ket qua duoc hien thi trong `GtkTextView` log va dialog.
