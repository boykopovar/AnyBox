#include <anybox/shader/GraphicsLayout.hpp>
#include "../Binary.hpp"

AnyBox::Shader::ShaderDigest AnyBox::Shader::HashGraphicsLayout(const Graphics::Container::RootSignatureLayout& root, const GraphicsShaderLayout& layout) {
    if (root.version != 1 && root.version != 2) throw std::invalid_argument("Graphics shader root signature version is invalid");
    if ((root.flags & ~0x83000FFFu) != 0 || (root.flags & 0x80) != 0) throw std::invalid_argument("Graphics shader root signature flags are invalid");
    if (root.staticSamplerCount != 0) throw std::runtime_error("Graphics shader static sampler layout identity is not implemented");
    if (root.parameters.size() > 64) throw std::length_error("Graphics shader root parameter count exceeds its capacity");
    if (layout.targetProfile != 1 || (layout.hostSubgroupSize != 32 && layout.hostSubgroupSize != 64)) throw std::invalid_argument("Graphics shader target profile or subgroup size is invalid");
    if (layout.renderTargetCount > layout.renderTargetFormats.size() || layout.sampleCount == 0 || layout.sampleCount > 64 || (layout.sampleCount & (layout.sampleCount - 1)) != 0) throw std::invalid_argument("Graphics shader render target or sample count is invalid");
    ArtifactFormat::Writer writer;
    constexpr std::array<std::byte, 8> domain{std::byte{'A'}, std::byte{'B'}, std::byte{'X'}, std::byte{'G'}, std::byte{'L'}, std::byte{'0'}, std::byte{'0'}, std::byte{'1'}};
    writer.Bytes(domain);
    writer.Bytes(layout.vertexSource);
    writer.Bytes(layout.pixelSource);
    writer.Word(layout.targetProfile);
    writer.Word(layout.hostSubgroupSize);
    for (std::size_t index = 0; index < layout.renderTargetFormats.size(); ++index) {
        auto format = layout.renderTargetFormats[index];
        if ((index < layout.renderTargetCount) != (format != 0)) throw std::invalid_argument("Graphics render target format array is not canonical");
        writer.Word(format);
    }
    for (auto value : {layout.renderTargetCount, layout.depthStencilFormat, layout.sampleCount, layout.sampleQuality}) writer.Word(value);
    writer.Word(root.version);
    writer.Word(root.flags);
    writer.Word(static_cast<std::uint32_t>(root.parameters.size()));
    for (const auto& parameter : root.parameters) {
        if (static_cast<std::uint32_t>(parameter.kind) > 4 || parameter.visibility > 7 || (parameter.flags & ~0xEu) != 0) throw std::invalid_argument("Graphics root parameter layout is invalid");
        if (parameter.ranges.size() > UINT32_MAX) throw std::length_error("Graphics root descriptor range count exceeds its size field");
        for (auto value : {static_cast<std::uint32_t>(parameter.kind), parameter.visibility, parameter.shaderRegister, parameter.space, parameter.wordCount, parameter.flags, static_cast<std::uint32_t>(parameter.ranges.size())}) writer.Word(value);
        for (const auto& range : parameter.ranges) {
            if (static_cast<std::uint32_t>(range.kind) > 3 || range.count == 0 || (range.flags & ~0x1000Fu) != 0) throw std::invalid_argument("Graphics root descriptor range layout is invalid");
            for (auto value : {static_cast<std::uint32_t>(range.kind), range.count, range.firstRegister, range.space, range.flags, range.tableOffset}) writer.Word(value);
        }
    }
    return HashShader(writer.Take());
}
