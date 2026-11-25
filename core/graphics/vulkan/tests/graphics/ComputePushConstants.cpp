#include "../Commands.hpp"
#include <anybox/vulkan/ComputePipeline.hpp>
#include <anybox/vulkan/Spirv.hpp>

void CheckComputeDeclaredPushConstants(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox::Vulkan;
    using namespace AnyBox::Vulkan::Tests;
    auto code = CompileCompute(R"(#version 450
layout(local_size_x=1) in;
layout(push_constant) uniform Data { uint values[32]; } data;
void main() { if (data.values[0] != 0) memoryBarrierBuffer(); }
)");
    auto range = ReadSpirvPushConstants(code);
    Require(range.size == 128, "Compute push constant fixture does not declare the full array");
    ComputePipeline pipeline(context, code, 4);
    Commands commands(context);
    auto buffer = commands.Begin();
    std::uint32_t value = 1;
    pipeline.Dispatch(buffer, std::as_bytes(std::span(&value, 1)), {1,1,1});
    commands.Submit(buffer);
}
