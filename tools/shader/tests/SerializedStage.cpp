#include "../Serialized.hpp"
#include <array>
#include <stdexcept>

void CheckSerializedStageTool() {
    using namespace AnyBox::Shader::Tools;
    auto set = [](auto& data, std::size_t offset, std::uint32_t value) {
        for (std::size_t byte = 0; byte < 4; ++byte) data[offset + byte] = static_cast<std::byte>(value >> (byte * 8));
    };
    std::array<std::byte, 44> code{};
    for (std::size_t offset = 0; offset < 36; offset += 12) {
        set(code, offset, 0xD5800000);
        set(code, offset + 4, 0x03FE0100);
    }
    set(code, 36, 0xBF810000);
    set(code, 40, 0xFFFFFFFF);
    std::array<std::byte, 48> metadata{};
    set(metadata, 0, 2);
    set(metadata, 4, 36);
    set(metadata, 8, 10);
    set(metadata, 12, 0x440);
    set(metadata, 16, 0x43);

    for (auto wave : {32u, 64u}) {
        auto output = InspectSerializedStageInstructions(code, metadata, wave);
        if (output.find("instructions=4 blocks=1") == std::string::npos) throw std::runtime_error("Serialized stage instruction extent test failed");
    }
    auto reject = [](auto&& callable) {
        bool caught = false;
        try { callable(); } catch (const std::exception&) { caught = true; }
        if (!caught) throw std::runtime_error("Serialized stage tool accepted invalid input");
    };
    reject([&] { (void)InspectSerializedStageInstructions(code, metadata, 16); });
    set(code, 36, 0xFFFFFFFF);
    reject([&] { (void)InspectSerializedStageInstructions(code, metadata, 64); });
}
