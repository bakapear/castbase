#include "Plugin.h"

#include <convar.h>

#include "Interfaces.h"
#include "Modules.h"

bool MyPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
  Interfaces::Load(interfaceFactory);

  ConVar_Register();
  Module::LoadAll();

  return true;
}

void MyPlugin::Unload() {
  Module::UnloadAll();
  ConVar_Unregister();

  Interfaces::Unload();
}
