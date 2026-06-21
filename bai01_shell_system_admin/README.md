# Bai 01 - Shell System Admin

## Muc tieu

Xay dung chuong trinh GTK ho tro quan tri he thong co ban tren Ubuntu: quan ly file/thu muc, tim kiem file, xem thong tin file, quan ly cron job, xem/doi thoi gian he thong va cai/go package bang apt.

## Chuc nang chinh

- Chon thu muc va liet ke file.
- Tao, xoa, doi ten, copy, move file hoac thu muc.
- Tim file theo ten hoac phan mo rong.
- Xem ten, kich thuoc, quyen, thoi gian sua doi.
- Them, xem, xoa cron job.
- Xem thoi gian he thong.
- Doi thoi gian he thong neu chay voi quyen sudo.
- Cai dat va go bo chuong trinh bang apt.

## Cau truc thu muc

```text
bai01_shell_system_admin/
├── src/main.c
├── scripts/system_ops.sh
├── screenshots/
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

Neu muon doi thoi gian he thong hoac cai/go package, chay app bang sudo:

```bash
sudo ./bin/shell_admin_gtk
```

## Demo

1. Bam **Browse** de chon thu muc, sau do **Refresh** de liet ke file.
2. Nhap ten file/thu muc va dung cac nut Create, Delete, Rename, Copy, Move.
3. Nhap tu khoa tim kiem, vi du `.c` hoac `main`, bam Search.
4. Chon file trong danh sach de xem thong tin.
5. Tao cron job bang bieu thuc cron va lenh, vi du `*/5 * * * *` va `date >> /tmp/demo_cron.log`.
6. Dung tab system de xem thoi gian, cai package nho nhu `htop`, hoac go package da cai.
