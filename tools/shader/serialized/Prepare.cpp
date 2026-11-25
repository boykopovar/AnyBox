#include "Prepare.hpp"
#include "../Files.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <anybox/shader/SerializedComputeSource.hpp>
#include <algorithm>
#include <iostream>

void AnyBox::Shader::Tools::PrepareSerializedCompute(const std::filesystem::path& input, const std::filesystem::path& root, const std::filesystem::path& directory, std::uint32_t subgroupSize) {
    auto data = ReadInput(input);
    auto signature = ReadInput(root);
    Graphics::Container::FspxView view(data);
    auto key = HashSerializedComputeSource({view.GetDispatchState(), view.GetAuxiliaryState(), view.GetCode(), view.GetRecords()}, HashSerializedRootSignature(signature), subgroupSize);
    auto name = DigestName(key);
    auto shaderPath = directory / (name + ".abxc");
    auto planPath = directory / (name + ".abxr");
    auto shaderExists = std::filesystem::exists(shaderPath);
    auto planExists = std::filesystem::exists(planPath);
    if (shaderExists != planExists) throw std::runtime_error("Prepared serialized compute cache entry is incomplete");

    if (shaderExists) {
        auto bytes = ReadInput(shaderPath);
        (void)DecodeComputeArtifact(bytes, key);
        (void)DecodeResourcePlan(ReadInput(planPath), HashShader(std::span(bytes).first(80)), ShaderRecompiler::IrShaderStage::Compute);
    } else {
        auto compiled = CompileSerializedCompute(data, signature, subgroupSize);
        auto shader = EncodeComputeArtifact(compiled.shader);
        auto plan = EncodeResourcePlan(compiled.resources, HashShader(std::span(shader).first(80)));
        std::filesystem::create_directories(directory);
        auto staging = directory / (name + ".prepare");
        if (!std::filesystem::create_directory(staging)) throw std::runtime_error("Prepared serialized compute staging directory already exists");
        try {
            WriteOutput(staging / "shader", shader);
            WriteOutput(staging / "plan", plan);
            if (std::filesystem::exists(shaderPath) || std::filesystem::exists(planPath)) throw std::runtime_error("Prepared serialized compute cache entry appeared during preparation");
            std::filesystem::rename(staging / "shader", shaderPath);
            std::filesystem::rename(staging / "plan", planPath);
            std::filesystem::remove(staging);
        } catch (...) {
            std::filesystem::remove(staging / "shader");
            std::filesystem::remove(staging / "plan");
            std::filesystem::remove(staging);
            throw;
        }
    }
    std::cout << "source=" << name << " cached=" << shaderExists << '\n';
    if (!std::cout) throw std::runtime_error("Cannot write serialized compute preparation results");
}
