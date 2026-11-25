#include "Options.hpp"
#include "../Files.hpp"
#include "../AuditTarget.hpp"
#include <anybox/shader/GraphicsCompiler.hpp>
#include <anybox/graphics/container/RootSignature.hpp>
#include <iostream>

void AnyBox::Shader::Tools::PrepareGraphics(const GraphicsOptions& options, bool verify) {
    auto vertex = ReadInput(options.vertex);
    auto pixel = options.pixel.empty() ? std::vector<std::byte>{} : ReadInput(options.pixel);
    auto root = Graphics::Container::ReadRootSignatureLayout(ReadInput(options.root));
    auto layout = options.layout;
    layout.vertexSource = HashShader(vertex);
    if (!pixel.empty()) layout.pixelSource = HashShader(pixel);
    auto identity = HashGraphicsLayout(root, layout);
    GraphicsArtifactKey vertexKey{GraphicsArtifactStage::Vertex, layout.vertexSource, identity};
    GraphicsArtifactKey pixelKey{GraphicsArtifactStage::Pixel, layout.pixelSource, identity};
    auto path = [&](const GraphicsArtifactKey& key) { return options.output / (DigestName(HashGraphicsArtifactKey(key)) + ".abxg"); };
    auto vertexPath = path(vertexKey);
    auto pixelPath = path(pixelKey);
    bool vertexCached = std::filesystem::exists(vertexPath);
    bool pixelCached = pixel.empty() || std::filesystem::exists(pixelPath);
    if (vertexCached) (void)DecodeGraphicsArtifact(ReadInput(vertexPath), vertexKey);
    if (!pixel.empty() && pixelCached) (void)DecodeGraphicsArtifact(ReadInput(pixelPath), pixelKey);
    if (verify && (!vertexCached || !pixelCached)) throw std::runtime_error("Prepared graphics shader is missing");
    if (!vertexCached || !pixelCached) {
        auto target = AuditTarget();
        target.subgroupSize = layout.hostSubgroupSize;
        std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
        capabilities.push_back(spv::CapabilityClipDistance);
        capabilities.push_back(spv::CapabilityCullDistance);
        target.supportedCapabilities = capabilities;
        auto compiled = CompileNativeGraphics(vertex, pixel, root, layout, target);
        auto vertexBytes = EncodeGraphicsArtifact(compiled.vertex);
        auto pixelBytes = compiled.pixel ? EncodeGraphicsArtifact(*compiled.pixel) : std::vector<std::byte>{};
        std::filesystem::create_directories(options.output);
        if (!vertexCached) WriteOutput(vertexPath, vertexBytes);
        if (!pixelCached) WriteOutput(pixelPath, pixelBytes);
    }
    for (const auto& key : pixel.empty() ? std::vector{vertexKey} : std::vector{vertexKey, pixelKey}) {
        auto file = path(key);
        auto artifact = DecodeGraphicsArtifact(ReadInput(file), key);
        std::cout << "stage=" << static_cast<std::uint32_t>(artifact.stage) << " file=" << file.filename().string() << " words=" << artifact.code.size() << " arguments=" << artifact.userRegisters.size() << " push_bytes=" << artifact.pushConstantSize << " wave=" << artifact.waveSize << " subgroup=" << artifact.hostSubgroupSize << '\n';
    }
    if (!std::cout) throw std::runtime_error("Cannot write graphics shader preparation results");
}
