#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/../.." && pwd)"
front_test_binary="/tmp/deeptrack-front-safety-test"
runtime_test_binary="/tmp/deeptrack-runtime-safety-test"

g++ \
  -std=c++17 \
  -Wall \
  -Wextra \
  -Werror \
  -pedantic \
  "${repo_root}/firmware/tests/front_safety_test.cpp" \
  -o "${front_test_binary}"

"${front_test_binary}"

g++ \
  -std=c++17 \
  -Wall \
  -Wextra \
  -Werror \
  -pedantic \
  "${repo_root}/firmware/tests/runtime_safety_test.cpp" \
  -o "${runtime_test_binary}"

"${runtime_test_binary}"
printf 'PASS: front gate, signed encoder, and per-side stall safety logic\n'
