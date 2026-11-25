#include "Requirements.hpp"
#include <spirv/unified1/spirv.hpp>
#include <stdexcept>

void AnyBox::Shader::Runtime::CheckRequirements(const Vulkan::Context& context, const ComputeArtifact& artifact) {
    ValidateComputeArtifact(artifact);
    const auto& limits = context.GetProperties().limits;
    const auto& subgroups = context.GetSubgroupProperties();
    if ((subgroups.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT) == 0) throw std::runtime_error("Offline compute pipeline requires compute subgroup support");
    if (artifact.executionFlags != 0 || artifact.splitStateLanes != 0 || artifact.scratchPoolSlotDwords != 0) throw std::runtime_error("Offline compute pipeline execution extensions are not implemented");
    if (artifact.pushConstantOffset != 0 || artifact.stencilReference != UINT32_MAX || artifact.memoryOffsetCount != 0 || artifact.dispatchThreadLimit || artifact.runtimeImageCount != 0 || artifact.runtimeStateDwords != 0) throw std::runtime_error("Offline compute pipeline runtime data extensions are not implemented");
    if (!artifact.userRegisters.empty() && artifact.pushDataStart == UINT32_MAX) throw std::runtime_error("Offline compute pipeline external shader data is not implemented");
    if (artifact.pushConstantSize > limits.maxPushConstantsSize || artifact.sharedMemoryBytes > limits.maxComputeSharedMemorySize) throw std::runtime_error("Offline compute pipeline exceeds the device constant or shared memory limit");
    std::uint64_t threads = 1;
    for (unsigned index = 0; index < 3; ++index) {
        if (artifact.threads[index] > limits.maxComputeWorkGroupSize[index]) throw std::runtime_error("Offline compute pipeline workgroup dimension exceeds the device limit");
        threads *= artifact.threads[index];
    }
    if (threads > limits.maxComputeWorkGroupInvocations) throw std::runtime_error("Offline compute pipeline workgroup size exceeds the device limit");

    for (std::size_t offset = 5; offset < artifact.code.size(); offset += artifact.code[offset] >> 16) {
        if ((artifact.code[offset] & 65535) != spv::OpCapability) continue;
        if ((artifact.code[offset] >> 16) != 2) throw std::invalid_argument("Offline compute pipeline SPIR-V capability is malformed");
        auto capability = artifact.code[offset + 1];
        VkSubgroupFeatureFlags required = 0;
        switch (capability) {
            case spv::CapabilityShader:
            case spv::CapabilityInt64:
            case spv::CapabilityPhysicalStorageBufferAddresses: break;
            case spv::CapabilityStorageBuffer8BitAccess:
                if (!context.SupportsByteStorage()) throw std::runtime_error("Offline compute pipeline requires byte buffer storage");
                break;
            case spv::CapabilityGroupNonUniform: required = VK_SUBGROUP_FEATURE_BASIC_BIT; break;
            case spv::CapabilityGroupNonUniformVote: required = VK_SUBGROUP_FEATURE_VOTE_BIT; break;
            case spv::CapabilityGroupNonUniformArithmetic: required = VK_SUBGROUP_FEATURE_ARITHMETIC_BIT; break;
            case spv::CapabilityGroupNonUniformBallot: required = VK_SUBGROUP_FEATURE_BALLOT_BIT; break;
            case spv::CapabilityGroupNonUniformShuffle: required = VK_SUBGROUP_FEATURE_SHUFFLE_BIT; break;
            case spv::CapabilityGroupNonUniformShuffleRelative: required = VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT; break;
            case spv::CapabilitySignedZeroInfNanPreserve:
                if (!context.GetFloatControls().shaderSignedZeroInfNanPreserveFloat32) throw std::runtime_error("Offline compute pipeline requires float32 signed zero, infinity and NaN preservation");
                break;
            default: throw std::runtime_error("Offline compute pipeline capability is not implemented for this target profile");
        }
        if ((subgroups.supportedOperations & required) != required) throw std::runtime_error("Offline compute pipeline requires unsupported subgroup operations");
    }
}
