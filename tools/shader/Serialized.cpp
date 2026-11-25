#include "Serialized.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <anybox/shader/Artifact.hpp>
#include <sstream>

std::string AnyBox::Shader::Tools::InspectSerializedCompute(std::span<const std::byte> data) {
    if (data.size() >= 4 && data[0] == std::byte{'D'} && data[1] == std::byte{'X'} && data[2] == std::byte{'B'} && data[3] == std::byte{'C'}) {
        Graphics::Container::DxbcView container(data);
        auto part = container.FindPart(0x43434341);
        if (!part) throw std::invalid_argument("Shader container has no serialized pipeline part");
        data = part->data;
    }
    Graphics::Container::FspxView view(data);
    std::ostringstream output;
    output << "dispatch_bytes=" << view.GetDispatchState().size() << " auxiliary_bytes=" << view.GetAuxiliaryState().size();
    output << " code_words=" << view.GetCode().size() / 4 << " record_bytes=" << view.GetRecords().size();
    output << " source=" << DigestName(HashShader(data)) << '\n';
    return output.str();
}
