#pragma once

#ifdef WIN32

#include <fcntl.h>
#include <io.h>
#include <windows.h>

// _popen replacement which hides window
static FILE* popen(const char* command, const char* mode) {
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  HANDLE hChildStd_IN_Rd = NULL;
  HANDLE hChildStd_IN_Wr = NULL;
  CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &sa, 0);
  SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));

  si.cb = sizeof(si);
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  si.hStdInput = hChildStd_IN_Rd;
  si.dwFlags |= STARTF_USESTDHANDLES;

  BOOL success = CreateProcessA(NULL, (LPSTR)command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

  if (!success) {
    CloseHandle(hChildStd_IN_Wr);
    CloseHandle(hChildStd_IN_Rd);
    return NULL;
  }

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(hChildStd_IN_Rd);

  int flags = _O_WRONLY;
  if (mode && strchr(mode, 'b')) flags |= _O_BINARY;

  int fd = _open_osfhandle((intptr_t)hChildStd_IN_Wr, flags);
  if (fd < 0) {
    CloseHandle(hChildStd_IN_Wr);
    return NULL;
  }

  return _fdopen(fd, mode);
}

#endif
