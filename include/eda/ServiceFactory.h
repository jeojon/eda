#pragma once

#include "IFootprintCodec.h"
#include "IFootprintEditor.h"
#include "IFootprintLibrary.h"
#include "IGeometryService.h"

#include <memory>

namespace eda
{

std::shared_ptr<IFootprintEditor> CreateFootprintEditor();
std::shared_ptr<IGeometryService> CreateGeometryService();
std::shared_ptr<IFootprintCodec> CreateFootprintCodec();
std::shared_ptr<IFootprintLibrary> CreateFootprintLibrary();

} // namespace eda
