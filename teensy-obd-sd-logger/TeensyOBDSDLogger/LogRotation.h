#pragma once
#include <stdint.h>
namespace automatic {
constexpr uint64_t LogFileLimit=250000000ull; // Decimal MB, includes header.
inline bool needsLogRotation(uint64_t current,uint64_t pending){
  return current>LogFileLimit || pending>LogFileLimit-current;
}
}
