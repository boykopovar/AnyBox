#include "TextureShaderDescription.hpp"
#include <stdexcept>

AnyBox::Descriptor::TextureViewDescription AnyBox::Graphics::ReadTextureShaderView(const D3D12_RESOURCE_DESC& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description, std::uint64_t address) {
    if (resource.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) throw std::runtime_error("Texture shader view dimension is not implemented");
    if ((resource.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) != 0) throw std::invalid_argument("Xbox texture does not allow shader resource views");
    Descriptor::TextureViewDescription view;
    view.address = address;
    if (!description) return view;
    if (description->Format != resource.Format) throw std::runtime_error("Texture shader view format reinterpretation is not implemented");
    view.componentMapping = description->Shader4ComponentMapping;
    if (description->ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D) {
        if (description->Texture2D.PlaneSlice != 0 || description->Texture2D.ResourceMinLODClamp != 0) throw std::runtime_error("Texture shader view plane or minimum LOD clamping is not implemented");
        view.firstMip = description->Texture2D.MostDetailedMip;
        view.mipCount = description->Texture2D.MipLevels;
        view.layerCount = 1;
    } else if (description->ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY) {
        if (description->Texture2DArray.PlaneSlice != 0 || description->Texture2DArray.ResourceMinLODClamp != 0) throw std::runtime_error("Texture shader view plane or minimum LOD clamping is not implemented");
        view.firstMip = description->Texture2DArray.MostDetailedMip;
        view.mipCount = description->Texture2DArray.MipLevels;
        view.firstLayer = description->Texture2DArray.FirstArraySlice;
        view.layerCount = description->Texture2DArray.ArraySize;
    } else throw std::runtime_error("Texture shader view type is not implemented");
    return view;
}
