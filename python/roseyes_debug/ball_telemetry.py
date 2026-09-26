"""Typed /eyes/ball sample."""

from __future__ import annotations

import json
from dataclasses import dataclass


@dataclass(frozen=True)
class BallTelemetry:
    """One decoded ball observation from the ESP."""

    found: bool
    x: float
    y: float
    diameter: float
    fps: float
    framesize: str
    color: str
    seq: int
    raw: str | None = None

    @classmethod
    def from_json(cls, text: str) -> BallTelemetry:
        """Parses a /eyes/ball JSON string (or keeps raw on decode failure)."""
        try:
            payload = json.loads(text)
        except json.JSONDecodeError:
            return cls(
                found=False,
                x=0.0,
                y=0.0,
                diameter=0.0,
                fps=0.0,
                framesize="?",
                color="none",
                seq=0,
                raw=text,
            )
        return cls(
            found=bool(payload.get("found", False)),
            x=float(payload.get("x", 0.0)),
            y=float(payload.get("y", 0.0)),
            diameter=float(payload.get("diameter", 0.0)),
            fps=float(payload.get("fps", 0.0)),
            framesize=str(payload.get("framesize", "?")),
            color=str(payload.get("color", "none")).lower(),
            seq=int(payload.get("seq", 0)),
        )
