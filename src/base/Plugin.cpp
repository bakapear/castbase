#include "Plugin.h"

#include <MinHook.h>
#include <convar.h>

#include <chrono>

#include "Hook.h"
#include "Interfaces.h"
#include "Modules.h"
#include "Sig.h"

class MyPlugin final : public Plugin {
 public:
  bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) override;
  void Unload() override;
  const char* GetPluginDescription() override { return PLUGIN_DESC; }

 private:
  bool started = false;
  long Start(void* p, int a, void* b);
  void* ptrStart;
  Hook hookStart;
};

static MyPlugin s_MyPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(MyPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, s_MyPlugin);

long MyPlugin::Start(void* p, int a, void* b) {
  auto fn = hookStart.GetTrampoline(&MyPlugin::Start);
  long ret = fn(p, a, b);

  Module::LoadAll();

  hookStart.Remove();
  started = true;
  return ret;
};

bool MyPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
  Interfaces::Load(interfaceFactory);

  MH_Initialize();
  ConVar_Register();

  if (!started) {
    ptrStart = Sig::Scan("engine.dll", "40 55 53 57 48 8D AC 24 70 FE FF FF 48 81 EC ? ? ? ? 8B DA", 0, 0);
    hookStart.Install(ptrStart, &MyPlugin::Start, this);
  } else {
    Module::LoadAll();
  }

  return true;
}

void MyPlugin::Unload() {
  Module::UnloadAll();
  ConVar_Unregister();
  MH_Uninitialize();
  Interfaces::Unload();
}
