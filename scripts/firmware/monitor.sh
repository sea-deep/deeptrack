#!/usr/bin/env bash
set -euo pipefail

port="${1:-}"
if [[ -z "${port}" ]]; then
  printf 'Usage: %s </dev/ttyUSBx|/dev/ttyACMx>\n' "$0" >&2
  exit 64
fi

case "${port}" in
  /dev/ttyUSB*|/dev/ttyACM*) ;;
  *)
    printf 'Refusing ambiguous port: %s\n' "${port}" >&2
    exit 64
    ;;
esac

if [[ ! -c "${port}" ]]; then
  printf 'Serial device does not exist: %s\nRun: arduino-cli board list\n' "${port}" >&2
  exit 66
fi

arduino-cli monitor --port "${port}" --config baudrate=115200
