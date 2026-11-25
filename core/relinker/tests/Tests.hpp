#pragma once

#include <anybox/relinker/Decoder.hpp>
#include <stdexcept>

namespace AnyBox::Relinker::Tests {
inline void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void CheckWriteback();
void CheckExecution();
void CheckImages();
void CheckCpu();
void CheckControlFlow();
void CheckInsert();
void CheckPatching();
void CheckExtract();
void CheckMonitor();
}
