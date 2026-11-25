#include <anybox/shader/Artifact.hpp>
#include <anybox/ExceptionTest.hpp>
#include "Tests.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

using namespace AnyBox::Shader;

void CheckGraphicsArtifacts();
void CheckGraphicsLayoutIdentity();

namespace {
void Require(bool condition) {
    if (!condition) throw std::runtime_error("Shader artifact test failed");
}

template<typename TFunction>
void Reject(TFunction&& function) {
    try { function(); }
    catch (const std::exception&) { return; }
    throw std::runtime_error("Shader artifact accepted invalid data");
}

void Check() {
    auto empty = HashShader({});
    Require(DigestName(empty) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    constexpr char source[] = "abc";
    auto digest = HashShader(std::as_bytes(std::span(source, 3)));
    Require(DigestName(digest) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    ComputeArtifact artifact;
    artifact.sourceHash = digest;
    artifact.threads = {1, 1, 1};
    artifact.pushConstantSize = 32;
    artifact.pushDataStart = 0;
    artifact.memoryOffset = 3;
    artifact.userRegisters = {0, 4, 11};
    artifact.descriptors = {{59, {}}, {60, {0, 1}}};
    artifact.buffers = {{ResourceClass::ShaderResource, 7, 2, 4, 1, BufferKind::Structured, 32}};
    artifact.code = AnyBox::Vulkan::Tests::CompileCompute("#version 450\nlayout(local_size_x=1) in; void main() {}\n");
    auto bytes = EncodeComputeArtifact(artifact);
    Require(DecodeComputeArtifact(bytes, digest) == artifact);
    Require(EncodeComputeArtifact(DecodeComputeArtifact(bytes, digest)) == bytes);
    Reject([&] { DecodeComputeArtifact(bytes, empty); });
    for (std::size_t size = 0; size < bytes.size(); ++size) Reject([&] { DecodeComputeArtifact(std::span(bytes).first(size), digest); });
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        auto changed = bytes;
        changed[index] ^= std::byte{1};
        Reject([&] { DecodeComputeArtifact(changed, digest); });
    }
    auto changed = bytes;
    changed.push_back(std::byte{0});
    Reject([&] { DecodeComputeArtifact(changed, digest); });
    auto mutate = [&](std::size_t word, std::uint32_t value) {
        auto selected = bytes;
        for (unsigned index = 0; index < 4; ++index) selected[80 + word * 4 + index] = static_cast<std::byte>((value >> (index * 8)) & 255);
        auto hash = HashShader(std::span(selected).subspan(80));
        std::copy(hash.begin(), hash.end(), selected.begin() + 48);
        Reject([&] { DecodeComputeArtifact(selected, digest); });
    };
    mutate(0, 2);
    mutate(1, 13);
    mutate(2, 2);
    mutate(3, 0);
    mutate(6, 128);
    mutate(8, 129);
    mutate(10, 32);
    mutate(12, 0);
    mutate(14, 2);
    mutate(17, 16);
    mutate(20, UINT32_MAX);
    mutate(21, UINT32_MAX);
    auto previous = bytes;
    previous[8] = std::byte{1};
    Reject([&] { DecodeComputeArtifact(previous, digest); });
    for (auto wave : {32u, 64u}) {
        for (auto subgroup : {32u, 64u}) {
            auto selected = artifact;
            selected.waveSize = wave;
            selected.hostSubgroupSize = subgroup;
            Require(DecodeComputeArtifact(EncodeComputeArtifact(selected), digest) == selected);
        }
    }
    auto invalid = artifact;
    invalid.waveSize = 64;
    invalid.executionFlags = NativeComputeInvocations;
    Require(DecodeComputeArtifact(EncodeComputeArtifact(invalid), digest) == invalid);
    invalid.hostSubgroupSize = 64;
    Reject([&] { EncodeComputeArtifact(invalid); });
    invalid.hostSubgroupSize = 32;
    invalid.waveSize = 32;
    Reject([&] { EncodeComputeArtifact(invalid); });
    invalid.waveSize = 64;
    invalid.splitStateLanes = 1;
    Reject([&] { EncodeComputeArtifact(invalid); });
    invalid = artifact;
    invalid.userRegisters = {0, 0};
    Reject([&] { EncodeComputeArtifact(invalid); });
    invalid = artifact;
    invalid.descriptors.push_back(invalid.descriptors.front());
    Reject([&] { EncodeComputeArtifact(invalid); });
    invalid = artifact;
    invalid.code.push_back(0);
    Reject([&] { EncodeComputeArtifact(invalid); });
}
}

int main() { return AnyBox::Tests::Run([] { Check(); CheckGraphicsArtifacts(); CheckGraphicsLayoutIdentity(); }); }
