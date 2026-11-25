function(anybox_add_opus)
    set(BUILD_TESTING OFF)
    set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
    set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
    set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
    add_subdirectory(opus)
    set_target_properties(opus PROPERTIES MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")
endfunction()

anybox_add_opus()

function(anybox_add_libdeflate)
    set(LIBDEFLATE_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
    set(LIBDEFLATE_BUILD_GZIP OFF CACHE BOOL "" FORCE)
    set(LIBDEFLATE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(LIBDEFLATE_GZIP_SUPPORT OFF CACHE BOOL "" FORCE)
    add_subdirectory(libdeflate)
endfunction()

anybox_add_libdeflate()

function(anybox_add_zlib)
    set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
    set(ZLIB_PREFIX anybox_z_ CACHE STRING "" FORCE)
    add_subdirectory(zlib)
endfunction()

anybox_add_zlib()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/FFmpeg.cmake")
add_subdirectory(Vulkan-Headers EXCLUDE_FROM_ALL)

function(anybox_add_glslang)
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_EXTERNAL OFF)
    set(GLSLANG_TESTS OFF)
    set(GLSLANG_ENABLE_INSTALL OFF)
    set(ENABLE_GLSLANG_BINARIES OFF)
    set(ENABLE_HLSL OFF)
    set(ENABLE_SPIRV ON)
    set(ENABLE_OPT OFF)
    add_subdirectory(glslang EXCLUDE_FROM_ALL)
endfunction()

anybox_add_glslang()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Zydis.cmake")

anybox_add_zydis()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/AddrLib.cmake")
anybox_add_addrlib()
