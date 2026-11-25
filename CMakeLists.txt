cmake_minimum_required(VERSION 3.25)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

project(AnyBox VERSION 0.1.0 LANGUAGES C CXX)

include(cmake/AnyBoxOptions.cmake)

set(ANYBOX_RUNTIME_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

include(CTest)
include(cmake/AnyBoxLibrary.cmake)

add_subdirectory(3rdparty)
include(cmake/OpusCompatibility.cmake)

add_subdirectory(core/common)
add_subdirectory(core/codecs)
add_subdirectory(core/graphics)
add_subdirectory(core/shader)
add_subdirectory(core/libs)
add_subdirectory(core/relinker)
add_subdirectory(tools/relinker)
add_subdirectory(tools/shader)
