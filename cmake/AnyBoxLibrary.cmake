include("${CMAKE_CURRENT_LIST_DIR}/CompilerRuntime.cmake")

function(anybox_export_symbols target definitionFile)
    get_filename_component(definitionPath "${definitionFile}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_link_options(${target} PRIVATE "/DEF:${definitionPath}")
    else()
        target_link_options(${target} PRIVATE "${definitionPath}")
    endif()
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${definitionPath}")
endfunction()

function(anybox_add_system_library target)
    add_library(${target} SHARED ${ARGN})
    target_compile_definitions(${target} PRIVATE ANYBOX_LIBRARY_NAME="${target}.dll")
    target_link_libraries(${target} PRIVATE anybox_common PUBLIC anybox_options)
    set_target_properties(${target} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${target}"
        RUNTIME_OUTPUT_DIRECTORY "${ANYBOX_RUNTIME_DIRECTORY}"
        PDB_OUTPUT_DIRECTORY "${ANYBOX_RUNTIME_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
    install(TARGETS ${target} RUNTIME DESTINATION . COMPONENT AnyBoxRuntime)
    anybox_add_compiler_runtime(${target})
    add_dependencies(libs ${target})
endfunction()

function(anybox_add_exception_test library)
    if(NOT BUILD_TESTING)
        return()
    endif()
    string(TOLOWER "${library}" testPrefix)
    set(testTarget "${testPrefix}_exception_tests")
    add_executable(${testTarget} tests/Exceptions.cpp)
    target_link_libraries(${testTarget} PRIVATE ${library} anybox_test_support)
    target_compile_definitions(${testTarget} PRIVATE ANYBOX_LIBRARY_NAME="$<TARGET_FILE_NAME:${library}>")
    set_target_properties(${testTarget} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ANYBOX_RUNTIME_DIRECTORY}")
    add_test(NAME "${testPrefix}.exceptions" COMMAND ${testTarget})
    set_tests_properties("${testPrefix}.exceptions" PROPERTIES TIMEOUT 15)
endfunction()
