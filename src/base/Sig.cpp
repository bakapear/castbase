#include "Sig.h"

#ifdef WIN32
#include <Windows.h>

// Include AFTER Windows.h
#include <Psapi.h>

namespace Sig {

bool IsHexChar(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'); }

void ScanBytesFromString(const char* input, ScanBytes* out) {
  const char* ptr = input;

  out->used = 0;

  for (; *ptr != 0; ptr++) {
    assert(out->used < MAX_SCAN_BYTES);

    if (IsHexChar(*ptr)) {
      assert(IsHexChar(*(ptr + 1)));

      out->bytes[out->used] = strtol(ptr, NULL, 16);
      out->used++;
      ptr++;
    } else if (*ptr == '?') {
      assert(*(ptr + 1) == '?');

      out->bytes[out->used] = -1;
      out->used++;
      ptr++;
    }
  }

  assert(out->used > 0);
}

bool CompareData(uint8_t* data, ScanBytes* pattern) {
  int32_t index = 0;
  int16_t* bytes = pattern->bytes;

  for (int32_t i = 0; i < pattern->used; i++) {
    int16_t byte = *bytes;

    if (byte > -1 && *data != byte) return false;

    data++;
    bytes++;
    index++;
  }

  return index == pattern->used;
}

void* FindPattern(void* start, int32_t search_length, ScanBytes* pattern) {
  int16_t length = pattern->used;

  for (int32_t i = 0; i <= search_length - length; i++) {
    uint8_t* addr = (uint8_t*)start + i;
    if (CompareData(addr, pattern)) return addr;
  }

  return NULL;
}

void* Virtual(void* ptr, int32_t index) {
  if (ptr == NULL) return NULL;
  void** vtable = *((void***)ptr);
  return vtable[index];
}

void* Scan(const char* dll, const char* pattern, uint8_t offset, uint32_t displacement) {
  MODULEINFO info;

  if (!GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(dll), &info, sizeof(MODULEINFO))) return NULL;

  ScanBytes pattern_bytes = {};
  ScanBytesFromString(pattern, &pattern_bytes);

  uint8_t* addr = (uint8_t*)FindPattern(info.lpBaseOfDll, info.SizeOfImage, &pattern_bytes);

  if (offset != 0) {
    addr += offset;
  }

  if (displacement != 0) {
    int32_t disp = *(int32_t*)addr;
    addr += disp;
    addr += displacement;
    return addr;
  }

  return addr;
}

}  // namespace Sig

#elif UNIX
// TODO
#endif