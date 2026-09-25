#!/usr/bin/env python3
"""Publish a short burst of test gaze samples for ROSEyes."""

from __future__ import annotations

import argparse
import sys
import time

import rclpy

from xbox_gaze.gaze_publisher import GazeRosPublisher


def parse_args(argv: list[str]) -> argparse.Namespace:
    """Parses CLI arguments."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gaze-x", type=float, default=0.6)
    parser.add_argument("--gaze-y", type=float, default=-0.2)
    parser.add_argument("--count", type=int, default=20)
    parser.add_argument("--period-s", type=float, default=0.05)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Publishes N gaze samples and one blink mid-burst."""
    args = parse_args(argv or sys.argv[1:])
    rclpy.init()
    publisher = GazeRosPublisher()
    try:
        for index in range(max(args.count, 1)):
            publisher.publish_gaze(args.gaze_x, args.gaze_y)
            if index == 5:
                publisher.publish_blink()
            rclpy.spin_once(publisher, timeout_sec=0.0)
            time.sleep(max(args.period_s, 0.01))
            print(f"published gaze #{index + 1}")
    finally:
        publisher.destroy_node()
        rclpy.shutdown()
    print("done")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
