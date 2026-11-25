#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace AnyBox::Shader::Tools {
std::string InspectSerializedCompute(std::span<const std::byte> data);
std::string InspectSerializedInstructions(std::span<const std::byte> data, std::uint32_t waveSize);
std::string InspectSerializedStageInstructions(std::span<const std::byte> code, std::span<const std::byte> records, std::uint32_t waveSize);
std::string InspectSerializedStageRoots(std::span<const std::byte> code, std::span<const std::byte> records, std::span<const std::byte> root, std::uint32_t visibility);
std::string InspectSerializedGraphicsState(std::span<const std::byte> state, std::span<const std::byte> auxiliary);
std::string AuditSerializedCompute(std::span<const std::byte> data);
std::string AuditSerializedCompute(std::span<const std::byte> data, std::span<const std::byte> reference, std::span<const std::byte> root);
}
