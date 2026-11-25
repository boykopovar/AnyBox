#include "TextureViews.hpp"
#include "TextureShaderDescription.hpp"

void AnyBox::Graphics::CreateTextureShaderView(Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description, std::uint64_t destination) {
    auto& texture = resource.GetTexture();
    D3D12_RESOURCE_DESC metadata{};
    const auto& source = resource.GetMetadata();
    std::memcpy(&metadata, &source, offsetof(D3D12_RESOURCE_DESC, Flags) + sizeof(metadata.Flags));
    auto view = ReadTextureShaderView(metadata, description, texture.GetAddress());
    auto words = Descriptor::EncodeTexture2D(texture.GetDescription(), view);
    texture.GetView(words);
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), words.data(), sizeof(words));
}
