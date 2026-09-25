#include "NvsKeyValueStore.h"

NvsKeyValueStore::NvsKeyValueStore() : open_(false) {}

bool NvsKeyValueStore::begin(const char* namespace_name, bool read_only) {
  open_ = preferences_.begin(namespace_name, read_only);
  return open_;
}

void NvsKeyValueStore::end() {
  if (open_) {
    preferences_.end();
    open_ = false;
  }
}

bool NvsKeyValueStore::getString(const char* key, char* buffer,
                                 size_t buffer_size) const {
  if (!open_ || key == nullptr || buffer == nullptr || buffer_size == 0) {
    return false;
  }
  const String value = preferences_.getString(key, "");
  if (value.length() == 0 && !preferences_.isKey(key)) {
    return false;
  }
  value.toCharArray(buffer, static_cast<unsigned int>(buffer_size));
  return true;
}

bool NvsKeyValueStore::putString(const char* key, const char* value) {
  if (!open_ || key == nullptr || value == nullptr) {
    return false;
  }
  return preferences_.putString(key, value) > 0;
}

uint16_t NvsKeyValueStore::getUInt16(const char* key,
                                     uint16_t default_value) const {
  if (!open_ || key == nullptr) {
    return default_value;
  }
  return preferences_.getUShort(key, default_value);
}

bool NvsKeyValueStore::putUInt16(const char* key, uint16_t value) {
  if (!open_ || key == nullptr) {
    return false;
  }
  return preferences_.putUShort(key, value) > 0;
}

bool NvsKeyValueStore::hasKey(const char* key) const {
  if (!open_ || key == nullptr) {
    return false;
  }
  return preferences_.isKey(key);
}
