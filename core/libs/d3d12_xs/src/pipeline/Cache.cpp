#include "Offline.hpp"
#include <filesystem>
#include <fstream>

namespace {
std::optional<AnyBox::Shader::ComputeArtifact> ReadPreparedCompute(std::span<const std::byte> code, bool required) {
    auto length = GetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", nullptr, 0);
    if (length == 0) {
        if (required) throw std::runtime_error("ANYBOX_SHADER_CACHE must identify the prepared shader directory");
        return std::nullopt;
    }
    std::wstring directory(length, L'\0');
    auto written = GetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", directory.data(), length);
    if (written == 0 || written >= length) throw std::runtime_error("Prepared shader directory could not be read consistently");
    directory.resize(written);
    auto digest = AnyBox::Shader::HashShader(code);
    auto name = AnyBox::Shader::DigestName(digest) + ".abxc";
    auto path = std::filesystem::path(directory) / name;
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        if (!required && !std::filesystem::exists(path)) return std::nullopt;
        throw std::runtime_error("Prepared compute shader could not be opened: " + name);
    }
    auto size = input.tellg();
    if (size <= 0 || static_cast<std::uint64_t>(size) > UINT32_MAX) throw std::length_error("Prepared compute shader size is invalid");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), size)) throw std::runtime_error("Prepared compute shader could not be read completely");
    return AnyBox::Shader::DecodeComputeArtifact(bytes, digest);
}
}

AnyBox::Shader::ComputeArtifact AnyBox::Graphics::LoadPreparedCompute(std::span<const std::byte> code) {
    return std::move(*ReadPreparedCompute(code, true));
}

std::optional<AnyBox::Shader::ComputeArtifact> AnyBox::Graphics::FindPreparedCompute(std::span<const std::byte> code) {
    return ReadPreparedCompute(code, false);
}
