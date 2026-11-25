target_sources(d3d12_xs PRIVATE src/device/SerializedCompute.cpp src/pipeline/native/Description.cpp src/pipeline/native/Prepared.cpp)
target_link_libraries(d3d12_xs PRIVATE anybox_shader_plan)
target_sources(d3d12_xs PRIVATE src/pipeline/native/Fixed.cpp)
