#pragma once

#include "../Files.hpp"
#include <chrono>
#include <stdexcept>

class GraphicsFiles {
public:
    GraphicsFiles() : directory(std::filesystem::current_path() / ("graphics-preparation-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Graphics preparation fixture directory already exists");
    }
    ~GraphicsFiles() {
        std::error_code error;
        for (const auto& file : files) std::filesystem::remove(file, error);
        std::filesystem::remove(directory, error);
    }
    std::filesystem::path Track(const std::string& name) {
        auto path = directory / name;
        files.push_back(path);
        return path;
    }
    std::filesystem::path Save(const std::string& name, std::span<const std::byte> bytes) {
        auto path = Track(name);
        AnyBox::Shader::Tools::WriteOutput(path, bytes);
        return path;
    }
    const std::filesystem::path& GetDirectory() const noexcept { return directory; }

private:
    std::filesystem::path directory;
    std::vector<std::filesystem::path> files;
};
