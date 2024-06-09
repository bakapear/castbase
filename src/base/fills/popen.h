#pragma once

#ifdef WIN32

#include <fcntl.h>
#include <io.h>
#include <stdio.h>

#include "Windows.h"

// _popen replacement which hides window
FILE* popen(const char* command, const char* mode) {
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  HANDLE hChildStd_IN_Rd = NULL;
  HANDLE hChildStd_IN_Wr = NULL;
  CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &sa, 0);
  SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  si.hStdInput = hChildStd_IN_Rd;
  si.dwFlags |= STARTF_USESTDHANDLES;

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  BOOL success = CreateProcess(NULL, (LPSTR)command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
  if (!success) {
    CloseHandle(hChildStd_IN_Wr);
    return NULL;
  }

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(hChildStd_IN_Rd);

  return _fdopen(_open_osfhandle((long long)hChildStd_IN_Wr, _O_WRONLY), mode);
}

#endif