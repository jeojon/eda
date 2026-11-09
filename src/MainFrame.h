#pragma once

#include "FootprintModel.h"
#include "FootprintLibrary.h"

#include <wx/wx.h>

#include <vector>

class FootprintCanvas;
enum class EditorTool;
class wxListBox;
class wxTreeCtrl;
class wxTreeEvent;
class wxTextCtrl;
class wxChoice;
class wxStaticText;

class MainFrame final : public wxFrame
{
public:
    MainFrame();

    void OnCanvasSelectionChanged();
    void OnCanvasDocumentChanged();
    void UpdateCursorPosition(double x, double y);

private:
    enum
    {
        ID_New = wxID_HIGHEST + 100,
        ID_Open,
        ID_Save,
        ID_SaveAs,
        ID_ReloadLibrary,
        ID_DeleteFootprint,
        ID_FitView,
        ID_ZoomIn,
        ID_ZoomOut,
        ID_ZoomReset,
        ID_ToggleGrid
    };

    void BuildMenu();
    void BuildLayout();
    wxPanel* BuildToolPanel(wxWindow* parent);
    wxPanel* BuildLibraryPanel(wxWindow* parent);
    wxWindow* BuildPropertyPanel(wxWindow* parent);

    void LoadLibrary();
    void SelectLibraryItem(const wxString& path);
    bool LoadFootprint(const wxString& path);
    void StartNewFootprint();
    bool SaveCurrent();
    bool SaveCurrentAs();
    bool ConfirmDiscardChanges();
    void SyncMetadataFromControls();
    void RefreshProperties();
    void SetDirty(bool dirty);
    void UpdateTitle();
    void SetTool(EditorTool tool);
    void SelectAllPadFields(bool enabled);
    bool ReadDoubleControl(wxTextCtrl* control, double* value) const;

    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnReloadLibrary(wxCommandEvent& event);
    void OnDeleteFootprint(wxCommandEvent& event);
    void OnLibrarySelected(wxTreeEvent& event);
    void OnApplyProperties(wxCommandEvent& event);
    void OnFitView(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);
    void OnZoomReset(wxCommandEvent& event);
    void OnToggleGrid(wxCommandEvent& event);
    void OnMetadataChanged(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnAbout(wxCommandEvent& event);

    Footprint currentFootprint_;
    FootprintLibrary library_;
    wxString currentPath_;
    bool dirty_ = false;
    bool updatingControls_ = false;

    FootprintCanvas* canvas_ = nullptr;
    wxTreeCtrl* libraryTree_ = nullptr;

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

