#ifndef ADDON_UPDATER_BIND_CALLBACK_H
#define ADDON_UPDATER_BIND_CALLBACK_H

namespace addon_updater {
template <typename T>
struct ActualType {
  using type = T;
};
template <typename T>
struct ActualType<T*> {
  using type = typename ActualType<T>::type;
};
template <typename T, uint32_t uid, typename CallerType>
struct Callback;
template <typename Ret, typename... Params, uint32_t uid, typename CallerType>
struct Callback<Ret(Params...), uid, CallerType> {
  using InvokeCallback = Ret (*)(Params...);
  template <typename... Args>
  static Ret callback(Args... args) {
    return Function(args...);
  }

  static InvokeCallback GetCallback(std::function<Ret(Params...)> fn) {
    Function = fn;
    return static_cast<InvokeCallback>(
        Callback<Ret(Params...), uid, CallerType>::callback);
  }

  static std::function<Ret(Params...)> Function;
};

template <typename Ret, typename... Params, uint32_t uid, typename CallerType>
std::function<Ret(Params...)>
    Callback<Ret(Params...), uid, CallerType>::Function;

#define BIND_CALLBACK(ptr_type, caller_type) \
  Callback<ActualType<ptr_type>::type, __COUNTER__, caller_type>::GetCallback
}

#endif
