#pragma once

#include <anybox/shader/Artifact.hpp>
#include <anybox/vulkan/Context.hpp>

namespace AnyBox::Shader::Runtime {
void CheckRequirements(const Vulkan::Context& context, const ComputeArtifact& artifact);
}
