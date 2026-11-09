#include "FootprintCanvas.h"

#include "MainFrame.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace
{
constexpr double kPi = 3.14159265358979323846;

wxColour Background() { return wxColour(247, 246, 241); }
wxColour MinorGrid() { return wxColour(228, 226, 218); }
wxColour MajorGrid() { return wxColour(208, 204, 194); }
wxColour PadFill() { return wxColour(192, 112, 42); }
wxColour PadEdge() { return wxColour(116, 62, 20); }
wxColour Selected() { return wxColour(225, 78, 44); }
wxColour TextColour() { return wxColour(34, 39, 45); }
} // namespace

FootprintCanvas::FootprintCanvas(wxWindow* parent,
                                 MainFrame* owner,
                                 std::shared_ptr<eda::IFootprintEditor> editor,
                                 std::shared_ptr<eda::IGeometryService> geometry)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxBORDER_SIMPLE | wxWANTS_CHARS),
      owner_(owner),
      editor_(std::move(editor)),
      geometry_(std::move(geometry))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(Background());

    Bind(wxEVT_PAINT, &FootprintCanvas::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &FootprintCanvas::OnLeftDown, this);
    Bind(wxEVT_RIGHT_DOWN, &FootprintCanvas::OnRightDown, this);
    Bind(wxEVT_MOTION, &FootprintCanvas::OnMotion, this);
    Bind(wxEVT_MOUSEWHEEL, &FootprintCanvas::OnMouseWheel, this);
    Bind(wxEVT_MIDDLE_DOWN, &FootprintCanvas::OnMiddleDown, this);
    Bind(wxEVT_MIDDLE_UP, &FootprintCanvas::OnMiddleUp, this);
    Bind(wxEVT_KEY_DOWN, &FootprintCanvas::OnKeyDown, this);
}

void FootprintCanvas::SetFootprint(eda::Footprint* footprint)
{
    footprint_ = footprint;
    selection_ = {};
    hasFirstPoint_ = false;
    polygonPoints_.clear();
    FitView();
    Refresh();
}

void FootprintCanvas::SetTool(EditorTool tool)
{
    tool_ = tool;
    selection_ = {};
    hasFirstPoint_ = false;
    polygonPoints_.clear();
    SetFocus();
    NotifySelectionChanged();
    Refresh();
}

void FootprintCanvas::FitView()
{
    if (footprint_ == nullptr || geometry_ == nullptr)
    {
        return;
    }

    const eda::Bounds bounds = geometry_->ComputeBounds(*footprint_);
    const wxSize size = GetClientSize();
    if (!bounds.valid || size.x < 50 || size.y < 50)
    {
        return;
    }

    const double width = std::max(1.0, bounds.maxX - bounds.minX);
    const double height = std::max(1.0, bounds.maxY - bounds.minY);
    zoom_ = std::clamp(std::min((size.x - 100.0) / width, (size.y - 100.0) / height),
                       4.0, 70.0);
    panX_ = -(bounds.minX + bounds.maxX) * 0.5 * zoom_;
    panY_ = (bounds.minY + bounds.maxY) * 0.5 * zoom_;
}

void FootprintCanvas::ZoomIn()
{
    zoom_ = std::clamp(zoom_ * 1.2, 2.0, 180.0);
    Refresh();
}

void FootprintCanvas::ZoomOut()
{
    zoom_ = std::clamp(zoom_ / 1.2, 2.0, 180.0);
    Refresh();
}

void FootprintCanvas::DeleteSelected()
{
    if (footprint_ == nullptr || editor_ == nullptr)
    {
        return;
    }
    if (editor_->RemoveItem(*footprint_, selection_.type, selection_.index))
    {
        selection_ = {};
        NotifySelectionChanged();
        NotifyChanged();
        Refresh();
    }
}

void FootprintCanvas::RefreshCanvas()
{
    FitView();
    selection_ = {};
    Refresh();
}

const eda::PadItem* FootprintCanvas::SelectedPad() const
{
    if (footprint_ == nullptr ||
        selection_.type != eda::ItemType::Pad ||
        selection_.index < 0 ||
        selection_.index >= static_cast<int>(footprint_->pads.size()))
    {
        return nullptr;
    }
    return &footprint_->pads[selection_.index];
}

bool FootprintCanvas::ApplySelectedPad(const eda::PadItem& pad)
{
    if (footprint_ == nullptr || editor_ == nullptr)
    {
        return false;
    }
    if (!editor_->UpdatePad(*footprint_, selection_.index, pad))
    {
        return false;
    }
    NotifyChanged();
    Refresh();
    return true;
}

void FootprintCanvas::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(Background()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc)
    {
        return;
    }

    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
    DrawGrid(gc.get());
    if (footprint_ != nullptr)
    {
        DrawPads(gc.get());
        DrawSilkscreen(gc.get());
    }
}

void FootprintCanvas::OnLeftDown(wxMouseEvent& event)
{
    SetFocus();
    if (footprint_ == nullptr || editor_ == nullptr)
    {
        return;
    }

    const wxPoint2DDouble raw = ScreenToModel(event.GetX(), event.GetY());
    const wxPoint2DDouble point = Snap(raw);

    if (tool_ == EditorTool::Select)
    {
        selection_ = geometry_->HitTest(*footprint_, raw.m_x, raw.m_y, 8.0 / zoom_);
        NotifySelectionChanged();
        Refresh();
        return;
    }

    if (tool_ == EditorTool::SmdPad)
    {
        editor_->AddSmdPad(*footprint_, point.m_x, point.m_y);
        NotifyChanged();
    }
    else if (tool_ == EditorTool::ThroughHolePin)
    {
        editor_->AddThroughHolePin(*footprint_, point.m_x, point.m_y);
        NotifyChanged();
    }
    else if (tool_ == EditorTool::Text)
    {
        const wxString value = wxGetTextFromUser(
            wxString::FromUTF8("输入丝印文本"),
            wxString::FromUTF8("添加文本"),
            "TEXT",
            this);
        if (!value.empty())
        {
            editor_->AddText(*footprint_, point.m_x, point.m_y, value.utf8_string());
            NotifyChanged();
        }
    }
    else if (tool_ == EditorTool::SilkPolygon)
    {
        polygonPoints_.push_back(point);
        Refresh();
        return;
    }
    else if (!hasFirstPoint_)
    {
        firstPoint_ = point;
        hasFirstPoint_ = true;
        Refresh();
        return;
    }
    else if (tool_ == EditorTool::SilkLine)
    {
        editor_->AddLine(*footprint_,
                         firstPoint_.m_x, firstPoint_.m_y,
                         point.m_x, point.m_y);
        hasFirstPoint_ = false;
        NotifyChanged();
    }
    else if (tool_ == EditorTool::SilkRect)
    {
        editor_->AddRect(*footprint_,
                         firstPoint_.m_x, firstPoint_.m_y,
                         point.m_x, point.m_y);
        hasFirstPoint_ = false;
        NotifyChanged();
    }
    else if (tool_ == EditorTool::SilkArc)
    {
        const double radius = std::hypot(point.m_x - firstPoint_.m_x,
                                         point.m_y - firstPoint_.m_y);
        editor_->AddArc(*footprint_, firstPoint_.m_x, firstPoint_.m_y, radius, radius);
        hasFirstPoint_ = false;
        NotifyChanged();
    }

    Refresh();
}

void FootprintCanvas::OnRightDown(wxMouseEvent&)
{
    if (tool_ == EditorTool::SilkPolygon && footprint_ != nullptr && editor_ != nullptr &&
        polygonPoints_.size() >= 3)
    {
        std::vector<std::pair<double, double>> points;
        for (const wxPoint2DDouble& point : polygonPoints_)
        {
            points.emplace_back(point.m_x, point.m_y);
        }
        editor_->AddPolygon(*footprint_, points);
        polygonPoints_.clear();
        NotifyChanged();
        Refresh();
    }
}

void FootprintCanvas::OnMotion(wxMouseEvent& event)
{
    if (middlePanning_ && event.MiddleIsDown())
    {
        const wxPoint current(event.GetX(), event.GetY());
        panX_ += current.x - lastMouseScreen_.x;
        panY_ += current.y - lastMouseScreen_.y;
        lastMouseScreen_ = current;
        Refresh();
        return;
    }
    if (owner_ != nullptr)
    {
        const wxPoint2DDouble model = ScreenToModel(event.GetX(), event.GetY());
        owner_->SetCursorPosition(model.m_x, model.m_y);
    }
}

void FootprintCanvas::OnMouseWheel(wxMouseEvent& event)
{
    const wxPoint2DDouble before = ScreenToModel(event.GetX(), event.GetY());
    zoom_ = std::clamp(zoom_ * (event.GetWheelRotation() > 0 ? 1.15 : 1.0 / 1.15),
                       2.0, 180.0);
    const wxSize size = GetClientSize();
    panX_ = event.GetX() - size.x * 0.5 - before.m_x * zoom_;
    panY_ = event.GetY() - size.y * 0.5 + before.m_y * zoom_;
    Refresh();
}

void FootprintCanvas::OnMiddleDown(wxMouseEvent& event)
{
    middlePanning_ = true;
    lastMouseScreen_ = wxPoint(event.GetX(), event.GetY());
    CaptureMouse();
}

void FootprintCanvas::OnMiddleUp(wxMouseEvent&)
{
    middlePanning_ = false;
    if (HasCapture())
    {
        ReleaseMouse();
    }
}

void FootprintCanvas::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_DELETE)
    {
        DeleteSelected();
        return;
    }
    if (event.GetKeyCode() == WXK_ESCAPE)
    {
        selection_ = {};
        hasFirstPoint_ = false;
        polygonPoints_.clear();
        NotifySelectionChanged();
        Refresh();
        return;
    }
    event.Skip();
}

wxPoint2DDouble FootprintCanvas::ModelToScreen(double x, double y) const
{
    const wxSize size = GetClientSize();
    return {size.x * 0.5 + panX_ + x * zoom_,
            size.y * 0.5 + panY_ - y * zoom_};
}

wxPoint2DDouble FootprintCanvas::ScreenToModel(double x, double y) const
{
    const wxSize size = GetClientSize();
    return {(x - size.x * 0.5 - panX_) / zoom_,
            -(y - size.y * 0.5 - panY_) / zoom_};
}

wxPoint2DDouble FootprintCanvas::Snap(const wxPoint2DDouble& point) const
{
    const double grid = GridSize();
    return {geometry_->SnapToGrid(point.m_x, grid),
            geometry_->SnapToGrid(point.m_y, grid)};
}

double FootprintCanvas::GridSize() const
{
    if (zoom_ >= 50.0) return 0.1;
    if (zoom_ >= 15.0) return 0.5;
    return 1.0;
}

void FootprintCanvas::DrawGrid(wxGraphicsContext* gc)
{
    const wxSize size = GetClientSize();
    const double step = GridSize();
    const wxPoint2DDouble topLeft = ScreenToModel(0, 0);
    const wxPoint2DDouble bottomRight = ScreenToModel(size.x, size.y);
    const double minX = std::min(topLeft.m_x, bottomRight.m_x);
    const double maxX = std::max(topLeft.m_x, bottomRight.m_x);
    const double minY = std::min(topLeft.m_y, bottomRight.m_y);
    const double maxY = std::max(topLeft.m_y, bottomRight.m_y);

    for (long i = static_cast<long>(std::floor(minX / step));
         i <= static_cast<long>(std::ceil(maxX / step)); ++i)
    {
        const wxPoint2DDouble a = ModelToScreen(i * step, minY);
        const wxPoint2DDouble b = ModelToScreen(i * step, maxY);
        gc->SetPen(wxPen(i % 5 == 0 ? MajorGrid() : MinorGrid(), 1));
        gc->StrokeLine(a.m_x, a.m_y, b.m_x, b.m_y);
    }
    for (long i = static_cast<long>(std::floor(minY / step));
         i <= static_cast<long>(std::ceil(maxY / step)); ++i)
    {
        const wxPoint2DDouble a = ModelToScreen(minX, i * step);
        const wxPoint2DDouble b = ModelToScreen(maxX, i * step);
        gc->SetPen(wxPen(i % 5 == 0 ? MajorGrid() : MinorGrid(), 1));
        gc->StrokeLine(a.m_x, a.m_y, b.m_x, b.m_y);
    }
}

void FootprintCanvas::DrawPads(wxGraphicsContext* gc)
{
    for (int i = 0; i < static_cast<int>(footprint_->pads.size()); ++i)
    {
        const eda::PadItem& pad = footprint_->pads[i];
        const wxPoint2DDouble center = ModelToScreen(pad.x, pad.y);
        const double w = std::max(2.0, pad.width * zoom_);
        const double h = std::max(2.0, pad.height * zoom_);
        const bool selected = selection_.type == eda::ItemType::Pad && selection_.index == i;

        gc->PushState();
        gc->Translate(center.m_x, center.m_y);
        gc->Rotate(pad.rotation * kPi / 180.0);
        gc->SetPen(wxPen(selected ? Selected() : PadEdge(), selected ? 3 : 1));
        gc->SetBrush(wxBrush(PadFill()));

        if (pad.kind == eda::PadKind::ThroughHole || pad.shape == eda::PadShape::Circle)
        {
            gc->DrawEllipse(-w * 0.5, -h * 0.5, w, h);
            if (pad.drill > 0.0)
            {
                const double drill = std::max(2.0, pad.drill * zoom_);
                gc->SetBrush(wxBrush(Background()));
                gc->SetPen(wxPen(PadEdge(), 1));
                gc->DrawEllipse(-drill * 0.5, -drill * 0.5, drill, drill);
            }
        }
        else if (pad.shape == eda::PadShape::Rect)
        {
            gc->DrawRectangle(-w * 0.5, -h * 0.5, w, h);
        }
        else
        {
            gc->DrawRoundedRectangle(-w * 0.5, -h * 0.5, w, h, std::min(w, h) * 0.18);
        }

        if (pad.clearance > 0.0)
        {
            const double clearance = pad.clearance * zoom_;
            gc->SetPen(wxPen(wxColour(196, 96, 78), 1, wxPENSTYLE_LONG_DASH));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->DrawRoundedRectangle(-w * 0.5 - clearance, -h * 0.5 - clearance,
                                     w + 2 * clearance, h + 2 * clearance, 3);
        }
        gc->PopState();

        gc->SetFont(wxFontInfo(8), TextColour());
        gc->DrawText(wxString::FromUTF8(pad.number), center.m_x + 4, center.m_y - 16);
    }
}

void FootprintCanvas::DrawSilkscreen(wxGraphicsContext* gc)
{
    const wxColour colour = SilkColour();
    for (int i = 0; i < static_cast<int>(footprint_->lines.size()); ++i)
    {
        const eda::SilkLine& line = footprint_->lines[i];
        const bool selected = selection_.type == eda::ItemType::Line && selection_.index == i;
        const wxPoint2DDouble a = ModelToScreen(line.x1, line.y1);
        const wxPoint2DDouble b = ModelToScreen(line.x2, line.y2);
        gc->SetPen(wxPen(selected ? Selected() : colour,
                         std::max(1, static_cast<int>(std::lround(line.width * zoom_)))));
        gc->StrokeLine(a.m_x, a.m_y, b.m_x, b.m_y);
    }

    for (int i = 0; i < static_cast<int>(footprint_->rects.size()); ++i)
    {
        const eda::SilkRect& rect = footprint_->rects[i];
        const wxPoint2DDouble a = ModelToScreen(rect.x1, rect.y1);
        const wxPoint2DDouble b = ModelToScreen(rect.x2, rect.y2);
        const bool selected = selection_.type == eda::ItemType::Rect && selection_.index == i;
        gc->SetPen(wxPen(selected ? Selected() : colour,
                         std::max(1, static_cast<int>(std::lround(rect.width * zoom_)))));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->DrawRectangle(std::min(a.m_x, b.m_x), std::min(a.m_y, b.m_y),
                          std::fabs(b.m_x - a.m_x), std::fabs(b.m_y - a.m_y));
    }

    for (int i = 0; i < static_cast<int>(footprint_->arcs.size()); ++i)
    {
        const eda::SilkArc& arc = footprint_->arcs[i];
        const bool selected = selection_.type == eda::ItemType::Arc && selection_.index == i;
        wxGraphicsPath path = gc->CreatePath();
        for (int s = 0; s <= 40; ++s)
        {
            const double angle = (arc.startDegrees + arc.sweepDegrees * s / 40.0) * kPi / 180.0;
            const wxPoint2DDouble p = ModelToScreen(arc.cx + std::cos(angle) * arc.rx,
                                                    arc.cy + std::sin(angle) * arc.ry);
            if (s == 0) path.MoveToPoint(p); else path.AddLineToPoint(p);
        }
        gc->SetPen(wxPen(selected ? Selected() : colour,
                         std::max(1, static_cast<int>(std::lround(arc.width * zoom_)))));
        gc->StrokePath(path);
    }

    for (int i = 0; i < static_cast<int>(footprint_->polygons.size()); ++i)
    {
        DrawPolygon(gc, footprint_->polygons[i],
                    selection_.type == eda::ItemType::Polygon && selection_.index == i);
    }

    for (int i = 0; i < static_cast<int>(footprint_->texts.size()); ++i)
    {
        const eda::SilkText& text = footprint_->texts[i];
        const wxPoint2DDouble p = ModelToScreen(text.x, text.y);
        const bool selected = selection_.type == eda::ItemType::Text && selection_.index == i;
        const wxFont font(wxFontInfo(std::max(6, static_cast<int>(std::lround(text.size * zoom_ * 0.75)))));
        gc->SetFont(font, selected ? Selected() : colour);
        gc->DrawText(wxString::FromUTF8(text.text), p.m_x, p.m_y);
    }
}

void FootprintCanvas::DrawPolygon(wxGraphicsContext* gc,
                                  const eda::SilkPolygon& polygon,
                                  bool selected)
{
    if (polygon.points.size() < 2)
    {
        return;
    }
    wxGraphicsPath path = gc->CreatePath();
    const wxPoint2DDouble first = ModelToScreen(polygon.points[0].first, polygon.points[0].second);
    path.MoveToPoint(first);
    for (std::size_t i = 1; i < polygon.points.size(); ++i)
    {
        const auto& p = polygon.points[i];
        path.AddLineToPoint(ModelToScreen(p.first, p.second));
    }
    path.CloseSubpath();
    gc->SetPen(wxPen(selected ? Selected() : SilkColour(),
                     std::max(1, static_cast<int>(std::lround(polygon.width * zoom_)))));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->StrokePath(path);
}

wxColour FootprintCanvas::SilkColour() const
{
    if (footprint_ != nullptr)
    {
        wxColour colour(wxString::FromUTF8(footprint_->silkColor));
        if (colour.IsOk())
        {
            return colour;
        }
    }
    return wxColour(26, 105, 132);
}

void FootprintCanvas::NotifyChanged()
{
    if (owner_ != nullptr)
    {
        owner_->OnCanvasChanged();
    }
}

void FootprintCanvas::NotifySelectionChanged()
{
    if (owner_ != nullptr)
    {
        owner_->OnSelectionChanged();
    }
}
