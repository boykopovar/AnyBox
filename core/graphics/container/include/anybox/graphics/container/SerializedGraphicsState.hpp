#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Graphics::Container {
struct SerializedStencilFace {
    std::uint32_t fail;
    std::uint32_t depthFail;
    std::uint32_t pass;
    std::uint32_t comparison;
};

struct SerializedDepthState {
    bool depthEnable;
    bool depthWrite;
    std::uint32_t comparison;
    bool stencilEnable;
    bool boundsEnable;
    bool separateBackFace;
    std::uint8_t readMask;
    std::uint8_t writeMask;
    std::uint8_t backReadMask;
    std::uint8_t backWriteMask;
    std::uint8_t reference;
    std::uint8_t backReference;
    std::uint8_t operationValue;
    std::uint8_t backOperationValue;
    SerializedStencilFace front;
    SerializedStencilFace back;
};

struct SerializedRasterizerState {
    std::uint32_t fill;
    std::uint32_t cull;
    bool frontCounterClockwise;
    std::int32_t depthBias;
    float depthBiasClamp;
    float slopeScaledDepthBias;
    bool depthClip;
    bool multisample;
    bool antialiasedLine;
    std::uint32_t conservative;
    std::uint32_t forcedSampleCount;
};

struct SerializedBlendTarget {
    bool enable;
    bool logicEnable;
    std::uint32_t source;
    std::uint32_t destination;
    std::uint32_t operation;
    std::uint32_t sourceAlpha;
    std::uint32_t destinationAlpha;
    std::uint32_t alphaOperation;
    std::uint32_t logicOperation;
    std::uint8_t writeMask;
};

struct SerializedGraphicsFixedState {
    SerializedDepthState depth;
    SerializedRasterizerState rasterizer;
    std::array<SerializedBlendTarget, 8> targets;
    std::array<std::uint32_t, 8> formats;
    bool alphaToCoverage;
    bool independentBlend;
    std::uint32_t sampleMask;
    std::uint32_t targetCount;
    std::uint32_t depthFormat;
    std::uint32_t samples;
    std::uint32_t sampleQuality;
    std::uint32_t topology;
    std::uint32_t stripCut;
    std::uint32_t flags;
};

SerializedGraphicsFixedState ReadSerializedGraphicsFixedState(std::span<const std::byte> state, std::span<const std::byte> auxiliary);
}
