"""Windows console quit helper (q / Esc) for interactive ROSEyes tools."""

from __future__ import annotations


def quit_requested() -> bool:
    """True when the user pressed q/Q/Esc in the console (Windows)."""
    try:
        import msvcrt
    except ImportError:
        return False
    while msvcrt.kbhit():
        key = msvcrt.getwch()
        if key in ("q", "Q", "\x1b"):
            return True
        # Skip special-key prefix (arrows etc.): second getwch consumes it.
        if key in ("\x00", "\xe0"):
            if msvcrt.kbhit():
                msvcrt.getwch()
    return False
