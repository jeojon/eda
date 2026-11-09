#include "eda/IFootprintEditor.h"

namespace eda
{
namespace
{

class FootprintEditorService final : public IFootprintEditor
{
public:
    // TODO(student): append a default SMD pad and assign its number.
    void AddSmdPad(Footprint&, double, double) const override
    {
    }

    // TODO(student): append a default THT pin and assign its number.
    void AddThroughHolePin(Footprint&, double, double) const override
    {
    }

    // TODO(student): append a silkscreen line.
    void AddLine(Footprint&, double, double, double, double) const override
    {
    }

    // TODO(student): append a silkscreen rectangle.
    void AddRect(Footprint&, double, double, double, double) const override
    {
    }

    // TODO(student): append a silkscreen arc.
    void AddArc(Footprint&, double, double, double, double) const override
    {
    }

    // TODO(student): append a silkscreen polygon.
    void AddPolygon(Footprint&, const std::vector<std::pair<double, double>>&) const override
    {
    }

    // TODO(student): append a text object.
    void AddText(Footprint&, double, double, const std::string&) const override
    {
    }

    // TODO(student): update one pad and keep the data consistent.
    bool UpdatePad(Footprint&, int, const PadItem&) const override
    {
        return false;
    }

    // TODO(student): move one pad.
    bool MovePad(Footprint&, int, double, double) const override
    {
        return false;
    }

    // TODO(student): remove a pad or silkscreen item.
    bool RemoveItem(Footprint&, ItemType, int) const override
    {
        return false;
    }
};

} // namespace

std::shared_ptr<IFootprintEditor> CreateFootprintEditor()
{
    return std::make_shared<FootprintEditorService>();
}

} // namespace eda

