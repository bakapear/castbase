#pragma once

#include <Color.h>
#include <dbg.h>

#include "Patterns.h"

extern const char* const PLUGIN_NAME;
extern const char* const PLUGIN_VERSION;
extern const char* const PLUGIN_DESC;

template <class... Parameters>
__forceinline void PluginMsg(const char* fmt, Parameters... param) {
  ConColorMsg(Color(0, 153, 153, 255), "[%s] ", PLUGIN_NAME);
  Msg(fmt, param...);
}
template <class... Parameters>
__forceinline void PluginWarning(const char* fmt, const Parameters&... param) {
  ConColorMsg(Color(0, 153, 153, 255), "[%s] ", PLUGIN_NAME);
  Warning(fmt, param...);
}
template <class... Parameters>
__forceinline void PluginColorMsg(const Color& color, const char* fmt, Parameters... param) {
  ConColorMsg(Color(0, 153, 153, 255), "[%s] ", PLUGIN_NAME);
  ConColorMsg(color, fmt, param...);
}
