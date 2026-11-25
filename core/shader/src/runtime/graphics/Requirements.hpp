#pragma once

#include <anybox/shader/GraphicsArtifact.hpp>
#include <anybox/vulkan/Context.hpp>

namespace AnyBox::Shader::Runtime {
void CheckGraphicsRequirements(const Vulkan::Context& context, const GraphicsArtifact& artifact);
}
