#pragma once

#include <convar.h>

#include "base/Hook.h"
#include "base/Modules.h"

class NoSteam : public Module {
 public:
  const char* GetModuleName() { return "NoSteam"; }

  bool Load();
  void Unload();

 private:
  // void CClientSteamContext::Activate()
  void ClientSteamContextActivate(void* p);
  Hook hookClientSteamContextActivate;
};

inline NoSteam m_NoSteam;
