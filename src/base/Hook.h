#pragma once

#include <subhook.h>

template <typename Instance, typename Func, typename... Args>
class MemberPointer {
 public:
  static Func Forwarder(Args... args) { return (_instance->*_func)(args...); }
  static void SetFunction(Func (Instance::*func)(Args...)) { _func = func; }
  static void SetInstance(Instance* instance) { _instance = instance; }

 private:
  static Func (Instance::*_func)(Args...);
  static Instance* _instance;
};

template <typename Instance, typename Func, typename... Args>
Func (Instance::*MemberPointer<Instance, Func, Args...>::_func)(Args...) = nullptr;

template <typename Instance, typename Func, typename... Args>
Instance* MemberPointer<Instance, Func, Args...>::_instance = nullptr;

class Hook : public subhook::Hook {
 public:
  template <typename Instance, typename Func, typename... Args>
  bool InstallMember(void* src, Func (Instance::*func)(Args...), Instance* instance, subhook::HookFlags flags = subhook::HookNoFlags) {
    MemberPointer<Instance, Func, Args...>::SetFunction(func);
    MemberPointer<Instance, Func, Args...>::SetInstance(instance);
    return Install(src, (void*)&MemberPointer<Instance, Func, Args...>::Forwarder, flags);
  }
};