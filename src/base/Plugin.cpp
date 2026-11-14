#include "Plugin.h"

#include <convar.h>

#include "Interfaces.h"
#include "Modules.h"

bool MyPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
  PluginOut("PLUGIN_LOADING");

  Interfaces::Load(interfaceFactory);

  ConVar_Register();
  Module::LoadAll();

  PluginOut("PLUGIN_LOADED");

  return true;
}

void MyPlugin::Unload() {
  PluginOut("PLUGIN_UNLOADING");

  Module::UnloadAll();
  ConVar_Unregister();

  Interfaces::Unload();

  PluginOut("PLUGIN_UNLOADED");
}
