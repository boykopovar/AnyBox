#include "State.hpp"
#include <cmath>

void AnyBox::Graphics::GraphicsFixedState::initializeRasterizer(const D3D12_RASTERIZER_DESC& source) {
    const auto& features = context.GetEnabledFeatures();
    if (source.FillMode != D3D12_FILL_MODE_SOLID && source.FillMode != D3D12_FILL_MODE_WIREFRAME) throw std::invalid_argument("Offline graphics fill mode is invalid");
    if (source.FillMode == D3D12_FILL_MODE_WIREFRAME && !features.fillModeNonSolid) throw std::runtime_error("Offline graphics wireframe rasterization is unsupported by the device");
    if (source.CullMode < D3D12_CULL_MODE_NONE || source.CullMode > D3D12_CULL_MODE_BACK) throw std::invalid_argument("Offline graphics cull mode is invalid");
    if (!source.DepthClipEnable && !features.depthClamp) throw std::runtime_error("Offline graphics depth clamp is unsupported by the device");
    if (source.ConservativeRaster != D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF || source.ForcedSampleCount != 0 || source.AntialiasedLineEnable || source.MultisampleEnable) throw std::runtime_error("Offline graphics conservative, forced sample or line rasterization mode is not implemented");
    if (!std::isfinite(source.DepthBiasClamp) || !std::isfinite(source.SlopeScaledDepthBias)) throw std::invalid_argument("Offline graphics depth bias is not finite");
    if (source.DepthBiasClamp != 0 && !features.depthBiasClamp) throw std::runtime_error("Offline graphics depth bias clamping is unsupported by the device");
    if (static_cast<std::int64_t>(static_cast<float>(source.DepthBias)) != source.DepthBias) throw std::runtime_error("Offline graphics depth bias cannot be represented exactly by the host pipeline");
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = !source.DepthClipEnable;
    rasterizer.polygonMode = source.FillMode == D3D12_FILL_MODE_SOLID ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE;
    rasterizer.cullMode = source.CullMode == D3D12_CULL_MODE_NONE ? VK_CULL_MODE_NONE : source.CullMode == D3D12_CULL_MODE_FRONT ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = source.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = source.DepthBias != 0 || source.SlopeScaledDepthBias != 0;
    rasterizer.depthBiasConstantFactor = static_cast<float>(source.DepthBias);
    rasterizer.depthBiasClamp = source.DepthBiasClamp;
    rasterizer.depthBiasSlopeFactor = source.SlopeScaledDepthBias;
    rasterizer.lineWidth = 1;
}
