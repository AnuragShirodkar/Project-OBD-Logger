#pragma once
#include <stdint.h>
#include <stddef.h>

namespace obd_mtp {
inline bool sameName(const char *a, const char *b) {
  while (*a && *b) {
    char x = *a++, y = *b++;
    if (x >= 'A' && x <= 'Z') x += 'a' - 'A';
    if (y >= 'A' && y <= 'Z') y += 'a' - 'A';
    if (x != y) return false;
  }
  return *a == *b;
}

// Filter ancestors too: hosts may request a flat list of all objects.
template <size_t NameCapacity, class Storage>
bool objectVisible(Storage &storage, uint32_t handle, uint32_t rootLimit) {
  for (unsigned depth = 0; depth < 256; ++depth) {
    if (handle < rootLimit || handle == UINT32_MAX) return false;
    char name[NameCapacity] = {};
    uint64_t size = 0;
    uint32_t parent = 0;
    uint16_t store = 0;
    storage.GetObjectInfo(handle, name, &size, &parent, &store);
    if (!name[0] || sameName(name, "System Volume Information") ||
        sameName(name, "mtpindex.dat")) return false;
    if (parent < rootLimit) return true;
    if (parent == handle) return false;
    handle = parent;
  }
  return false;
}
}
