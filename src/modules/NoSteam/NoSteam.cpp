#include "NoSteam.h"

#include "base/Sig.h"
#include "tier0/icommandline.h"

void NoSteam::ClientSteamContextActivate(void* p) {}

bool NoSteam::Load() {
  if (CommandLine()->CheckParm("-nosteam")) {
    void* ptrClientSteamContextActivate = Sig::Scan(SIGPAT_ClientSteamContextActivate);
    if (!hookClientSteamContextActivate.Install(ptrClientSteamContextActivate, &NoSteam::ClientSteamContextActivate, this)) return false;
  }

  return true;
}

void NoSteam::Unload() { hookClientSteamContextActivate.Remove(); }
