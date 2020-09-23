#ifndef ADDON_UPDATER_DEFER_H
#define ADDON_UPDATER_DEFER_H

namespace addon_updater {
#define CONCAT_INTERNAL(x, y) x##y
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)

template <typename T>
struct ExitScope {
  T lambda;
  ExitScope(T lambda) : lambda(lambda) {}
  ~ExitScope() { lambda(); }
  ExitScope(const ExitScope&);

 private:
  ExitScope& operator=(const ExitScope&);
};

class ExitScopeHelp {
 public:
  template <typename T>
  ExitScope<T> operator+(T t) {
    return t;
  }
};

#define defer const auto& CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()
}

#endif  // !ADDON_UPDATER_DEFER_H
