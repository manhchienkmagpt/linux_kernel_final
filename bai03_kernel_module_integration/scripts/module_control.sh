#!/usr/bin/env bash
set -eu

cmd="${1:-}"
module_name="kfile_manager"
script_path="$(readlink -f "$0")"
project_dir="$(cd "$(dirname "$script_path")/.." && pwd)"
ko_path="$project_dir/src/kfile_manager.ko"

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
    need_root "$cmd"
    if lsmod | grep -q "^${module_name}"; then
      echo "$module_name is already loaded."
      exit 0
    fi
    if [[ ! -f "$ko_path" ]]; then
      echo "Module file not found: $ko_path" >&2
      echo "Build module first: make module" >&2
      exit 1
    fi
    mkdir -p /tmp/kfile_manager_root
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
      [[ -e /dev/kfile_manager ]] && ls -l /dev/kfile_manager
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
