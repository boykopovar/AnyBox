#pragma once

#include "../State.hpp"
#include "../stream/Description.hpp"

namespace AnyBox::Graphics {
PipelineState* CreatePreparedGraphics(Device& device, const PipelineStream& stream);
}
