#include "../graphics/Options.hpp"
#include "../../../core/graphics/container/tests/Fixtures.hpp"

void CheckGraphicsOptions() {
    using namespace AnyBox::Shader::Tools;
    using namespace Fixtures;
    constexpr std::array<std::string_view, 14> valid{"--vertex", "v", "--pixel", "p", "--root", "r", "--output", "o", "--render-target", "28", "--render-target", "10", "--subgroup", "64"};
    auto options = ParseGraphicsOptions(valid);
    Require(options.vertex == "v" && options.pixel == "p" && options.root == "r" && options.output == "o");
    Require(options.layout.hostSubgroupSize == 64 && options.layout.renderTargetCount == 2 && options.layout.renderTargetFormats[1] == 10);
    Reject<std::invalid_argument>([&] { ParseGraphicsOptions({}); });
    Reject<std::invalid_argument>([&] { ParseGraphicsOptions(std::span(valid).first(13)); });
    auto reject = [&](std::string_view name, std::string_view value) {
        std::vector<std::string_view> arguments(valid.begin(), valid.end());
        arguments.push_back(name);
        arguments.push_back(value);
        Reject<std::invalid_argument>([&] { ParseGraphicsOptions(arguments); });
    };
    reject("--vertex", "duplicate");
    reject("--unknown", "1");
    reject("--samples", "3");
    reject("--samples", "0");
    reject("--quality", "-1");
    reject("--quality", "4294967296");
    reject("--quality", "1x");
    reject("--render-target", "0");
    reject("--depth-format", "");
    std::vector<std::string_view> excessive(valid.begin(), valid.end());
    for (unsigned index = 0; index < 7; ++index) { excessive.push_back("--render-target"); excessive.push_back("28"); }
    Reject<std::length_error>([&] { ParseGraphicsOptions(excessive); });
}
