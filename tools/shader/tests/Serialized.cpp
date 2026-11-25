#include "../Serialized.hpp"
#include "SerializedReference.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <anybox/ExceptionTest.hpp>
#include <array>
#include <stdexcept>

void CheckSerializedStageTool();

namespace {
using namespace AnyBox::Graphics::Container;
using namespace AnyBox::Shader::Tools;

void Require(bool value) {
    if (!value) throw std::runtime_error("Serialized shader tool test failed");
}

template<class TCallable>
void Reject(TCallable&& callable) {
    bool caught = false;
    try { callable(); } catch (const std::exception&) { caught = true; }
    Require(caught);
}

void Set(std::span<std::byte> data, std::size_t offset, std::uint32_t value) {
    for (std::size_t byte = 0; byte < 4; ++byte) data[offset + byte] = static_cast<std::byte>(value >> (byte * 8));
}

void CheckTool(std::uint32_t wave) {
    std::array<std::byte, 28> dispatch{};
    Set(dispatch, 0, 0x4005CF00);
    Set(dispatch, 4, 0x6C090102);
    Set(dispatch, 12, 1 | (1 << 16));
    Set(dispatch, 16, 1 | (wave == 32 ? 0x100000u : 0u));
    Set(dispatch, 20, 0x402C0000);
    std::array<std::byte, 4> code{};
    Set(code, 0, 0xBF810000);
    std::array<Part, 2> parts{{{0, dispatch}, {6, code}}};
    auto payload = BuildFspx(parts);
    auto info = InspectSerializedCompute(payload);
    Require(info.find("threads=1,1,1") != std::string::npos && info.find("wave=" + std::to_string(wave)) != std::string::npos);
    auto instructions = InspectSerializedInstructions(payload, wave);
    Require(instructions.find("instructions=1 blocks=1") != std::string::npos);
    auto audit = AuditSerializedCompute(payload);
    Require(audit.starts_with("words=") && audit.find("images=0") != std::string::npos);
    auto outer = BuildDxbc(std::array<Part, 1>{{{0x43434341, payload}}});
    Require(InspectSerializedCompute(outer) == info);
    Require(InspectSerializedInstructions(outer, wave) == instructions);
    Require(AuditSerializedCompute(outer) == audit);
    Reject([&] { (void)InspectSerializedInstructions(payload, wave == 32 ? 64u : 32u); });
    auto missing = BuildDxbc(std::array<Part, 1>{{{0x12345678, payload}}});
    Reject([&] { (void)AuditSerializedCompute(missing); });
    Set(dispatch, 8, 1);
    auto unsupported = BuildFspx(parts);
    Reject([&] { (void)AuditSerializedCompute(unsupported); });
    Set(dispatch, 8, 0);
    Set(code, 0, 0xFFFFFFFF);
    auto invalid = BuildFspx(parts);
    Reject([&] { (void)AuditSerializedCompute(invalid); });
}

void CheckReference() {
    SerializedTests::ReferenceFixture fixture;
    auto payload = fixture.Payload();
    auto reference = fixture.Source();
    auto audit = AuditSerializedCompute(payload, reference, fixture.root);
    Require(audit.starts_with("words=") && audit.find("images=0") != std::string::npos);
    auto identity = fixture;
    identity.hash[4] ^= std::byte{1};
    Reject([&] { (void)AuditSerializedCompute(payload, identity.Source(), fixture.root); });
    identity = fixture;
    identity.hash[0] = std::byte{1};
    Reject([&] { (void)AuditSerializedCompute(payload, identity.Source(), fixture.root); });
    auto threads = fixture;
    Set(threads.program, 0x13C, 1);
    Reject([&] { (void)AuditSerializedCompute(payload, threads.Source(), fixture.root); });
    Reject([&] { (void)AuditSerializedCompute(payload, reference, BuildDxbc({})); });
    auto resources = fixture;
    resources.program.resize(0x1A4);
    Set(resources.program, 0xF0, 0x180);
    Set(resources.program, 0xFC, 1);
    Reject([&] { (void)AuditSerializedCompute(payload, resources.Source(), fixture.root); });
}
}

int main() {
    return AnyBox::Tests::Run([] { CheckTool(32); CheckTool(64); CheckReference(); CheckSerializedStageTool(); });
}
