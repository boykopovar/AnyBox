#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>

namespace AnyBox::Shader {
void RemoveDisabledPixelExportSources(ShaderRecompiler::IrProgram& program);
}
