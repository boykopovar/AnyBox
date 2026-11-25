#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace AnyBox::Shader::Tools {
std::string InspectSerializedCompute(std::span<const std::byte> data);
}
