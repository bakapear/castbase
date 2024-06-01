#pragma once

#include <convar.h>

#include "base/Modules.h"

class Resolution : public Module {
 public:
  const char* GetModuleName() { return "Resolution"; }

  void Load() override {};
  void Unload() override {};

 private:
  CON_COMMAND_MEMBER_F(Resolution, "resolution_set", resolution_set, "Set the current exact windowed resolution", 0);
};

inline Resolution m_Resolution;