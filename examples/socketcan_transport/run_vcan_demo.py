#!/usr/bin/env python3

import argparse
import subprocess
import sys
import time
from pathlib import Path


def run(command):
    print("+ " + " ".join(str(part) for part in command), flush=True)
    return subprocess.run(command, check=False, text=True)


def main():
    parser = argparse.ArgumentParser(description="Run the SocketCAN diagnostics vcan demo.")
    parser.add_argument("--interface", default="vcan0")
    parser.add_argument("--device", required=True)
    parser.add_argument("--tester", required=True)
    args = parser.parse_args()

    if subprocess.run(["ip", "link", "show", args.interface], stdout=subprocess.DEVNULL,
                      stderr=subprocess.DEVNULL).returncode != 0:
        print(f"socketcan demo: {args.interface} is not available; skipping", file=sys.stderr)
        return 0

    device = subprocess.Popen(
        [str(Path(args.device)), "--interface", args.interface, "--count", "4"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    try:
        time.sleep(0.2)
        tester = run([str(Path(args.tester)), "--interface", args.interface])
        device_output, _ = device.communicate(timeout=5.0)
    except subprocess.TimeoutExpired:
        device.kill()
        device_output, _ = device.communicate()
        print(device_output, end="")
        print("socketcan demo: device did not exit after request count", file=sys.stderr)
        return 1

    print(device_output, end="")
    if tester.returncode != 0:
        return tester.returncode
    return device.returncode


if __name__ == "__main__":
    raise SystemExit(main())
