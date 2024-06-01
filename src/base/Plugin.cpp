#include "Plugin.h"

#include <convar.h>

#include <chrono>

#include "Common.h"
#include "Interfaces.h"
#include "Modules.h"

class MyPlugin final : public Plugin {
 public:
  bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) override;
  void Unload() override;
  const char* GetPluginDescription() override { return PLUGIN_DESC; }
};

static MyPlugin s_MyPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(MyPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, s_MyPlugin);

bool MyPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
  Interfaces::Load(interfaceFactory);

  Module::LoadAll();

  ConVar_Register();

  PluginMsg("Finished loading %d modules!\n", Module::size());

  return true;
}

void MyPlugin::Unload() {
  PluginMsg("Unloading plugin...\n");

  ConVar_Unregister();

  Module::UnloadAll();
  Interfaces::Unload();

  PluginMsg("Finished unloading!\n");
}
