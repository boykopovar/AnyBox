#include "Fixture.hpp"
#include "Fixtures.hpp"
#include <anybox/ExceptionTest.hpp>

void CheckBufferArguments();
void CheckRootArguments();
void CheckRootWriter();
void CheckNativeRootWriter();
void CheckSerializedComputeRoots();
void CheckSerializedGraphicsRoots();

namespace {
void CheckBinding(unsigned category, unsigned stride) {
    ResourceTests::Fixture fixture;
    fixture.Set(category, fixture.Buffer(category, stride));
    AnyBox::Shader::Dxil::ResourceLayout layout(fixture.Module());
    Fixtures::Require(layout.GetBindings().size() == 1);
    const auto& binding = layout.GetBindings().front();
    Fixtures::Require(static_cast<unsigned>(binding.resourceClass) == category && binding.rangeId == 7 && binding.space == 2 && binding.firstRegister == 4 && binding.count == 3);
    if (category == 2) Fixtures::Require(binding.kind == AnyBox::Shader::BufferKind::Constant && binding.byteSize == 52);
    else Fixtures::Require(binding.stride == stride && binding.globallyCoherent == (category == 1));
    for (unsigned index = 4; index < 7; ++index) {
        auto handle = layout.ResolveHandle(*fixture.Handle(category, index));
        Fixtures::Require(handle.binding == &binding && !handle.nonUniform);
    }
    for (unsigned index : {0u, 3u, 7u, UINT32_MAX}) Fixtures::Reject<std::runtime_error>([&] { layout.ResolveHandle(*fixture.Handle(category, index)); });
    auto dynamic = layout.ResolveHandle(*fixture.Handle(category, 0, true));
    Fixtures::Require(dynamic.nonUniform && dynamic.index != nullptr);
    Fixtures::Reject<std::runtime_error>([&] { layout.ResolveHandle(*fixture.Handle((category + 1) % 3, 4)); });
}

void CheckInvalid(unsigned stride, unsigned firstRegister, unsigned count) {
    ResourceTests::Fixture fixture;
    fixture.Set(0, fixture.Buffer(0, stride, firstRegister, count));
    Fixtures::Reject<std::runtime_error>([&] { AnyBox::Shader::Dxil::ResourceLayout layout(fixture.Module()); });
}
}

int main() {
    return AnyBox::Tests::Run([] {
        ResourceTests::CheckComputeEntries();
        CheckBufferArguments();
        CheckRootArguments();
        CheckRootWriter();
        CheckNativeRootWriter();
        CheckSerializedComputeRoots();
        CheckSerializedGraphicsRoots();
        for (unsigned category : {0u, 1u}) {
            CheckBinding(category, 0);
            CheckBinding(category, 32);
        }
        CheckBinding(2, 0);
        CheckInvalid(3, 0, 1);
        CheckInvalid(2052, 0, 1);
        CheckInvalid(32, 0, 0);
        CheckInvalid(32, UINT32_MAX, 2);
        ResourceTests::Fixture fixture;
        fixture.Set(0, fixture.Buffer(0, 32, 4, UINT32_MAX));
        AnyBox::Shader::Dxil::ResourceLayout layout(fixture.Module());
        Fixtures::Require(layout.ResolveHandle(*fixture.Handle(0, UINT32_MAX)).binding->count == UINT32_MAX);
    });
}
