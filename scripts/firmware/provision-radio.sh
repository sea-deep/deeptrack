#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage: $0 ROVER_STA_MAC GATEWAY_STA_MAC CHANNEL"
  echo "Example: $0 24:6F:28:AA:BB:CC 24:6F:28:11:22:33 1"
}

if [[ $# -ne 3 ]]; then
  usage >&2
  exit 2
fi

rover_mac="$(printf '%s' "$1" | tr '[:lower:]' '[:upper:]')"
gateway_mac="$(printf '%s' "$2" | tr '[:lower:]' '[:upper:]')"
channel="$3"
mac_pattern='^([0-9A-F]{2}:){5}[0-9A-F]{2}$'

if [[ ! "$rover_mac" =~ $mac_pattern || ! "$gateway_mac" =~ $mac_pattern ]]; then
  echo "Both addresses must be measured six-byte STA MACs in AA:BB:CC:DD:EE:FF form." >&2
  exit 2
fi
if [[ "$rover_mac" == "$gateway_mac" ]]; then
  echo "Rover and gateway MAC addresses must differ." >&2
  exit 2
fi
if [[ ! "$channel" =~ ^([1-9]|1[0-4])$ ]]; then
  echo "Channel must be an integer from 1 through 14." >&2
  exit 2
fi
if ! command -v openssl >/dev/null 2>&1; then
  echo "openssl is required to generate independent random PMK/LMK keys." >&2
  exit 1
fi

script_dir="$(cd -- "$(dirname -- "$0")" && pwd)"
repo_root="$(cd -- "$script_dir/../.." && pwd)"
target="$repo_root/firmware/shared/DeeptrackRadioConfig.local.h"
if [[ -e "$target" ]]; then
  echo "Refusing to overwrite existing radio keys: $target" >&2
  echo "Move the file aside explicitly before rotating measured peers/keys." >&2
  exit 73
fi
temporary="$(mktemp "$repo_root/firmware/shared/.radio-config.XXXXXX")"
trap 'rm -f -- "$temporary"' EXIT

mac_initializer() {
  printf '%s' "$1" | sed 's/:/, 0x/g; s/^/0x/'
}

key_initializer() {
  openssl rand -hex 16 | sed 's/../0x&, /g; s/, $//'
}

rover_bytes="$(mac_initializer "$rover_mac")"
gateway_bytes="$(mac_initializer "$gateway_mac")"
pmk_bytes="$(key_initializer)"
lmk_bytes="$(key_initializer)"

{
  echo "#pragma once"
  echo
  echo "#include <stdint.h>"
  echo
  echo "namespace DeepTrack {"
  echo "namespace RadioConfig {"
  echo "constexpr bool ENABLED = true;"
  echo "constexpr uint8_t CHANNEL = $channel;"
  echo "constexpr uint8_t ROVER_MAC[6] = {$rover_bytes};"
  echo "constexpr uint8_t GATEWAY_MAC[6] = {$gateway_bytes};"
  echo "constexpr uint8_t PMK[16] = {$pmk_bytes};"
  echo "constexpr uint8_t LMK[16] = {$lmk_bytes};"
  echo "}  // namespace RadioConfig"
  echo "}  // namespace DeepTrack"
} > "$temporary"

chmod 600 "$temporary"
mv -- "$temporary" "$target"
trap - EXIT
echo "Wrote ignored radio configuration: $target"
echo "Recompile and upload both mission targets. Never commit this key file."
