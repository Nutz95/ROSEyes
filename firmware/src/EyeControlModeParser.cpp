#include "EyeControlModeParser.h"

#include <ctype.h>
#include <string.h>

namespace {

bool equalsIgnoreCase(const char* left, const char* right) {
  if (left == nullptr || right == nullptr) {
    return false;
  }
  while (*left != '\0' && *right != '\0') {
    const int a = tolower(static_cast<unsigned char>(*left));
    const int b = tolower(static_cast<unsigned char>(*right));
    if (a != b) {
      return false;
    }
    ++left;
    ++right;
  }
  return *left == '\0' && *right == '\0';
}

}  // namespace

bool EyeControlModeParser::tryParse(const char* text, EyeControlMode& out_mode) {
  if (text == nullptr) {
    return false;
  }
  // Trim leading spaces.
  while (*text != '\0' && isspace(static_cast<unsigned char>(*text))) {
    ++text;
  }
  if (equalsIgnoreCase(text, "autonomous") || equalsIgnoreCase(text, "auto")) {
    out_mode = EyeControlMode::Autonomous;
    return true;
  }
  if (equalsIgnoreCase(text, "piloted") || equalsIgnoreCase(text, "pilot")) {
    out_mode = EyeControlMode::Piloted;
    return true;
  }
  return false;
}

const char* EyeControlModeParser::toCString(EyeControlMode mode) {
  switch (mode) {
    case EyeControlMode::Autonomous:
      return "autonomous";
    case EyeControlMode::Piloted:
      return "piloted";
  }
  return "autonomous";
}
