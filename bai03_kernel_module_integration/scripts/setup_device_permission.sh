#!/usr/bin/env bash
set -euo pipefail

device="/dev/kfile_manager"

if [[ ! -e "$device" ]]; then
  echo "$device does not exist. Load the module first." >&2
  exit 1
fi

chmod 666 "$device"
echo "Permission updated: $(ls -l "$device")"
