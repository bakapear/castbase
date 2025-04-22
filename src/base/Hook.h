#pragma once

#include <MinHook.h>

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
Func (Instance::* MemberPointer<Instance, Func, Args...>::_func)(Args...) = nullptr;

template <typename Instance, typename Func, typename... Args>
Instance* MemberPointer<Instance, Func, Args...>::_instance = nullptr;

class Hook {
 public:
  void Create(void* src, void* dest) {
    MH_CreateHook(src, dest, &_trampoline);
    _src = src;
  }

  int Enable() { return MH_EnableHook(_src); }
  int Remove() { return MH_RemoveHook(_src); }

  int Install(void* src, void* dest) {
    Create(src, dest);
    return Enable();
  }

  template <typename Instance, typename Func, typename... Args>
  int Install(void* originalFunction, Func (Instance::*func)(Args...), Instance* instance) {
    MemberPointer<Instance, Func, Args...>::SetFunction(func);
    MemberPointer<Instance, Func, Args...>::SetInstance(instance);
    return Install(originalFunction, (void*)&MemberPointer<Instance, Func, Args...>::Forwarder);
  }

  template <typename Func>
  struct FnTraits;

  // Normal function
  template <typename Name, typename... Args>
  struct FnTraits<Name (*)(Args...)> {
    using FnType = Name (*)(Args...);
  };

  // Member function
  template <typename Name, typename Class, typename... Args>
  struct FnTraits<Name (Class::*)(Args...)> {
    using FnType = Name (*)(Args...);
  };

  template <typename Func>
  typename FnTraits<Func>::FnType GetTrampoline(Func function) {
    return reinterpret_cast<typename FnTraits<Func>::FnType>(_trampoline);
  }

 private:
  void* _src;
  void* _trampoline;
};