#include "Audit.hpp"
#include "NativeAudit.hpp"
#include <anybox/graphics/container/Dxil.hpp>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t Integer(const std::array<std::byte, 8>& data, unsigned offset) {
    std::uint32_t value = 0;
    for (unsigned index = 0; index < 4; ++index) value |= std::to_integer<std::uint32_t>(data[offset + index]) << (index * 8);
    return value;
}
}

int AnyBox::Shader::Tools::AuditStream() {
    SetBinaryInput();
    std::size_t count = 0;
    std::size_t compiled = 0;
    std::array<std::byte, 8> header;
    while (std::cin.read(reinterpret_cast<char*>(header.data()), header.size())) {
        auto kind = Integer(header, 0);
        auto size = Integer(header, 4);
        if (kind > 1 || size == 0) throw std::invalid_argument("Shader audit record header is invalid");
        std::vector<std::byte> data(size);
        if (!std::cin.read(reinterpret_cast<char*>(data.data()), size)) throw std::runtime_error("Shader audit record is truncated");
        std::string result;
        try {
            if (kind == 1) throw std::runtime_error("Xbox native pipeline metadata frontend is not implemented");
            Graphics::Container::DxbcView container(data);
            auto part = container.FindPart(0x4C495844);
            if (!part) throw std::runtime_error("Legacy shader bytecode frontend is not implemented");
            Graphics::Container::DxilView dxil(part->data);
            if (!dxil.HasBitcode()) {
                auto words = AuditNative(data);
                ++compiled;
                result = "compiled\t" + std::to_string(words);
            } else {
                if ((dxil.GetProgramVersion() >> 16) != 5) throw std::runtime_error("Graphics shader DXIL frontend is not implemented");
                auto words = AuditCompute(data);
                ++compiled;
                result = "compiled\t" + std::to_string(words);
            }
        } catch (const std::exception& error) {
            std::string diagnostic(error.what());
            for (auto& character : diagnostic) {
                if (character == '\n' || character == '\r' || character == '\t') character = ' ';
            }
            result = "failed\t" + diagnostic;
        }
        std::cout << "record\t" << count++ << '\t' << kind << '\t' << result << '\n' << std::flush;
        if (!std::cout) throw std::runtime_error("Cannot write shader audit result");
    }
    if (std::cin.bad() || std::cin.gcount() != 0) throw std::runtime_error("Shader audit stream header is truncated");
    if (count == 0) throw std::invalid_argument("Shader audit stream is empty");
    std::cout << "summary\t" << count << '\t' << compiled << '\n';
    if (!std::cout) throw std::runtime_error("Cannot write shader audit summary");
    return compiled == count ? 0 : 2;
}
