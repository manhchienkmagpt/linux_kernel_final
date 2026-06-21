#!/usr/bin/env bash
set -euo pipefail

device="/dev/simple_kmod"

if [[ ! -e "$device" ]]; then
  echo "$device does not exist. Load the module first." >&2
  exit 1
fi

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Run with sudo: sudo $0" >&2
  exit 1
fi

chmod 666 "$device"
echo "Permission updated: $(ls -l "$device")"
