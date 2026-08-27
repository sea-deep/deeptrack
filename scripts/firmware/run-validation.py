#!/usr/bin/env python3
"""Run one bounded rover validation command with an automatic safety lease."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
import time

import serial


PORT_PATTERN = re.compile(r"^/dev/tty(?:ACM|USB)\d+$")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run one DEEPTRACK physical-validation command."
    )
    parser.add_argument("port", help="explicit rover port, e.g. /dev/ttyACM1")
    parser.add_argument("command", help='quoted command, e.g. "test noise 10"')
    parser.add_argument(
        "--log", type=pathlib.Path, help="also save the complete serial transcript"
    )
    args = parser.parse_args()

    if not PORT_PATTERN.fullmatch(args.port):
        parser.error("port must be an explicit /dev/ttyACM# or /dev/ttyUSB#")
    command = " ".join(args.command.strip().lower().split())
    if not command.startswith("test ") or command in {
        "test heartbeat",
        "test status",
    }:
        parser.error("command must start a physical test (noise, side, straight, turn)")

    log_file = args.log.open("w", encoding="utf-8") if args.log else None
    completed = False
    started = time.monotonic()
    next_heartbeat = started
    try:
        with serial.Serial(args.port, 115200, timeout=0.1, write_timeout=0.5) as rover:
            rover.dtr = False
            rover.rts = False
            time.sleep(1.0)
            rover.reset_input_buffer()
            rover.write((command + "\n").encode())
            rover.flush()
            while time.monotonic() - started < 80.0:
                now = time.monotonic()
                if now >= next_heartbeat:
                    rover.write(b"test heartbeat\n")
                    rover.flush()
                    next_heartbeat = now + 0.2
                line = rover.readline()
                if not line:
                    continue
                text = line.decode("utf-8", "backslashreplace").rstrip("\r\n")
                print(text, flush=True)
                if log_file:
                    log_file.write(text + "\n")
                    log_file.flush()
                if text.startswith("VALIDATION_RESULT "):
                    completed = True
                if text.startswith("REJECTED "):
                    print("ERROR firmware rejected the validation command", file=sys.stderr)
                    return 2
                if completed and text.startswith("VALIDATION_IDLE "):
                    return 0
            print("ERROR validation transcript timed out; sending stop", file=sys.stderr)
            return 1
    except serial.SerialException as error:
        print(f"ERROR serial link failed: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("CANCELLED by operator; sending stop", file=sys.stderr)
        return 130
    finally:
        if not completed:
            try:
                with serial.Serial(args.port, 115200, timeout=0.2) as rover:
                    rover.write(b"stop\n")
                    rover.flush()
            except serial.SerialException:
                # Firmware's 750 ms lease is the independent fallback.
                pass
        if log_file:
            log_file.close()


if __name__ == "__main__":
    raise SystemExit(main())
