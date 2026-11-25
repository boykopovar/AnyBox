#include "Offline.hpp"
#include "Cache.hpp"
#include <anybox/shader/Requirements.hpp>

AnyBox::Shader::ComputeArtifact AnyBox::Graphics::LoadPreparedCompute(std::span<const std::byte> code) {
    auto digest = Shader::HashShader(code);
    auto bytes = ReadPreparedArtifact(Shader::DigestName(digest) + ".abxc", "Prepared compute shader");
    auto artifact = Shader::DecodeComputeArtifact(bytes, digest);
    auto required = Shader::GetComputeWaveSize(code);
    if (required && artifact.waveSize != *required) throw std::runtime_error("Prepared compute shader wave size does not match its source");
    return artifact;
}
