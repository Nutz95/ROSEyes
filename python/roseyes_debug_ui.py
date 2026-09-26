#!/usr/bin/env python3
"""Launch the ROSEyes debug cockpit (requires activated ROS env)."""

from __future__ import annotations

from roseyes_debug.app import DebugCockpitApp


def main() -> int:
    """Entry point."""
    return DebugCockpitApp().run()


if __name__ == "__main__":
    raise SystemExit(main())
