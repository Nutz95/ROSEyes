"""Typed /eyes/perf sample."""

from __future__ import annotations

import json
from dataclasses import dataclass


@dataclass(frozen=True)
class PerfTelemetry:
    """One decoded perf snapshot from the ESP."""

    heap_free: int
    heap_size: int
    heap_pct: float
    heap_min: int
    psram_free: int
    psram_size: int
    psram_pct: float
    loop_hz: float
    cpu0_pct: float
    cpu1_pct: float
    wifi_rssi: int
    ball_fps: float
    uptime_s: int
    raw: str | None = None

    @classmethod
    def from_json(cls, text: str) -> PerfTelemetry:
        """Parses a /eyes/perf JSON string (or keeps raw on decode failure)."""
        try:
            payload = json.loads(text)
        except json.JSONDecodeError:
            return cls(
                heap_free=0,
                heap_size=0,
                heap_pct=0.0,
                heap_min=0,
                psram_free=0,
                psram_size=0,
                psram_pct=0.0,
                loop_hz=0.0,
                cpu0_pct=0.0,
                cpu1_pct=0.0,
                wifi_rssi=0,
                ball_fps=0.0,
                uptime_s=0,
                raw=text,
            )
        return cls(
            heap_free=int(payload.get("heap_free", 0)),
            heap_size=int(payload.get("heap_size", 0)),
            heap_pct=float(payload.get("heap_pct", 0.0)),
            heap_min=int(payload.get("heap_min", 0)),
            psram_free=int(payload.get("psram_free", 0)),
            psram_size=int(payload.get("psram_size", 0)),
            psram_pct=float(payload.get("psram_pct", 0.0)),
            loop_hz=float(payload.get("loop_hz", 0.0)),
            cpu0_pct=float(payload.get("cpu0_pct", 0.0)),
            cpu1_pct=float(payload.get("cpu1_pct", 0.0)),
            wifi_rssi=int(payload.get("wifi_rssi", 0)),
            ball_fps=float(payload.get("ball_fps", 0.0)),
            uptime_s=int(payload.get("uptime_s", 0)),
        )

    def status_line(self) -> str:
        """Formats a one-line footer string for the debug UI."""
        if self.raw is not None:
            return f"perf: {self.raw}"
        return (
            f"perf: heap={self.heap_pct}% ({self.heap_free}/{self.heap_size}) "
            f"psram={self.psram_pct}% ({self.psram_free}/{self.psram_size}) "
            f"cpu0={self.cpu0_pct}% cpu1={self.cpu1_pct}% "
            f"loop={self.loop_hz}Hz rssi={self.wifi_rssi} "
            f"ball_fps={self.ball_fps} up={self.uptime_s}s"
        )
