#include "Computer.hpp"
#include <limits>
#include <stdexcept>

AnyBox::XG::TextureComputer& AnyBox::XG::TextureComputer::from(IXGTextureComputer* self) {
    if (!self || self->vtable != &table) throw std::invalid_argument("Texture computer object is null or foreign");
    return *static_cast<TextureComputer*>(self);
}

std::uint32_t AnyBox::XG::TextureComputer::AddRef(IXGTextureComputer* self) {
    auto& count = from(self).references;
    auto previous = count.load(std::memory_order_relaxed);
    do {
        if (previous == 0 || previous == std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Texture computer reference count is invalid");
    } while (!count.compare_exchange_weak(previous, previous + 1, std::memory_order_relaxed));
    return previous + 1;
}

std::uint32_t AnyBox::XG::TextureComputer::Release(IXGTextureComputer* self) {
    auto& object = from(self);
    auto previous = object.references.fetch_sub(1, std::memory_order_acq_rel);
    if (previous == 0) throw std::logic_error("Texture computer reference count underflowed");
    if (previous == 1) delete &object;
    return previous - 1;
}
