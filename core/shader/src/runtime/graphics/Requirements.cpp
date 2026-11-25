#include "Requirements.hpp"
#include <spirv/unified1/spirv.hpp>

void AnyBox::Shader::Runtime::CheckGraphicsRequirements(const Vulkan::Context& context, const GraphicsArtifact& artifact) {
    ValidateGraphicsArtifact(artifact);
    const auto& limits = context.GetProperties().limits;
    const auto& subgroups = context.GetSubgroupProperties();
    const auto& features = context.GetEnabledFeatures();
    auto stage = artifact.stage == GraphicsArtifactStage::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
    if (subgroups.subgroupSize != artifact.hostSubgroupSize) throw std::runtime_error("Offline graphics shader requires a matching host subgroup size");
    if (artifact.stencilReference != UINT32_MAX || artifact.memoryOffsetCount != 0 || artifact.runtimeImageCount != 0 || artifact.runtimeStateDwords != 0) throw std::runtime_error("Offline graphics shader runtime data extensions are not implemented");
    if (artifact.pushDataStart == UINT32_MAX && (artifact.memoryOffset != 0 || !artifact.userRegisters.empty())) throw std::runtime_error("Offline graphics shader external argument storage is not implemented");
    if (artifact.pushConstantOffset + artifact.pushConstantSize > limits.maxPushConstantsSize) throw std::runtime_error("Offline graphics shader exceeds the device push constant limit");
    for (const auto& descriptor : artifact.descriptors) {
        if ((descriptor.kind != 59 && descriptor.kind != 60) || !descriptor.resources.empty()) throw std::runtime_error("Offline graphics shader descriptor kind is not implemented");
        if (descriptor.kind == 60 && !(stage == VK_SHADER_STAGE_VERTEX_BIT ? features.vertexPipelineStoresAndAtomics : features.fragmentStoresAndAtomics)) throw std::runtime_error("Offline graphics shader fault reporting requires shader storage writes");
    }

    for (std::size_t offset = 5; offset < artifact.code.size(); offset += artifact.code[offset] >> 16) {
        if ((artifact.code[offset] & 65535) != spv::OpCapability) continue;
        if ((artifact.code[offset] >> 16) != 2) throw std::invalid_argument("Offline graphics shader SPIR-V capability is malformed");
        auto capability = artifact.code[offset + 1];
        VkSubgroupFeatureFlags required = 0;
        switch (capability) {
            case spv::CapabilityShader:
            case spv::CapabilityInt64:
            case spv::CapabilityPhysicalStorageBufferAddresses: break;
            case spv::CapabilityClipDistance:
                if (!features.shaderClipDistance) throw std::runtime_error("Offline graphics shader requires clip distances");
                break;
            case spv::CapabilityCullDistance:
                if (!features.shaderCullDistance) throw std::runtime_error("Offline graphics shader requires cull distances");
                break;
            case spv::CapabilityStorageBuffer8BitAccess:
                if (!context.SupportsByteStorage()) throw std::runtime_error("Offline graphics shader requires byte buffer storage");
                break;
            case spv::CapabilityGroupNonUniform: required = VK_SUBGROUP_FEATURE_BASIC_BIT; break;
            case spv::CapabilityGroupNonUniformVote: required = VK_SUBGROUP_FEATURE_VOTE_BIT; break;
            case spv::CapabilityGroupNonUniformArithmetic: required = VK_SUBGROUP_FEATURE_ARITHMETIC_BIT; break;
            case spv::CapabilityGroupNonUniformBallot: required = VK_SUBGROUP_FEATURE_BALLOT_BIT; break;
            case spv::CapabilityGroupNonUniformShuffle: required = VK_SUBGROUP_FEATURE_SHUFFLE_BIT; break;
            case spv::CapabilityGroupNonUniformShuffleRelative: required = VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT; break;
            case spv::CapabilitySignedZeroInfNanPreserve:
                if (!context.GetFloatControls().shaderSignedZeroInfNanPreserveFloat32) throw std::runtime_error("Offline graphics shader requires float32 signed zero, infinity and NaN preservation");
                break;
            default: throw std::runtime_error("Offline graphics shader capability is not implemented for this target profile");
        }
        if (required && ((subgroups.supportedStages & stage) == 0 || (subgroups.supportedOperations & required) != required)) throw std::runtime_error("Offline graphics shader requires unsupported subgroup operations");
    }
}
