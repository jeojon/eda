#pragma once

#include "FootprintTypes.h"

#include <memory>

namespace eda
{

class IGeometryService
{
public:
    virtual ~IGeometryService() = default;

    virtual Bounds ComputeBounds(const Footprint& footprint) const = 0;
    virtual HitResult HitTest(const Footprint& footprint, double x, double y, double tolerance) const = 0;
    virtual double SnapToGrid(double value, double gridSize) const = 0;
};

} // namespace eda

