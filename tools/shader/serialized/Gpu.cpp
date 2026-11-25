#include "Prepare.hpp"
#include "../Files.hpp"
#include "../Compilation.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <anybox/shader/SerializedComputeSource.hpp>

void AnyBox::Shader::Tools::CheckSerializedComputeGpu(const std::filesystem::path& input, const std::filesystem::path& root, const std::filesystem::path& directory, std::uint32_t subgroupSize) {
    auto data = ReadInput(input);
    Graphics::Container::FspxView view(data);
    auto key = HashSerializedComputeSource({view.GetDispatchState(), view.GetAuxiliaryState(), view.GetCode(), view.GetRecords()}, HashSerializedRootSignature(ReadInput(root)), subgroupSize);
    auto name = DigestName(key);
    auto bytes = ReadInput(directory / (name + ".abxc"));
    auto shader = DecodeComputeArtifact(bytes, key);
    (void)DecodeResourcePlan(ReadInput(directory / (name + ".abxr")), HashShader(std::span(bytes).first(80)), ShaderRecompiler::IrShaderStage::Compute);
    CheckArtifactGpu(shader);
}
