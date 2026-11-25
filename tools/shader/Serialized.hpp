#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace AnyBox::Shader::Tools {
std::string InspectSerializedCompute(std::span<const std::byte> data);
std::string InspectSerializedInstructions(std::span<const std::byte> data, std::uint32_t waveSize);
std::string AuditSerializedCompute(std::span<const std::byte> data);
}
