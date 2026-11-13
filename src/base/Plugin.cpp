#include "Plugin.h"

#include <MinHook.h>
#include <convar.h>

#include <chrono>

#include "Interfaces.h"
#include "Modules.h"
#include "Sig.h"

void MyPlugin::PostInit() {
  auto fn = hookPostInit.GetTrampoline(&MyPlugin::PostInit);
  fn();

  ConVar_Register();
  Module::LoadAll();
}

bool MyPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
  Interfaces::Load(interfaceFactory);

  MH_Initialize();

  void* ptrPostInit = Sig::Virtual(Interfaces::ClientDLL, SIGVTI_PostInit);
  hookPostInit.Install(ptrPostInit, &MyPlugin::PostInit, this);

  return true;
}

void MyPlugin::Unload() {
  Module::UnloadAll();
  ConVar_Unregister();
  MH_Uninitialize();
  Interfaces::Unload();
}
