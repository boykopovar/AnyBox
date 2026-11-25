#include <anybox/shader/Artifact.hpp>
#include <anybox/platform/Windows.hpp>
#include <bcrypt.h>
#include <limits>
#include <stdexcept>
#include <memory>

namespace {
void CheckStatus(NTSTATUS status) {
    if (status < 0) throw std::runtime_error("Windows SHA-256 computation failed: " + std::to_string(status));
}

class Algorithm {
public:
    Algorithm() { CheckStatus(BCryptOpenAlgorithmProvider(&handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0)); }
    ~Algorithm() { BCryptCloseAlgorithmProvider(handle, 0); }
    BCRYPT_ALG_HANDLE GetHandle() const noexcept { return handle; }

private:
    BCRYPT_ALG_HANDLE handle = nullptr;
};

struct HashDeleter {
    void operator()(void* handle) const noexcept { BCryptDestroyHash(handle); }
};
}

AnyBox::Shader::ShaderDigest AnyBox::Shader::HashShader(std::span<const std::byte> data) {
    return HashShaderParts(std::array{data});
}

AnyBox::Shader::ShaderDigest AnyBox::Shader::HashShaderParts(std::span<const std::span<const std::byte>> parts) {
    ShaderDigest result;
    static Algorithm algorithm;
    BCRYPT_HASH_HANDLE handle = nullptr;
    CheckStatus(BCryptCreateHash(algorithm.GetHandle(), &handle, nullptr, 0, nullptr, 0, 0));
    std::unique_ptr<void, HashDeleter> hash(handle);
    auto* output = reinterpret_cast<PUCHAR>(result.data());
    for (auto data : parts) {
        if (data.size() > std::numeric_limits<ULONG>::max()) throw std::length_error("Shader digest input exceeds the Windows hash API limit");
        auto* input = reinterpret_cast<PUCHAR>(const_cast<std::byte*>(data.data()));
        if (!data.empty()) CheckStatus(BCryptHashData(handle, input, static_cast<ULONG>(data.size()), 0));
    }
    CheckStatus(BCryptFinishHash(handle, output, static_cast<ULONG>(result.size()), 0));
    return result;
}

std::string AnyBox::Shader::DigestName(const ShaderDigest& digest) {
    constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2);
    for (auto byte : digest) {
        auto value = std::to_integer<unsigned>(byte);
        result.push_back(alphabet[value >> 4]);
        result.push_back(alphabet[value & 15]);
    }
    return result;
}
