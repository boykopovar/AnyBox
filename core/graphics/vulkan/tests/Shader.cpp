#include "Tests.hpp"
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

namespace {
class Process {
public:
    Process() { if (!glslang_initialize_process()) throw std::runtime_error("Cannot initialize the test GLSL compiler"); }
    ~Process() { glslang_finalize_process(); }
};
}

std::vector<std::uint32_t> AnyBox::Vulkan::Tests::CompileShader(const char* source, VkShaderStageFlagBits stage) {
    static Process process;
    if (!source) throw std::invalid_argument("Test shader source is null");
    glslang_input_t input{};
    input.language = GLSLANG_SOURCE_GLSL;
    switch (stage) {
    case VK_SHADER_STAGE_COMPUTE_BIT: input.stage = GLSLANG_STAGE_COMPUTE; break;
    case VK_SHADER_STAGE_VERTEX_BIT: input.stage = GLSLANG_STAGE_VERTEX; break;
    case VK_SHADER_STAGE_FRAGMENT_BIT: input.stage = GLSLANG_STAGE_FRAGMENT; break;
    default: throw std::invalid_argument("Test shader stage is not implemented");
    }
    input.client = GLSLANG_CLIENT_VULKAN;
    input.client_version = GLSLANG_TARGET_VULKAN_1_2;
    input.target_language = GLSLANG_TARGET_SPV;
    input.target_language_version = GLSLANG_TARGET_SPV_1_5;
    input.code = source;
    input.default_version = 450;
    input.default_profile = GLSLANG_CORE_PROFILE;
    input.messages = static_cast<glslang_messages_t>(GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT);
    input.resource = glslang_default_resource();
    std::unique_ptr<glslang_shader_t, decltype(&glslang_shader_delete)> shader(glslang_shader_create(&input), glslang_shader_delete);
    if (!shader) throw std::runtime_error("Cannot create the test GLSL shader");
    if (!glslang_shader_preprocess(shader.get(), &input)) throw std::runtime_error(glslang_shader_get_info_log(shader.get()));
    input.code = glslang_shader_get_preprocessed_code(shader.get());
    if (!glslang_shader_parse(shader.get(), &input)) throw std::runtime_error(glslang_shader_get_info_log(shader.get()));

    std::unique_ptr<glslang_program_t, decltype(&glslang_program_delete)> program(glslang_program_create(), glslang_program_delete);
    if (!program) throw std::runtime_error("Cannot create the test GLSL program");
    glslang_program_add_shader(program.get(), shader.get());
    if (!glslang_program_link(program.get(), input.messages)) throw std::runtime_error(glslang_program_get_info_log(program.get()));
    glslang_program_SPIRV_generate(program.get(), input.stage);
    auto messages = glslang_program_SPIRV_get_messages(program.get());
    if (messages && *messages) throw std::runtime_error(messages);
    std::vector<std::uint32_t> code(glslang_program_SPIRV_get_size(program.get()));
    if (code.empty()) throw std::runtime_error("Test GLSL compiler produced no SPIR-V");
    glslang_program_SPIRV_get(program.get(), code.data());
    return code;
}

std::vector<std::uint32_t> AnyBox::Vulkan::Tests::CompileCompute(const char* source) {
    return CompileShader(source, VK_SHADER_STAGE_COMPUTE_BIT);
}
