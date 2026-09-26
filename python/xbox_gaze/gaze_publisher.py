"""ROS 2 publisher that sends gaze / blink / mode for ROSEyes."""

from __future__ import annotations

from geometry_msgs.msg import Vector3
from rclpy.node import Node
from std_msgs.msg import Empty, String


class GazeRosPublisher(Node):
    """Publishes /eyes/gaze, /eyes/blink, and /eyes/mode."""

    def __init__(self, topic_name: str = "eyes/gaze") -> None:
        """Creates publishers used by the Xbox bridge."""
        super().__init__("roseyes_xbox_gaze_publisher")
        self._gaze_publisher = self.create_publisher(Vector3, topic_name, 10)
        self._blink_publisher = self.create_publisher(Empty, "eyes/blink", 10)
        self._mode_publisher = self.create_publisher(String, "eyes/mode", 10)

    def publish_gaze(self, x: float, y: float) -> None:
        """Publishes a normalized gaze sample."""
        message = Vector3()
        message.x = x
        message.y = y
        message.z = 0.0
        self._gaze_publisher.publish(message)

    def publish_blink(self) -> None:
        """Publishes an empty blink trigger."""
        self._blink_publisher.publish(Empty())

    def publish_mode(self, mode: str) -> None:
        """Publishes control mode (`autonomous` or `piloted`)."""
        message = String()
        message.data = mode
        self._mode_publisher.publish(message)
