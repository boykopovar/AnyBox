#include "State.hpp"
#include <anybox/graphics/container/SerializedGraphicsShader.hpp>

AnyBox::Graphics::Container::SerializedGraphicsShaderState AnyBox::Graphics::Container::ReadSerializedGraphicsShaderState(std::span<const std::byte> state, NativeHardwareStage stage) {
    auto registers = SerializedGraphics::ReadRegisters(state);
    std::size_t offset;
    switch (stage) {
    case NativeHardwareStage::Vertex: offset = 28; break;
    case NativeHardwareStage::Pixel: offset = 48; break;
    default: throw std::invalid_argument("Xbox serialized graphics shader stage is invalid");
    }
    auto resources = ReadInteger(registers, offset);
    auto control = ReadInteger(registers, offset + 4);
    if ((resources & 0x30500000u) != 0) throw std::runtime_error("Xbox serialized graphics privilege, debugging and FP16 overflow modes are not implemented");
    if ((control & ~0x0800003Eu) != 0) throw std::runtime_error("Xbox serialized graphics scratch, traps, shared registers and extended system inputs are not implemented");
    auto userRegisters = ((control >> 1) & 31) | ((control >> 22) & 32);
    if (userRegisters > 32) throw std::invalid_argument("Xbox serialized graphics user register count exceeds its hardware limit");
    NativeStageState shader{stage, (resources >> 12) & 255, (resources & 0x200000) != 0, (resources & 0x800000) != 0};
    return {shader, userRegisters};
}
