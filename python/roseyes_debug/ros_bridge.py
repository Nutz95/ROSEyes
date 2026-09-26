"""ROS bridge for the ROSEyes debug cockpit."""

from __future__ import annotations

from collections.abc import Callable

from geometry_msgs.msg import Vector3
from rclpy.node import Node
from std_msgs.msg import Empty, String, UInt8MultiArray

from roseyes_debug.ball_telemetry import BallTelemetry
from roseyes_debug.perf_telemetry import PerfTelemetry
from roseyes_debug.range_telemetry import RangeTelemetry


class DebugRosBridge(Node):
    """Publishes commands and receives status/ball/perf/range/jpeg."""

    def __init__(
        self,
        on_status: Callable[[str], None],
        on_ball: Callable[[BallTelemetry], None],
        on_perf: Callable[[PerfTelemetry], None],
        on_range: Callable[[RangeTelemetry], None],
        on_jpeg: Callable[[bytes], None],
    ) -> None:
        """Creates pubs/subs used by the Tk cockpit."""
        super().__init__("roseyes_debug_ui")
        self._on_status = on_status
        self._on_ball = on_ball
        self._on_perf = on_perf
        self._on_range = on_range
        self._on_jpeg = on_jpeg

        self._gaze_pub = self.create_publisher(Vector3, "eyes/gaze", 10)
        self._blink_pub = self.create_publisher(Empty, "eyes/blink", 10)
        self._mode_pub = self.create_publisher(String, "eyes/mode", 10)
        self._snap_pub = self.create_publisher(Empty, "eyes/camera/snap", 10)

        self.create_subscription(String, "eyes/status", self._on_status_msg, 10)
        self.create_subscription(String, "eyes/ball", self._on_ball_msg, 10)
        self.create_subscription(String, "eyes/perf", self._on_perf_msg, 10)
        self.create_subscription(String, "eyes/range", self._on_range_msg, 10)
        self.create_subscription(
            UInt8MultiArray, "eyes/camera/jpeg", self._on_jpeg_msg, 10
        )

    def publish_gaze(self, x: float, y: float) -> None:
        """Publishes normalized gaze."""
        message = Vector3()
        message.x = x
        message.y = y
        message.z = 0.0
        self._gaze_pub.publish(message)

    def publish_blink(self) -> None:
        """Requests one blink."""
        self._blink_pub.publish(Empty())

    def publish_mode(self, mode: str) -> None:
        """Publishes autonomous or piloted."""
        message = String()
        message.data = mode
        self._mode_pub.publish(message)

    def request_snap(self) -> None:
        """Asks the ESP for one JPEG."""
        self._snap_pub.publish(Empty())

    def _on_status_msg(self, message: String) -> None:
        self._on_status(message.data)

    def _on_ball_msg(self, message: String) -> None:
        self._on_ball(BallTelemetry.from_json(message.data))

    def _on_perf_msg(self, message: String) -> None:
        self._on_perf(PerfTelemetry.from_json(message.data))

    def _on_range_msg(self, message: String) -> None:
        self._on_range(RangeTelemetry.from_json(message.data))

    def _on_jpeg_msg(self, message: UInt8MultiArray) -> None:
        self._on_jpeg(bytes(message.data))
