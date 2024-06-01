#include "Sig.h"

#define INRANGE(x, a, b) (x >= a && x <= b)
#define getBits(x) (INRANGE((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xa) : (INRANGE(x, '0', '9') ? x - '0' : 0))
#define getByte(x) (getBits(x[0]) << 4 | getBits(x[1]))

void* AsmTools::Relative(void* PtrToRel, int Offset) {
  UINT8* ptr = (UINT8*)PtrToRel + Offset;
  int offset = *(int*)ptr;
  return ptr + offset + sizeof(offset);
}

UINT_PTR Sig::FindPattern(UINT_PTR Start, UINT_PTR End, const char* Pattern) {
  const char* pat = Pattern;
  UINT_PTR firstMatch = NULL;
  for (UINT_PTR pCur = Start; pCur < End; pCur++) {
    if (!*pat) return firstMatch;
    if (*(PBYTE)pat == '\?' || *(BYTE*)pCur == getByte(pat)) {
      if (!firstMatch) firstMatch = pCur;
      if (!pat[2]) return firstMatch;
      if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?')
        pat += 3;
      else
        pat += 2;
    } else {
      pat = Pattern;
      firstMatch = 0;
    }
  }
  return NULL;
}

UINT_PTR Sig::FindDup(UINT_PTR Start, size_t Len, BYTE Val, size_t Count) {
  if (!Start || Len < Count) return 0;

  for (BYTE* pos = (BYTE*)Start; pos < (BYTE*)Start + Len - Count; pos++) {
    for (size_t i = 0; i < Len; i) {
      if (pos[i] != Val) break;
      if (i == Len - 1) return (UINT_PTR)pos;
    }
  }
  return 0;
}

UINT_PTR Sig::FindCave(const char* Module, size_t Len) {
  UINT_PTR code = 0;
  size_t codelen = 0;
  if (!(code = GetCodeRegion(Module, &codelen))) return 0;

  UINT_PTR pos = code + codelen - 1;
  for (; pos >= code; pos--)  // Loop from end of code until no more padding is found
    if (*(BYTE*)pos != 0 && *(BYTE*)pos != 0x90 && *(BYTE*)pos != 0xCC) break;

  UINT_PTR cavelen = (code + codelen - 1) - pos;
  return cavelen > 0xF + Len ? pos + 0x8 : 0;  // Return 8 bytes ahead to prevent overwriting potentially valid code with a 0
}

UINT_PTR Sig::FindPattern(const char* Module, const char* Pattern) {
  HMODULE hMod = GetModuleHandleA(Module);
  UINT_PTR base = GetBaseOfCode(hMod);
  return FindPattern(base, base + GetSizeOfCode(hMod), Pattern);
}

void* Sig::Scan(const char* Module, const char* Pattern, size_t Offset) {
  HMODULE hMod = GetModuleHandleA(Module);
  UINT_PTR base = GetBaseOfCode(hMod);
  UINT_PTR pattern = FindPattern(base, base + GetSizeOfCode(hMod), Pattern);
  return *AsmTools::Relative<void**>(pattern, Offset);
}

UINT_PTR Sig::GetBaseOfCode(HMODULE Module) {
  PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)Module;
  PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)(((UINT_PTR)Module) + pDOSHeader->e_lfanew);
  return (UINT_PTR)Module + pNTHeaders->OptionalHeader.BaseOfCode;
}

DWORD Sig::GetSizeOfCode(HMODULE Module) {
  PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)Module;
  PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)(((UINT_PTR)Module) + pDOSHeader->e_lfanew);
  return pNTHeaders->OptionalHeader.SizeOfCode;
}