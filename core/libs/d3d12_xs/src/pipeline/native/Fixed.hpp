#pragma once

#include "../stream/Graphics.hpp"
#include <anybox/graphics/container/SerializedGraphicsState.hpp>

namespace AnyBox::Graphics {
GraphicsPipelineDescription MakeSerializedFixedDescription(const Container::SerializedGraphicsFixedState& source);
}
