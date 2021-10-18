#pragma once

#include <fstream>
#include <sstream>


#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include <string>

#include "Singleton.h"
#include "LogHelper.h"

class CLuaScriptSystem
{
public:
  CLuaScriptSystem() : m_L(nullptr) {}
  virtual ~CLuaScriptSystem() { Shutdown(); }

public:
  bool Setup() {
    // create state
    m_L = ::luaL_newstate();
    if (!m_L) return false;

    // open all library
    luaL_openlibs(m_L);
    return true;
  }

  void Shutdown() {
    // close lua state
    if (m_L) {
      lua_close(m_L);
      m_L = nullptr;
    }
  }

public:
  void RunScript(const char* script, size_t len, const char* file_path) {
    lua_pushcclosure(m_L, OnError, 0);
    int errfunc = lua_gettop(m_L);
    std::string show_path;
    if (file_path != nullptr) show_path = file_path;
    if (show_path.size() > 48) show_path = show_path.substr(show_path.size() - 48);
    if (luaL_loadbuffer(m_L, script, len, show_path.c_str()) == 0) {
      lua_pushstring(m_L, file_path);
      lua_pcall(m_L, 1, 1, errfunc);
    } else {
      PrintError(m_L, file_path, lua_tostring(m_L, -1));
    }
    lua_remove(m_L, errfunc);
    lua_pop(m_L, 1);
  }

  bool RunScript(const char* script_file_path) {
    bool bResult = false;
    do {
      if (luaL_loadfile(m_L, script_file_path) != 0) {
        LogHelper::instance().logError("Error: {}", lua_tostring(m_L, -1));
        break;
      }

      if (lua_pcall(m_L, 0, 0, -2) != 0) {
        LogHelper::instance().logError("Error: {}", lua_tostring(m_L, -1));
        break;
      }
      bResult = true;
    } while (false);

    if (!bResult) {
      std::stringstream ss;
      ss << "======= stack =======" << "\n";
      for (int i = 1; i <= lua_gettop(m_L); ++i) {
        ss << "@" << i << " = " << luabridge::LuaRef::fromStack(m_L, i) << "\n";
      }
      LogHelper::instance().logError(ss.str().c_str());
    }

    return bResult;
  }

private:
  static void CallStack(lua_State* L, int n, std::string& stack_info) {
    lua_Debug ar;
    if (lua_getstack(L, n, &ar) != 1) return;

    lua_getinfo(L, "nSlu", &ar);

    char text[32] = { 0 };
#ifdef _WIN32
    sprintf_s(text, "%d", ar.currentline);
#else
    sprintf(text, "%d", ar.currentline);
#endif

    if (ar.name) {
      stack_info.append(ar.name).append("() : line ").append(text).append(" [").append(ar.source).append("]\n");
    } else {
      stack_info.append("unknown : line ").append(text).append(" [").append(ar.source).append("]\n");
    }

    CallStack(L, n + 1, stack_info);
  }

  static int OnError(lua_State* L) {
    std::string content;
    const char* error = lua_tostring(L, -1);
    if (error != nullptr) content = error;
    lua_pop(L, 1);

    content.append("\n");
    CallStack(L, 0, content);
    //printf("%s\n", content.c_str());
    LogHelper::instance().logError("Error: lua error -- {}", content.c_str());

    return 0;
  }
  static void PrintError(lua_State* L, const char* name, const char* message) {

    std::string content = name;
    content.append("\n");
    content.append(message);
    lua_pop(L, 1);
    content.append("\n");
    CallStack(L, 0, content);
    //printf("%s\n", content.c_str());
    LogHelper::instance().logError("Error: lua error -- {}", content.c_str());
  }

  inline int InvokePush(int count) {
    return count;
  }

  template<typename T1, typename ...T2>
  inline int InvokePush(int count, T1 arg, T2 ...args) {
    luabridge::push(m_L, arg);
    return InvokePush(count + 1, args...);
  }

public:
  template<typename R, typename ...T>
  R Invoke(const char* name, T ...args) {
    R r = R();
    if (m_L == nullptr) return r;

    lua_pushcclosure(m_L, OnError, 0);
    int errfunc = lua_gettop(m_L);

    lua_getglobal(m_L, name);
    if (lua_isfunction(m_L, -1)) {
      int count = InvokePush(0, args...);
      lua_pcall(m_L, count, 1, errfunc);
      if (!lua_isnil(m_L, -1)) {
        r = luabridge::Stack<R>::get(m_L, -1);
      }

      lua_pop(m_L, 1);
    } else {
      std::string content;
      content.append("attempt to call global '");
      content.append(name);
      content.append("' (not a function)");
      //printf("%s\n", content.c_str());
      LogHelper::instance().logError("Error: lua invoke error -- {}", content.c_str());
    }

    lua_remove(m_L, errfunc);
    return r;
  }

  template<typename ...T>
  void Invoke(const char* name, T ...args) {
    if (m_L == nullptr) return;

    lua_pushcclosure(m_L, OnError, 0);
    int errfunc = lua_gettop(m_L);

    lua_getglobal(m_L, name);
    if (lua_isfunction(m_L, -1)) {
      int count = InvokePush(0, args...);
      lua_pcall(m_L, count, 0, errfunc);
    } else {
      std::string content;
      content.append("attempt to call global '");
      content.append(name);
      content.append("' (not a function)");
      //printf("%s\n", content.c_str());
      LogHelper::instance().logError("Error: lua invoke error -- {}", content.c_str());
    }

    lua_remove(m_L, errfunc);
  }

public:
  lua_State* GetLuaState() { return m_L; }

protected:
  lua_State* m_L;                 // lua state
};

typedef CSingleton<CLuaScriptSystem> LuaScriptSystem;
