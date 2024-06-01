#include "Modules.h"

std::list<Module*>& Module::GetModuleList() {
  static std::list<Module*> list;
  return list;
}