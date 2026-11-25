#include <anybox/shader/ResourcePlan.hpp>
#include <anybox/ExceptionTest.hpp>
#include "../native/RootFixture.hpp"
#include <Optimization/ResourceMaterializer.hpp>
#include <algorithm>
#include <array>

void CheckNativeComputeArguments();
void CheckNativeGraphicsArguments();

namespace {
using namespace AnyBox::Shader;
using namespace ShaderRecompiler;
using namespace Fixtures;

class Memory {
public:
    static bool Read(void* context, std::uint64_t address, std::uint32_t* value) {
        auto& memory = *static_cast<Memory*>(context);
        if (address != 0x10000 || !value) return false;
        ++memory.hits;
        *value = 0x12345678;
        return true;
    }
    std::uint32_t GetHits() const noexcept { return hits; }

private:
    std::uint32_t hits = 0;
};

IrResourcePlan MakePlan() {
    NativeTests::RootFixture fixture(32, 16);
    auto translation = fixture.Translate();
    (void)CompileCompute(translation.program, translation.input, BufferTests::Target(), {0, 0, 0, 128});
    return ResourceMaterializer{}.ExtractPlan(translation.program);
}

ResourceSnapshot Evaluate(const IrResourcePlan& plan) {
    std::array<std::uint32_t, 128> registers{};
    registers[0] = 0x10000;
    Memory memory;
    SrtRuntime runtime;
    runtime.userData = registers;
    runtime.readMemory = &Memory::Read;
    runtime.userContext = &memory;
    ResourceSnapshot snapshot;
    ResourceMaterializer{}.Materialize(plan, runtime, snapshot);
    Require(memory.GetHits() != 0 && snapshot.srtPoison.empty() && snapshot.nullRootReads == 0);
    Require(std::ranges::find(snapshot.flattenedSrt, 0x12345678u) != snapshot.flattenedSrt.end());
    return snapshot;
}

void Rehash(std::vector<std::byte>& data) {
    Set(data, 12, static_cast<std::uint32_t>(data.size() - 80));
    auto hash = HashShader(std::span(data).subspan(80));
    std::ranges::copy(hash, data.begin() + 48);
}

void Check() {
    auto source = HashShader(std::as_bytes(std::span("synthetic resource plan", 23)));
    auto plan = MakePlan();
    auto expected = Evaluate(plan);
    auto bytes = EncodeResourcePlan(plan, source);
    auto restored = DecodeResourcePlan(bytes, source, IrShaderStage::Compute);
    auto actual = Evaluate(restored);
    Require(actual.flattenedSrt == expected.flattenedSrt && actual.userData == expected.userData);
    Require(restored.stage == plan.stage && restored.userDataBase == plan.userDataBase && restored.userDataCount == plan.userDataCount);
    Require(EncodeResourcePlan(restored, source) == bytes);
    for (std::size_t size = 0; size < bytes.size(); ++size) Reject<std::exception>([&] { (void)DecodeResourcePlan(std::span(bytes).first(size), source, IrShaderStage::Compute); });
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        auto damaged = bytes;
        damaged[index] ^= std::byte{1};
        Reject<std::exception>([&] { (void)DecodeResourcePlan(damaged, source, IrShaderStage::Compute); });
    }
    auto wrongSource = source;
    wrongSource[0] ^= std::byte{1};
    Reject<std::exception>([&] { (void)DecodeResourcePlan(bytes, wrongSource, IrShaderStage::Compute); });
    Reject<std::exception>([&] { (void)DecodeResourcePlan(bytes, source, IrShaderStage::Vertex); });
    Reject<std::exception>([&] { (void)DecodeResourcePlan(bytes, source, IrShaderStage::Unknown); });
    auto version = bytes;
    Set(version, 80, ResourcePlanFormatVersion + 1);
    Rehash(version);
    Reject<std::exception>([&] { (void)DecodeResourcePlan(version, source, IrShaderStage::Compute); });
    auto truncated = bytes;
    truncated.pop_back();
    Rehash(truncated);
    Reject<std::exception>([&] { (void)DecodeResourcePlan(truncated, source, IrShaderStage::Compute); });
    auto trailing = bytes;
    trailing.push_back(std::byte{});
    Reject<std::exception>([&] { (void)DecodeResourcePlan(trailing, source, IrShaderStage::Compute); });
    plan.srtPlanComplete = false;
    Reject<std::exception>([&] { (void)EncodeResourcePlan(plan, source); });
    plan.srtPlanComplete = true;
    plan.resourceTrackingComplete = false;
    Reject<std::exception>([&] { (void)EncodeResourcePlan(plan, source); });
    plan.resourceTrackingComplete = true;
    plan.stage = IrShaderStage::Unknown;
    Reject<std::exception>([&] { (void)EncodeResourcePlan(plan, source); });
    plan.stage = IrShaderStage::Compute;
    plan.userDataBase = 129;
    Reject<std::exception>([&] { (void)EncodeResourcePlan(plan, source); });
}
}

int main() {
    return AnyBox::Tests::Run([] { Check(); CheckNativeComputeArguments(); CheckNativeGraphicsArguments(); });
}
