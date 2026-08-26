#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/../.." && pwd)"
source "${script_dir}/targets.sh"

fqbn="${DEEPTRACK_FQBN:-esp32:esp32:esp32}"
target="${1:-}"

if [[ "${target}" == "all" ]]; then
  for item in rover-mission rover-diagnostics motor-test gateway-mission gateway-diagnostics; do
    "${script_dir}/compile.sh" "${item}"
  done
  exit 0
fi

if ! sketch_rel="$(deeptrack_target_sketch "${target}")"; then
  printf 'Usage: %s <target|all>\n\nTargets:\n' "$0" >&2
  deeptrack_print_targets >&2
  exit 64
fi

sketch_path="${repo_root}/${sketch_rel}"
build_path="/tmp/deeptrack-arduino/${target}"
mkdir -p "${build_path}"

printf 'Compiling %s\nSketch: %s\nFQBN: %s\n' \
  "${target}" "${sketch_path}" "${fqbn}"

arduino-cli compile \
  --fqbn "${fqbn}" \
  --build-path "${build_path}" \
  "${sketch_path}"
