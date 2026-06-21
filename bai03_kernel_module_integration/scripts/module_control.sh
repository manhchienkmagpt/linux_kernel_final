#!/usr/bin/env bash
set -u

cmd="${1:-}"
module_name="access_monitor"
script_path="$(readlink -f "$0")"
project_dir="$(cd "$(dirname "$script_path")/.." && pwd)"
ko_path="$project_dir/src/access_monitor.ko"
protected_path="${2:-/tmp/protected}"

need_root() {
  if [[ "$(id -u)" -ne 0 ]]; then
    if command -v sudo >/dev/null 2>&1; then
      exec sudo bash "$script_path" "$@"
    fi
    echo "This action requires root. Run the app with sudo." >&2
    exit 1
  fi
}

case "$cmd" in
  load)
    need_root "$cmd" "$protected_path"
    if lsmod | grep -q "^${module_name}"; then
      echo "$module_name is already loaded."
      exit 0
    fi
    mkdir -p "$protected_path"
    insmod "$ko_path" protected_path="$protected_path"
    echo "Loaded $ko_path protected_path=$protected_path"
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
      [[ -e /dev/access_monitor ]] && ls -l /dev/access_monitor
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
