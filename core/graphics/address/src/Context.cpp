#include <anybox/address/Context.hpp>
#include <addrinterface.h>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>

namespace {
void* Allocate(const ADDR_ALLOCSYSMEM_INPUT* input) { return std::malloc(input->sizeInBytes); }

ADDR_E_RETURNCODE Free(const ADDR_FREESYSMEM_INPUT* input) {
    std::free(input->pVirtAddr);
    return ADDR_OK;
}
}

AnyBox::Address::Context::Context(const HardwareConfiguration& hardware) : surfaceCopy(hardware == SeriesXHardware) {
    ADDR_CREATE_INPUT input{};
    input.size = sizeof(input);
    input.chipEngine = hardware.engine;
    input.chipFamily = hardware.family;
    input.chipRevision = hardware.revision;
    input.regValue.gbAddrConfig = hardware.addressConfiguration;
    input.createFlags.fillSizeFields = 1;
    input.callbacks.allocSysMem = Allocate;
    input.callbacks.freeSysMem = Free;
    ADDR_CREATE_OUTPUT output{};
    output.size = sizeof(output);
    auto result = AddrCreate(&input, &output);
    if (result != ADDR_OK || !output.hLib) throw std::runtime_error("AddrLib context creation failed: " + std::to_string(result));
    handle = output.hLib;
}

AnyBox::Address::Context::~Context() {
    if (AddrDestroy(handle) != ADDR_OK) std::terminate();
}
