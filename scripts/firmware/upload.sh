#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/../.." && pwd)"
source "${script_dir}/targets.sh"

fqbn="${DEEPTRACK_FQBN:-esp32:esp32:esp32}"
target="${1:-}"
port="${2:-}"

if ! sketch_rel="$(deeptrack_target_sketch "${target}")" || [[ -z "${port}" ]]; then
  printf 'Usage: %s <target> </dev/ttyUSBx|/dev/ttyACMx>\n\nTargets:\n' "$0" >&2
  deeptrack_print_targets >&2
  exit 64
fi

case "${port}" in
  /dev/ttyUSB*|/dev/ttyACM*) ;;
  *)
    printf 'Refusing ambiguous port: %s\nUse an explicit /dev/ttyUSB* or /dev/ttyACM* device.\n' "${port}" >&2
    exit 64
    ;;
esac

if [[ ! -c "${port}" ]]; then
  printf 'Serial device does not exist: %s\nRun: arduino-cli board list\n' "${port}" >&2
  exit 66
fi

"${script_dir}/compile.sh" "${target}"

sketch_path="${repo_root}/${sketch_rel}"
build_path="/tmp/deeptrack-arduino/${target}"

printf 'Uploading %s to %s\n' "${target}" "${port}"
arduino-cli upload \
  --port "${port}" \
  --fqbn "${fqbn}" \
  --input-dir "${build_path}" \
  "${sketch_path}"
