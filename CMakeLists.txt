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

if(WIN32)
    message("WIN")
    set(protobuf_source_dir "H:/protobuf")
    set(lua_source_dir "H:/lua-5.4.0/src")
    set(CMAKE_PREFIX_PATH "D:\\Qt\\Qt5.13.2/5.13.2\\msvc2017_64")
    set(runtime_output_dir "E:\\IGG\\COLX\\src\\Common\\WhiteBox")
elseif(APPLE)
    message("APPLE")
    set(protobuf_source_dir "~/Work/protobuf-3.12.3")
    set(lua_source_dir "~/Work/lua-5.4.0/src")
    set(CMAKE_PREFIX_PATH "~/work/Qt5.12.9/5.12.9/clang_64")
    set(runtime_output_dir "${PROJECT_SOURCE_DIR}/bin")
endif()

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
    add_definitions(-DHAVE_PTHREAD)
endif (CMAKE_USE_PTHREADS_INIT)

include_directories(thirdparty)

add_subdirectory(thirdparty)
add_subdirectory(src)

set_target_properties(${APP_NAME} PROPERTIES
    OUTPUT_NAME ${APP_NAME}
    DEBUG_POSTFIX "d"
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output_dir}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})