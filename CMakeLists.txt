cmake_minimum_required(VERSION 3.25)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

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

if(BUILD_TESTING)
    add_executable(anybox_export_tests core/libs/tests/Exports.cpp)
    target_link_libraries(anybox_export_tests PRIVATE anybox_options)
    add_dependencies(anybox_export_tests libs)
    set_target_properties(anybox_export_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ANYBOX_RUNTIME_DIRECTORY}")
    add_test(NAME anybox.exports COMMAND anybox_export_tests)
    set_tests_properties(anybox.exports PROPERTIES TIMEOUT 15)
endif()
