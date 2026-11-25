#include <anybox/vulkan/ShaderModule.hpp>
#include <anybox/vulkan/Spirv.hpp>

AnyBox::Vulkan::ShaderModule::ShaderModule(Context& context, std::span<const std::uint32_t> code, std::string_view entryPoint) : context(context) {
    if (code.size() < 5 || code[0] != 0x07230203 || code[3] == 0 || code[4] != 0) throw std::invalid_argument("Vulkan shader module header is invalid");
    for (std::size_t offset = 5; offset < code.size();) {
        auto count = code[offset] >> 16;
        if (count == 0 || count > code.size() - offset) throw std::invalid_argument("Vulkan shader module instruction is truncated");
        offset += count;
    }
    constants = ReadSpirvPushConstants(code, entryPoint);
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size_bytes();
    info.pCode = code.data();
    Check(context.GetFunctions().createShaderModule(context.GetDevice(), &info, nullptr, &module), "Create Vulkan shader module");
}

AnyBox::Vulkan::ShaderModule::~ShaderModule() {
    context.GetFunctions().destroyShaderModule(context.GetDevice(), module, nullptr);
}
