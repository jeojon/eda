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

wxString U8(const char* text)
{
    return wxString::FromUTF8(text);
}

wxColour CanvasBackground()
{
    return wxColour(247, 246, 241);
}

wxColour GridMinorColour()
{
    return wxColour(228, 226, 218);
}

wxColour GridMajorColour()
{
    return wxColour(208, 204, 194);
}

wxColour PadColour()
{
    return wxColour(192, 112, 42);
}

wxColour PadEdgeColour()
{
    return wxColour(116, 62, 20);
}

wxColour SilkColour()
{
    return wxColour(26, 105, 132);
}

wxColour SelectionColour()
{
    return wxColour(225, 78, 44);
}

wxColour TextColour()
{
    return wxColour(34, 39, 45);
}
} // namespace

FootprintCanvas::FootprintCanvas(wxWindow* parent, MainFrame* owner)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxBORDER_SIMPLE | wxWANTS_CHARS),
      owner_(owner)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(CanvasBackground());

    Bind(wxEVT_PAINT, &FootprintCanvas::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &FootprintCanvas::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &FootprintCanvas::OnLeftUp, this);
    Bind(wxEVT_LEFT_DCLICK, &FootprintCanvas::OnLeftDoubleClick, this);
    Bind(wxEVT_RIGHT_DOWN, &FootprintCanvas::OnRightDown, this);
    Bind(wxEVT_MOTION, &FootprintCanvas::OnMotion, this);
    Bind(wxEVT_MOUSEWHEEL, &FootprintCanvas::OnMouseWheel, this);
    Bind(wxEVT_MIDDLE_DOWN, &FootprintCanvas::OnMiddleDown, this);
    Bind(wxEVT_MIDDLE_UP, &FootprintCanvas::OnMiddleUp, this);
    Bind(wxEVT_KEY_DOWN, &FootprintCanvas::OnKeyDown, this);
    Bind(wxEVT_SIZE, &FootprintCanvas::OnSize, this);
}

void FootprintCanvas::SetFootprint(Footprint* footprint)
{
    footprint_ = footprint;
    ClearSelection();
    CancelPending();
    fitPending_ = true;
    Refresh();
}

void FootprintCanvas::SetTool(EditorTool tool)
{
    tool_ = tool;
    CancelPending();
    if (tool_ != EditorTool::Select)
    {
        ClearSelection();
    }
    SetFocus();
    Refresh();
}

void FootprintCanvas::FitView()
{
    if (footprint_ == nullptr)
    {
        return;
    }

    const Bounds bounds = footprint_->GetBounds();
    const wxSize size = GetClientSize();
    if (size.x < 50 || size.y < 50)
    {
        fitPending_ = true;
        return;
    }
    const double availableWidth = std::max(80, size.x - 100);
    const double availableHeight = std::max(80, size.y - 100);
    const double width = std::max(1.0, bounds.maxX - bounds.minX);
    const double height = std::max(1.0, bounds.maxY - bounds.minY);

    zoom_ = Clamp(std::min(availableWidth / width, availableHeight / height), 4.0, 70.0);
    const double centerX = (bounds.minX + bounds.maxX) * 0.5;
    const double centerY = (bounds.minY + bounds.maxY) * 0.5;
    panX_ = -centerX * zoom_;
    panY_ = centerY * zoom_;
    fitPending_ = false;
}

void FootprintCanvas::ZoomIn()
{
    zoom_ = Clamp(zoom_ * 1.2, 2.0, 180.0);
    Refresh();
}

void FootprintCanvas::ZoomOut()
{
    zoom_ = Clamp(zoom_ / 1.2, 2.0, 180.0);
    Refresh();
}

void FootprintCanvas::ZoomReset()
{
    zoom_ = 24.0;
    panX_ = 0.0;
    panY_ = 0.0;
    Refresh();
}

void FootprintCanvas::ToggleGrid()
{
    showGrid_ = !showGrid_;
    Refresh();
}

void FootprintCanvas::DeleteSelected()
{
    if (footprint_ == nullptr || selectionType_ == SelectionType::None)
    {
        return;
    }

    switch (selectionType_)
    {
    case SelectionType::Pad:
        if (selectionIndex_ >= 0 && selectionIndex_ < static_cast<int>(footprint_->pads.size()))
        {
            footprint_->pads.erase(footprint_->pads.begin() + selectionIndex_);
        }
        break;
    case SelectionType::Line:
        if (selectionIndex_ >= 0 && selectionIndex_ < static_cast<int>(footprint_->lines.size()))
        {
            footprint_->lines.erase(footprint_->lines.begin() + selectionIndex_);
        }
        break;
    case SelectionType::Rect:
        if (selectionIndex_ >= 0 && selectionIndex_ < static_cast<int>(footprint_->rects.size()))
        {
            footprint_->rects.erase(footprint_->rects.begin() + selectionIndex_);
        }
        break;
    case SelectionType::Arc:
        if (selectionIndex_ >= 0 && selectionIndex_ < static_cast<int>(footprint_->arcs.size()))
        {
            footprint_->arcs.erase(footprint_->arcs.begin() + selectionIndex_);
        }
        break;
    case SelectionType::Polygon:
        if (selectionIndex_ >= 0 && selectionIndex_ < static_cast<int>(footprint_->polygons.size()))
        {
            footprint_->polygons.erase(footprint_->polygons.begin() + selectionIndex_);
        }
        break;
    case SelectionType::Text:
        if (selectionIndex_ >= 0 && selectionIndex_ < static_cast<int>(footprint_->texts.size()))
        {
            footprint_->texts.erase(footprint_->texts.begin() + selectionIndex_);
        }
        break;
    default:
        break;
    }

    ClearSelection();
    NotifyDocumentChanged();
    Refresh();
}

void FootprintCanvas::CancelPending()
{
    hasFirstPoint_ = false;
    polygonPoints_.clear();
}

const PadItem* FootprintCanvas::GetSelectedPad() const
{
    if (footprint_ == nullptr ||
        selectionType_ != SelectionType::Pad ||
        selectionIndex_ < 0 ||
        selectionIndex_ >= static_cast<int>(footprint_->pads.size()))
    {
        return nullptr;
    }
    return &footprint_->pads[selectionIndex_];
}

bool FootprintCanvas::ApplySelectedPad(const PadItem& updated)
{
    if (footprint_ == nullptr ||
        selectionType_ != SelectionType::Pad ||
        selectionIndex_ < 0 ||
        selectionIndex_ >= static_cast<int>(footprint_->pads.size()))
    {
        return false;
    }

    footprint_->pads[selectionIndex_] = updated;
    NotifySelectionChanged();
    NotifyDocumentChanged();
    Refresh();
    return true;
}

wxString FootprintCanvas::GetSelectionDescription() const
{
    if (footprint_ == nullptr || selectionType_ == SelectionType::None)
    {
        return U8("未选中对象");
    }

    if (selectionType_ == SelectionType::Pad &&
        selectionIndex_ >= 0 &&
        selectionIndex_ < static_cast<int>(footprint_->pads.size()))
    {
        const PadItem& pad = footprint_->pads[selectionIndex_];
        return (pad.kind == PadKind::Smd ? U8("SMD") : U8("THT")) +
            U8(" 焊盘 ") + wxString::FromUTF8(pad.number);
    }

    switch (selectionType_)
    {
    case SelectionType::Line: return U8("丝印线段");
    case SelectionType::Rect: return U8("丝印矩形");
    case SelectionType::Arc: return U8("丝印圆弧");
    case SelectionType::Polygon: return U8("丝印多边形");
    case SelectionType::Text: return U8("丝印文本");
    default: return U8("未选中对象");
    }
}

void FootprintCanvas::RefreshCanvas()
{
    fitPending_ = true;
    ClearSelection();
    Refresh();
}

void FootprintCanvas::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(CanvasBackground()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc)
    {
        return;
    }

    if (fitPending_)
    {
        FitView();
    }

    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
    if (showGrid_)
    {
        DrawGrid(gc.get());
    }

    if (footprint_ != nullptr)
    {
        DrawPads(gc.get());
        DrawSilkscreen(gc.get());
        DrawSelection(gc.get());
        DrawPending(gc.get());
    }

    gc->SetFont(wxFontInfo(9).Family(wxFONTFAMILY_SWISS), TextColour());
    const wxString scaleText = wxString::Format(
        "Zoom %.1f px/mm | Grid %.2f mm | Tool: %s",
        zoom_,
        CurrentGridStep(),
        GetTool() == EditorTool::Select ? "Select" :
        GetTool() == EditorTool::SmdPad ? "SMD Pad" :
        GetTool() == EditorTool::ThroughHolePin ? "THT Pin" :
        GetTool() == EditorTool::SilkLine ? "Line" :
        GetTool() == EditorTool::SilkRect ? "Rect" :
        GetTool() == EditorTool::SilkArc ? "Arc" :
        GetTool() == EditorTool::SilkPolygon ? "Polygon" : "Text");
    gc->DrawText(scaleText, 12, GetClientSize().y - 24);
}

void FootprintCanvas::OnLeftDown(wxMouseEvent& event)
{
    SetFocus();
    const wxPoint2DDouble rawPoint = ScreenToModel(event.GetX(), event.GetY());
    const wxPoint2DDouble point = SnapToGrid(rawPoint);

    if (tool_ == EditorTool::Select)
    {
        if (HitTest(rawPoint))
        {
            BeginDrag(point);
        }
        else
        {
            ClearSelection();
            Refresh();
        }
        return;
    }

    if (tool_ == EditorTool::SmdPad)
    {
        AddSmdPad(point);
    }
    else if (tool_ == EditorTool::ThroughHolePin)
    {
        AddThroughHolePin(point);
    }
    else if (tool_ == EditorTool::Text)
    {
        AddText(point);
    }
    else if (tool_ == EditorTool::SilkPolygon)
    {
        polygonPoints_.push_back(point);
        Refresh();
    }
    else if (!hasFirstPoint_)
    {
        firstPoint_ = point;
        hasFirstPoint_ = true;
        Refresh();
    }
    else if (tool_ == EditorTool::SilkLine)
    {
        FinishLine(point);
    }
    else if (tool_ == EditorTool::SilkRect)
    {
        FinishRect(point);
    }
    else if (tool_ == EditorTool::SilkArc)
    {
        FinishArc(point);
    }
}

void FootprintCanvas::OnLeftUp(wxMouseEvent&)
{
    if (dragging_)
    {
        dragging_ = false;
        NotifyDocumentChanged();
    }
    if (HasCapture())
    {
        ReleaseMouse();
    }
}

void FootprintCanvas::OnLeftDoubleClick(wxMouseEvent&)
{
    if (tool_ == EditorTool::SilkPolygon)
    {
        FinishPolygon();
    }
}

void FootprintCanvas::OnRightDown(wxMouseEvent&)
{
    if (tool_ == EditorTool::SilkPolygon && polygonPoints_.size() >= 3)
    {
        FinishPolygon();
        return;
    }
    CancelPending();
    Refresh();
}

void FootprintCanvas::OnMotion(wxMouseEvent& event)
{
    if (panning_ && event.MiddleIsDown())
    {
        const wxPoint current(event.GetX(), event.GetY());
        panX_ += current.x - lastScreenPoint_.x;
        panY_ += current.y - lastScreenPoint_.y;
        lastScreenPoint_ = current;
        Refresh();
        return;
    }

    if (dragging_ && event.LeftIsDown())
    {
        DragSelection(SnapToGrid(ScreenToModel(event.GetX(), event.GetY())));
        Refresh();
    }
    else
    {
        const wxPoint2DDouble model = ScreenToModel(event.GetX(), event.GetY());
        if (owner_ != nullptr)
        {
            owner_->UpdateCursorPosition(model.m_x, model.m_y);
        }
    }
}

void FootprintCanvas::OnMouseWheel(wxMouseEvent& event)
{
    const wxPoint2DDouble before = ScreenToModel(event.GetX(), event.GetY());
    const double factor = event.GetWheelRotation() > 0 ? 1.15 : 1.0 / 1.15;
    zoom_ = Clamp(zoom_ * factor, 2.0, 180.0);

    const wxSize size = GetClientSize();
    panX_ = event.GetX() - size.x * 0.5 - before.m_x * zoom_;
    panY_ = event.GetY() - size.y * 0.5 + before.m_y * zoom_;
    Refresh();
}

void FootprintCanvas::OnMiddleDown(wxMouseEvent& event)
{
    panning_ = true;
    lastScreenPoint_ = wxPoint(event.GetX(), event.GetY());
    CaptureMouse();
}

void FootprintCanvas::OnMiddleUp(wxMouseEvent&)
{
    panning_ = false;
    if (HasCapture())
    {
        ReleaseMouse();
    }
}

void FootprintCanvas::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_BACK)
    {
        DeleteSelected();
        return;
    }

    if (event.GetKeyCode() == WXK_ESCAPE)
    {
        CancelPending();
        ClearSelection();
        Refresh();
        return;
    }

    event.Skip();
}

void FootprintCanvas::OnSize(wxSizeEvent& event)
{
    if (fitPending_)
    {
        FitView();
    }
    Refresh();
    event.Skip();
}

wxPoint2DDouble FootprintCanvas::ModelToScreen(double x, double y) const
{
    const wxSize size = GetClientSize();
    return wxPoint2DDouble(
        size.x * 0.5 + panX_ + x * zoom_,
        size.y * 0.5 + panY_ - y * zoom_);
}

wxPoint2DDouble FootprintCanvas::ScreenToModel(double x, double y) const
{
    const wxSize size = GetClientSize();
    return wxPoint2DDouble(
        (x - size.x * 0.5 - panX_) / zoom_,
        -(y - size.y * 0.5 - panY_) / zoom_);
}

wxPoint2DDouble FootprintCanvas::SnapToGrid(const wxPoint2DDouble& point) const
{
    const double grid = CurrentGridStep();
    return wxPoint2DDouble(
        std::round(point.m_x / grid) * grid,
        std::round(point.m_y / grid) * grid);
}

double FootprintCanvas::CurrentGridStep() const
{
    if (zoom_ >= 50.0)
    {
        return 0.1;
    }
    if (zoom_ >= 15.0)
    {
        return 0.5;
    }
    return 1.0;
}

void FootprintCanvas::DrawGrid(wxGraphicsContext* gc)
{
    const wxSize size = GetClientSize();
    const wxPoint2DDouble topLeft = ScreenToModel(0, 0);
    const wxPoint2DDouble bottomRight = ScreenToModel(size.x, size.y);

    const double majorStep = 5.0 * CurrentGridStep();
    const double minorStep = CurrentGridStep();
    const double minX = std::min(topLeft.m_x, bottomRight.m_x);
    const double maxX = std::max(topLeft.m_x, bottomRight.m_x);
    const double minY = std::min(topLeft.m_y, bottomRight.m_y);
    const double maxY = std::max(topLeft.m_y, bottomRight.m_y);

    const long startX = static_cast<long>(std::floor(minX / minorStep));
    const long endX = static_cast<long>(std::ceil(maxX / minorStep));
    const long startY = static_cast<long>(std::floor(minY / minorStep));
    const long endY = static_cast<long>(std::ceil(maxY / minorStep));

    for (long i = startX; i <= endX; ++i)
    {
        const double x = i * minorStep;
        const bool major = std::fabs(std::fmod(x, majorStep)) < minorStep * 0.25;
        const wxPoint2DDouble p1 = ModelToScreen(x, minY);
        const wxPoint2DDouble p2 = ModelToScreen(x, maxY);
        gc->SetPen(wxPen(major ? GridMajorColour() : GridMinorColour(), major ? 1 : 1));
        gc->StrokeLine(p1.m_x, p1.m_y, p2.m_x, p2.m_y);
    }

    for (long i = startY; i <= endY; ++i)
    {
        const double y = i * minorStep;
        const bool major = std::fabs(std::fmod(y, majorStep)) < minorStep * 0.25;
        const wxPoint2DDouble p1 = ModelToScreen(minX, y);
        const wxPoint2DDouble p2 = ModelToScreen(maxX, y);
        gc->SetPen(wxPen(major ? GridMajorColour() : GridMinorColour(), 1));
        gc->StrokeLine(p1.m_x, p1.m_y, p2.m_x, p2.m_y);
    }

    const wxPoint2DDouble origin = ModelToScreen(footprint_ != nullptr ? footprint_->originX : 0.0,
                                                  footprint_ != nullptr ? footprint_->originY : 0.0);
    gc->SetPen(wxPen(wxColour(160, 50, 40), 1));
    gc->StrokeLine(origin.m_x - 12, origin.m_y, origin.m_x + 12, origin.m_y);
    gc->StrokeLine(origin.m_x, origin.m_y - 12, origin.m_x, origin.m_y + 12);
}

void FootprintCanvas::DrawPads(wxGraphicsContext* gc)
{
    for (int index = 0; index < static_cast<int>(footprint_->pads.size()); ++index)
    {
        const PadItem& pad = footprint_->pads[index];
        const wxPoint2DDouble center = ModelToScreen(pad.x, pad.y);
        const double w = std::max(2.0, pad.width * zoom_);
        const double h = std::max(2.0, pad.height * zoom_);

        gc->PushState();
        gc->Translate(center.m_x, center.m_y);
        gc->Rotate(pad.rotation * kPi / 180.0);

        const bool selected = selectionType_ == SelectionType::Pad && selectionIndex_ == index;
        gc->SetPen(wxPen(selected ? SelectionColour() : PadEdgeColour(), selected ? 3 : 1));
        gc->SetBrush(wxBrush(PadColour()));

        if (pad.kind == PadKind::ThroughHole || pad.shape == PadShape::Circle)
        {
            gc->DrawEllipse(-w * 0.5, -h * 0.5, w, h);
            if (pad.drill > 0.0)
            {
                const double drill = std::max(2.0, pad.drill * zoom_);
                gc->SetBrush(wxBrush(CanvasBackground()));
                gc->SetPen(wxPen(PadEdgeColour(), 1));
                gc->DrawEllipse(-drill * 0.5, -drill * 0.5, drill, drill);
            }
        }
        else if (pad.shape == PadShape::Rect)
        {
            gc->DrawRectangle(-w * 0.5, -h * 0.5, w, h);
        }
        else
        {
            gc->DrawRoundedRectangle(-w * 0.5, -h * 0.5, w, h, std::min(w, h) * 0.18);
        }

        if (pad.maskExpansion > 0.0)
        {
            const double expansion = pad.maskExpansion * zoom_;
            gc->SetPen(wxPen(wxColour(124, 136, 151), 1, wxPENSTYLE_DOT));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->DrawRectangle(-w * 0.5 - expansion, -h * 0.5 - expansion,
                              w + 2.0 * expansion, h + 2.0 * expansion);
        }

        if (pad.clearance > 0.0)
        {
            const double clearance = pad.clearance * zoom_;
            gc->SetPen(wxPen(wxColour(196, 96, 78), 1, wxPENSTYLE_LONG_DASH));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->DrawRoundedRectangle(
                -w * 0.5 - clearance,
                -h * 0.5 - clearance,
                w + 2.0 * clearance,
                h + 2.0 * clearance,
                std::min(w, h) * 0.18);
        }

        gc->PopState();

        gc->SetFont(wxFontInfo(8).Family(wxFONTFAMILY_SWISS), TextColour());
        gc->DrawText(wxString::FromUTF8(pad.number), center.m_x + 4, center.m_y - 16);
    }
}

void FootprintCanvas::DrawSilkscreen(wxGraphicsContext* gc)
{
    wxColour silkColour(wxString::FromUTF8(footprint_->silkColor));
    if (!silkColour.IsOk())
    {
        silkColour = SilkColour();
    }

    for (int index = 0; index < static_cast<int>(footprint_->lines.size()); ++index)
    {
        const SilkLine& line = footprint_->lines[index];
        const bool selected = selectionType_ == SelectionType::Line && selectionIndex_ == index;
        gc->SetPen(wxPen(selected ? SelectionColour() : silkColour,
                         std::max(1, static_cast<int>(std::lround(line.width * zoom_)))));
        const wxPoint2DDouble p1 = ModelToScreen(line.x1, line.y1);
        const wxPoint2DDouble p2 = ModelToScreen(line.x2, line.y2);
        gc->StrokeLine(p1.m_x, p1.m_y, p2.m_x, p2.m_y);
    }

    for (int index = 0; index < static_cast<int>(footprint_->rects.size()); ++index)
    {
        const SilkRect& rect = footprint_->rects[index];
        const bool selected = selectionType_ == SelectionType::Rect && selectionIndex_ == index;
        const wxPoint2DDouble p1 = ModelToScreen(rect.x1, rect.y1);
        const wxPoint2DDouble p2 = ModelToScreen(rect.x2, rect.y2);
        gc->SetPen(wxPen(selected ? SelectionColour() : silkColour,
                         std::max(1, static_cast<int>(std::lround(rect.width * zoom_)))));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->DrawRectangle(std::min(p1.m_x, p2.m_x), std::min(p1.m_y, p2.m_y),
                          std::fabs(p2.m_x - p1.m_x), std::fabs(p2.m_y - p1.m_y));
    }

    for (int index = 0; index < static_cast<int>(footprint_->arcs.size()); ++index)
    {
        const SilkArc& arc = footprint_->arcs[index];
        const bool selected = selectionType_ == SelectionType::Arc && selectionIndex_ == index;
        wxGraphicsPath path = gc->CreatePath();
        const int steps = 40;
        for (int step = 0; step <= steps; ++step)
        {
            const double degrees = arc.startDegrees + arc.sweepDegrees * step / steps;
            const double radians = degrees * kPi / 180.0;
            const double x = arc.cx + std::cos(radians) * arc.rx;
            const double y = arc.cy + std::sin(radians) * arc.ry;
            const wxPoint2DDouble point = ModelToScreen(x, y);
            if (step == 0)
            {
                path.MoveToPoint(point);
            }
            else
            {
                path.AddLineToPoint(point);
            }
        }
        gc->SetPen(wxPen(selected ? SelectionColour() : silkColour,
                         std::max(1, static_cast<int>(std::lround(arc.width * zoom_)))));
        gc->StrokePath(path);
    }

    for (int index = 0; index < static_cast<int>(footprint_->polygons.size()); ++index)
    {
        const SilkPolygon& polygon = footprint_->polygons[index];
        if (polygon.points.size() < 2)
        {
            continue;
        }
        const bool selected = selectionType_ == SelectionType::Polygon && selectionIndex_ == index;
        wxGraphicsPath path = gc->CreatePath();
        const wxPoint2DDouble first = ModelToScreen(polygon.points.front().first, polygon.points.front().second);
        path.MoveToPoint(first);
        for (std::size_t i = 1; i < polygon.points.size(); ++i)
        {
            path.AddLineToPoint(ModelToScreen(polygon.points[i].first, polygon.points[i].second));
        }
        path.CloseSubpath();
        gc->SetPen(wxPen(selected ? SelectionColour() : silkColour,
                         std::max(1, static_cast<int>(std::lround(polygon.width * zoom_)))));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->StrokePath(path);
    }

    for (int index = 0; index < static_cast<int>(footprint_->texts.size()); ++index)
    {
        DrawTextItem(gc, footprint_->texts[index],
                     selectionType_ == SelectionType::Text && selectionIndex_ == index);
    }
}

void FootprintCanvas::DrawSelection(wxGraphicsContext*)
{
}

void FootprintCanvas::DrawPending(wxGraphicsContext* gc)
{
    if (!hasFirstPoint_ && polygonPoints_.empty())
    {
        return;
    }

    gc->SetPen(wxPen(SelectionColour(), 1, wxPENSTYLE_SHORT_DASH));

    if (hasFirstPoint_)
    {
        const wxPoint2DDouble first = ModelToScreen(firstPoint_.m_x, firstPoint_.m_y);
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->DrawEllipse(first.m_x - 4.0, first.m_y - 4.0, 8.0, 8.0);

        if (tool_ == EditorTool::SilkArc)
        {
            const double viewRadius = std::max(10.0, zoom_ * 1.0);
            gc->DrawEllipse(first.m_x - viewRadius, first.m_y - viewRadius,
                            viewRadius * 2.0, viewRadius * 2.0);
        }
    }

    if (!polygonPoints_.empty())
    {
        wxGraphicsPath path = gc->CreatePath();
        const wxPoint2DDouble first = ModelToScreen(polygonPoints_.front().m_x, polygonPoints_.front().m_y);
        path.MoveToPoint(first);
        for (std::size_t i = 1; i < polygonPoints_.size(); ++i)
        {
            path.AddLineToPoint(ModelToScreen(polygonPoints_[i].m_x, polygonPoints_[i].m_y));
        }
        gc->StrokePath(path);
        for (const wxPoint2DDouble& point : polygonPoints_)
        {
            const wxPoint2DDouble screen = ModelToScreen(point.m_x, point.m_y);
            gc->SetBrush(wxBrush(SelectionColour()));
            gc->DrawEllipse(screen.m_x - 3.5, screen.m_y - 3.5, 7.0, 7.0);
        }
    }
}

void FootprintCanvas::DrawTextItem(wxGraphicsContext* gc, const SilkText& text, bool selected)
{
    const wxPoint2DDouble point = ModelToScreen(text.x, text.y);
    const wxFont font(wxFontInfo(std::max(6, static_cast<int>(std::lround(text.size * zoom_ * 0.75))))
                          .Family(wxFONTFAMILY_SWISS));
    wxColour silkColour(wxString::FromUTF8(footprint_->silkColor));
    if (!silkColour.IsOk())
    {
        silkColour = SilkColour();
    }
    gc->SetFont(font, selected ? SelectionColour() : silkColour);
    gc->DrawText(wxString::FromUTF8(text.text), point.m_x, point.m_y);
}

void FootprintCanvas::AddSmdPad(const wxPoint2DDouble& point)
{
    if (footprint_ == nullptr)
    {
        return;
    }

    PadItem pad;
    pad.kind = PadKind::Smd;
    pad.shape = PadShape::RoundedRect;
    pad.number = std::to_string(NextPadNumber());
    pad.name = pad.number;
    pad.x = point.m_x;
    pad.y = point.m_y;
    pad.width = 1.3;
    pad.height = 1.5;
    pad.maskExpansion = 0.1;
    footprint_->pads.push_back(pad);

    selectionType_ = SelectionType::Pad;
    selectionIndex_ = static_cast<int>(footprint_->pads.size()) - 1;
    NotifySelectionChanged();
    NotifyDocumentChanged();
    Refresh();
}

void FootprintCanvas::AddThroughHolePin(const wxPoint2DDouble& point)
{
    if (footprint_ == nullptr)
    {
        return;
    }

    PadItem pad;
    pad.kind = PadKind::ThroughHole;
    pad.shape = PadShape::Circle;
    pad.number = std::to_string(NextPadNumber());
    pad.name = pad.number;
    pad.x = point.m_x;
    pad.y = point.m_y;
    pad.width = 1.524;
    pad.height = 1.524;
    pad.drill = 0.965;
    pad.maskExpansion = 0.1;
    footprint_->pads.push_back(pad);

    selectionType_ = SelectionType::Pad;
    selectionIndex_ = static_cast<int>(footprint_->pads.size()) - 1;
    NotifySelectionChanged();
    NotifyDocumentChanged();
    Refresh();
}

void FootprintCanvas::AddText(const wxPoint2DDouble& point)
{
    if (footprint_ == nullptr)
    {
        return;
    }

    const wxString value = wxGetTextFromUser(U8("输入丝印文本"), U8("添加文本"), "TEXT", this);
    if (value.empty())
    {
        return;
    }

    SilkText text;
    text.x = point.m_x;
    text.y = point.m_y;
    text.size = 0.9;
    text.text = value.utf8_string();
    footprint_->texts.push_back(text);

    selectionType_ = SelectionType::Text;
    selectionIndex_ = static_cast<int>(footprint_->texts.size()) - 1;
    NotifySelectionChanged();
    NotifyDocumentChanged();
    Refresh();
}

void FootprintCanvas::FinishLine(const wxPoint2DDouble& point)
{
    SilkLine line;
    line.x1 = firstPoint_.m_x;
    line.y1 = firstPoint_.m_y;
    line.x2 = point.m_x;
    line.y2 = point.m_y;
    footprint_->lines.push_back(line);
    hasFirstPoint_ = false;
    NotifyDocumentChanged();
    Refresh();
}

void FootprintCanvas::FinishRect(const wxPoint2DDouble& point)
{
    SilkRect rect;
    rect.x1 = firstPoint_.m_x;
    rect.y1 = firstPoint_.m_y;
    rect.x2 = point.m_x;
    rect.y2 = point.m_y;
    footprint_->rects.push_back(rect);
    hasFirstPoint_ = false;
    NotifyDocumentChanged();
    Refresh();
}

void FootprintCanvas::FinishArc(const wxPoint2DDouble& point)
{
    SilkArc arc;
    arc.cx = firstPoint_.m_x;
    arc.cy = firstPoint_.m_y;
    arc.rx = std::hypot(point.m_x - firstPoint_.m_x, point.m_y - firstPoint_.m_y);
    arc.ry = arc.rx;
    arc.startDegrees = 0.0;
    arc.sweepDegrees = 180.0;
    footprint_->arcs.push_back(arc);
    hasFirstPoint_ = false;
    NotifyDocumentChanged();
    Refresh();
}

void FootprintCanvas::FinishPolygon()
{
    if (footprint_ == nullptr || polygonPoints_.size() < 3)
    {
        CancelPending();
        Refresh();
        return;
    }

    SilkPolygon polygon;
    for (const wxPoint2DDouble& point : polygonPoints_)
    {
        polygon.points.emplace_back(point.m_x, point.m_y);
    }
    footprint_->polygons.push_back(polygon);
    polygonPoints_.clear();
    NotifyDocumentChanged();
    Refresh();
}

bool FootprintCanvas::HitTest(const wxPoint2DDouble& point)
{
    if (footprint_ == nullptr)
    {
        return false;
    }

    for (int i = static_cast<int>(footprint_->pads.size()) - 1; i >= 0; --i)
    {
        if (HitPad(point, i))
        {
            selectionType_ = SelectionType::Pad;
            selectionIndex_ = i;
            NotifySelectionChanged();
            return true;
        }
    }

    for (int i = static_cast<int>(footprint_->texts.size()) - 1; i >= 0; --i)
    {
        if (HitText(point, footprint_->texts[i]))
        {
            selectionType_ = SelectionType::Text;
            selectionIndex_ = i;
            NotifySelectionChanged();
            return true;
        }
    }

    for (int i = static_cast<int>(footprint_->lines.size()) - 1; i >= 0; --i)
    {
        if (HitLine(point, footprint_->lines[i]))
        {
            selectionType_ = SelectionType::Line;
            selectionIndex_ = i;
            NotifySelectionChanged();
            return true;
        }
    }

    for (int i = static_cast<int>(footprint_->rects.size()) - 1; i >= 0; --i)
    {
        if (HitRect(point, footprint_->rects[i]))
        {
            selectionType_ = SelectionType::Rect;
            selectionIndex_ = i;
            NotifySelectionChanged();
            return true;
        }
    }

    for (int i = static_cast<int>(footprint_->arcs.size()) - 1; i >= 0; --i)
    {
        if (HitArc(point, footprint_->arcs[i]))
        {
            selectionType_ = SelectionType::Arc;
            selectionIndex_ = i;
            NotifySelectionChanged();
            return true;
        }
    }

    for (int i = static_cast<int>(footprint_->polygons.size()) - 1; i >= 0; --i)
    {
        if (HitPolygon(point, footprint_->polygons[i]))
        {
            selectionType_ = SelectionType::Polygon;
            selectionIndex_ = i;
            NotifySelectionChanged();
            return true;
        }
    }

    return false;
}

bool FootprintCanvas::HitPad(const wxPoint2DDouble& point, int index) const
{
    const PadItem& pad = footprint_->pads[index];
    const double tolerance = 8.0 / zoom_;
    return std::fabs(point.m_x - pad.x) <= pad.width * 0.5 + tolerance &&
           std::fabs(point.m_y - pad.y) <= pad.height * 0.5 + tolerance;
}

bool FootprintCanvas::HitLine(const wxPoint2DDouble& point, const SilkLine& line) const
{
    const double tolerance = std::max(0.25, 6.0 / zoom_);
    return DistanceToSegment(point.m_x, point.m_y, line.x1, line.y1, line.x2, line.y2) <= tolerance;
}

bool FootprintCanvas::HitRect(const wxPoint2DDouble& point, const SilkRect& rect) const
{
    const double minX = std::min(rect.x1, rect.x2);
    const double maxX = std::max(rect.x1, rect.x2);
    const double minY = std::min(rect.y1, rect.y2);
    const double maxY = std::max(rect.y1, rect.y2);
    const double tolerance = 6.0 / zoom_;
    const double distance = std::min({
        std::fabs(point.m_x - minX),
        std::fabs(point.m_x - maxX),
        std::fabs(point.m_y - minY),
        std::fabs(point.m_y - maxY)});
    const bool inside = point.m_x >= minX - tolerance && point.m_x <= maxX + tolerance &&
                        point.m_y >= minY - tolerance && point.m_y <= maxY + tolerance;
    return inside && distance <= tolerance;
}

bool FootprintCanvas::HitArc(const wxPoint2DDouble& point, const SilkArc& arc) const
{
    const double dx = point.m_x - arc.cx;
    const double dy = point.m_y - arc.cy;
    double angle = std::atan2(dy / std::max(arc.ry, 1e-9), dx / std::max(arc.rx, 1e-9)) * 180.0 / kPi;
    if (angle < 0.0)
    {
        angle += 360.0;
    }

    double start = std::fmod(arc.startDegrees, 360.0);
    if (start < 0.0)
    {
        start += 360.0;
    }
    double end = start + arc.sweepDegrees;
    if (arc.sweepDegrees >= 0.0)
    {
        if (angle < start)
        {
            angle += 360.0;
        }
    }
    else
    {
        if (angle > start)
        {
            angle -= 360.0;
        }
        end = start + arc.sweepDegrees;
    }

    const double radius = std::hypot(dx, dy);
    const double averageRadius = std::max(0.1, (arc.rx + arc.ry) * 0.5);
    const double tolerance = std::max(0.25, 7.0 / zoom_);
    return radius >= averageRadius - tolerance && radius <= averageRadius + tolerance &&
           angle >= std::min(start, end) && angle <= std::max(start, end);
}

bool FootprintCanvas::HitPolygon(const wxPoint2DDouble& point, const SilkPolygon& polygon) const
{
    if (polygon.points.size() < 3)
    {
        return false;
    }

    for (std::size_t i = 0; i < polygon.points.size(); ++i)
    {
        const auto& a = polygon.points[i];
        const auto& b = polygon.points[(i + 1) % polygon.points.size()];
        if (DistanceToSegment(point.m_x, point.m_y, a.first, a.second, b.first, b.second) <= 6.0 / zoom_)
        {
            return true;
        }
    }
    return false;
}

bool FootprintCanvas::HitText(const wxPoint2DDouble& point, const SilkText& text) const
{
    return point.m_x >= text.x - 0.2 &&
           point.m_x <= text.x + text.size * 0.7 * std::max<std::size_t>(1, text.text.size()) &&
           point.m_y >= text.y - 0.2 &&
           point.m_y <= text.y + text.size + 0.2;
}

void FootprintCanvas::BeginDrag(const wxPoint2DDouble& modelPoint)
{
    dragging_ = true;
    dragStartModel_ = modelPoint;
    dragLastModel_ = modelPoint;
    CaptureMouse();
}

void FootprintCanvas::DragSelection(const wxPoint2DDouble& modelPoint)
{
    if (footprint_ == nullptr || selectionType_ == SelectionType::None)
    {
        return;
    }

    const double dx = modelPoint.m_x - dragLastModel_.m_x;
    const double dy = modelPoint.m_y - dragLastModel_.m_y;
    if (dx == 0.0 && dy == 0.0)
    {
        return;
    }

    if (selectionType_ == SelectionType::Pad)
    {
        PadItem& pad = footprint_->pads[selectionIndex_];
        pad.x += dx;
        pad.y += dy;
    }
    else if (selectionType_ == SelectionType::Line)
    {
        SilkLine& line = footprint_->lines[selectionIndex_];
        line.x1 += dx;
        line.x2 += dx;
        line.y1 += dy;
        line.y2 += dy;
    }
    else if (selectionType_ == SelectionType::Rect)
    {
        SilkRect& rect = footprint_->rects[selectionIndex_];
        rect.x1 += dx;
        rect.x2 += dx;
        rect.y1 += dy;
        rect.y2 += dy;
    }
    else if (selectionType_ == SelectionType::Arc)
    {
        SilkArc& arc = footprint_->arcs[selectionIndex_];
        arc.cx += dx;
        arc.cy += dy;
    }
    else if (selectionType_ == SelectionType::Polygon)
    {
        SilkPolygon& polygon = footprint_->polygons[selectionIndex_];
        for (auto& point : polygon.points)
        {
            point.first += dx;
            point.second += dy;
        }
    }
    else if (selectionType_ == SelectionType::Text)
    {
        SilkText& text = footprint_->texts[selectionIndex_];
        text.x += dx;
        text.y += dy;
    }

    dragLastModel_ = modelPoint;
}

void FootprintCanvas::ClearSelection()
{
    selectionType_ = SelectionType::None;
    selectionIndex_ = -1;
    dragging_ = false;
    if (HasCapture())
    {
        ReleaseMouse();
    }
    NotifySelectionChanged();
}

void FootprintCanvas::NotifySelectionChanged()
{
    if (owner_ != nullptr)
    {
        owner_->OnCanvasSelectionChanged();
    }
}

void FootprintCanvas::NotifyDocumentChanged()
{
    if (owner_ != nullptr)
    {
        owner_->OnCanvasDocumentChanged();
    }
}

int FootprintCanvas::NextPadNumber() const
{
    if (footprint_ == nullptr)
    {
        return 1;
    }
    return static_cast<int>(footprint_->pads.size()) + 1;
}

double FootprintCanvas::DistanceToSegment(double px, double py,
                                          double x1, double y1,
                                          double x2, double y2) const
{
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared <= 1e-12)
    {
        return std::hypot(px - x1, py - y1);
    }

    const double t = Clamp(((px - x1) * dx + (py - y1) * dy) / lengthSquared, 0.0, 1.0);
    return std::hypot(px - (x1 + t * dx), py - (y1 + t * dy));
}
