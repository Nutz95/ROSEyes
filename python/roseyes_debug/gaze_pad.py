"""Clickable gaze pad (normalized [-1, 1])."""

from __future__ import annotations

import tkinter as tk
from collections.abc import Callable


class GazePad:
    """Square canvas: click/drag publishes normalized gaze."""

    def __init__(
        self,
        parent: tk.Misc,
        size: int = 200,
        on_gaze: Callable[[float, float], None] | None = None,
    ) -> None:
        """Builds the pad inside parent."""
        self._size = size
        self._on_gaze = on_gaze
        self._x = 0.0
        self._y = 0.0
        self._enabled = True

        self.canvas = tk.Canvas(
            parent, width=size, height=size, bg="#1a1a1a", highlightthickness=1
        )
        self.canvas.bind("<Button-1>", self._on_pointer)
        self.canvas.bind("<B1-Motion>", self._on_pointer)
        self._draw()

    def set_enabled(self, enabled: bool) -> None:
        """Enables or greys out pointer input."""
        self._enabled = enabled
        self.canvas.configure(bg="#1a1a1a" if enabled else "#333333")

    def set_gaze(self, x: float, y: float) -> None:
        """Updates the marker without publishing (e.g. Xbox mirror)."""
        self._x = max(-1.0, min(1.0, x))
        self._y = max(-1.0, min(1.0, y))
        self._draw()

    def _on_pointer(self, event: tk.Event) -> None:
        if not self._enabled:
            return
        nx = (event.x / self._size) * 2.0 - 1.0
        ny = (event.y / self._size) * 2.0 - 1.0
        self._x = max(-1.0, min(1.0, nx))
        self._y = max(-1.0, min(1.0, ny))
        self._draw()
        if self._on_gaze is not None:
            self._on_gaze(self._x, self._y)

    def _draw(self) -> None:
        self.canvas.delete("all")
        mid = self._size / 2.0
        self.canvas.create_line(mid, 0, mid, self._size, fill="#555555")
        self.canvas.create_line(0, mid, self._size, mid, fill="#555555")
        px = (self._x + 1.0) * 0.5 * self._size
        py = (self._y + 1.0) * 0.5 * self._size
        r = 8
        self.canvas.create_oval(
            px - r, py - r, px + r, py + r, fill="#66ccff", outline=""
        )
