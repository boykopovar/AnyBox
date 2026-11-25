#include "Stages.hpp"
#include <algorithm>
#include <anybox/graphics/container/Dxil.hpp>

std::uint32_t AnyBox::Shader::NativeGraphics::UserRegisterCount(const Graphics::Container::NativeBindings& bindings) {
    std::uint32_t count = 0;
    for (const auto& root : bindings.roots) {
        if (root.registerCount == 0 || root.registerCount > 256u - root.firstUserRegister) throw std::invalid_argument("Native graphics user register range is invalid");
        if (root.firstUserRegister >= 16) count = std::max(count, 2u);
        else count = std::max(count, std::min(16u, root.firstUserRegister + root.registerCount));
    }
    return count;
}

AnyBox::Shader::GraphicsCompilation AnyBox::Shader::CompileNativeGraphics(std::span<const std::byte> vertex, std::span<const std::byte> pixel, const Graphics::Container::RootSignatureLayout& root, const GraphicsShaderLayout& layout, const ShaderRecompiler::SpirvTargetOptions& target) {
    if (vertex.empty() || layout.vertexSource != HashShader(vertex) || (pixel.empty() ? layout.pixelSource != ShaderDigest{} : layout.pixelSource != HashShader(pixel))) throw std::invalid_argument("Native graphics compilation source hashes disagree with the pipeline layout");
    if (target.subgroupSize != layout.hostSubgroupSize) throw std::invalid_argument("Native graphics compilation subgroup size disagrees with the pipeline layout");
    auto checkStage = [](std::span<const std::byte> code, std::uint32_t stage) {
        Graphics::Container::DxbcView container(code);
        auto part = container.FindPart(0x4C495844);
        if (!part || (Graphics::Container::DxilView(part->data).GetProgramVersion() >> 16) != stage) throw std::invalid_argument("Native graphics source container has the wrong shader stage");
    };
    checkStage(vertex, 1);
    if (!pixel.empty()) checkStage(pixel, 0);
    auto identity = HashGraphicsLayout(root, layout);
    auto vertexPrograms = Graphics::Container::UnpackNativePrograms(vertex);
    if (vertexPrograms.size() != 1) throw std::runtime_error("Native graphics compilation requires one vertex hardware program");
    auto compiled = NativeGraphics::CompileVertex(vertexPrograms.front(), root, {GraphicsArtifactStage::Vertex, layout.vertexSource, identity}, target);
    GraphicsCompilation result{std::move(compiled), std::nullopt};
    if (!pixel.empty()) {
        auto pixelPrograms = Graphics::Container::UnpackNativePrograms(pixel);
        if (pixelPrograms.size() != 1) throw std::runtime_error("Native graphics compilation requires one pixel hardware program");
        auto outputs = Graphics::Container::ReadNativeVertexOutputs(vertexPrograms.front());
        result.pixel = NativeGraphics::CompilePixel(pixelPrograms.front(), outputs, root, layout, {GraphicsArtifactStage::Pixel, layout.pixelSource, identity}, target);
    }
    return result;
}
