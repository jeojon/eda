#include "FootprintModel.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace
{
void ExpandBounds(Bounds& bounds, double x, double y)
{
    if (!bounds.valid)
    {
        bounds.minX = bounds.maxX = x;
        bounds.minY = bounds.maxY = y;
        bounds.valid = true;
        return;
    }

    bounds.minX = std::min(bounds.minX, x);
    bounds.minY = std::min(bounds.minY, y);
    bounds.maxX = std::max(bounds.maxX, x);
    bounds.maxY = std::max(bounds.maxY, y);
}

void AddRectToBounds(Bounds& bounds, double x1, double y1, double x2, double y2)
{
    ExpandBounds(bounds, x1, y1);
    ExpandBounds(bounds, x2, y2);
}

std::string EscapeText(const std::string& text)
{
    std::string escaped;
    escaped.reserve(text.size());
    for (char ch : text)
    {
        if (ch == '\n' || ch == '\r')
        {
            escaped.push_back(' ');
        }
        else
        {
            escaped.push_back(ch);
        }
    }
    return escaped;
}

void AddSilkRectangle(Footprint& footprint, double x1, double y1, double x2, double y2, double width)
{
    SilkLine top;
    top.x1 = x1;
    top.y1 = y1;
    top.x2 = x2;
    top.y2 = y1;
    top.width = width;

    SilkLine right = top;
    right.x1 = x2;
    right.y1 = y1;
    right.x2 = x2;
    right.y2 = y2;

    SilkLine bottom = top;
    bottom.x1 = x2;
    bottom.y1 = y2;
    bottom.x2 = x1;
    bottom.y2 = y2;

    SilkLine left = top;
    left.x1 = x1;
    left.y1 = y2;
    left.x2 = x1;
    left.y2 = y1;

    footprint.lines.push_back(top);
    footprint.lines.push_back(right);
    footprint.lines.push_back(bottom);
    footprint.lines.push_back(left);
}
} // namespace

std::string Trim(std::string value)
{
    auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };

    value.erase(value.begin(), std::find_if(value.begin(), value.end(),
        [&](char ch) { return !isSpace(static_cast<unsigned char>(ch)); }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
        [&](char ch) { return !isSpace(static_cast<unsigned char>(ch)); }).base(), value.end());
    return value;
}

double Clamp(double value, double low, double high)
{
    return std::max(low, std::min(value, high));
}

void Footprint::Clear()
{
    *this = Footprint();
}

Bounds Footprint::GetBounds() const
{
    Bounds bounds;
    for (const PadItem& pad : pads)
    {
        const double halfW = pad.width * 0.5;
        const double halfH = pad.height * 0.5;
        AddRectToBounds(bounds, pad.x - halfW, pad.y - halfH, pad.x + halfW, pad.y + halfH);
    }

    for (const SilkLine& line : lines)
    {
        ExpandBounds(bounds, line.x1, line.y1);
        ExpandBounds(bounds, line.x2, line.y2);
    }

    for (const SilkRect& rect : rects)
    {
        AddRectToBounds(bounds, rect.x1, rect.y1, rect.x2, rect.y2);
    }

    for (const SilkArc& arc : arcs)
    {
        AddRectToBounds(bounds, arc.cx - arc.rx, arc.cy - arc.ry, arc.cx + arc.rx, arc.cy + arc.ry);
    }

    for (const SilkPolygon& polygon : polygons)
    {
        for (const auto& point : polygon.points)
        {
            ExpandBounds(bounds, point.first, point.second);
        }
    }

    for (const SilkText& text : texts)
    {
        AddRectToBounds(bounds, text.x, text.y, text.x + text.size * 0.6 * text.text.size(), text.y + text.size);
    }

    if (!bounds.valid)
    {
        ExpandBounds(bounds, -5.0, -5.0);
        ExpandBounds(bounds, 5.0, 5.0);
    }

    return bounds;
}

std::string Footprint::Serialize() const
{
    std::ostringstream output;
    output << std::setprecision(10);
    output << "EDALAB1_FOOTPRINT 1\n";
    output << "NAME " << EscapeText(name) << '\n';
    output << "CATEGORY " << EscapeText(category) << '\n';
    output << "DESC " << EscapeText(description) << '\n';
    output << "SILKCOLOR " << EscapeText(silkColor) << '\n';
    output << "ORIGIN " << originX << ' ' << originY << '\n';

    for (const PadItem& pad : pads)
    {
        output << "PAD "
               << (pad.kind == PadKind::Smd ? "SMD" : "THT") << ' '
               << (pad.shape == PadShape::Rect ? "RECT" :
                   pad.shape == PadShape::Circle ? "CIRCLE" : "ROUNDED") << ' '
               << pad.number << ' '
               << pad.name << ' '
               << pad.x << ' '
               << pad.y << ' '
               << pad.width << ' '
               << pad.height << ' '
               << pad.rotation << ' '
               << pad.drill << ' '
               << pad.maskExpansion << ' '
               << pad.clearance << '\n';
    }

    for (const SilkLine& line : lines)
    {
        output << "LINE " << line.x1 << ' ' << line.y1 << ' '
               << line.x2 << ' ' << line.y2 << ' ' << line.width << '\n';
    }

    for (const SilkRect& rect : rects)
    {
        output << "RECT " << rect.x1 << ' ' << rect.y1 << ' '
               << rect.x2 << ' ' << rect.y2 << ' ' << rect.width << '\n';
    }

    for (const SilkArc& arc : arcs)
    {
        output << "ARC " << arc.cx << ' ' << arc.cy << ' '
               << arc.rx << ' ' << arc.ry << ' '
               << arc.startDegrees << ' ' << arc.sweepDegrees << ' '
               << arc.width << '\n';
    }

    for (const SilkPolygon& polygon : polygons)
    {
        output << "POLY " << polygon.points.size();
        for (const auto& point : polygon.points)
        {
            output << ' ' << point.first << ' ' << point.second;
        }
        output << ' ' << polygon.width << '\n';
    }

    for (const SilkText& text : texts)
    {
        output << "TEXT " << text.x << ' ' << text.y << ' '
               << text.size << ' ' << EscapeText(text.text) << '\n';
    }

    output << "END\n";
    return output.str();
}

bool Footprint::Deserialize(const std::string& text, std::string* error)
{
    Clear();

    std::istringstream input(text);
    std::string line;
    bool sawHeader = false;

    while (std::getline(input, line))
    {
        line = Trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream row(line);
        std::string keyword;
        row >> keyword;

        if (keyword == "EDALAB1_FOOTPRINT")
        {
            int version = 0;
            row >> version;
            sawHeader = version == 1;
        }
        else if (keyword == "NAME")
        {
            std::getline(row, name);
            name = Trim(name);
        }
        else if (keyword == "DESC")
        {
            std::getline(row, description);
            description = Trim(description);
        }
        else if (keyword == "CATEGORY")
        {
            std::getline(row, category);
            category = Trim(category);
        }
        else if (keyword == "SILKCOLOR")
        {
            std::getline(row, silkColor);
            silkColor = Trim(silkColor);
        }
        else if (keyword == "ORIGIN")
        {
            row >> originX >> originY;
        }
        else if (keyword == "PAD")
        {
            PadItem pad;
            std::string kind;
            std::string shape;
            row >> kind >> shape >> pad.number >> pad.name;
            row >> pad.x >> pad.y >> pad.width >> pad.height
                >> pad.rotation >> pad.drill >> pad.maskExpansion;
            if (!(row >> pad.clearance))
            {
                pad.clearance = 0.2;
            }
            pad.kind = kind == "THT" ? PadKind::ThroughHole : PadKind::Smd;
            pad.shape = shape == "RECT" ? PadShape::Rect :
                        shape == "CIRCLE" ? PadShape::Circle : PadShape::RoundedRect;
            pads.push_back(pad);
        }
        else if (keyword == "LINE")
        {
            SilkLine silk;
            row >> silk.x1 >> silk.y1 >> silk.x2 >> silk.y2 >> silk.width;
            lines.push_back(silk);
        }
        else if (keyword == "RECT")
        {
            SilkRect rect;
            row >> rect.x1 >> rect.y1 >> rect.x2 >> rect.y2 >> rect.width;
            rects.push_back(rect);
        }
        else if (keyword == "ARC")
        {
            SilkArc arc;
            row >> arc.cx >> arc.cy >> arc.rx >> arc.ry
                >> arc.startDegrees >> arc.sweepDegrees >> arc.width;
            arcs.push_back(arc);
        }
        else if (keyword == "POLY")
        {
            SilkPolygon polygon;
            std::size_t count = 0;
            row >> count;
            for (std::size_t i = 0; i < count; ++i)
            {
                double x = 0.0;
                double y = 0.0;
                row >> x >> y;
                polygon.points.emplace_back(x, y);
            }
            row >> polygon.width;
            polygons.push_back(polygon);
        }
        else if (keyword == "TEXT")
        {
            SilkText item;
            row >> item.x >> item.y >> item.size;
            std::getline(row, item.text);
            item.text = Trim(item.text);
            texts.push_back(item);
        }
        else if (keyword == "END")
        {
            break;
        }
    }

    if (!sawHeader)
    {
        if (error != nullptr)
        {
            *error = "Missing EDALAB1_FOOTPRINT header.";
        }
        return false;
    }

    if (name.empty())
    {
        name = "Untitled";
    }
    if (category.empty())
    {
        category = "Uncategorized";
    }
    if (silkColor.empty())
    {
        silkColor = "#1A6984";
    }

    return true;
}

Footprint Footprint::CreateResistor0805()
{
    Footprint footprint;
    footprint.name = "R_0805";
    footprint.category = "Passive";
    footprint.description = "0805 SMD resistor/capacitor land pattern, mm";

    PadItem pad1;
    pad1.kind = PadKind::Smd;
    pad1.shape = PadShape::RoundedRect;
    pad1.number = "1";
    pad1.name = "1";
    pad1.x = -0.9;
    pad1.width = 1.3;
    pad1.height = 1.5;
    pad1.maskExpansion = 0.1;
    pad1.clearance = 0.2;

    PadItem pad2 = pad1;
    pad2.number = "2";
    pad2.name = "2";
    pad2.x = 0.9;

    footprint.pads.push_back(pad1);
    footprint.pads.push_back(pad2);

    AddSilkRectangle(footprint, -0.5, -0.8, 0.5, 0.8, 0.12);
    footprint.texts.push_back({0.0, 1.1, 0.8, "R?"});
    return footprint;
}

Footprint Footprint::CreateHeader2()
{
    Footprint footprint;
    footprint.name = "HEADER2_1";
    footprint.category = "Connector";
    footprint.description = "2.54 mm through-hole header, 2 pins";

    PadItem pin1;
    pin1.kind = PadKind::ThroughHole;
    pin1.shape = PadShape::Rect;
    pin1.number = "1";
    pin1.name = "1";
    pin1.x = 0.0;
    pin1.width = 1.524;
    pin1.height = 1.524;
    pin1.drill = 0.965;
    pin1.maskExpansion = 0.1;
    pin1.clearance = 0.2;

    PadItem pin2 = pin1;
    pin2.shape = PadShape::Circle;
    pin2.number = "2";
    pin2.name = "2";
    pin2.x = 2.54;

    footprint.pads.push_back(pin1);
    footprint.pads.push_back(pin2);
    AddSilkRectangle(footprint, -1.0, -1.0, 3.54, 1.0, 0.15);
    footprint.texts.push_back({2.54, 1.25, 0.8, "J?"});
    return footprint;
}

Footprint Footprint::CreateDip14()
{
    Footprint footprint;
    footprint.name = "DIP14";
    footprint.category = "Logic IC";
    footprint.description = "DIP-14, 2.54 mm pitch, 7.62 mm row spacing";
    footprint.originY = 7.62;

    for (int pin = 1; pin <= 14; ++pin)
    {
        PadItem pad;
        pad.kind = PadKind::ThroughHole;
        pad.shape = pin == 1 ? PadShape::Rect : PadShape::Circle;
        pad.number = std::to_string(pin);
        pad.name = std::to_string(pin);
        pad.x = pin <= 7 ? -3.81 : 3.81;
        pad.y = (pin <= 7 ? (7 - pin) * 2.54 : (pin - 8) * 2.54) - footprint.originY;
        pad.width = 1.524;
        pad.height = 1.524;
        pad.drill = 0.711;
        pad.maskExpansion = 0.1;
        pad.clearance = 0.2;
        footprint.pads.push_back(pad);
    }

    AddSilkRectangle(footprint, -5.4, -8.92, 5.4, 8.92, 0.15);
    SilkArc notch;
    notch.cx = 0.0;
    notch.cy = 8.92;
    notch.rx = 0.9;
    notch.ry = 0.9;
    notch.startDegrees = 180.0;
    notch.sweepDegrees = 180.0;
    notch.width = 0.15;
    footprint.arcs.push_back(notch);
    footprint.texts.push_back({-5.0, 9.4, 0.8, "U?"});
    return footprint;
}

Footprint Footprint::CreateSo14()
{
    Footprint footprint;
    footprint.name = "SO14";
    footprint.category = "Logic IC";
    footprint.description = "SOIC-14 narrow, 1.27 mm pitch";
    footprint.originY = 3.81;

    for (int pin = 1; pin <= 14; ++pin)
    {
        PadItem pad;
        pad.kind = PadKind::Smd;
        pad.shape = PadShape::RoundedRect;
        pad.number = std::to_string(pin);
        pad.name = std::to_string(pin);
        pad.x = pin <= 7 ? -2.6 : 2.6;
        pad.y = (pin <= 7 ? (7 - pin) * 1.27 : (pin - 8) * 1.27) - footprint.originY;
        pad.width = 1.65;
        pad.height = 0.51;
        pad.maskExpansion = 0.08;
        pad.clearance = 0.15;
        footprint.pads.push_back(pad);
    }

    AddSilkRectangle(footprint, -3.2, -4.61, 3.2, 4.61, 0.12);
    footprint.texts.push_back({-2.8, 5.0, 0.75, "U?"});
    return footprint;
}
