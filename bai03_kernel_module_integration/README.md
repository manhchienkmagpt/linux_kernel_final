# Bai 03 - Kernel Module Integration

## Muc tieu

Xay dung Linux Kernel Module dang character device va ung dung GTK user-space de load/unload module, doc/ghi device `/dev/simple_kmod`, xem log kernel va kiem tra trang thai module.

## Chuc nang chinh

- Kernel module co `init`, `exit`, `open`, `read`, `write`, `release`.
- Khi load module, tu dong tao class/device `/dev/simple_kmod`.
- Ghi log bang `printk`.
- App GTK co nut:
  - Load module bang `insmod`.
  - Unload module bang `rmmod`.
  - Check module bang `lsmod`.
  - Xem log lien quan bang `dmesg`.
  - Ghi du lieu vao `/dev/simple_kmod`.
  - Doc du lieu tu `/dev/simple_kmod`.
- Script setup quyen device bang `chmod`.

## Cau truc thu muc

```text
bai03_kernel_module_integration/
├── src/
│   ├── Makefile
│   ├── simple_kmod.c
│   └── main.c
├── scripts/
│   ├── module_control.sh
│   └── setup_device_permission.sh
├── docs/
├── Makefile
├── README.md
├── FUNCTION_BREAKDOWN.md
└── CODE_EXPLANATION.md
```

## Cai thu vien

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-3-dev linux-headers-$(uname -r) kmod
```

## Build

```bash
make
```

Lenh nay build ca GTK app va kernel module `src/simple_kmod.ko`.

## Chay

```bash
make run
```

Load/unload module can quyen root. Trong app co the goi script bang `pkexec` neu he thong co policykit, hoac chay app bang sudo:

```bash
sudo ./bin/kernel_module_gtk
```

## Demo

1. Build bang `make`.
2. Mo app bang `sudo ./bin/kernel_module_gtk`.
3. Bam **Load Module**.
4. Bam **Check Status** de thay `simple_kmod` da load.
5. Nhap text va bam **Write Device**.
6. Bam **Read Device** de doc lai du lieu.
7. Bam **Kernel Logs** de xem log `dmesg`.
8. Bam **Unload Module** sau khi demo xong.

## Kiem tra thu cong

```bash
sudo insmod src/simple_kmod.ko
lsmod | grep simple_kmod
ls -l /dev/simple_kmod
echo "hello" | sudo tee /dev/simple_kmod
sudo cat /dev/simple_kmod
sudo dmesg | tail -30
sudo rmmod simple_kmod
```
