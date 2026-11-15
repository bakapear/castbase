#pragma once

#include <list>

#include "Common.h"

class Module {
 public:
  Module() { GetModuleList().push_back(this); }
  virtual ~Module() { GetModuleList().remove(this); }

  static void LoadAll() {
    PluginMsg("Loading modules...\n");

    int loaded = 0;

    for (auto mod : GetModuleList()) {
      if (mod->Load()) {
        loaded++;
        PluginSuccess("+ %s\n", mod->GetModuleName());
      } else {
        PluginWarning("- %s\n", mod->GetModuleName());
      }
    }

    PluginMsg("Loaded %i/%i modules!\n", loaded, Module::size());
  }

  static void UnloadAll() {
    PluginMsg("Unloading modules...\n");

    for (auto mod : GetModuleList()) {
      mod->Unload();
      PluginWarning("- %s\n", mod->GetModuleName());
    }

    PluginMsg("Unloaded %i modules!\n", Module::size());
  }

  static size_t size() { return GetModuleList().size(); }

 protected:
  virtual bool Load() = 0;
  virtual void Unload() = 0;
  virtual const char* GetModuleName() = 0;

 private:
  static std::list<Module*>& GetModuleList();
};