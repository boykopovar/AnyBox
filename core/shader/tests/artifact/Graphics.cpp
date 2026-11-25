#include <anybox/shader/GraphicsArtifact.hpp>
#include "Tests.hpp"
#include <algorithm>

namespace {
void Require(bool value) {
    if (!value) throw std::runtime_error("Graphics artifact test failed");
}

template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Graphics artifact accepted invalid data");
}

AnyBox::Shader::GraphicsArtifact Make(AnyBox::Shader::GraphicsArtifactStage stage) {
    using namespace AnyBox::Shader;
    GraphicsArtifact artifact;
    artifact.stage = stage;
    constexpr std::array<std::byte, 3> source{std::byte{1}, std::byte{2}, std::byte{3}};
    artifact.sourceHash = HashShader(source);
    artifact.layoutHash = HashShader({});
    artifact.waveSize = 64;
    artifact.hostSubgroupSize = 32;
    artifact.pushConstantOffset = 4;
    artifact.pushConstantSize = 64;
    artifact.pushDataStart = 1;
    artifact.memoryOffset = 2;
    artifact.memoryOffsetCount = 1;
    artifact.runtimeStateDwords = 1;
    artifact.userRegisters = {0, 4};
    artifact.descriptors = {{59, {}}, {60, {0, 1}}};
    artifact.buffers = {{ResourceClass::ShaderResource, 7, 2, 4, 1, BufferKind::Structured, 32}};
    artifact.code = stage == GraphicsArtifactStage::Vertex ? AnyBox::Vulkan::Tests::CompileShader("#version 450\nvoid main() { gl_Position = vec4(0,0,0,1); }", VK_SHADER_STAGE_VERTEX_BIT) : AnyBox::Vulkan::Tests::CompileShader("#version 450\nlayout(location=0) out vec4 color; void main() { color = vec4(0,1,0,1); }", VK_SHADER_STAGE_FRAGMENT_BIT);
    return artifact;
}
}

void CheckGraphicsArtifacts() {
    using namespace AnyBox::Shader;
    for (auto stage : {GraphicsArtifactStage::Vertex, GraphicsArtifactStage::Pixel}) {
        auto artifact = Make(stage);
        auto key = artifact.GetKey();
        auto bytes = EncodeGraphicsArtifact(artifact);
        Require(DecodeGraphicsArtifact(bytes, key) == artifact);
        Require(EncodeGraphicsArtifact(DecodeGraphicsArtifact(bytes, key)) == bytes);
        auto other = key;
        other.sourceHash[0] ^= std::byte{1};
        Require(HashGraphicsArtifactKey(other) != HashGraphicsArtifactKey(key));
        Reject([&] { DecodeGraphicsArtifact(bytes, other); });
        other = key;
        other.layoutHash[0] ^= std::byte{1};
        Require(HashGraphicsArtifactKey(other) != HashGraphicsArtifactKey(key));
        Reject([&] { DecodeGraphicsArtifact(bytes, other); });
        other = key;
        other.stage = stage == GraphicsArtifactStage::Vertex ? GraphicsArtifactStage::Pixel : GraphicsArtifactStage::Vertex;
        Require(HashGraphicsArtifactKey(other) != HashGraphicsArtifactKey(key));
        Reject([&] { DecodeGraphicsArtifact(bytes, other); });
        for (std::size_t size = 0; size < bytes.size(); ++size) Reject([&] { DecodeGraphicsArtifact(std::span(bytes).first(size), key); });
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            auto changed = bytes;
            changed[index] ^= std::byte{1};
            Reject([&] { DecodeGraphicsArtifact(changed, key); });
        }
        auto invalid = artifact;
        invalid.stage = other.stage;
        Reject([&] { EncodeGraphicsArtifact(invalid); });
        invalid = artifact;
        invalid.dispatchThreadLimit = true;
        Reject([&] { EncodeGraphicsArtifact(invalid); });
        auto mutate = [&](std::size_t word, std::uint32_t value) {
            auto changed = bytes;
            for (unsigned index = 0; index < 4; ++index) changed[80 + word * 4 + index] = static_cast<std::byte>(value >> (index * 8));
            auto hash = HashShader(std::span(changed).subspan(80));
            std::ranges::copy(hash, changed.begin() + 48);
            Reject([&] { DecodeGraphicsArtifact(changed, key); });
        };
        mutate(9, 2);
        mutate(10, 13);
        mutate(12, 128);
        mutate(15, 129);
        mutate(20, 2);
        mutate(23, UINT32_MAX);
        bytes.push_back(std::byte{});
        Reject([&] { DecodeGraphicsArtifact(bytes, key); });
    }
}
