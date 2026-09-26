#!/usr/bin/env python3
"""Echo a ROSEyes topic until q/Esc (returns cleanly to a host menu)."""

from __future__ import annotations

import argparse
import sys

import rclpy
from geometry_msgs.msg import Vector3
from rclpy.node import Node
from std_msgs.msg import String

from xbox_gaze.quit_key import quit_requested


def parse_args(argv: list[str]) -> argparse.Namespace:
    """Parses CLI arguments."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "topic",
        choices=("status", "ball", "gaze", "perf"),
        help="Short name under /eyes/",
    )
    return parser.parse_args(argv)


class TopicEchoNode(Node):
    """rclpy Node holding the subscription (required — callbacks need a Node)."""

    def __init__(self, topic_short: str) -> None:
        """Subscribes to /eyes/<topic_short>."""
        super().__init__(f"roseyes_echo_{topic_short}")
        full = f"eyes/{topic_short}"
        if topic_short == "gaze":
            self.create_subscription(Vector3, full, self._on_gaze, 10)
        else:
            self.create_subscription(String, full, self._on_string, 10)
        self.get_logger().info(f"Echoing /{full} — press q to return to menu")

    def _on_gaze(self, message: Vector3) -> None:
        print(f"x={message.x:.3f} y={message.y:.3f} z={message.z:.3f}", flush=True)

    def _on_string(self, message: String) -> None:
        print(message.data, flush=True)


def main(argv: list[str] | None = None) -> int:
    """Echo until q/Esc or Ctrl+C."""
    args = parse_args(argv or sys.argv[1:])
    rclpy.init()
    node = TopicEchoNode(args.topic)
    try:
        while rclpy.ok():
            if quit_requested():
                print("Back to menu.", flush=True)
                break
            rclpy.spin_once(node, timeout_sec=0.1)
    except KeyboardInterrupt:
        print("\nBack to menu.", flush=True)
    finally:
        node.destroy_node()
        rclpy.shutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
