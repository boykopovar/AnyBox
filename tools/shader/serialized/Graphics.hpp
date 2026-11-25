#pragma once

#include <span>
#include <string_view>

namespace AnyBox::Shader::Tools {
bool RunSerializedGraphicsCommand(std::span<const std::string_view> arguments);
void AuditSerializedPixel(std::span<const std::string_view> arguments);
}
