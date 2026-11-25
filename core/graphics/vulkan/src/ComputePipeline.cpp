#include <anybox/vulkan/ComputePipeline.hpp>
#include <spirv/unified1/spirv.hpp>
#include <algorithm>
#include <cstring>
#include <string_view>

namespace {
using SpirvWords = std::span<const std::uint32_t>;

template<class Filter>
SpirvWords FindSpirvDeclaration(SpirvWords words, spv::Op expected, Filter matches) {
    while (!words.empty()) {
        auto count = words.front() >> spv::WordCountShift;
        if (count == 0 || count > words.size()) throw std::invalid_argument("Vulkan compute shader declaration is truncated");
        auto opcode = static_cast<spv::Op>(words.front() & spv::OpCodeMask);
        if (opcode == spv::OpFunction) break;
        auto instruction = words.first(count);
        if (opcode == expected && matches(instruction)) return instruction;
        words = words.subspan(count);
    }
    return {};
}

std::uint64_t DeclaredPushConstantArrayEnd(SpirvWords code, std::string_view entryPoint) {
    if (code[1] < 0x00010400) return 0;
    auto words = code.subspan(5);
    auto nameWords = entryPoint.size() / sizeof(std::uint32_t) + 1;
    auto entry = FindSpirvDeclaration(words, spv::OpEntryPoint, [&](SpirvWords instruction) {
        if (instruction.size() < 3 + nameWords || instruction[1] != spv::ExecutionModelGLCompute) return false;
        auto name = std::as_bytes(instruction.subspan(3));
        return name[entryPoint.size()] == std::byte{} && std::memcmp(name.data(), entryPoint.data(), entryPoint.size()) == 0;
    });
    if (entry.empty()) return 0;
    auto interfaces = entry.subspan(3 + nameWords);
    auto variable = FindSpirvDeclaration(words, spv::OpVariable, [&](SpirvWords instruction) {
        if (instruction.size() != 4 || instruction[3] != spv::StorageClassPushConstant) return false;
        return std::ranges::find(interfaces, instruction[2]) != interfaces.end();
    });
    if (variable.empty()) return 0;

    auto pointer = FindSpirvDeclaration(words, spv::OpTypePointer, [&](SpirvWords instruction) {
        return instruction.size() == 4 && instruction[1] == variable[1] && instruction[2] == spv::StorageClassPushConstant;
    });
    if (pointer.empty()) return 0;
    auto block = FindSpirvDeclaration(words, spv::OpTypeStruct, [&](SpirvWords instruction) {
        return instruction.size() == 3 && instruction[1] == pointer[3];
    });
    if (block.empty()) return 0;
    auto array = FindSpirvDeclaration(words, spv::OpTypeArray, [&](SpirvWords instruction) {
        return instruction.size() == 4 && instruction[1] == block[2];
    });
    if (array.empty()) return 0;

    auto element = FindSpirvDeclaration(words, spv::OpTypeInt, [&](SpirvWords instruction) {
        return instruction.size() == 4 && instruction[1] == array[2] && instruction[2] == 32 && instruction[3] == 0;
    });
    auto stride = FindSpirvDeclaration(words, spv::OpDecorate, [&](SpirvWords instruction) {
        return instruction.size() == 4 && instruction[1] == array[1] && instruction[2] == spv::DecorationArrayStride && instruction[3] == sizeof(std::uint32_t);
    });
    auto offset = FindSpirvDeclaration(words, spv::OpMemberDecorate, [&](SpirvWords instruction) {
        return instruction.size() == 5 && instruction[1] == block[1] && instruction[2] == 0 && instruction[3] == spv::DecorationOffset;
    });
    auto length = FindSpirvDeclaration(words, spv::OpConstant, [&](SpirvWords instruction) {
        return instruction.size() == 4 && instruction[2] == array[3];
    });
    if (element.empty() || stride.empty() || offset.empty() || length.empty()) return 0;
    return static_cast<std::uint64_t>(offset[4]) + static_cast<std::uint64_t>(length[3]) * stride[3];
}
}

AnyBox::Vulkan::ComputePipeline::ComputePipeline(Context& context, std::span<const std::uint32_t> code, std::uint32_t pushConstantSize, const char* entryPoint, std::span<const VkDescriptorSetLayout> descriptorLayouts) : context(context), pushConstantSize(pushConstantSize), descriptorSetCount(static_cast<std::uint32_t>(descriptorLayouts.size())) {
    if (code.size() < 5 || code.front() != 0x07230203 || !entryPoint || *entryPoint == '\0') throw std::invalid_argument("Vulkan compute shader code or entry point is invalid");
    if (pushConstantSize % 4 != 0 || pushConstantSize > context.GetProperties().limits.maxPushConstantsSize) throw std::invalid_argument("Vulkan compute push constant size is invalid");
    if (descriptorLayouts.size() > context.GetProperties().limits.maxBoundDescriptorSets) throw std::invalid_argument("Vulkan compute descriptor set count exceeds the device limit");
    for (auto value : descriptorLayouts) if (!value) throw std::invalid_argument("Vulkan compute descriptor set layout is null");
    const auto& functions = context.GetFunctions();
    VkShaderModule module = VK_NULL_HANDLE;
    try {
        auto rangeSize = std::max<std::uint64_t>(pushConstantSize, DeclaredPushConstantArrayEnd(code, entryPoint));
        if (rangeSize % 4 != 0 || rangeSize > context.GetProperties().limits.maxPushConstantsSize) throw std::invalid_argument("Vulkan compute push constant block exceeds the supported range");
        VkShaderModuleCreateInfo shader{};
        shader.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader.codeSize = code.size_bytes();
        shader.pCode = code.data();
        Check(functions.createShaderModule(context.GetDevice(), &shader, nullptr, &module), "Create Vulkan compute shader module");

        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<std::uint32_t>(rangeSize)};
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = descriptorSetCount;
        layoutInfo.pSetLayouts = descriptorLayouts.data();
        layoutInfo.pushConstantRangeCount = range.size == 0 ? 0 : 1;
        layoutInfo.pPushConstantRanges = range.size == 0 ? nullptr : &range;
        Check(functions.createPipelineLayout(context.GetDevice(), &layoutInfo, nullptr, &layout), "Create Vulkan compute pipeline layout");

        VkComputePipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module = module;
        info.stage.pName = entryPoint;
        info.layout = layout;
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
