#include "Computer.hpp"

const XGTextureComputerTable AnyBox::XG::TextureComputer::table{
    AddRef, Release, GetResourceLayout, GetSize, GetAlignment, GetMipOffset, GetTexelOffset, GetTexelCoordinates,
    CopyIntoSubresource, CopyFromSubresource, GetResourceTiling, GetPlaneDescriptor, IsTiledResource
};
