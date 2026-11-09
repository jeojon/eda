#include "FootprintModel.h"

#include <cmath>
#include <iostream>
#include <string>

namespace
{
bool Near(double a, double b)
{
    return std::fabs(a - b) < 1e-9;
}

bool Check(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}
} // namespace

int main()
{
    bool ok = true;

    const Footprint resistor = Footprint::CreateResistor0805();
    ok &= Check(resistor.name == "R_0805", "resistor name");
    ok &= Check(resistor.silkColor == "#1A6984", "default silk color");
    ok &= Check(resistor.pads.size() == 2, "resistor pad count");
    ok &= Check(Near(resistor.pads[0].x, -0.9), "resistor pad 1 x");
    ok &= Check(Near(resistor.pads[1].x, 0.9), "resistor pad 2 x");

    const Footprint header = Footprint::CreateHeader2();
    ok &= Check(header.pads.size() == 2, "header pin count");
    ok &= Check(header.pads[0].kind == PadKind::ThroughHole, "header is THT");
    ok &= Check(Near(header.pads[1].x - header.pads[0].x, 2.54), "header pitch");
    ok &= Check(Near(header.pads[0].drill, 0.965), "header drill");
    ok &= Check(Near(header.pads[0].clearance, 0.2), "header clearance");

    const Footprint dip = Footprint::CreateDip14();
    ok &= Check(dip.pads.size() == 14, "DIP14 pin count");
    ok &= Check(dip.pads[0].shape == PadShape::Rect, "DIP14 pin 1 marker");
    ok &= Check(Near(dip.pads[0].y, 7.62), "DIP14 pin 1 at top-left");
    ok &= Check(Near(dip.pads[7].y, -7.62), "DIP14 pin 8 at bottom-right");
    ok &= Check(Near(std::fabs(dip.pads[6].y - dip.pads[0].y), 15.24), "DIP14 row length");

    const std::string encoded = dip.Serialize();
    Footprint decoded;
    std::string error;
    ok &= Check(decoded.Deserialize(encoded, &error), "deserialize DIP14");
    ok &= Check(decoded.name == dip.name, "roundtrip name");
    ok &= Check(decoded.pads.size() == dip.pads.size(), "roundtrip pad count");
    ok &= Check(decoded.lines.size() == dip.lines.size(), "roundtrip line count");
    ok &= Check(decoded.arcs.size() == dip.arcs.size(), "roundtrip arc count");
    ok &= Check(Near(decoded.pads[13].x, 3.81), "roundtrip pad 14 x");

    const Bounds bounds = dip.GetBounds();
    ok &= Check(bounds.valid, "bounds valid");
    ok &= Check(bounds.maxY >= 8.0, "bounds contains DIP14 pins");

    if (ok)
    {
        std::cout << "All model tests passed.\n";
        return 0;
    }
    return 1;
}
