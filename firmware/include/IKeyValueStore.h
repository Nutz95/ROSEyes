#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Minimal string key/value persistence used by WiFi credential storage.
 */
class IKeyValueStore {
 public:
  virtual ~IKeyValueStore() = default;

  /** Opens the named namespace for read/write access. */
  virtual bool begin(const char* namespace_name, bool read_only) = 0;

  /** Closes the active namespace. */
  virtual void end() = 0;

  /** Reads a string value into buffer; returns false if missing or truncated. */
  virtual bool getString(const char* key, char* buffer, size_t buffer_size) const = 0;

  /** Writes a string value for key. */
  virtual bool putString(const char* key, const char* value) = 0;

  /** Reads an unsigned 16-bit integer; returns default_value when missing. */
  virtual uint16_t getUInt16(const char* key, uint16_t default_value) const = 0;

  /** Writes an unsigned 16-bit integer. */
  virtual bool putUInt16(const char* key, uint16_t value) = 0;

  /** Returns true when the key exists. */
  virtual bool hasKey(const char* key) const = 0;
};
