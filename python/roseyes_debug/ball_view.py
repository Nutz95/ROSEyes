"""Ball detection overlay (position, size, color, camera meta)."""

from __future__ import annotations

import tkinter as tk
from typing import Any


_COLOR_FILL = {
    "red": "#e53935",
    "green": "#43a047",
    "none": "#888888",
}


class BallView:
    """Canvas showing last /eyes/ball sample in normalized space."""

    def __init__(self, parent: tk.Misc, size: int = 200) -> None:
        """Builds the view inside parent."""
        self._size = size
        self.canvas = tk.Canvas(
            parent, width=size, height=size, bg="#101820", highlightthickness=1
        )
        self.info = tk.StringVar(master=parent.winfo_toplevel(), value="ball: waiting…")
        self._draw_empty()

    def update(self, payload: dict[str, Any]) -> None:
        """Redraws from a ball JSON dict."""
        found = bool(payload.get("found", False))
        x = float(payload.get("x", 0.0))
        y = float(payload.get("y", 0.0))
        diameter = float(payload.get("diameter", 0.0))
        fps = float(payload.get("fps", 0.0))
        framesize = str(payload.get("framesize", "?"))
        color = str(payload.get("color", "none")).lower()
        seq = payload.get("seq", "?")

        self.canvas.delete("all")
        mid = self._size / 2.0
        self.canvas.create_line(mid, 0, mid, self._size, fill="#2a3a4a")
        self.canvas.create_line(0, mid, self._size, mid, fill="#2a3a4a")

        if found:
            px = (x + 1.0) * 0.5 * self._size
            py = (y + 1.0) * 0.5 * self._size
            radius = max(4.0, diameter * self._size * 0.5)
            fill = _COLOR_FILL.get(color, "#888888")
            self.canvas.create_oval(
                px - radius,
                py - radius,
                px + radius,
                py + radius,
                outline=fill,
                width=2,
            )
            self.canvas.create_oval(
                px - 3, py - 3, px + 3, py + 3, fill=fill, outline=""
            )
            self.info.set(
                f"found {color}  x={x:.2f} y={y:.2f}  "
                f"d={diameter:.2f}  {framesize}  {fps:.1f} fps  seq={seq}"
            )
        else:
            self.info.set(
                f"not found  {framesize}  {fps:.1f} fps  color={color}  seq={seq}"
            )

    def _draw_empty(self) -> None:
        mid = self._size / 2.0
        self.canvas.create_line(mid, 0, mid, self._size, fill="#2a3a4a")
        self.canvas.create_line(0, mid, self._size, mid, fill="#2a3a4a")
