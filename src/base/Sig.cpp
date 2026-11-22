#include "Sig.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifdef WIN32
#include <Windows.h>
// Include AFTER Windows.h
#include <Psapi.h>
#else
#include <elf.h>
#include <link.h>
#endif

namespace Sig {

bool IsHexChar(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }

void ScanBytesFromString(const char* input, ScanBytes* out) {
  out->used = 0;

  for (const char* ptr = input; *ptr; ptr++) {
    assert(out->used < MAX_SCAN_BYTES);

    if (IsHexChar(*ptr)) {
      assert(IsHexChar(*(ptr + 1)));
      out->bytes[out->used++] = strtol(ptr, nullptr, 16);
      ptr++;
    } else if (*ptr == '?') {
      assert(*(ptr + 1) == '?');
      out->bytes[out->used++] = -1;
      ptr++;
    }
  }

  assert(out->used > 0);
}

bool CompareData(uint8_t* data, ScanBytes* pattern) {
  for (int32_t i = 0; i < pattern->used; i++) {
    int16_t byte = pattern->bytes[i];

    if (byte != -1 && data[i] != byte) return false;
  }

  return true;
}

void* FindPattern(void* start, int32_t search_length, ScanBytes* pattern) {
  int32_t length = pattern->used;

  for (int32_t i = 0; i <= search_length - length; i++) {
    uint8_t* addr = (uint8_t*)start + i;

    if (CompareData(addr, pattern)) return addr;
  }

  return nullptr;
}

void* Virtual(void* ptr, int32_t index) {
  if (!ptr) return nullptr;

  void** vtable = *((void***)ptr);
  return vtable[index];
}

#ifdef WIN32

void* Scan(const char* module, const char* pattern, uint8_t offset, uint32_t displacement) {
  MODULEINFO info = {};

  HMODULE handle = GetModuleHandleA(module);

  if (!handle || !GetModuleInformation(GetCurrentProcess(), handle, &info, sizeof(info))) return nullptr;

  ScanBytes bytes = {};
  ScanBytesFromString(pattern, &bytes);

  uint8_t* addr = (uint8_t*)FindPattern(info.lpBaseOfDll, (int32_t)info.SizeOfImage, &bytes);

  if (!addr) return nullptr;

  addr += offset;

  if (displacement) {
    int32_t disp;
    memcpy(&disp, addr, sizeof(disp));

    addr += disp + displacement;
  }

  return addr;
}

#else

struct ModuleInfo {
  const char* name;
  uintptr_t base = 0;
};

static int FindModule(struct dl_phdr_info* info, size_t, void* data) {
  ModuleInfo* module = (ModuleInfo*)data;

  if (!info->dlpi_name || !*info->dlpi_name) return 0;

  if (!strstr(info->dlpi_name, module->name)) return 0;

  module->base = info->dlpi_addr;
  return 1;
}

void* Scan(const char* so, const char* pattern, uint8_t offset, uint32_t displacement) {
  ModuleInfo module = {};
  module.name = so;

  dl_iterate_phdr(FindModule, &module);

  if (!module.base) return nullptr;

  ScanBytes bytes = {};
  ScanBytesFromString(pattern, &bytes);

  struct SearchContext {
    ScanBytes* pattern;
    void* result;
  } context = {&bytes, nullptr};

  struct SegmentData {
    SearchContext* context;
    uintptr_t base;
  } segment = {&context, module.base};

  auto callback = [](struct dl_phdr_info* info, size_t, void* data) -> int {
    SegmentData* segment = (SegmentData*)data;

    if (info->dlpi_addr != segment->base) return 0;

    for (int i = 0; i < info->dlpi_phnum; i++) {
      const ElfW(Phdr)* phdr = &info->dlpi_phdr[i];

      if (phdr->p_type != PT_LOAD || !(phdr->p_flags & PF_R)) continue;

      uint8_t* start = (uint8_t*)(info->dlpi_addr + phdr->p_vaddr);

      void* result = FindPattern(start, (int32_t)phdr->p_memsz, segment->context->pattern);

      if (result) {
        segment->context->result = result;
        return 1;
      }
    }

    return 1;
  };

  dl_iterate_phdr(callback, &segment);

  uint8_t* addr = (uint8_t*)context.result;

  if (!addr) return nullptr;

  addr += offset;

  if (displacement) {
    int32_t disp;
    memcpy(&disp, addr, sizeof(disp));

    addr += disp + displacement;
  }

  return addr;
}

#endif

}  // namespace Sig
