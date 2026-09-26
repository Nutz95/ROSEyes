"""Xbox controller left-stick reader for ROSEyes gaze publishing."""

from __future__ import annotations

from dataclasses import dataclass

import pygame


@dataclass(frozen=True)
class ControllerSample:
    """Normalized stick sample plus edge-triggered blink button."""

    x: float
    y: float
    blink_pressed: bool


class XboxGazeStickReader:
    """Reads the left analog stick and A-button blink from joystick 0."""

    def __init__(self, deadzone: float = 0.12) -> None:
        """
        Args:
            deadzone: Absolute stick magnitude below which output is zeroed.
        """
        self._deadzone = deadzone
        self._joystick: pygame.joystick.Joystick | None = None
        self._previous_button_a = False

    def open(self) -> None:
        """Initializes pygame joystick subsystem and opens joystick 0."""
        pygame.init()
        pygame.joystick.init()
        if pygame.joystick.get_count() < 1:
            raise RuntimeError("No joystick detected. Connect an Xbox controller.")
        self._joystick = pygame.joystick.Joystick(0)
        self._joystick.init()
        self._previous_button_a = False
        print(f"controller: {self._joystick.get_name()}", flush=True)

    def close(self) -> None:
        """Shuts down pygame."""
        pygame.quit()
        self._joystick = None

    def read(self) -> ControllerSample:
        """Polls events and returns deadzoned stick + blink edge."""
        if self._joystick is None:
            raise RuntimeError("Joystick is not open.")
        pygame.event.pump()
        raw_x = self._joystick.get_axis(0)
        raw_y = self._joystick.get_axis(1)
        button_a = self._joystick.get_button(0) != 0
        blink_pressed = button_a and not self._previous_button_a
        self._previous_button_a = button_a
        return ControllerSample(
            x=self._apply_deadzone(raw_x),
            y=self._apply_deadzone(raw_y),
            blink_pressed=blink_pressed,
        )

    def _apply_deadzone(self, value: float) -> float:
        if abs(value) < self._deadzone:
            return 0.0
        if value > 1.0:
            return 1.0
        if value < -1.0:
            return -1.0
        return value
