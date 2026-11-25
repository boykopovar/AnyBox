#include "Cache.hpp"
#include <anybox/platform/Windows.hpp>
#include <filesystem>
#include <fstream>

std::vector<std::byte> AnyBox::Graphics::ReadPreparedArtifact(const std::string& name, const char* role) {
    auto length = GetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", nullptr, 0);
    if (length == 0) throw std::runtime_error("ANYBOX_SHADER_CACHE must identify the prepared shader directory");
    std::wstring directory(length, L'\0');
    auto written = GetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", directory.data(), length);
    if (written == 0 || written >= length) throw std::runtime_error("Prepared shader directory could not be read consistently");
    directory.resize(written);
    auto path = std::filesystem::path(directory) / name;
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error(std::string(role) + " is missing: " + name);
    auto size = input.tellg();
    if (size <= 0 || static_cast<std::uint64_t>(size) > UINT32_MAX) throw std::length_error(std::string(role) + " size is invalid");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), size)) throw std::runtime_error(std::string(role) + " could not be read completely");
    return bytes;
}
