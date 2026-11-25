#include "../Tests.hpp"
#include <anybox/vulkan/Spirv.hpp>

namespace {
template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    bool caught = false;
    try { callable(); } catch (const TException&) { caught = true; }
    AnyBox::Vulkan::Tests::Require(caught, "Invalid SPIR-V push constant layout was accepted");
}
}

void CheckSpirvPushConstants() {
    using namespace AnyBox::Vulkan;
    using namespace AnyBox::Vulkan::Tests;
    struct Case { const char* source; std::uint32_t offset; std::uint32_t size; };
    constexpr std::array cases{
        Case{R"(#version 450
layout(push_constant) uniform Data { uint values[32]; } data;
void main() { gl_Position = vec4(float(data.values[0])); }
)", 0, 128},
        Case{R"(#version 450
layout(push_constant) uniform Data { layout(offset=16) vec4 colors[2]; layout(offset=64) mat2 matrix; } data;
void main() { gl_Position = data.colors[0] + vec4(data.matrix[0],0,0); }
)", 16, 64},
        Case{R"(#version 450
layout(push_constant,row_major) uniform Data { mat2x3 matrix; } data;
void main() { gl_Position = vec4(data.matrix[0],1); }
)", 0, 24},
        Case{R"(#version 450
layout(push_constant,column_major) uniform Data { mat2x3 matrix; } data;
void main() { gl_Position = vec4(data.matrix[0],1); }
)", 0, 32},
        Case{R"(#version 450
struct Inner { uvec2 words; float scalar; };
layout(push_constant) uniform Data { Inner records[2]; } data;
void main() { gl_Position = vec4(vec2(data.records[1].words),data.records[0].scalar,1); }
)", 0, 32},
        Case{R"(#version 450
layout(push_constant) uniform Data { layout(offset=20) float value; } data;
void main() { gl_Position = vec4(data.value); }
)", 20, 4}
    };
    for (const auto& item : cases) {
        auto code = CompileShader(item.source, VK_SHADER_STAGE_VERTEX_BIT);
        auto range = ReadSpirvPushConstants(code);
        Require(range.stageFlags == VK_SHADER_STAGE_VERTEX_BIT && range.offset == item.offset && range.size == item.size, "SPIR-V declared push constant range is incorrect");
        Reject<std::invalid_argument>([&] { ReadSpirvPushConstants(code, "missing"); });
        code[5] = 0;
        Reject<std::invalid_argument>([&] { ReadSpirvPushConstants(code); });
    }
    auto empty = CompileShader("#version 450\nvoid main() { gl_Position = vec4(0,0,0,1); }", VK_SHADER_STAGE_VERTEX_BIT);
    auto range = ReadSpirvPushConstants(empty);
    Require(range.stageFlags == VK_SHADER_STAGE_VERTEX_BIT && range.size == 0, "SPIR-V module without push constants declares a range");
}
