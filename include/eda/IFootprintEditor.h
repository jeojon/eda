#pragma once

#include "FootprintTypes.h"

#include <memory>
#include <string>

namespace eda
{

class IFootprintEditor
{
public:
    virtual ~IFootprintEditor() = default;

    virtual void AddSmdPad(Footprint& footprint, double x, double y) const = 0;
    virtual void AddThroughHolePin(Footprint& footprint, double x, double y) const = 0;
    virtual void AddLine(Footprint& footprint, double x1, double y1, double x2, double y2) const = 0;
    virtual void AddRect(Footprint& footprint, double x1, double y1, double x2, double y2) const = 0;
    virtual void AddArc(Footprint& footprint, double cx, double cy, double rx, double ry) const = 0;
    virtual void AddPolygon(Footprint& footprint, const std::vector<std::pair<double, double>>& points) const = 0;
    virtual void AddText(Footprint& footprint, double x, double y, const std::string& text) const = 0;

    virtual bool UpdatePad(Footprint& footprint, int index, const PadItem& updated) const = 0;
    virtual bool MovePad(Footprint& footprint, int index, double dx, double dy) const = 0;
    virtual bool RemoveItem(Footprint& footprint, ItemType type, int index) const = 0;
};

} // namespace eda

