#pragma once

#include <Preferences.h>
#include <stddef.h>
#include <stdint.h>

#include "IKeyValueStore.h"

/**
 * ESP32 Preferences (NVS) backend for IKeyValueStore.
 */
class NvsKeyValueStore : public IKeyValueStore {
 public:
  /** Creates a closed Preferences-backed store. */
  NvsKeyValueStore();

  /** Opens an NVS namespace via Preferences. */
  bool begin(const char* namespace_name, bool read_only) override;

  /** Closes Preferences. */
  void end() override;

  /** Reads a string preference into buffer. */
  bool getString(const char* key, char* buffer, size_t buffer_size) const override;

  /** Writes a string preference. */
  bool putString(const char* key, const char* value) override;

  /** Reads a uint16 preference. */
  uint16_t getUInt16(const char* key, uint16_t default_value) const override;

  /** Writes a uint16 preference. */
  bool putUInt16(const char* key, uint16_t value) override;

  /** Returns true when the key exists in NVS. */
  bool hasKey(const char* key) const override;

 private:
  mutable Preferences preferences_;
  bool open_;
};
