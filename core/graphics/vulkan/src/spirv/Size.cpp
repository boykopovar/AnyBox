#include "Layout.hpp"
#include <spirv/unified1/spirv.hpp>
#include <algorithm>

std::uint64_t AnyBox::Vulkan::Spirv::Layout::size(std::uint32_t id, const Member& layout) {
    if (!active.insert(id).second || active.size() > 64) throw std::invalid_argument("SPIR-V push constant type is recursive or too deeply nested");
    auto declaration = type(id);
    auto opcode = declaration[0] & 65535;
    std::uint64_t result = 0;
    if (opcode == spv::OpTypeInt || opcode == spv::OpTypeFloat) {
        if (declaration.size() < 3 || (declaration[2] != 8 && declaration[2] != 16 && declaration[2] != 32 && declaration[2] != 64)) throw std::runtime_error("SPIR-V push constant scalar width is not implemented");
        result = declaration[2] / 8;
    } else if (opcode == spv::OpTypeVector) {
        if (declaration.size() != 4 || declaration[3] < 2 || declaration[3] > 4) throw std::invalid_argument("SPIR-V push constant vector is invalid");
        result = size(declaration[2], {}) * declaration[3];
    } else if (opcode == spv::OpTypeMatrix) {
        if (declaration.size() != 4 || declaration[3] < 2 || declaration[3] > 4 || !layout.matrixStride || *layout.matrixStride == 0) throw std::invalid_argument("SPIR-V push constant matrix layout is incomplete");
        auto column = type(declaration[2]);
        if (column.size() != 4 || (column[0] & 65535) != spv::OpTypeVector) throw std::invalid_argument("SPIR-V matrix column type is invalid");
        auto major = layout.rowMajor ? column[3] : declaration[3];
        auto minor = layout.rowMajor ? declaration[3] : column[3];
        if (size(column[2], {}) * minor > *layout.matrixStride) throw std::invalid_argument("SPIR-V matrix stride is smaller than its vector");
        result = static_cast<std::uint64_t>(*layout.matrixStride) * major;
    } else if (opcode == spv::OpTypeArray) {
        auto stride = strides.find(id);
        if (declaration.size() != 4 || stride == strides.end()) throw std::invalid_argument("SPIR-V push constant array layout is incomplete");
        if (size(declaration[2], layout) > stride->second) throw std::invalid_argument("SPIR-V array stride is smaller than its element");
        result = constant(declaration[3]) * stride->second;
    } else if (opcode == spv::OpTypeStruct) {
        if (declaration.size() < 3) throw std::invalid_argument("SPIR-V push constant structure is empty");
        for (std::size_t index = 2; index < declaration.size(); ++index) {
            auto found = members.find((static_cast<std::uint64_t>(id) << 32) | (index - 2));
            if (found == members.end() || !found->second.offset) throw std::invalid_argument("SPIR-V push constant member offset is missing");
            auto end = *found->second.offset + size(declaration[index], found->second);
            result = std::max(result, end);
        }
    } else if (opcode == spv::OpTypePointer && declaration.size() == 4 && declaration[2] == spv::StorageClassPhysicalStorageBuffer) {
        result = 8;
    } else {
        throw std::runtime_error("SPIR-V push constant type is not implemented");
    }
    active.erase(id);
    if (result > UINT32_MAX) throw std::length_error("SPIR-V push constant layout size overflows");
    return result;
}
