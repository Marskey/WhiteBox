cmake_minimum_required(VERSION 3.1.0)
set(APP_NAME "WhiteBox")
project(${APP_NAME} VERSION 0.1.0)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# specify the C++ standard 
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

set(protobuf_source_dir "H:/protobuf")
set(lua_source_dir "H:/lua-5.4.0/src")

set(CMAKE_PREFIX_PATH "D:\\Qt\\Qt5.13.2/5.13.2\\msvc2017_64")
find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)

include_directories(thirdparty)
include_directories(${protobuf_source_dir}/src)
include_directories(${lua_source_dir})

add_subdirectory(thirdparty)
add_subdirectory(src)

add_executable(${APP_NAME} WIN32 ${whitebox_sources})

target_link_libraries(${APP_NAME}
libprotobuf
fmt
cjson
liblua
libluabridge
Qt5::Widgets
Qt5::Core
Qt5::Gui
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})