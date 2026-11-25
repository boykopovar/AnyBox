#include "Options.hpp"
#include <charconv>
#include <unordered_set>
#include <stdexcept>

namespace {
std::uint32_t Number(std::string_view value) {
    std::uint32_t number = 0;
    auto result = std::from_chars(value.data(), value.data() + value.size(), number);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) throw std::invalid_argument("Graphics shader option requires an unsigned decimal integer");
    return number;
}
}

AnyBox::Shader::Tools::GraphicsOptions AnyBox::Shader::Tools::ParseGraphicsOptions(std::span<const std::string_view> arguments) {
    GraphicsOptions result;
    std::unordered_set<std::string_view> seen;
    if (arguments.size() % 2 != 0) throw std::invalid_argument("Graphics shader options require a value for each named argument");
    for (std::size_t index = 0; index < arguments.size(); index += 2) {
        auto name = arguments[index];
        auto value = arguments[index + 1];
        if (value.empty()) throw std::invalid_argument("Graphics shader option value is empty");
        if (name != "--render-target" && !seen.insert(name).second) throw std::invalid_argument("Graphics shader option is duplicated");
        if (name == "--vertex") result.vertex = value;
        else if (name == "--pixel") result.pixel = value;
        else if (name == "--root") result.root = value;
        else if (name == "--output") result.output = value;
        else if (name == "--subgroup") result.layout.hostSubgroupSize = Number(value);
        else if (name == "--samples") result.layout.sampleCount = Number(value);
        else if (name == "--quality") result.layout.sampleQuality = Number(value);
        else if (name == "--depth-format") result.layout.depthStencilFormat = Number(value);
        else if (name == "--render-target") {
            if (result.layout.renderTargetCount == result.layout.renderTargetFormats.size()) throw std::length_error("Graphics shader render target count exceeds its capacity");
            auto format = Number(value);
            if (format == 0) throw std::invalid_argument("Graphics shader render target format is unknown");
            result.layout.renderTargetFormats[result.layout.renderTargetCount++] = format;
        } else throw std::invalid_argument("Graphics shader option is unknown");
    }
    if (result.vertex.empty() || result.root.empty() || result.output.empty()) throw std::invalid_argument("Graphics shader preparation requires --vertex, --root and --output");
    if (result.layout.hostSubgroupSize != 32 && result.layout.hostSubgroupSize != 64) throw std::invalid_argument("Graphics shader subgroup size must be 32 or 64");
    auto samples = result.layout.sampleCount;
    if (samples == 0 || samples > 64 || (samples & (samples - 1)) != 0) throw std::invalid_argument("Graphics shader sample count is invalid");
    return result;
}
