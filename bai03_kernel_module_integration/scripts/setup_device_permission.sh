#!/usr/bin/env bash
set -euo pipefail

device="/proc/mouse_monitor"

if [[ ! -e "$device" ]]; then
  echo "$device does not exist. Load the module first." >&2
  exit 1
fi

echo "$device is a read-only proc status file."
echo "No permission update is required. Test with: cat $device"
