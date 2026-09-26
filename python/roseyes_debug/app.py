"""Tk debug cockpit: gaze pad / Xbox, ball view, JPEG snap, telemetry."""

from __future__ import annotations

import io
import threading
import time
import tkinter as tk
from tkinter import ttk
from typing import Any

import rclpy
from PIL import Image, ImageTk

from roseyes_debug.ball_view import BallView
from roseyes_debug.gaze_pad import GazePad
from roseyes_debug.ros_bridge import DebugRosBridge
from xbox_gaze.stick_reader import XboxGazeStickReader


class DebugCockpitApp:
    """Single-window host debugger for ROSEyes."""

    def __init__(self) -> None:
        """Starts ROS + builds the UI."""
        self._root = tk.Tk()
        self._root.title("ROSEyes debug cockpit")
        self._root.protocol("WM_DELETE_WINDOW", self._on_close)

        rclpy.init()
        self._bridge = DebugRosBridge(
            on_status=self._on_status,
            on_ball=self._on_ball,
            on_perf=self._on_perf,
            on_jpeg=self._on_jpeg,
        )
        self._photo: ImageTk.PhotoImage | None = None
        self._input_mode = tk.StringVar(master=self._root, value="pad")
        self._status_text = tk.StringVar(master=self._root, value="status: —")
        self._perf_text = tk.StringVar(master=self._root, value="perf: —")
        self._snap_text = tk.StringVar(master=self._root, value="JPEG: click Snap")
        self._xbox_reader: XboxGazeStickReader | None = None
        self._xbox_error = tk.StringVar(master=self._root, value="")
        self._spinning = True
        self._next_mode_s = 0.0
        self._hold_piloted = True

        self._build_ui()

        self._spin_thread = threading.Thread(target=self._spin, daemon=True)
        self._spin_thread.start()
        self._root.after(50, self._tick)

    def _build_ui(self) -> None:
        root = self._root
        root.columnconfigure(0, weight=1)
        root.rowconfigure(1, weight=1)

        toolbar = ttk.Frame(root, padding=6)
        toolbar.grid(row=0, column=0, sticky="ew")
        ttk.Label(toolbar, text="Gaze input:").pack(side=tk.LEFT)
        ttk.Radiobutton(
            toolbar,
            text="Pad",
            value="pad",
            variable=self._input_mode,
            command=self._on_input_mode,
        ).pack(side=tk.LEFT, padx=4)
        ttk.Radiobutton(
            toolbar,
            text="Xbox",
            value="xbox",
            variable=self._input_mode,
            command=self._on_input_mode,
        ).pack(side=tk.LEFT, padx=4)
        ttk.Button(toolbar, text="Autonomous", command=self._mode_auto).pack(
            side=tk.LEFT, padx=6
        )
        ttk.Button(toolbar, text="Piloted", command=self._mode_piloted).pack(
            side=tk.LEFT
        )
        ttk.Button(toolbar, text="Blink", command=self._blink).pack(
            side=tk.LEFT, padx=6
        )
        ttk.Button(toolbar, text="Snap", command=self._snap).pack(side=tk.LEFT)
        ttk.Label(toolbar, textvariable=self._xbox_error, foreground="#c62828").pack(
            side=tk.LEFT, padx=8
        )

        body = ttk.Frame(root, padding=6)
        body.grid(row=1, column=0, sticky="nsew")
        body.columnconfigure(0, weight=0)
        body.columnconfigure(1, weight=0)
        body.columnconfigure(2, weight=1)
        body.rowconfigure(1, weight=1)

        ttk.Label(body, text="Gaze pad").grid(row=0, column=0, sticky="w")
        ttk.Label(body, text="Ball detect").grid(row=0, column=1, sticky="w", padx=8)
        ttk.Label(body, text="Camera JPEG").grid(row=0, column=2, sticky="w")

        self._pad = GazePad(body, size=220, on_gaze=self._publish_gaze)
        self._pad.canvas.grid(row=1, column=0, sticky="nw")

        ball_frame = ttk.Frame(body)
        ball_frame.grid(row=1, column=1, sticky="nw", padx=8)
        self._ball = BallView(ball_frame, size=220)
        self._ball.canvas.pack()
        ttk.Label(ball_frame, textvariable=self._ball.info, wraplength=220).pack(
            anchor="w", pady=4
        )

        jpeg_frame = ttk.Frame(body)
        jpeg_frame.grid(row=1, column=2, sticky="nsew")
        ttk.Label(jpeg_frame, textvariable=self._snap_text).pack(anchor="w")
        self._jpeg_label = ttk.Label(jpeg_frame)
        self._jpeg_label.pack(anchor="nw", pady=4)

        footer = ttk.Frame(root, padding=6)
        footer.grid(row=2, column=0, sticky="ew")
        ttk.Label(footer, textvariable=self._status_text).pack(anchor="w")
        ttk.Label(footer, textvariable=self._perf_text).pack(anchor="w")

    def _on_input_mode(self) -> None:
        pad_mode = self._input_mode.get() == "pad"
        self._pad.set_enabled(pad_mode)
        self._xbox_error.set("")
        if pad_mode:
            self._close_xbox()
            self._hold_piloted = True
            self._bridge.publish_mode("piloted")
        else:
            try:
                if self._xbox_reader is None:
                    self._xbox_reader = XboxGazeStickReader()
                    self._xbox_reader.open()
                self._hold_piloted = True
                self._bridge.publish_mode("piloted")
            except RuntimeError as exc:
                self._xbox_error.set(str(exc))
                self._input_mode.set("pad")
                self._pad.set_enabled(True)

    def _close_xbox(self) -> None:
        if self._xbox_reader is not None:
            self._xbox_reader.close()
            self._xbox_reader = None

    def _publish_gaze(self, x: float, y: float) -> None:
        self._hold_piloted = True
        self._bridge.publish_mode("piloted")
        self._bridge.publish_gaze(x, y)

    def _mode_auto(self) -> None:
        self._hold_piloted = False
        self._bridge.publish_mode("autonomous")

    def _mode_piloted(self) -> None:
        self._hold_piloted = True
        self._bridge.publish_mode("piloted")

    def _blink(self) -> None:
        self._bridge.publish_blink()

    def _snap(self) -> None:
        self._snap_text.set("JPEG: waiting…")
        self._bridge.request_snap()

    def _on_status(self, text: str) -> None:
        self._root.after(0, lambda: self._status_text.set(f"status: {text}"))

    def _on_ball(self, payload: dict[str, Any]) -> None:
        self._root.after(0, lambda: self._ball.update(payload))

    def _on_perf(self, payload: dict[str, Any]) -> None:
        def apply() -> None:
            if "raw" in payload:
                self._perf_text.set(f"perf: {payload['raw']}")
                return
            self._perf_text.set(
                "perf: heap={heap_pct}% ({heap_free}/{heap_size}) "
                "psram={psram_pct}% ({psram_free}/{psram_size}) "
                "cpu0={cpu0_pct}% cpu1={cpu1_pct}% loop={loop_hz}Hz "
                "rssi={wifi_rssi} ball_fps={ball_fps} up={uptime_s}s".format(
                    **{
                        k: payload.get(k, "?")
                        for k in (
                            "heap_pct",
                            "heap_free",
                            "heap_size",
                            "psram_pct",
                            "psram_free",
                            "psram_size",
                            "cpu0_pct",
                            "cpu1_pct",
                            "loop_hz",
                            "wifi_rssi",
                            "ball_fps",
                            "uptime_s",
                        )
                    }
                )
            )

        self._root.after(0, apply)

    def _on_jpeg(self, payload: bytes) -> None:
        def apply() -> None:
            try:
                image = Image.open(io.BytesIO(payload))
                image.thumbnail((480, 360))
                self._photo = ImageTk.PhotoImage(image)
                self._jpeg_label.configure(image=self._photo)
                self._snap_text.set(f"JPEG: {len(payload)} bytes")
            except Exception as exc:  # noqa: BLE001
                self._snap_text.set(f"JPEG decode failed: {exc}")

        self._root.after(0, apply)

    def _spin(self) -> None:
        while self._spinning and rclpy.ok():
            rclpy.spin_once(self._bridge, timeout_sec=0.05)

    def _tick(self) -> None:
        if not self._spinning:
            return
        now = time.monotonic()
        if self._hold_piloted and now >= self._next_mode_s:
            self._bridge.publish_mode("piloted")
            self._next_mode_s = now + 1.0
        if self._input_mode.get() == "xbox" and self._xbox_reader is not None:
            try:
                sample = self._xbox_reader.read()
            except RuntimeError as exc:
                self._xbox_error.set(str(exc))
            else:
                gaze_y = sample.y
                self._pad.set_gaze(sample.x, gaze_y)
                if abs(sample.x) > 0.12 or abs(gaze_y) > 0.12:
                    self._hold_piloted = True
                    self._bridge.publish_gaze(sample.x, gaze_y)
                if sample.blink_pressed:
                    self._bridge.publish_blink()
        self._root.after(33, self._tick)

    def _on_close(self) -> None:
        self._spinning = False
        self._close_xbox()
        self._bridge.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()
        self._root.destroy()

    def run(self) -> int:
        """Blocks on the Tk main loop."""
        self._root.mainloop()
        return 0
