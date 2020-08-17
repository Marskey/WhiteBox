set(luabridge_includes
    detail/CFunctions.h
    detail/dump.h
    detail/LuaHelpers.h
    detail/Stack.h
    detail/ClassInfo.h
    detail/FuncTraits.h
    detail/LuaRef.h
    detail/TypeList.h
    detail/Config.h
    detail/Iterator.h
    detail/Namespace.h
    detail/TypeTraits.h
    detail/Constructor.h
    detail/LuaException.h
    detail/Security.h
    detail/Userdata.h
    List.h
    Map.h
    RefCountedPtr.h
    Vector.h
    LuaBridge.h
    RefCountedObject.h
    UnorderedMap.h
)

add_library(libluabridge ${luabridge_includes})
