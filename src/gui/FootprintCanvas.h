#pragma once

#include "eda/FootprintTypes.h"
#include "eda/IFootprintEditor.h"
#include "eda/IGeometryService.h"

#include <wx/wx.h>

#include <memory>
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

class FootprintCanvas final : public wxPanel
{
public:
    FootprintCanvas(wxWindow* parent,
                    MainFrame* owner,
                    std::shared_ptr<eda::IFootprintEditor> editor,
                    std::shared_ptr<eda::IGeometryService> geometry);

    void SetFootprint(eda::Footprint* footprint);
    eda::Footprint* GetFootprint() const { return footprint_; }
    void SetTool(EditorTool tool);
    EditorTool GetTool() const { return tool_; }

    void FitView();
    void ZoomIn();
    void ZoomOut();
    void DeleteSelected();
    void RefreshCanvas();

    const eda::HitResult& Selection() const { return selection_; }
    const eda::PadItem* SelectedPad() const;
    bool ApplySelectedPad(const eda::PadItem& pad);

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnMiddleDown(wxMouseEvent& event);
    void OnMiddleUp(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    wxPoint2DDouble ModelToScreen(double x, double y) const;
    wxPoint2DDouble ScreenToModel(double x, double y) const;
    wxPoint2DDouble Snap(const wxPoint2DDouble& point) const;
    double GridSize() const;

    void DrawGrid(wxGraphicsContext* gc);
    void DrawPads(wxGraphicsContext* gc);
    void DrawSilkscreen(wxGraphicsContext* gc);
    void DrawPolygon(wxGraphicsContext* gc, const eda::SilkPolygon& polygon, bool selected);
    wxColour SilkColour() const;

    void NotifyChanged();
    void NotifySelectionChanged();

    MainFrame* owner_ = nullptr;
    std::shared_ptr<eda::IFootprintEditor> editor_;
    std::shared_ptr<eda::IGeometryService> geometry_;
    eda::Footprint* footprint_ = nullptr;

    EditorTool tool_ = EditorTool::Select;
    eda::HitResult selection_;
    double zoom_ = 24.0;
    double panX_ = 0.0;
    double panY_ = 0.0;

    bool hasFirstPoint_ = false;
    wxPoint2DDouble firstPoint_;
    std::vector<wxPoint2DDouble> polygonPoints_;

    bool middlePanning_ = false;
    wxPoint lastMouseScreen_;
};

