#!/usr/bin/env python3

import argparse
import os
import pty
import select
import subprocess
import time
from pathlib import Path


def bridge_until_done(device, tester, master_a, master_b, timeout):
    deadline = time.monotonic() + timeout

    os.set_blocking(master_a, False)
    os.set_blocking(master_b, False)

    while time.monotonic() < deadline:
        if tester.poll() is not None and device.poll() is not None:
            return True

        readable, _, _ = select.select([master_a, master_b], [], [], 0.05)
        for fd in readable:
            try:
                data = os.read(fd, 4096)
            except BlockingIOError:
                continue
            if not data:
                continue
            os.write(master_b if fd == master_a else master_a, data)

    return False


def main():
    parser = argparse.ArgumentParser(description="Run the serial diagnostics PTY demo.")
    parser.add_argument("--device", required=True)
    parser.add_argument("--tester", required=True)
    args = parser.parse_args()

    master_device, slave_device = pty.openpty()
    master_tester, slave_tester = pty.openpty()
    device_port = os.ttyname(slave_device)
    tester_port = os.ttyname(slave_tester)

    device = subprocess.Popen(
        [str(Path(args.device)), "--port", device_port, "--count", "4"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    try:
        time.sleep(0.2)
        tester = subprocess.Popen(
            [str(Path(args.tester)), "--port", tester_port],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        if not bridge_until_done(device, tester, master_device, master_tester, 5.0):
            device.kill()
            tester.kill()
            print("serial demo: timed out while bridging PTYs")
            return 1

        tester_output, _ = tester.communicate(timeout=1.0)
        device_output, _ = device.communicate(timeout=1.0)
    finally:
        for fd in (master_device, slave_device, master_tester, slave_tester):
            os.close(fd)

    print("+ " + str(Path(args.tester)) + " --port " + tester_port, flush=True)
    print(tester_output, end="")
    print(device_output, end="")

    if tester.returncode != 0:
        return tester.returncode
    return device.returncode


if __name__ == "__main__":
    raise SystemExit(main())
