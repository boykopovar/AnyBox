#include <anybox/shader/Native.hpp>
#include <anybox/ExceptionTest.hpp>
#include "NativeFixtures.hpp"
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <utility>

void CheckNativeCompute();
void CheckRawNativeFrontend();
void CheckNativeHashPrologue();
void CheckNativeRawRootArguments();
void CheckNativePixel();
void CheckNativeLinkage();
void CheckNativeVertex();
void CheckNativeVertexOffsets();
void CheckNativeInterpolation();
void CheckNativePixelExportSources();
void CheckNativeFloatingProfile();
void CheckNativeRootArguments();
void CheckNativeGraphicsRootArguments();
void CheckNativeDirectResources();
void CheckNativeGraphicsCompilation();
void CheckNativeFlow();
void CheckNativeScratchWrites();
void CheckNativeImages();
void CheckNativeFmaDecoding();
void CheckNativePermlaneDecoding();
void CheckDescriptorChoices();
void CheckFirstLaneGuards();
void CheckBallotFirstLaneGuards();
void CheckBooleanFirstLaneGuards();
void CheckBallotLoopGuards();

int main() {
    return AnyBox::Tests::Run([] {
        CheckNativeCompute();
        CheckRawNativeFrontend();
        CheckNativeHashPrologue();
        CheckNativeRawRootArguments();
        CheckNativePixel();
        CheckNativeLinkage();
        CheckNativeVertex();
        CheckNativeVertexOffsets();
        CheckNativeInterpolation();
        CheckNativePixelExportSources();
        CheckNativeFloatingProfile();
        CheckNativeRootArguments();
        CheckNativeGraphicsRootArguments();
        CheckNativeDirectResources();
        CheckNativeGraphicsCompilation();
        CheckNativeFlow();
        CheckNativeScratchWrites();
        CheckNativeImages();
        CheckNativeFmaDecoding();
        CheckNativePermlaneDecoding();
        CheckDescriptorChoices();
        CheckFirstLaneGuards();
        CheckBallotFirstLaneGuards();
        CheckBooleanFirstLaneGuards();
        CheckBallotLoopGuards();
        using namespace Fixtures;
        auto data = MakeNativeProgram();
        AnyBox::Shader::NativeFrontend frontend(data);
        Require(frontend.GetWaveSize() == 32 && frontend.GetCode().size() == 6);
        Require(frontend.GetDecoded().instructions.size() == 2 && frontend.GetGraph().blocks.size() == 1);
        Require(frontend.GetDecoded().instructions[1].op == ShaderRecompiler::RdnaOpcode::SEndpgm);
        auto* code = frontend.GetCode().data();
        AnyBox::Shader::NativeFrontend moved(std::move(frontend));
        Require(moved.GetCode().data() == code && moved.GetDecoded().code.data() == code);
        AnyBox::Shader::NativeFrontend assigned(data);
        assigned = std::move(moved);
        Require(assigned.GetCode().data() == code && assigned.GetDecoded().code.data() == code);
        auto wide = data;
        Set(wide, 0xD4, 0);
        Require(AnyBox::Shader::NativeFrontend(wide).GetWaveSize() == 64);
        Set(data, 0x168, 0xBF820100);
        Reject<std::out_of_range>([&] { AnyBox::Shader::NativeFrontend rejected(data); });
        Set(data, 0x168, 0xFFFFFFFF);
        Reject<std::exception>([&] { AnyBox::Shader::NativeFrontend rejected(data); });
    });
}
