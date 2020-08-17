cmake_minimum_required(VERSION 3.1.0)
set(APP_NAME "WhiteBox")
project(${APP_NAME} VERSION 3.1.5)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# specify the C++ standard 
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(protobuf_source_dir "H:/protobuf")
set(lua_source_dir "H:/lua-5.4.0/src")

set(CMAKE_PREFIX_PATH "D:\\Qt\\Qt5.13.2/5.13.2\\msvc2017_64")

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

include_directories(thirdparty)

add_subdirectory(thirdparty)
add_subdirectory(src)

set_target_properties(${APP_NAME} PROPERTIES
    OUTPUT_NAME ${APP_NAME}
    DEBUG_POSTFIX "d"
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})