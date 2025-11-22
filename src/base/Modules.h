#pragma once

#include <list>

#include "Common.h"

class Module {
 public:
  Module() { GetModuleList().push_back(this); }
  virtual ~Module() { GetModuleList().remove(this); }

  static void LoadAll() {
    for (auto mod : GetModuleList()) {
      mod->Load();
      PluginMsg("Loaded %s Module\n", mod->GetModuleName());
    }
  }

  static void UnloadAll() {
    for (auto mod : GetModuleList()) {
      mod->Unload();
      PluginMsg("Unloaded %s Module\n", mod->GetModuleName());
    }
  }

  static size_t size() { return GetModuleList().size(); }

 protected:
  virtual void Load() = 0;
  virtual void Unload() = 0;
  virtual const char* GetModuleName() = 0;

 private:
  static std::list<Module*>& GetModuleList();
};