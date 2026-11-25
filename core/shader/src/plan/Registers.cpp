#include "Binary.hpp"

std::vector<std::uint32_t> AnyBox::Shader::ReadResourceRegisters(const ShaderRecompiler::IrResourcePlan& plan, std::span<const std::uint32_t> shaderRegisters) {
    using namespace ShaderRecompiler;
    PlanFormat::Validate(plan);
    std::array<bool, 128> selected{};
    for (auto index : shaderRegisters) {
        if (index >= selected.size() || selected[index]) throw std::invalid_argument("Prepared shader user register index is invalid or repeated");
        selected[index] = true;
    }
    for (std::size_t index = 0; index < plan.valueStorage.Size(); ++index) {
        const auto& value = plan.valueStorage[index];
        if (value.Opcode() != IrOpcode::GetUserData) continue;
        if (value.ArgumentCount() != 1 || value.Argument(0)->Type() != IrType::ScalarReg) throw std::invalid_argument("Prepared resource user register expression is invalid");
        auto reg = value.Argument(0)->Register();
        if (reg.bank != RegisterBank::UserData || reg.index < plan.userDataBase || reg.index - plan.userDataBase >= plan.userDataCount) throw std::invalid_argument("Prepared resource user register is outside its declared range");
        selected[reg.index] = true;
    }
    std::vector<std::uint32_t> result;
    for (std::uint32_t index = 0; index < selected.size(); ++index) if (selected[index]) result.push_back(index);
    return result;
}
