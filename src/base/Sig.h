#pragma once

#include <stdint.h>

#define MAX_SCAN_BYTES 256

struct ScanBytes {
  // A value of -1 means unknown byte
  int16_t bytes[MAX_SCAN_BYTES];
  int16_t used;
};

namespace Sig {

// bool IsHexChar(char c);
// void ScanBytesFromString(const char* input, ScanBytes* out);
// bool CompareData(uint8_t* data, ScanBytes* pattern);
// void* FindPattern(void* start, int32_t search_length, ScanBytes* pattern);

void* Virtual(void* ptr, int32_t index);
void* Scan(const char* dll, const char* pattern, uint8_t offset, uint32_t displacement);
}  // namespace Sig
