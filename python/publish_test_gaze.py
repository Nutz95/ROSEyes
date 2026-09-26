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
    parser.add_argument("--count", type=int, default=40)
    parser.add_argument("--period-s", type=float, default=0.08)
    parser.add_argument(
        "--sweep",
        action="store_true",
        help="Animate left/right/center instead of a fixed gaze (visible motion).",
    )
    return parser.parse_args(argv)


def sample_gaze(args: argparse.Namespace, index: int) -> tuple[float, float]:
    """Returns (x, y) for this sample index."""
    if not args.sweep:
        return args.gaze_x, args.gaze_y
    phase = index % 12
    if phase < 4:
        return 0.8, 0.0
    if phase < 8:
        return -0.8, 0.0
    return 0.0, 0.0


def main(argv: list[str] | None = None) -> int:
    """Publishes N gaze samples and one blink mid-burst."""
    args = parse_args(argv or sys.argv[1:])
    rclpy.init()
    publisher = GazeRosPublisher()
    try:
        for index in range(max(args.count, 1)):
            gaze_x, gaze_y = sample_gaze(args, index)
            publisher.publish_gaze(gaze_x, gaze_y)
            if index == 5:
                publisher.publish_blink()
            rclpy.spin_once(publisher, timeout_sec=0.0)
            time.sleep(max(args.period_s, 0.01))
            print(f"published gaze #{index + 1} x={gaze_x:.2f} y={gaze_y:.2f}")
        print("done publishing; ESP idle resumes after gaze timeout (~2.5s)")
    finally:
        publisher.destroy_node()
        rclpy.shutdown()
    print("done")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
