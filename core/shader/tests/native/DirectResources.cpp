#include <anybox/shader/NativeArguments.hpp>
#include "NativeFixtures.hpp"

void CheckNativeDirectResources() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    using namespace Fixtures;
    RootSignatureLayout root{2, 0, 0, {}};
    root.parameters.push_back({RootParameterKind::ShaderResource, 0, 7, 3, 0, 0, {}});
    NativeBindings native;
    native.roots.push_back({NativeRootKind::Descriptor, 0, 2, 7, 8, 0});
    native.resources.push_back({NativeResourceKind::ShaderResource, NativeRootKind::Descriptor, 0, 7, 1, 4, 8, 0});
    auto buffers = ReadNativeDirectBufferBindings(native, root, 1);
    Require(buffers == std::vector<BufferBinding>{{ResourceClass::ShaderResource, 0, 3, 7, 1, BufferKind::Raw}});
    Require(ReadNativeDirectBufferBindings({}, root, 5).empty());
    auto invalid = native;
    invalid.resources[0].rootParameter = 1;
    Reject<std::invalid_argument>([&] { ReadNativeDirectBufferBindings(invalid, root, 1); });
    invalid = native;
    invalid.resources[0].kind = NativeResourceKind::Sampler;
    Reject<std::runtime_error>([&] { ReadNativeDirectBufferBindings(invalid, root, 1); });
    invalid = native;
    invalid.resources[0].rootKind = NativeRootKind::Table;
    Reject<std::runtime_error>([&] { ReadNativeDirectBufferBindings(invalid, root, 1); });
    invalid = native;
    invalid.resources[0].firstShaderRegister = 8;
    Reject<std::invalid_argument>([&] { ReadNativeDirectBufferBindings(invalid, root, 1); });
    invalid = native;
    invalid.roots.clear();
    Reject<std::invalid_argument>([&] { ReadNativeDirectBufferBindings(invalid, root, 1); });
    invalid = native;
    invalid.resources.push_back(invalid.resources[0]);
    Reject<std::invalid_argument>([&] { ReadNativeDirectBufferBindings(invalid, root, 1); });
    root.parameters[0].visibility = 5;
    Reject<std::invalid_argument>([&] { ReadNativeDirectBufferBindings(native, root, 1); });
    Require(ReadNativeDirectBufferBindings(native, root, 5) == buffers);
    root.parameters[0].kind = RootParameterKind::UnorderedAccess;
    native.resources[0].kind = NativeResourceKind::UnorderedAccess;
    auto unordered = ReadNativeDirectBufferBindings(native, root, 5);
    Require(unordered.size() == 1 && unordered[0].resourceClass == ResourceClass::UnorderedAccess);
}
