#include <anybox/vulkan/ComputePipeline.hpp>
#include <spirv/unified1/spirv.hpp>
#include <cstring>
#include <optional>
#include <string_view>

AnyBox::Vulkan::ComputePipeline::ComputePipeline(Context& context, std::span<const std::uint32_t> code, std::uint32_t pushConstantSize, const char* entryPoint, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::uint32_t subgroupSize) : context(context), pushConstantSize(pushConstantSize), descriptorSetCount(static_cast<std::uint32_t>(descriptorLayouts.size())) {
    if (code.size() < 5 || code.front() != 0x07230203 || !entryPoint || *entryPoint == '\0') throw std::invalid_argument("Vulkan compute shader code or entry point is invalid");
    if (pushConstantSize % 4 != 0 || pushConstantSize > context.GetProperties().limits.maxPushConstantsSize) throw std::invalid_argument("Vulkan compute push constant size is invalid");
    if (descriptorLayouts.size() > context.GetProperties().limits.maxBoundDescriptorSets) throw std::invalid_argument("Vulkan compute descriptor set count exceeds the device limit");
    for (auto value : descriptorLayouts) if (!value) throw std::invalid_argument("Vulkan compute descriptor set layout is null");
    const auto& functions = context.GetFunctions();
    VkShaderModule module = VK_NULL_HANDLE;
    try {
        VkShaderModuleCreateInfo shader{};
        shader.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader.codeSize = code.size_bytes();
        shader.pCode = code.data();
        Check(functions.createShaderModule(context.GetDevice(), &shader, nullptr, &module), "Create Vulkan compute shader module");

        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantSize};
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = descriptorSetCount;
        layoutInfo.pSetLayouts = descriptorLayouts.data();
        layoutInfo.pushConstantRangeCount = pushConstantSize == 0 ? 0 : 1;
        layoutInfo.pPushConstantRanges = pushConstantSize == 0 ? nullptr : &range;
        Check(functions.createPipelineLayout(context.GetDevice(), &layoutInfo, nullptr, &layout), "Create Vulkan compute pipeline layout");

        VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup{};
        subgroup.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
        VkComputePipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module = module;
        info.stage.pName = entryPoint;
        info.layout = layout;
        if (subgroupSize) configureComputeSubgroups(code, entryPoint, subgroupSize, info.stage, subgroup);
        Check(functions.createComputePipelines(context.GetDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline), "Create Vulkan compute pipeline");
    } catch (...) {
        if (module) functions.destroyShaderModule(context.GetDevice(), module, nullptr);
        release();
        throw;
    }
    functions.destroyShaderModule(context.GetDevice(), module, nullptr);
}

AnyBox::Vulkan::ComputePipeline::~ComputePipeline() { release(); }

void AnyBox::Vulkan::ComputePipeline::release() noexcept {
    const auto& functions = context.GetFunctions();
    if (pipeline) functions.destroyPipeline(context.GetDevice(), pipeline, nullptr);
    if (layout) functions.destroyPipelineLayout(context.GetDevice(), layout, nullptr);
}

void AnyBox::Vulkan::ComputePipeline::Dispatch(VkCommandBuffer commands, std::span<const std::byte> constants, const std::array<std::uint32_t, 3>& groups, std::span<const VkDescriptorSet> descriptors) const {
    if (!commands || constants.size() != pushConstantSize) throw std::invalid_argument("Vulkan compute commands or push constants are invalid");
    if (descriptors.size() != descriptorSetCount) throw std::invalid_argument("Vulkan compute descriptor sets do not match the pipeline layout");
    for (auto value : descriptors) if (!value) throw std::invalid_argument("Vulkan compute descriptor set is null");
    const auto& limits = context.GetProperties().limits;
    for (std::size_t index = 0; index < groups.size(); ++index) {
        if (groups[index] > limits.maxComputeWorkGroupCount[index]) throw std::out_of_range("Vulkan compute dispatch exceeds the workgroup count limit");
    }
    const auto& functions = context.GetFunctions();
    functions.bindPipeline(commands, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    if (!descriptors.empty()) functions.bindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, descriptorSetCount, descriptors.data(), 0, nullptr);
    if (!constants.empty()) functions.pushConstants(commands, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantSize, constants.data());
    functions.dispatch(commands, groups[0], groups[1], groups[2]);
}

namespace {
using SpirvWords = std::span<const std::uint32_t>;

template<class Match>
SpirvWords FindSpirvInstruction(SpirvWords words, spv::Op opcode, Match match) {
    while (!words.empty()) {
        auto count = words.front() >> spv::WordCountShift;
        if (count == 0 || count > words.size()) throw std::invalid_argument("Vulkan compute shader declaration is truncated");
        auto instruction = static_cast<spv::Op>(words.front() & spv::OpCodeMask);
        if (instruction == spv::OpFunction) break;
        if (instruction == opcode && match(words.first(count))) return words.first(count);
        words = words.subspan(count);
    }
    return {};
}

std::optional<std::array<std::uint32_t, 3>> ReadSpirvWorkgroup(SpirvWords words, std::string_view entryPoint) {
    auto nameWords = entryPoint.size() / sizeof(std::uint32_t) + 1;
    auto entry = FindSpirvInstruction(words, spv::OpEntryPoint, [&](SpirvWords instruction) {
        if (instruction.size() < 3 + nameWords || instruction[1] != spv::ExecutionModelGLCompute) return false;
        auto name = std::as_bytes(instruction.subspan(3));
        return name[entryPoint.size()] == std::byte{} && std::memcmp(name.data(), entryPoint.data(), entryPoint.size()) == 0;
    });
    if (entry.empty()) throw std::invalid_argument("Vulkan compute shader entry point is missing");

    auto builtin = FindSpirvInstruction(words, spv::OpDecorate, [](SpirvWords instruction) {
        return instruction.size() == 4 && instruction[2] == spv::DecorationBuiltIn && instruction[3] == spv::BuiltInWorkgroupSize;
    });
    if (!builtin.empty()) {
        auto composite = FindSpirvInstruction(words, spv::OpConstantComposite, [&](SpirvWords instruction) {
            return instruction.size() == 6 && instruction[2] == builtin[1];
        });
        if (composite.empty()) return std::nullopt;
        std::array<std::uint32_t, 3> size;
        for (std::size_t index = 0; index < size.size(); ++index) {
            auto component = FindSpirvInstruction(words, spv::OpConstant, [&](SpirvWords instruction) {
                return instruction.size() == 4 && instruction[2] == composite[3 + index];
            });
            if (component.empty()) return std::nullopt;
            size[index] = component[3];
        }
        return size;
    }

    auto localSize = FindSpirvInstruction(words, spv::OpExecutionMode, [&](SpirvWords instruction) {
        return instruction.size() == 6 && instruction[1] == entry[2] && instruction[2] == spv::ExecutionModeLocalSize;
    });
    if (localSize.empty()) return std::nullopt;
    return std::array{localSize[3], localSize[4], localSize[5]};
}
}

void AnyBox::Vulkan::ComputePipeline::configureComputeSubgroups(SpirvWords code, const char* entryPoint, std::uint32_t subgroupSize, VkPipelineShaderStageCreateInfo& stage, VkPipelineShaderStageRequiredSubgroupSizeCreateInfo& requiredSize) const {
    auto words = code.subspan(5);
    auto subgroupCapability = FindSpirvInstruction(words, spv::OpCapability, [](SpirvWords instruction) {
        return instruction.size() == 2 && instruction[1] == spv::CapabilityGroupNonUniform;
    });
    auto waveDependent = !subgroupCapability.empty();
    auto differentWidth = subgroupSize != context.GetSubgroupProperties().subgroupSize;
    if (!waveDependent && !differentWidth) return;

    auto workgroup = ReadSpirvWorkgroup(words, entryPoint);
    if (!workgroup) {
        if (differentWidth) throw std::runtime_error("Required Vulkan compute subgroup size needs a fixed workgroup size");
        return;
    }
    const auto& dimensions = *workgroup;
    const auto& limits = context.GetProperties().limits;
    std::uint64_t invocations = 1;
    for (std::size_t index = 0; index < dimensions.size(); ++index) {
        auto size = dimensions[index];
        if (size == 0 || size > limits.maxComputeWorkGroupSize[index] || invocations > limits.maxComputeWorkGroupInvocations / size) throw std::invalid_argument("Vulkan compute workgroup exceeds the device limits");
        invocations *= size;
    }
    if (context.CanRequireComputeSubgroupSize(subgroupSize, invocations)) {
        requiredSize.requiredSubgroupSize = subgroupSize;
        stage.pNext = &requiredSize;
    } else if (differentWidth) {
        throw std::runtime_error("Required Vulkan compute subgroup size is unsupported");
    }
    if (waveDependent && context.SupportsComputeFullSubgroups() && dimensions[0] % subgroupSize == 0) stage.flags |= VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;
}
