add_library(anybox_pipeline_stream STATIC Parser.cpp Schema.cpp Compute.cpp Graphics.cpp Defaults.cpp)
target_link_libraries(anybox_pipeline_stream PUBLIC anybox_common)
