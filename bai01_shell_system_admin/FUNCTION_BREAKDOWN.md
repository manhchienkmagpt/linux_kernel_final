# Phan ra chuc nang - Bai 01

## Module giao dien GTK

- Tao cua so chinh, notebook gom 3 tab: File Manager, Cron, System.
- Input: thao tac nguoi dung tren nut, entry, tree view.
- Output: bang danh sach file, text log, dialog bao loi/thanh cong.

## Module quan ly file

- `refresh_file_list`: doc noi dung thu muc hien tai bang `g_dir_open`, `g_dir_read_name`.
- `create_item`: tao file bang `g_file_set_contents` hoac thu muc bang `g_mkdir`.
- `delete_item`: xoa file bang `g_remove`, xoa thu muc rong bang `g_rmdir`.
- `rename_item`: doi ten bang `g_rename`.
- `copy_item`: copy file bang `g_file_copy`.
- `move_item`: move/rename bang `g_rename`, neu khac filesystem co the bao loi.
- `search_files`: duyet thu muc va loc theo chuoi nhap.
- Input: duong dan thu muc, ten file, ten moi, duong dan dich.
- Output: cap nhat danh sach file va log ket qua.

## Module thong tin file

- Lay thong tin bang `g_stat`.
- Hien thi ten, kich thuoc, permission dang octal, thoi gian sua doi.
- Input: file duoc chon.
- Output: text trong khung File Info.

## Module cron

- Goi `scripts/system_ops.sh cron-list` de xem cron hien co.
- Goi `scripts/system_ops.sh cron-add "<expr>" "<command>"` de them job.
- Goi `scripts/system_ops.sh cron-remove-line <index>` de xoa job theo dong.
- Input: cron expression, lenh shell, index job.
- Output: danh sach cron va log loi neu crontab khong hop le.

## Module system/apt/time

- `time-now`: lay thoi gian he thong bang `date`.
- `time-set`: doi thoi gian bang `date -s`, can sudo/root.
- `apt-install`, `apt-remove`: goi `apt-get`.
- Input: chuoi thoi gian hoac ten package.
- Output: log stdout/stderr cua lenh.

## Luong xu ly chinh

1. Khoi tao GTK va widget.
2. Nguoi dung thao tac tren tab.
3. Callback xu ly input va goi backend C/script.
4. Ket qua duoc day vao log va dialog.
5. Neu thao tac thay doi file/cron, giao dien refresh danh sach lien quan.
