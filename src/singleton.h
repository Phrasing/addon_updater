#ifndef SINGLETON_H
#define SINGLETON_H
#pragma once

template <typename T>
class Singleton {
 public:
  static T& GetInstance();
  static T* GetInstancePtr();
  static void Destroy();

 protected:
  Singleton();

  ~Singleton();

 private:
  Singleton(const Singleton& rhs) = delete;
  void operator==(const Singleton& rhs) = delete;

 protected:
  static T* instance_;
};

template <typename T>
T* Singleton<T>::instance_ = nullptr;

template <typename T>
Singleton<T>::Singleton() {
  instance_ = static_cast<T*>(this);
}

template <typename T>
Singleton<T>::~Singleton() {
  instance_ = nullptr;
}

template <typename T>
T& Singleton<T>::GetInstance() {
  return *(GetInstancePtr());
}

template <typename T>
T* Singleton<T>::GetInstancePtr() {
  if (!instance_) {
    Singleton<T>::instance_ = new T();
  }

  return instance_;
}

template <typename T>
void Singleton<T>::Destroy() {
  delete Singleton<T>::instance_;
  Singleton<T>::instance_ = nullptr;
}

#endif  // !SINGLETON_H
