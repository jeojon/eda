#include "eda/IGeometryService.h"

namespace eda
{
namespace
{

class GeometryService final : public IGeometryService
{
public:
    // TODO(student): compute bounds over pads, lines, rects, arcs, polygons and text.
    Bounds ComputeBounds(const Footprint&) const override
    {
        return {};
    }

    // TODO(student): implement hit testing in model coordinates.
    HitResult HitTest(const Footprint&, double, double, double) const override
    {
        return {};
    }

    // TODO(student): round a model coordinate to the current grid.
    double SnapToGrid(double value, double) const override
    {
        return value;
    }
};

} // namespace

std::shared_ptr<IGeometryService> CreateGeometryService()
{
    return std::make_shared<GeometryService>();
}

} // namespace eda

