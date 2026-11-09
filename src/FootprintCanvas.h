#pragma once

#include "FootprintModel.h"

#include <wx/wx.h>

#include <vector>

class MainFrame;

enum class EditorTool
{
    Select,
    SmdPad,
    ThroughHolePin,
    SilkLine,
    SilkRect,
    SilkArc,
    SilkPolygon,
    Text
};

enum class SelectionType
{
    None,
    Pad,
    Line,
    Rect,
    Arc,
    Polygon,
    Text
};

class FootprintCanvas final : public wxPanel
{
public:
    FootprintCanvas(wxWindow* parent, MainFrame* owner);

    void SetFootprint(Footprint* footprint);
    Footprint* GetFootprint() const { return footprint_; }

    void SetTool(EditorTool tool);
    EditorTool GetTool() const { return tool_; }

    void FitView();
    void ZoomIn();
    void ZoomOut();
    void ZoomReset();
    void ToggleGrid();
    bool IsGridVisible() const { return showGrid_; }
    void DeleteSelected();
    void CancelPending();

    SelectionType GetSelectionType() const { return selectionType_; }
    int GetSelectionIndex() const { return selectionIndex_; }
    const PadItem* GetSelectedPad() const;
    bool ApplySelectedPad(const PadItem& updated);
    wxString GetSelectionDescription() const;

    void RefreshCanvas();

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnLeftDoubleClick(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnMiddleDown(wxMouseEvent& event);
    void OnMiddleUp(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnSize(wxSizeEvent& event);

    wxPoint2DDouble ModelToScreen(double x, double y) const;
    wxPoint2DDouble ScreenToModel(double x, double y) const;
    wxPoint2DDouble SnapToGrid(const wxPoint2DDouble& point) const;
    double CurrentGridStep() const;

    void DrawGrid(wxGraphicsContext* gc);
    void DrawPads(wxGraphicsContext* gc);
    void DrawSilkscreen(wxGraphicsContext* gc);
    void DrawSelection(wxGraphicsContext* gc);
    void DrawPending(wxGraphicsContext* gc);
    void DrawTextItem(wxGraphicsContext* gc, const SilkText& text, bool selected);

    void AddSmdPad(const wxPoint2DDouble& point);
    void AddThroughHolePin(const wxPoint2DDouble& point);
    void AddText(const wxPoint2DDouble& point);
    void FinishLine(const wxPoint2DDouble& point);
    void FinishRect(const wxPoint2DDouble& point);
    void FinishArc(const wxPoint2DDouble& point);
    void FinishPolygon();

    bool HitTest(const wxPoint2DDouble& point);
    bool HitPad(const wxPoint2DDouble& point, int index) const;
    bool HitLine(const wxPoint2DDouble& point, const SilkLine& line) const;
    bool HitRect(const wxPoint2DDouble& point, const SilkRect& rect) const;
    bool HitArc(const wxPoint2DDouble& point, const SilkArc& arc) const;
    bool HitPolygon(const wxPoint2DDouble& point, const SilkPolygon& polygon) const;
    bool HitText(const wxPoint2DDouble& point, const SilkText& text) const;

    void BeginDrag(const wxPoint2DDouble& modelPoint);
    void DragSelection(const wxPoint2DDouble& modelPoint);
    void ClearSelection();
    void NotifySelectionChanged();
    void NotifyDocumentChanged();

    int NextPadNumber() const;
    double DistanceToSegment(double px, double py,
                             double x1, double y1,
                             double x2, double y2) const;

    Footprint* footprint_ = nullptr;
    MainFrame* owner_ = nullptr;
    EditorTool tool_ = EditorTool::Select;

    double zoom_ = 24.0;
    double panX_ = 0.0;
    double panY_ = 0.0;
    bool fitPending_ = true;
    bool showGrid_ = true;

    SelectionType selectionType_ = SelectionType::None;
    int selectionIndex_ = -1;
    bool dragging_ = false;
    wxPoint2DDouble dragStartModel_;
    wxPoint2DDouble dragLastModel_;

    bool hasFirstPoint_ = false;
    wxPoint2DDouble firstPoint_;
    std::vector<wxPoint2DDouble> polygonPoints_;

    bool panning_ = false;
    wxPoint lastScreenPoint_;
};

