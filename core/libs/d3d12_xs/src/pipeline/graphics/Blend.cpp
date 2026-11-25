#include "State.hpp"

namespace {
VkBlendFactor Factor(D3D12_BLEND value) {
    switch (value) {
        case D3D12_BLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
        case D3D12_BLEND_ONE: return VK_BLEND_FACTOR_ONE;
        case D3D12_BLEND_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
        case D3D12_BLEND_INV_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case D3D12_BLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
        case D3D12_BLEND_INV_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case D3D12_BLEND_DEST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
        case D3D12_BLEND_INV_DEST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case D3D12_BLEND_DEST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
        case D3D12_BLEND_INV_DEST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case D3D12_BLEND_SRC_ALPHA_SAT: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case D3D12_BLEND_BLEND_FACTOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case D3D12_BLEND_INV_BLEND_FACTOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case D3D12_BLEND_SRC1_COLOR: return VK_BLEND_FACTOR_SRC1_COLOR;
        case D3D12_BLEND_INV_SRC1_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case D3D12_BLEND_SRC1_ALPHA: return VK_BLEND_FACTOR_SRC1_ALPHA;
        case D3D12_BLEND_INV_SRC1_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default: throw std::invalid_argument("Offline graphics blend factor is invalid");
    }
}
VkBlendOp Operation(D3D12_BLEND_OP value) {
    if (value < D3D12_BLEND_OP_ADD || value > D3D12_BLEND_OP_MAX) throw std::invalid_argument("Offline graphics blend operation is invalid");
    return static_cast<VkBlendOp>(value - 1);
}
VkLogicOp Logic(D3D12_LOGIC_OP value) {
    constexpr std::array values{VK_LOGIC_OP_CLEAR, VK_LOGIC_OP_SET, VK_LOGIC_OP_COPY, VK_LOGIC_OP_COPY_INVERTED, VK_LOGIC_OP_NO_OP, VK_LOGIC_OP_INVERT, VK_LOGIC_OP_AND, VK_LOGIC_OP_NAND, VK_LOGIC_OP_OR, VK_LOGIC_OP_NOR, VK_LOGIC_OP_XOR, VK_LOGIC_OP_EQUIVALENT, VK_LOGIC_OP_AND_REVERSE, VK_LOGIC_OP_AND_INVERTED, VK_LOGIC_OP_OR_REVERSE, VK_LOGIC_OP_OR_INVERTED};
    if (value < D3D12_LOGIC_OP_CLEAR || value > D3D12_LOGIC_OP_OR_INVERTED) throw std::invalid_argument("Offline graphics logic operation is invalid");
    return values[value];
}
bool Dual(D3D12_BLEND value) { return value >= D3D12_BLEND_SRC1_COLOR && value <= D3D12_BLEND_INV_SRC1_ALPHA; }
}

void AnyBox::Graphics::GraphicsFixedState::initializeBlend(const D3D12_BLEND_DESC& source, std::uint32_t count) {
    const auto& features = context.GetEnabledFeatures();
    if (source.IndependentBlendEnable && !features.independentBlend) throw std::runtime_error("Offline graphics independent blending is unsupported by the device");
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = count;
    blend.pAttachments = colors.data();
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto& input = source.RenderTarget[source.IndependentBlendEnable ? index : 0];
        if (input.BlendEnable && input.LogicOpEnable) throw std::invalid_argument("Offline graphics blending and logic operations cannot both be enabled");
        if ((input.RenderTargetWriteMask & ~15) != 0) throw std::invalid_argument("Offline graphics color write mask is invalid");
        if (input.LogicOpEnable && !features.logicOp) throw std::runtime_error("Offline graphics logic operations are unsupported by the device");
        if (index == 0) { blend.logicOpEnable = input.LogicOpEnable != FALSE; if (input.LogicOpEnable) blend.logicOp = Logic(input.LogicOp); }
        if (blend.logicOpEnable != static_cast<VkBool32>(input.LogicOpEnable != FALSE) || (input.LogicOpEnable && blend.logicOp != Logic(input.LogicOp))) throw std::runtime_error("Offline graphics per-attachment logic operations are not implemented");
        auto& output = colors[index];
        output.blendEnable = input.BlendEnable != FALSE;
        output.colorWriteMask = input.RenderTargetWriteMask;
        if (!input.BlendEnable) continue;
        bool dual = Dual(input.SrcBlend) || Dual(input.DestBlend) || Dual(input.SrcBlendAlpha) || Dual(input.DestBlendAlpha);
        if (dual && (!features.dualSrcBlend || index >= context.GetProperties().limits.maxFragmentDualSrcAttachments)) throw std::runtime_error("Offline graphics dual source blending exceeds the device limit");
        output.srcColorBlendFactor = Factor(input.SrcBlend);
        output.dstColorBlendFactor = Factor(input.DestBlend);
        output.colorBlendOp = Operation(input.BlendOp);
        output.srcAlphaBlendFactor = Factor(input.SrcBlendAlpha);
        output.dstAlphaBlendFactor = Factor(input.DestBlendAlpha);
        output.alphaBlendOp = Operation(input.BlendOpAlpha);
    }
}
