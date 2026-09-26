#!/usr/bin/env python3
"""Deprecated: use roseyes_debug_ui.py (full cockpit)."""

from __future__ import annotations

from roseyes_debug.app import DebugCockpitApp


def main() -> int:
    """Entry point — redirects to the debug cockpit."""
    return DebugCockpitApp().run()


if __name__ == "__main__":
    raise SystemExit(main())
