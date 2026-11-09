#pragma once

#include "eda/FootprintTypes.h"
#include "eda/IFootprintCodec.h"
#include "eda/IFootprintEditor.h"
#include "eda/IFootprintLibrary.h"
#include "eda/IGeometryService.h"

#include <wx/wx.h>

#include <memory>
#include <vector>

class FootprintCanvas;
enum class EditorTool;
class wxChoice;
class wxListBox;
class wxTextCtrl;
class wxStaticText;

class MainFrame final : public wxFrame
{
public:
    MainFrame();

    void OnCanvasChanged();
    void OnSelectionChanged();
    void SetCursorPosition(double x, double y);

private:
    void BuildMenu();
    void BuildLayout();
    wxPanel* BuildToolPanel(wxWindow* parent);
    wxPanel* BuildLibraryPanel(wxWindow* parent);
    wxWindow* BuildPropertyPanel(wxWindow* parent);

    void ReloadLibrary();
    void LoadSelectedLibraryEntry();
    void NewFootprint();
    bool SaveCurrent();
    bool SaveAs();
    void RefreshProperties();
    void SyncMetadata();
    void SetDirty(bool dirty);
    void SetTool(EditorTool tool);
    void ShowError(const wxString& message);

    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnReload(wxCommandEvent& event);
    void OnLibrarySelected(wxCommandEvent& event);
    void OnApplyPad(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnFit(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    std::shared_ptr<eda::IFootprintEditor> editorService_;
    std::shared_ptr<eda::IGeometryService> geometryService_;
    std::shared_ptr<eda::IFootprintCodec> codec_;
    std::shared_ptr<eda::IFootprintLibrary> libraryService_;

    eda::Footprint currentFootprint_;
    std::wstring currentPath_;
    bool dirty_ = false;
    bool updatingControls_ = false;

    FootprintCanvas* canvas_ = nullptr;
    wxListBox* libraryList_ = nullptr;
    std::vector<std::wstring> libraryPaths_;

    wxTextCtrl* footprintName_ = nullptr;
    wxTextCtrl* footprintCategory_ = nullptr;
    wxTextCtrl* footprintDescription_ = nullptr;
    wxTextCtrl* silkColor_ = nullptr;
    wxStaticText* selectionLabel_ = nullptr;

    wxChoice* padKind_ = nullptr;
    wxTextCtrl* padNumber_ = nullptr;
    wxTextCtrl* padName_ = nullptr;
    wxTextCtrl* padX_ = nullptr;
    wxTextCtrl* padY_ = nullptr;
    wxTextCtrl* padWidth_ = nullptr;
    wxTextCtrl* padHeight_ = nullptr;
    wxTextCtrl* padRotation_ = nullptr;
    wxTextCtrl* padDrill_ = nullptr;
    wxTextCtrl* padMask_ = nullptr;
    wxTextCtrl* padClearance_ = nullptr;
};

