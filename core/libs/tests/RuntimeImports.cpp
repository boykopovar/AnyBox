#include <windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
constexpr std::array runtimePrefixes{std::string_view("msvcp"), std::string_view("vcruntime"), std::string_view("concrt"), std::string_view("vccorlib")};

bool IsCompilerRuntime(std::string name) {
    std::ranges::transform(name, name.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return std::ranges::any_of(runtimePrefixes, [&](std::string_view prefix) { return name.starts_with(prefix); });
}

void CheckName(const std::filesystem::path& path, const std::byte* base, std::uint32_t address) {
    std::string name(reinterpret_cast<const char*>(base + address));
    if (IsCompilerRuntime(name)) throw std::runtime_error(path.filename().string() + " imports " + name);
}

void CheckImports(const std::filesystem::path& path) {
    auto release = [](HMODULE module) { FreeLibrary(module); };
    std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(release)> module(LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_AS_IMAGE_RESOURCE), release);
    if (!module) throw std::runtime_error("Cannot map " + path.string());
    auto base = reinterpret_cast<const std::byte*>(reinterpret_cast<std::uintptr_t>(module.get()) & ~std::uintptr_t{3});
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) throw std::runtime_error(path.string() + " has no DOS header");
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) throw std::runtime_error(path.string() + " is not a PE32+ image");
    const auto& directories = nt->OptionalHeader.DataDirectory;
    if (auto address = directories[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress; address != 0) {
        for (auto entry = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + address); entry->Name != 0; ++entry) CheckName(path, base, entry->Name);
    }
    if (auto address = directories[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress; address != 0) {
        for (auto entry = reinterpret_cast<const IMAGE_DELAYLOAD_DESCRIPTOR*>(base + address); entry->DllNameRVA != 0; ++entry) CheckName(path, base, entry->DllNameRVA);
    }
}
}

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc < 2) throw std::invalid_argument("No system libraries were given");
        for (int index = 1; index < argc; ++index) CheckImports(argv[index]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
