# Phan ra chuc nang - Bai 03

## Kernel Module `access_monitor`

- File: `src/access_monitor.c`
- Module name: `access_monitor`
- Device config: `/dev/access_monitor`
- Protected path mac dinh: `/tmp/protected`
- Module parameter:

```bash
sudo insmod src/access_monitor.ko protected_path=/tmp/protected
```

## Module init / exit

- `access_monitor_init`:
  - Tao character device `/dev/access_monitor`.
  - Dang ky kprobe cho `vfs_write`.
  - Dang ky kprobe cho `vfs_unlink`.
  - Log `access_monitor loaded`.

- `access_monitor_exit`:
  - Huy kprobe.
  - Huy device/class/cdev.
  - Log `access_monitor unloaded`.

## Kprobe Monitor

- WRITE:
  - Kprobe gan vao `vfs_write`.
  - Lay `struct file *`.
  - Dung `d_path` de lay path.
  - Neu path nam trong protected path thi log event.

- DELETE:
  - Kprobe gan vao `vfs_unlink`.
  - Lay `struct dentry *`.
  - Dung `dentry_path_raw` de lay path.
  - Neu path nam trong protected path thi log event.

## Protected Path

- Luu trong buffer gioi han `PATH_MAX_LEN`.
- Co spinlock bao ve khi doc/ghi de phu hop ngu canh kprobe.
- Co the cau hinh luc load module bang module parameter.
- Co the doi khi dang chay bang ghi vao `/dev/access_monitor`.

## Event Logging

- Log format:

```text
[access_monitor] PID=... COMM=... ACTION=WRITE PATH=...
[access_monitor] PID=... COMM=... ACTION=DELETE PATH=...
```

- `PID`: process id.
- `COMM`: ten process.
- `ACTION`: WRITE hoac DELETE.
- `PATH`: duong dan file neu lay duoc.

## GUI Module Control

- Build Module: chay `make module`.
- Load Module: chay `scripts/module_control.sh load`.
- Unload Module: chay `scripts/module_control.sh unload`.
- Check Status: chay `scripts/module_control.sh status`.
- Clean Build: chay `make clean`.
- Lenh sudo se hien dialog nhap mat khau neu can.

## GUI Monitor Config

- Nhap protected path.
- Set Protected Path: ghi path vao `/dev/access_monitor`.
- Read Current Path: doc path hien tai tu `/dev/access_monitor`.
- Create Test File: tao `/tmp/protected/test.txt`.
- Write Test File: ghi data vao file test de tao event WRITE.
- Delete Test File: xoa file test de tao event DELETE.

## GUI Event Log

- Refresh dmesg: doc log kernel.
- Filter access_monitor: chi hien event cua module.
- Search: loc theo tu khoa.
- Clear View: xoa man hinh log.
- Hien theo cot:
  - Time
  - PID
  - Process
  - Action
  - Path
