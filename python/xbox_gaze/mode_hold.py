"""Periodic /eyes/mode republish so ESP reconnect does not stick in Autonomous."""

from __future__ import annotations

from collections.abc import Callable


class ModeHold:
    """Re-publishes a held mode on a timer (piloted hold for host gaze)."""

    def __init__(
        self,
        publish_mode: Callable[[str], None],
        period_s: float = 1.0,
    ) -> None:
        """
        @param publish_mode callback that sends autonomous|piloted
        @param period_s re-publish interval while holding piloted
        """
        self._publish_mode = publish_mode
        self._period_s = max(period_s, 0.2)
        self._hold_piloted = False
        self._next_s = 0.0

    def set_piloted(self, enabled: bool) -> None:
        """Starts or stops piloted hold; publishes only when the hold state changes."""
        if enabled == self._hold_piloted:
            return
        self._hold_piloted = enabled
        self._publish_mode("piloted" if enabled else "autonomous")
        self._next_s = 0.0

    def holding_piloted(self) -> bool:
        """True when piloted is being held."""
        return self._hold_piloted

    def tick(self, now_s: float) -> None:
        """Re-publishes piloted when the hold timer elapses."""
        if not self._hold_piloted:
            return
        if now_s < self._next_s:
            return
        self._publish_mode("piloted")
        self._next_s = now_s + self._period_s
