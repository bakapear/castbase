#pragma once

#include <subhook.h>

template <typename T, typename Ret, typename... Args>
class MemberPointer {
 public:
  static Ret Forwarder(Args... args) { return (_instance->*_func)(args...); }
  static void SetFunction(Ret (T::*func)(Args...)) { _func = func; }
  static void SetInstance(T* instance) { _instance = instance; }

 private:
  static Ret (T::*_func)(Args...);
  static T* _instance;
};

template <typename T, typename Ret, typename... Args>
Ret (T::*MemberPointer<T, Ret, Args...>::_func)(Args...) = nullptr;

template <typename T, typename Ret, typename... Args>
T* MemberPointer<T, Ret, Args...>::_instance = nullptr;

class Hook : public subhook::Hook {
 public:
  template <typename T, typename Ret, typename... Args>
  bool InstallMember(void* src, Ret (T::*func)(Args...), T* instance, subhook::HookFlags flags = subhook::HookNoFlags) {
    MemberPointer<T, Ret, Args...>::SetFunction(func);
    MemberPointer<T, Ret, Args...>::SetInstance(instance);
    return Install(src, (void*)&MemberPointer<T, Ret, Args...>::Forwarder, flags);
  }
};