#pragma once

#include <Color.h>
#include <dbg.h>

#ifdef WIN32
#include "patterns/Windows.h"
#else
#include "patterns/Linux.h"
#endif

extern const char* const PLUGIN_NAME;
extern const char* const PLUGIN_VERSION;
extern const char* const PLUGIN_DESC;

template <class... Parameters>
FORCEINLINE void PluginMsg(const char* fmt, Parameters... param) {
  ConColorMsg(Color(0, 153, 153, 255), "[%s] ", PLUGIN_NAME);
  Msg(fmt, param...);
}

template <class... Parameters>
FORCEINLINE void PluginWarning(const char* fmt, const Parameters&... param) {
  ConColorMsg(Color(0, 153, 153, 255), "[%s] ", PLUGIN_NAME);
  Warning(fmt, param...);
}

template <class... Parameters>
FORCEINLINE void PluginSuccess(const char* fmt, const Parameters&... param) {
  ConColorMsg(Color(0, 153, 153, 255), "[%s] ", PLUGIN_NAME);
  ConColorMsg(Color(0, 153, 0, 255), fmt, param...);
}

template <class... Parameters>
FORCEINLINE void PluginColorMsg(const Color& color, const char* fmt, Parameters... param) {
  ConColorMsg(Color(0, 153, 153, 255), "[%s] ", PLUGIN_NAME);
  ConColorMsg(color, fmt, param...);
}

template <class... Parameters>
FORCEINLINE void PluginOut(const char* fmt, Parameters... param) {
  printf("[%s] ", PLUGIN_NAME);
  printf(fmt, param...);
  printf("\n");
  fflush(stdout);
}
