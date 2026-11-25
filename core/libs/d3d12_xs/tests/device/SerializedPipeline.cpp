#include "Tests.hpp"
#include "PreparedCache.hpp"
#include "../../src/pipeline/State.hpp"
#include "../../src/pipeline/native/Description.hpp"
#include "../../src/pipeline/native/Pipeline.hpp"
#include "SerializedReference.hpp"
#include "serialized/Prepare.hpp"
#include <algorithm>

namespace AnyBox::Graphics::Tests {
namespace {
void CheckSerializedPipelineMode(Device* device, std::uint32_t control) {
    SerializedTests::ReferenceFixture source;
    Fixtures::Set(source.records, 20, control);
    auto prepared = Shader::Tools::CompileSerializedCompute(source.Payload(), source.root, device->GetGpuContext().GetSubgroupProperties().subgroupSize);
    auto bytes = Shader::EncodeComputeArtifact(prepared.shader);
    auto resources = Shader::EncodeResourcePlan(prepared.resources, Shader::HashShader(std::span(bytes).first(80)));
    void* object = nullptr;
    Require(Method<decltype(&Device::CreateRootSignature)>(device, 16)(device, 0, source.root.data(), source.root.size(), RootSignatureId, &object) == S_OK, "Serialized pipeline root creation failed");
    auto releaseRoot = [](RootSignature* root) { Method<decltype(&RootSignature::Release)>(root, 2)(root); };
    std::unique_ptr<RootSignature, decltype(releaseRoot)> root(static_cast<RootSignature*>(object), releaseRoot);
    SerializedComputeDescription description{root.get(), source.dispatch.data(), source.dispatch.size(), nullptr, 0, source.code.data(), source.code.size(), source.records.data(), source.records.size()};
    auto create = Method<decltype(&Device::DeserializeComputePipelineStateX)>(device, 67);
    PreparedCache cache;
    void* output = reinterpret_cast<void*>(1);
    Reject<std::invalid_argument>([&] { create(device, nullptr, PipelineStateId, &output); });
    Require(output == nullptr, "Invalid serialized pipeline retained its output");
    Reject<std::invalid_argument>([&] { create(device, &description, PipelineStateId, nullptr); });
    Reject<std::invalid_argument>([&] { create(device, &description, Guid{}, &output); });
    Reject<std::runtime_error>([&] { create(device, &description, PipelineStateId, &output); });
    Require(output == nullptr, "Missing serialized pipeline cache retained its output");
    cache.Save(prepared.shader, resources);
    Require(create(device, &description, PipelineStateId, &output) == S_OK && output, "Serialized compute pipeline creation failed through slot 67");
    auto* pipeline = static_cast<PipelineState*>(output);
    Require(pipeline->GetNativeCompute() && !pipeline->GetOffline() && pipeline->IsOffline(), "Serialized pipeline selected an incorrect backend");
    Require(pipeline->GetRootSignature() == root.get(), "Serialized pipeline lost its original root signature");
    CheckInitialPipelines(device, pipeline);
    CheckPipelineBindings(device, pipeline);
    AnyBox::Graphics::Commands::RootState arguments(root->GetLayout());
    ShaderRecompiler::ResourceSnapshot snapshot;
    auto data = pipeline->GetNativeCompute()->GetArguments().Materialize(arguments, {}, snapshot);
    Require(std::ranges::all_of(data, [](auto value) { return value == 0; }), "Empty native shader produced unexpected user registers");
    Require(Method<decltype(&PipelineState::Release)>(pipeline, 2)(pipeline) == 0, "Serialized pipeline retained its last reference");

    auto invalid = description;
    invalid.codeSize -= 1;
    Reject<std::invalid_argument>([&] { create(device, &invalid, PipelineStateId, &output); });
    invalid = description;
    invalid.root = nullptr;
    Reject<std::invalid_argument>([&] { create(device, &invalid, PipelineStateId, &output); });
    invalid = description;
    invalid.dispatchSize = 0;
    Reject<std::invalid_argument>([&] { create(device, &invalid, PipelineStateId, &output); });
    std::array<std::byte, 20> auxiliary{};
    invalid = description;
    invalid.auxiliaryState = auxiliary.data();
    invalid.auxiliarySize = auxiliary.size();
    Reject<std::runtime_error>([&] { create(device, &invalid, PipelineStateId, &output); });
    auxiliary[4] = std::byte{3};
    invalid.auxiliarySize -= 1;
    Reject<std::runtime_error>([&] { create(device, &invalid, PipelineStateId, &output); });
    invalid.auxiliarySize = auxiliary.size();
    auxiliary[8] = std::byte{1};
    Reject<std::runtime_error>([&] { create(device, &invalid, PipelineStateId, &output); });
    resources.back() ^= std::byte{1};
    cache.Save(prepared.shader, resources);
    Reject<std::invalid_argument>([&] { create(device, &description, PipelineStateId, &output); });
    Require(output == nullptr, "Corrupt serialized resource cache retained its output");
}
}
}

void AnyBox::Graphics::Tests::CheckSerializedPipeline(Device* device) {
    CheckSerializedPipelineMode(device, 0x440);
    CheckSerializedPipelineMode(device, 0x8440);
}
