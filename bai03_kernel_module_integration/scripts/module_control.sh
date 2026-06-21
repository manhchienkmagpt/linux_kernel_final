#!/usr/bin/env bash
set -u

cmd="${1:-}"
module_name="simple_kmod"
ko_path="src/simple_kmod.ko"

need_root() {
  if [[ "$(id -u)" -ne 0 ]]; then
    if command -v pkexec >/dev/null 2>&1; then
      exec pkexec "$0" "$@"
    fi
    echo "This action requires root. Run the app with sudo or install pkexec." >&2
    exit 1
  fi
}

case "$cmd" in
  load)
    need_root "$cmd"
    if lsmod | grep -q "^${module_name}"; then
      echo "$module_name is already loaded."
      exit 0
    fi
    insmod "$ko_path"
    echo "Loaded $ko_path"
    ;;
  unload)
    need_root "$cmd"
    if ! lsmod | grep -q "^${module_name}"; then
      echo "$module_name is not loaded."
      exit 0
    fi
    rmmod "$module_name"
    echo "Unloaded $module_name"
    ;;
  status)
    if lsmod | grep -q "^${module_name}"; then
      lsmod | grep "^${module_name}"
      [[ -e /dev/simple_kmod ]] && ls -l /dev/simple_kmod
    else
      echo "$module_name is not loaded."
    fi
    ;;
  dmesg)
    dmesg | grep "$module_name" | tail -40
    ;;
  *)
    echo "Usage: $0 {load|unload|status|dmesg}" >&2
    exit 2
    ;;
esac
