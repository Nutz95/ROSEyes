"""Typed /eyes/range sample."""

from __future__ import annotations

import json
from dataclasses import dataclass


@dataclass(frozen=True)
class RangeTelemetry:
    """One decoded TOF sample from the ESP."""

    mm: int
    ok: bool
    status: int
    strength: int
    seq: int
    raw: str | None = None

    @classmethod
    def from_json(cls, text: str) -> RangeTelemetry:
        """Parses a /eyes/range JSON string (or keeps raw on decode failure)."""
        try:
            payload = json.loads(text)
        except json.JSONDecodeError:
            return cls(mm=0, ok=False, status=0, strength=0, seq=0, raw=text)
        return cls(
            mm=int(payload.get("mm", 0)),
            ok=bool(payload.get("ok", False)),
            status=int(payload.get("status", 0)),
            strength=int(payload.get("strength", 0)),
            seq=int(payload.get("seq", 0)),
        )

    def status_line(self) -> str:
        """One-line footer text for the debug cockpit."""
        if self.raw is not None:
            return f"range: {self.raw}"
        meters = self.mm / 1000.0
        flag = "ok" if self.ok else f"weak/st={self.status}"
        return (
            f"range: {self.mm} mm ({meters:.2f} m) {flag} "
            f"str={self.strength} seq={self.seq}"
        )
