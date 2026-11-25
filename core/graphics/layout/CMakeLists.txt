add_library(anybox_texture_layout STATIC src/Format.cpp src/Description.cpp src/Swizzle.cpp src/Surface.cpp)
target_link_libraries(anybox_texture_layout PUBLIC anybox_address anybox_options)
target_include_directories(anybox_texture_layout PUBLIC include)
