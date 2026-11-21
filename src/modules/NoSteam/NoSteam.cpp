#include "NoSteam.h"

#include "base/Sig.h"

void NoSteam::ClientSteamContextActivate(void* p) {
  auto fn = hookClientSteamContextActivate.GetTrampoline(&NoSteam::ClientSteamContextActivate);
  if (!steam_disable.GetBool()) fn(p);
}

bool NoSteam::Load() {
  void* ptrClientSteamContextActivate = Sig::Scan(SIGPAT_ClientSteamContextActivate);
  if (!hookClientSteamContextActivate.Install(ptrClientSteamContextActivate, &NoSteam::ClientSteamContextActivate, this)) return false;

  return true;
}

void NoSteam::Unload() { hookClientSteamContextActivate.Remove(); }
