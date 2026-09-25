#include "InMemoryKeyValueStore.h"

#include <cstdio>
#include <cstring>

InMemoryKeyValueStore::InMemoryKeyValueStore() : open_(false) {}

bool InMemoryKeyValueStore::begin(const char* /*namespace_name*/,
                                  bool /*read_only*/) {
  open_ = true;
  return true;
}

void InMemoryKeyValueStore::end() { open_ = false; }

bool InMemoryKeyValueStore::getString(const char* key, char* buffer,
                                      size_t buffer_size) const {
  if (!open_ || key == nullptr || buffer == nullptr || buffer_size == 0) {
    return false;
  }
  const auto iterator = string_values_.find(key);
  if (iterator == string_values_.end()) {
    return false;
  }
  std::snprintf(buffer, buffer_size, "%s", iterator->second.c_str());
  return true;
}

bool InMemoryKeyValueStore::putString(const char* key, const char* value) {
  if (!open_ || key == nullptr || value == nullptr) {
    return false;
  }
  string_values_[key] = value;
  return true;
}

uint16_t InMemoryKeyValueStore::getUInt16(const char* key,
                                          uint16_t default_value) const {
  if (!open_ || key == nullptr) {
    return default_value;
  }
  const auto iterator = uint16_values_.find(key);
  if (iterator == uint16_values_.end()) {
    return default_value;
  }
  return iterator->second;
}

bool InMemoryKeyValueStore::putUInt16(const char* key, uint16_t value) {
  if (!open_ || key == nullptr) {
    return false;
  }
  uint16_values_[key] = value;
  return true;
}

bool InMemoryKeyValueStore::hasKey(const char* key) const {
  if (!open_ || key == nullptr) {
    return false;
  }
  return string_values_.count(key) > 0 || uint16_values_.count(key) > 0;
}
