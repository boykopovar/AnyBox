#pragma once

#include "Description.hpp"

namespace AnyBox::Graphics {
struct GraphicsPipelineDescription {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC state{};
    BOOL depthBounds = FALSE;
    D3D12_VIEW_INSTANCING_DESC viewInstancing{};
};
GraphicsPipelineDescription ReadGraphicsPipeline(const PipelineStream& stream);
GraphicsPipelineDescription DefaultGraphicsPipeline();
}
