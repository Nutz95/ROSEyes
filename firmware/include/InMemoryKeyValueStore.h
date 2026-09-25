#pragma once

#include <map>
#include <string>

#include "IKeyValueStore.h"

/**
 * In-memory IKeyValueStore used by native unit tests.
 */
class InMemoryKeyValueStore : public IKeyValueStore {
 public:
  /** Creates an empty in-memory store. */
  InMemoryKeyValueStore();

  /** Opens a logical namespace (always succeeds). */
  bool begin(const char* namespace_name, bool read_only) override;

  /** Clears the open flag. */
  void end() override;

  /** Copies a stored string into buffer. */
  bool getString(const char* key, char* buffer, size_t buffer_size) const override;

  /** Stores a string value. */
  bool putString(const char* key, const char* value) override;

  /** Returns a stored uint16 or default_value. */
  uint16_t getUInt16(const char* key, uint16_t default_value) const override;

  /** Stores a uint16 value. */
  bool putUInt16(const char* key, uint16_t value) override;

  /** Reports whether key exists. */
  bool hasKey(const char* key) const override;

 private:
  bool open_;
  std::map<std::string, std::string> string_values_;
  std::map<std::string, uint16_t> uint16_values_;
};
