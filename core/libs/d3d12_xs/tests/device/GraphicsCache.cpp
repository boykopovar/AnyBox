#include "PreparedCache.hpp"
#include "../../src/pipeline/Cache.hpp"
#include "../../../../graphics/vulkan/tests/Tests.hpp"

void CheckGraphicsCache() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    PreparedCache cache;
    GraphicsArtifact vertex;
    vertex.sourceHash = HashShader({});
    vertex.layoutHash = HashShader(std::as_bytes(std::span("layout", 6)));
    vertex.code = AnyBox::Vulkan::Tests::CompileShader("#version 450\nvoid main() { gl_Position = vec4(0,0,0,1); }", VK_SHADER_STAGE_VERTEX_BIT);
    auto key = vertex.GetKey();
    Reject<std::runtime_error>([&] { LoadPreparedGraphics(key); });
    cache.Save(vertex);
    Require(LoadPreparedGraphics(key) == vertex, "Prepared vertex shader changed during loading");

    auto other = key;
    other.layoutHash[0] ^= std::byte{1};
    Reject<std::runtime_error>([&] { LoadPreparedGraphics(other); });
    cache.Save(other, vertex);
    Reject<std::invalid_argument>([&] { LoadPreparedGraphics(other); });
    other = key;
    other.sourceHash[0] ^= std::byte{1};
    cache.Save(other, vertex);
    Reject<std::invalid_argument>([&] { LoadPreparedGraphics(other); });

    other = key;
    other.stage = GraphicsArtifactStage::Pixel;
    cache.Save(other, vertex);
    Reject<std::invalid_argument>([&] { LoadPreparedGraphics(other); });
    auto pixel = vertex;
    pixel.stage = GraphicsArtifactStage::Pixel;
    pixel.code = AnyBox::Vulkan::Tests::CompileShader("#version 450\nlayout(location=0) out vec4 color; void main() { color = vec4(0,1,0,1); }", VK_SHADER_STAGE_FRAGMENT_BIT);
    cache.Save(pixel);
    Require(LoadPreparedGraphics(pixel.GetKey()) == pixel, "Prepared pixel shader changed during loading");
    Require(LoadPreparedGraphics(key) == vertex, "Prepared pixel shader replaced the vertex artifact");
}
