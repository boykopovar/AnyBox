#include "../graphics/Options.hpp"
#include "Files.hpp"
#include "../../../core/shader/tests/native/GraphicsFixture.hpp"
#include <anybox/ExceptionTest.hpp>

void CheckGraphicsOptions();

int main() {
    return AnyBox::Tests::Run([] {
        using namespace AnyBox::Shader;
        using namespace AnyBox::Shader::Tools;
        using namespace Fixtures;
        CheckGraphicsOptions();
        GraphicsFiles files;
        GraphicsOptions options;
        auto vertex = MakeGraphicsSource(false);
        auto pixel = MakeGraphicsSource(true);
        options.vertex = files.Save("vertex.dxbc", vertex);
        options.pixel = files.Save("pixel.dxbc", pixel);
        std::array<std::uint32_t, 6> rootWords{2, 0, 24, 0, 24, 0};
        auto rootBytes = AnyBox::Graphics::Container::BuildDxbc(std::array<AnyBox::Graphics::Container::Part, 1>{{{0x30535452, std::as_bytes(std::span(rootWords))}}});
        options.root = files.Save("root.dxbc", rootBytes);
        options.output = files.GetDirectory();
        options.layout.renderTargetCount = 1;
        options.layout.renderTargetFormats[0] = 28;
        auto layout = options.layout;
        layout.vertexSource = HashShader(vertex);
        layout.pixelSource = HashShader(pixel);
        auto root = AnyBox::Graphics::Container::ReadRootSignatureLayout(rootBytes);
        auto identity = HashGraphicsLayout(root, layout);
        GraphicsArtifactKey vertexKey{GraphicsArtifactStage::Vertex, layout.vertexSource, identity};
        GraphicsArtifactKey pixelKey{GraphicsArtifactStage::Pixel, layout.pixelSource, identity};
        auto vertexPath = files.Track(DigestName(HashGraphicsArtifactKey(vertexKey)) + ".abxg");
        auto pixelPath = files.Track(DigestName(HashGraphicsArtifactKey(pixelKey)) + ".abxg");
        Reject<std::runtime_error>([&] { PrepareGraphics(options, true); });
        PrepareGraphics(options, false);
        auto vertexBytes = ReadInput(vertexPath);
        auto pixelBytes = ReadInput(pixelPath);
        auto compiledVertex = DecodeGraphicsArtifact(vertexBytes, vertexKey);
        auto compiledPixel = DecodeGraphicsArtifact(pixelBytes, pixelKey);
        Require(compiledVertex.stage == GraphicsArtifactStage::Vertex && compiledPixel.stage == GraphicsArtifactStage::Pixel);
        Require(compiledVertex.userRegisters.empty() && compiledPixel.userRegisters.empty());
        PrepareGraphics(options, true);
        PrepareGraphics(options, false);
        Require(ReadInput(vertexPath) == vertexBytes && ReadInput(pixelPath) == pixelBytes);
        std::array<std::byte, 1> invalid{};
        WriteOutput(pixelPath, invalid);
        Reject<std::invalid_argument>([&] { PrepareGraphics(options, true); });
        Reject<std::invalid_argument>([&] { PrepareGraphics(options, false); });
        Require(ReadInput(pixelPath) == std::vector<std::byte>(invalid.begin(), invalid.end()));
        Require(std::filesystem::remove(pixelPath));
        Reject<std::runtime_error>([&] { PrepareGraphics(options, true); });
        PrepareGraphics(options, false);
        Require(ReadInput(vertexPath) == vertexBytes && ReadInput(pixelPath) == pixelBytes);
    });
}
