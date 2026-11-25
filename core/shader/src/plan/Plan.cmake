add_library(anybox_shader_plan STATIC src/plan/Encode.cpp src/plan/Decode.cpp)
target_sources(anybox_shader_plan PRIVATE src/plan/Registers.cpp src/plan/NativeArguments.cpp)

target_link_libraries(anybox_shader_plan PUBLIC anybox_shader_artifact anybox_shader_recompiler PRIVATE anybox_options)
target_link_libraries(anybox_shader_plan PUBLIC anybox_shader_arguments)

target_include_directories(anybox_shader_plan PUBLIC include)

if(BUILD_TESTING)
    add_executable(shader_resource_plan_tests tests/plan/Main.cpp "${PROJECT_SOURCE_DIR}/core/graphics/container/tests/NativeFixtures.cpp")
    target_sources(shader_resource_plan_tests PRIVATE tests/plan/NativeArguments.cpp)
    target_link_libraries(shader_resource_plan_tests PRIVATE anybox_shader_plan anybox_native_shader anybox_test_support libdeflate_static)
    target_include_directories(shader_resource_plan_tests PRIVATE ../graphics/container/tests)
    target_compile_definitions(shader_resource_plan_tests PRIVATE ANYBOX_LIBRARY_NAME="shader_resource_plan")
    set_target_properties(shader_resource_plan_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ANYBOX_RUNTIME_DIRECTORY}")
    anybox_add_compiler_runtime(shader_resource_plan_tests)
    add_test(NAME shader.resource_plan COMMAND shader_resource_plan_tests)
    set_tests_properties(shader.resource_plan PROPERTIES TIMEOUT 15)
endif()
