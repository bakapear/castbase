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
  bool init = false;

  // bool CAudioSourceCache::Init( unsigned int memSize )
  bool AudioSourceCacheInit(void* p, int a);
  Hook hookAudioSourceCacheInit;
};

static MyPlugin s_MyPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(MyPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, s_MyPlugin);

bool MyPlugin::AudioSourceCacheInit(void* p, int a) {
  auto fn = hookAudioSourceCacheInit.GetTrampoline(&MyPlugin::AudioSourceCacheInit);
  long ret = fn(p, a);
  hookAudioSourceCacheInit.Remove();

  init = true;
  Module::LoadAll();

  return ret;
};

bool MyPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
  Interfaces::Load(interfaceFactory);

  MH_Initialize();
  ConVar_Register();

  if (!init) {
    void* ptrAudioSourceCacheInit = Sig::Scan(SIGPAT_AudioSourceCacheInit);
    hookAudioSourceCacheInit.Install(ptrAudioSourceCacheInit, &MyPlugin::AudioSourceCacheInit, this);
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
