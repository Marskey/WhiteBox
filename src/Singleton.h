#pragma once

template <typename T>
class CSingleton
{
public:
  static T& instance();
  CSingleton(const CSingleton& rhs) = delete;
  CSingleton& operator=(const CSingleton& rhs) = delete;
private:
  CSingleton();
  static T* m_obj;
};

template <typename T>
T* CSingleton<T>::m_obj = new T();

template <typename T>
T& CSingleton<T>::instance() {
  return *m_obj;
}
