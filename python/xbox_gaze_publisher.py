#!/usr/bin/env python3
"""Publish Xbox left-stick position to /eyes/gaze for ROSEyes."""

from __future__ import annotations

import argparse
import sys
import time

import rclpy

from xbox_gaze.gaze_publisher import GazeRosPublisher
from xbox_gaze.quit_key import quit_requested
from xbox_gaze.stick_reader import XboxGazeStickReader


def parse_args(argv: list[str]) -> argparse.Namespace:
    """Parses CLI arguments."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--rate-hz",
        type=float,
        default=15.0,
        help="Publish rate while the stick is deflected (default 15)",
    )
    parser.add_argument("--deadzone", type=float, default=0.12, help="Stick deadzone")
    parser.add_argument(
        "--invert-y",
        action="store_true",
        default=False,
        help="Invert vertical axis (panel mount already flips Y in firmware)",
    )
    parser.add_argument(
        "--no-invert-y",
        action="store_false",
        dest="invert_y",
        help="Do not invert vertical axis (default)",
    )
    parser.add_argument(
        "--mode-period-s",
        type=float,
        default=1.0,
        help="Re-publish /eyes/mode piloted this often (ESP may miss a one-shot)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Entry point: Xbox stick -> ROS /eyes/gaze (+ keep mode=piloted)."""
    args = parse_args(argv or sys.argv[1:])
    period_s = 1.0 / max(args.rate_hz, 1.0)
    mode_period_s = max(args.mode_period_s, 0.2)

    reader = XboxGazeStickReader(deadzone=args.deadzone)
    reader.open()

    rclpy.init()
    publisher = GazeRosPublisher()
    print("Publishing Xbox left stick to /eyes/gaze")
    print("Keeping /eyes/mode=piloted (required — autonomous ignores host gaze)")
    print("A = blink. Release stick -> stop gaze pubs; ESP idle after timeout.")
    print(f"rate={args.rate_hz} Hz while active. Press q to return to menu.")

    next_mode_s = 0.0
    try:
        while rclpy.ok():
            if quit_requested():
                print("Back to menu.")
                break

            now_s = time.monotonic()
            if now_s >= next_mode_s:
                publisher.publish_mode("piloted")
                next_mode_s = now_s + mode_period_s

            sample = reader.read()
            gaze_y = -sample.y if args.invert_y else sample.y
            active = abs(sample.x) > args.deadzone or abs(gaze_y) > args.deadzone

            if sample.blink_pressed:
                publisher.publish_blink()

            if active:
                publisher.publish_gaze(sample.x, gaze_y)

            rclpy.spin_once(publisher, timeout_sec=0.0)
            time.sleep(period_s)
    except KeyboardInterrupt:
        print("\nBack to menu.")
    finally:
        publisher.destroy_node()
        rclpy.shutdown()
        reader.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
