#include <anybox/shader/Inspection.hpp>
#include "Audit.hpp"
#include "NativeAudit.hpp"
#include "Compilation.hpp"
#include "Files.hpp"
#include "Prepare.hpp"
#include "Serialized.hpp"
#include "serialized/Prepare.hpp"
#include "graphics/Options.hpp"
#include <anybox/shader/Resources.hpp>
#include <anybox/shader/Compute.hpp>
#include <anybox/graphics/container/SerializedStage.hpp>
#include <array>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    try {
        if (argc == 5 && std::string_view(argv[1]) == "--serialized-stage-code") {
            auto wave = std::string_view(argv[4]);
            if (wave != "32" && wave != "64") throw std::invalid_argument("Serialized stage wave size must be 32 or 64");
            std::cout << AnyBox::Shader::Tools::InspectSerializedStageInstructions(AnyBox::Shader::Tools::ReadInput(argv[2]), AnyBox::Shader::Tools::ReadInput(argv[3]), wave == "32" ? 32u : 64u);
            if (!std::cout) throw std::runtime_error("Cannot write serialized stage instructions");
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--serialized-stage-info") {
            auto metadata = AnyBox::Graphics::Container::ReadSerializedStageMetadata(AnyBox::Shader::Tools::ReadInput(argv[2]), AnyBox::Shader::Tools::ReadInput(argv[3]));
            std::cout << "code_words=" << metadata.codeWords << " control=" << metadata.control << " resources=" << metadata.resources.size() << " declarations=" << metadata.declarations.size() << '\n';
            if (!std::cout) throw std::runtime_error("Cannot write serialized stage information");
            return 0;
        }
        if ((argc == 5 || argc == 6) && (std::string_view(argv[1]) == "--prepare-serialized-compute" || std::string_view(argv[1]) == "--check-serialized-compute")) {
            auto subgroup = argc == 6 ? std::string_view(argv[5]) : "32";
            if (subgroup != "32" && subgroup != "64") throw std::invalid_argument("Serialized compute subgroup size must be 32 or 64");
            if (std::string_view(argv[1]) == "--check-serialized-compute") AnyBox::Shader::Tools::CheckSerializedComputeGpu(argv[2], argv[3], argv[4], subgroup == "32" ? 32u : 64u);
            else AnyBox::Shader::Tools::PrepareSerializedCompute(argv[2], argv[3], argv[4], subgroup == "32" ? 32u : 64u);
            return 0;
        }
        if (argc == 5 && std::string_view(argv[1]) == "--serialized-audit") {
            std::cout << AnyBox::Shader::Tools::AuditSerializedCompute(AnyBox::Shader::Tools::ReadInput(argv[2]), AnyBox::Shader::Tools::ReadInput(argv[3]), AnyBox::Shader::Tools::ReadInput(argv[4]));
            if (!std::cout) throw std::runtime_error("Cannot write serialized native argument audit results");
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--serialized-audit") {
            std::cout << AnyBox::Shader::Tools::AuditSerializedCompute(AnyBox::Shader::Tools::ReadInput(argv[2]));
            if (!std::cout) throw std::runtime_error("Cannot write serialized pipeline audit results");
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--serialized-code") {
            auto wave = std::string_view(argv[3]);
            if (wave != "32" && wave != "64") throw std::invalid_argument("Serialized shader wave size must be 32 or 64");
            std::cout << AnyBox::Shader::Tools::InspectSerializedInstructions(AnyBox::Shader::Tools::ReadInput(argv[2]), wave == "32" ? 32u : 64u);
            if (!std::cout) throw std::runtime_error("Cannot write serialized pipeline instructions");
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--serialized-info") {
            std::cout << AnyBox::Shader::Tools::InspectSerializedCompute(AnyBox::Shader::Tools::ReadInput(argv[2]));
            if (!std::cout) throw std::runtime_error("Cannot write serialized pipeline information");
            return 0;
        }
        if (argc >= 2 && (std::string_view(argv[1]) == "--prepare-graphics" || std::string_view(argv[1]) == "--verify-graphics")) {
            std::vector<std::string_view> arguments(argv + 2, argv + argc);
            AnyBox::Shader::Tools::PrepareGraphics(AnyBox::Shader::Tools::ParseGraphicsOptions(arguments), std::string_view(argv[1]) == "--verify-graphics");
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--prepare-compute") {
            AnyBox::Shader::Tools::PrepareCompute(argv[2], argv[3]);
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--audit-stream") return AnyBox::Shader::Tools::AuditStream();
        bool nativeCompile = argc == 4 && std::string_view(argv[1]) == "--compile-native";
        bool artifactCommand = nativeCompile || (argc == 4 && (std::string_view(argv[1]) == "--compile" || std::string_view(argv[1]) == "--verify" || std::string_view(argv[1]) == "--check-gpu"));
        if (!artifactCommand && (argc != 3 || (std::string_view(argv[1]) != "--inspect" && std::string_view(argv[1]) != "--bindings" && std::string_view(argv[1]) != "--compute" && std::string_view(argv[1]) != "--native-code" && std::string_view(argv[1]) != "--native-info" && std::string_view(argv[1]) != "--native-ir" && std::string_view(argv[1]) != "--native-audit"))) throw std::invalid_argument("Usage: anybox_shader <--inspect|--bindings|--compute|--native-code|--native-info|--native-ir|--native-audit> <shader.dxbc> | <--compile|--compile-native|--verify|--check-gpu> <shader.dxbc> <shader.abxc> | --prepare-compute <binary> <directory> | --audit-stream");
        auto data = AnyBox::Shader::Tools::ReadInput(argv[2]);
        if (artifactCommand) {
            auto artifact = nativeCompile ? AnyBox::Shader::Tools::CompileNative(data) : std::string_view(argv[1]) == "--compile" ? AnyBox::Shader::Tools::CompileDxil(data) : AnyBox::Shader::DecodeComputeArtifact(AnyBox::Shader::Tools::ReadInput(argv[3]), AnyBox::Shader::HashShader(data));
            if (nativeCompile || std::string_view(argv[1]) == "--compile") AnyBox::Shader::Tools::WriteOutput(argv[3], AnyBox::Shader::EncodeComputeArtifact(artifact));
            if (std::string_view(argv[1]) == "--check-gpu") AnyBox::Shader::Tools::CheckArtifactGpu(artifact);
            std::cout << "source=" << AnyBox::Shader::DigestName(artifact.sourceHash) << " words=" << artifact.code.size() << " buffers=" << artifact.buffers.size() << " arguments=" << artifact.userRegisters.size() << " push_bytes=" << artifact.pushConstantSize << " wave=" << artifact.waveSize << " subgroup=" << artifact.hostSubgroupSize << '\n';
        } else if (std::string_view(argv[1]) == "--native-audit") {
            auto words = AnyBox::Shader::Tools::AuditNative(data);
            std::cout << "words=" << words << '\n';
        } else if (std::string_view(argv[1]) == "--native-code") {
            std::cout << AnyBox::Shader::Tools::InspectNativeCode(data);
        } else if (std::string_view(argv[1]) == "--native-info") {
            std::cout << AnyBox::Shader::Tools::InspectNative(data);
        } else if (std::string_view(argv[1]) == "--native-ir") {
            std::cout << AnyBox::Shader::Tools::InspectNativeIr(data);
        } else if (std::string_view(argv[1]) == "--inspect") {
            std::cout << AnyBox::Shader::InspectDxil(data);
        } else if (std::string_view(argv[1]) == "--compute") {
            auto info = AnyBox::Shader::ReadDxilComputeInfo(data);
            std::cout << "entry=" << info.entryPoint << " model=cs_6_" << info.shaderModelMinor << " threads=" << info.threads[0] << ',' << info.threads[1] << ',' << info.threads[2] << " flags=" << info.flags << " wave=" << info.minimumWaveSize << ',' << info.maximumWaveSize << ',' << info.preferredWaveSize << '\n';
        } else {
            constexpr std::array classes{"SRV", "UAV", "CBV", "Sampler"};
            constexpr std::array kinds{"raw", "structured", "constant"};
            for (const auto& binding : AnyBox::Shader::ReadDxilBufferBindings(data)) {
                std::cout << classes.at(static_cast<unsigned>(binding.resourceClass)) << " range=" << binding.rangeId << " space=" << binding.space << " register=" << binding.firstRegister << " count=" << binding.count << " kind=" << kinds.at(static_cast<unsigned>(binding.kind)) << " stride=" << binding.stride << " bytes=" << binding.byteSize << '\n';
            }
        }
        if (!std::cout) throw std::runtime_error("Cannot write shader assembly");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
