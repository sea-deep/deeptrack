#!/usr/bin/env bash

# Shared target resolver. This file is sourced by the compile/upload scripts.

deeptrack_target_sketch() {
  case "${1:-}" in
    rover-mission|rover-phase2) printf '%s\n' 'rover/DeeptrackRover' ;;
    rover-diagnostics) printf '%s\n' 'rover/RoverDiagnostics' ;;
    motor-test) printf '%s\n' 'rover/TB6612Test' ;;
    gateway-mission) printf '%s\n' 'gateway/DeeptrackGateway' ;;
    gateway-diagnostics) printf '%s\n' 'gateway' ;;
    *) return 1 ;;
  esac
}

deeptrack_print_targets() {
  printf '%s\n' \
    'rover-mission        Integrated production-track rover firmware' \
    'rover-phase2         Compatibility alias for rover-mission' \
    'rover-diagnostics    Existing full rover bench diagnostic' \
    'motor-test           Existing basic TB6612 wiring test' \
    'gateway-mission      Production gateway radio/watchdog/NDJSON bridge' \
    'gateway-diagnostics  Existing gateway LCD/LED bench test'
}
