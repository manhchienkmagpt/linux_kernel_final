#!/usr/bin/env bash
set -u

cmd="${1:-}"

case "$cmd" in
  cron-list)
    crontab -l 2>/dev/null || true
    ;;
  cron-add)
    expr="${2:-}"
    job="${3:-}"
    if [[ -z "$expr" || -z "$job" ]]; then
      echo "Usage: cron-add '<cron expr>' '<command>'" >&2
      exit 2
    fi
    tmp="$(mktemp)"
    crontab -l 2>/dev/null > "$tmp" || true
    printf '%s %s\n' "$expr" "$job" >> "$tmp"
    crontab "$tmp"
    rm -f "$tmp"
    echo "Added cron job: $expr $job"
    ;;
  cron-remove-line)
    line="${2:-}"
    if ! [[ "$line" =~ ^[0-9]+$ ]] || [[ "$line" -lt 1 ]]; then
      echo "Line number must be >= 1" >&2
      exit 2
    fi
    tmp="$(mktemp)"
    crontab -l 2>/dev/null | awk -v n="$line" 'NR != n { print }' > "$tmp" || true
    crontab "$tmp"
    rm -f "$tmp"
    echo "Removed cron line $line"
    ;;
  time-now)
    date '+%Y-%m-%d %H:%M:%S %Z'
    ;;
  time-set)
    new_time="${2:-}"
    if [[ -z "$new_time" ]]; then
      echo "Usage: time-set 'YYYY-MM-DD HH:MM:SS'" >&2
      exit 2
    fi
    if [[ "$(id -u)" -ne 0 ]]; then
      echo "Changing system time requires sudo/root." >&2
      exit 1
    fi
    date -s "$new_time"
    ;;
  apt-install)
    pkg="${2:-}"
    [[ -n "$pkg" ]] || { echo "Package name is required" >&2; exit 2; }
    apt-get install -y "$pkg"
    ;;
  apt-remove)
    pkg="${2:-}"
    [[ -n "$pkg" ]] || { echo "Package name is required" >&2; exit 2; }
    apt-get remove -y "$pkg"
    ;;
  *)
    echo "Unknown command: $cmd" >&2
    exit 2
    ;;
esac
