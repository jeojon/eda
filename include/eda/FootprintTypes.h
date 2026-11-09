#pragma once

#include <string>
#include <utility>
#include <vector>

namespace eda
{

enum class PadKind
{
    Smd,
    ThroughHole
};

enum class PadShape
{
    Rect,
    RoundedRect,
    Circle
};

struct PadItem
{
    PadKind kind = PadKind::Smd;
    PadShape shape = PadShape::RoundedRect;
    std::string number = "1";
    std::string name;
    double x = 0.0;
    double y = 0.0;
    double width = 1.0;
    double height = 1.0;
    double rotation = 0.0;
    double drill = 0.0;
    double maskExpansion = 0.1;
    double clearance = 0.2;
};

struct SilkLine
{
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    double width = 0.15;
};

struct SilkRect
{
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    double width = 0.15;
};

struct SilkArc
{
    double cx = 0.0;
    double cy = 0.0;
    double rx = 1.0;
    double ry = 1.0;
    double startDegrees = 0.0;
    double sweepDegrees = 180.0;
    double width = 0.15;
};

struct SilkPolygon
{
    std::vector<std::pair<double, double>> points;
    double width = 0.15;
};

struct SilkText
{
    double x = 0.0;
    double y = 0.0;
    double size = 1.0;
    std::string text = "TEXT";
};

struct Bounds
{
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    bool valid = false;
};

struct Footprint
{
    std::string name = "Untitled";
    std::string category = "Uncategorized";
    std::string description;
    std::string silkColor = "#1A6984";
    double originX = 0.0;
    double originY = 0.0;

    std::vector<PadItem> pads;
    std::vector<SilkLine> lines;
    std::vector<SilkRect> rects;
    std::vector<SilkArc> arcs;
    std::vector<SilkPolygon> polygons;
    std::vector<SilkText> texts;
};

enum class ItemType
{
    None,
    Pad,
    Line,
    Rect,
    Arc,
    Polygon,
    Text
};

struct HitResult
{
    ItemType type = ItemType::None;
    int index = -1;

    explicit operator bool() const { return type != ItemType::None; }
};

struct LibraryEntry
{
    std::wstring path;
    std::string name;
    std::string category;
};

} // namespace eda

