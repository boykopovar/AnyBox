add_library(anybox_shader_arguments STATIC src/dxil/arguments/Plan.cpp src/dxil/arguments/RootPlan.cpp src/dxil/arguments/RootMatch.cpp src/dxil/arguments/Writer.cpp)
target_sources(anybox_shader_arguments PRIVATE src/dxil/arguments/Tables.cpp)

target_sources(anybox_shader_arguments PRIVATE src/dxil/arguments/native/Binding.cpp src/dxil/arguments/native/Writer.cpp)
target_sources(anybox_shader_arguments PRIVATE src/dxil/arguments/native/Serialized.cpp)
target_link_libraries(anybox_shader_arguments PUBLIC anybox_container anybox_commands anybox_options)
target_include_directories(anybox_shader_arguments PUBLIC include)
