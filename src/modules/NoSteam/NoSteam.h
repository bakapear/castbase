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
  ConVar steam_disable{"steam_disable", "1", FCVAR_NONE, "Skips initializing steam during game launch"};

  // void CClientSteamContext::Activate()
  void ClientSteamContextActivate(void* p);
  Hook hookClientSteamContextActivate;
};

inline NoSteam m_NoSteam;
