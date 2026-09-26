#pragma once

#include "EyeControlMode.h"

/**
 * Parses and formats EyeControlMode topic tokens.
 */
class EyeControlModeParser {
 public:
  /**
   * Maps "autonomous" / "piloted" (case-insensitive) into out_mode.
   * @return false when the token is unknown
   */
  static bool tryParse(const char* text, EyeControlMode& out_mode);

  /** Returns the canonical lowercase token for mode. */
  static const char* toCString(EyeControlMode mode);
};
