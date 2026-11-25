#include "../serialized/Prepare.hpp"
#include "SerializedReference.hpp"
#include "Files.hpp"
#include <anybox/ExceptionTest.hpp>
#include <anybox/shader/SerializedComputeSource.hpp>

namespace {
void Check(std::uint32_t subgroup) {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Shader::Tools;
    using namespace Fixtures;
    GraphicsFiles files;
    SerializedTests::ReferenceFixture fixture;
    auto payload = fixture.Payload();
    auto input = files.Save("input.fspx", payload);
    auto signature = files.Save("root.dxbc", fixture.root);
    auto compiled = CompileSerializedCompute(payload, fixture.root, subgroup);
    auto key = compiled.shader.sourceHash;
    Require(compiled.shader.hostSubgroupSize == subgroup && compiled.shader.waveSize == 32);
    Require(compiled.shader.threads == std::array<std::uint32_t, 3>{4, 2, 2});
    Require(compiled.shader.sharedMemoryBytes == 1024);
    auto shader = files.Track(DigestName(key) + ".abxc");
    auto plan = files.Track(DigestName(key) + ".abxr");
    PrepareSerializedCompute(input, signature, files.GetDirectory(), subgroup);
    auto shaderBytes = ReadInput(shader);
    auto planBytes = ReadInput(plan);
    auto restored = DecodeComputeArtifact(shaderBytes, key);
    Require(restored == compiled.shader);
    auto planIdentity = HashShader(std::span(shaderBytes).first(80));
    (void)DecodeResourcePlan(planBytes, planIdentity, ShaderRecompiler::IrShaderStage::Compute);
    auto shaderTime = std::filesystem::last_write_time(shader);
    auto planTime = std::filesystem::last_write_time(plan);
    PrepareSerializedCompute(input, signature, files.GetDirectory(), subgroup);
    Require(std::filesystem::last_write_time(shader) == shaderTime && std::filesystem::last_write_time(plan) == planTime);
    auto damaged = planBytes;
    damaged.back() ^= std::byte{1};
    WriteOutput(plan, damaged);
    Reject<std::invalid_argument>([&] { PrepareSerializedCompute(input, signature, files.GetDirectory(), subgroup); });
    Require(ReadInput(plan) == damaged && ReadInput(shader) == shaderBytes);
    WriteOutput(plan, planBytes);
    auto other = planIdentity;
    other[0] ^= std::byte{1};
    auto mismatched = EncodeResourcePlan(compiled.resources, other);
    WriteOutput(plan, mismatched);
    Reject<std::invalid_argument>([&] { PrepareSerializedCompute(input, signature, files.GetDirectory(), subgroup); });
    Require(std::filesystem::remove(plan));
    Reject<std::runtime_error>([&] { PrepareSerializedCompute(input, signature, files.GetDirectory(), subgroup); });
    Require(ReadInput(shader) == shaderBytes);
    Reject<std::invalid_argument>([&] { (void)CompileSerializedCompute(payload, fixture.root, 16); });
    fixture.records[0] ^= std::byte{1};
    Reject<std::runtime_error>([&] { (void)CompileSerializedCompute(fixture.Payload(), fixture.root, subgroup); });
}
}

int main() {
    return AnyBox::Tests::Run([] { Check(32); Check(64); });
}
