#include "MainFrame.h"

#include "FootprintCanvas.h"
#include "eda/ServiceFactory.h"

#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>

#include <algorithm>

namespace
{
wxString U8(const char* text)
{
    return wxString::FromUTF8(text);
}

wxTextCtrl* Field(wxWindow* parent)
{
    return new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxSize(110, -1));
}

bool ReadDouble(wxTextCtrl* field, double* value)
{
    return field->GetValue().ToDouble(value);
}
} // namespace

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, U8("EDA 实验一 - 课程骨架"),
              wxDefaultPosition, wxSize(1360, 860)),
      editorService_(eda::CreateFootprintEditor()),
      geometryService_(eda::CreateGeometryService()),
      codec_(eda::CreateFootprintCodec()),
      libraryService_(eda::CreateFootprintLibrary())
{
    SetMinSize(wxSize(1120, 700));
    BuildMenu();
    BuildLayout();

    CreateStatusBar(2);
    const int widths[] = {-1, 280};
    SetStatusWidths(2, widths);
    SetStatusText(U8("课程骨架：请在 src/todo 中实现逻辑"), 0);
    SetStatusText(U8("光标: -- mm"), 1);

    Bind(wxEVT_MENU, &MainFrame::OnNew, this, wxID_NEW);
    Bind(wxEVT_MENU, &MainFrame::OnOpen, this, wxID_OPEN);
    Bind(wxEVT_MENU, &MainFrame::OnSave, this, wxID_SAVE);
    Bind(wxEVT_MENU, &MainFrame::OnSaveAs, this, wxID_SAVEAS);
    Bind(wxEVT_MENU, &MainFrame::OnReload, this, wxID_REFRESH);
    Bind(wxEVT_MENU, &MainFrame::OnDelete, this, wxID_DELETE);
    Bind(wxEVT_MENU, &MainFrame::OnFit, this, wxID_ZOOM_FIT);
    Bind(wxEVT_MENU, &MainFrame::OnZoomIn, this, wxID_ZOOM_IN);
    Bind(wxEVT_MENU, &MainFrame::OnZoomOut, this, wxID_ZOOM_OUT);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

    ReloadLibrary();
    if (!libraryPaths_.empty())
    {
        libraryList_->SetSelection(0);
        LoadSelectedLibraryEntry();
    }
    else
    {
        NewFootprint();
    }

    Centre();
}

void MainFrame::BuildMenu()
{
    auto* fileMenu = new wxMenu();
    fileMenu->Append(wxID_NEW, U8("新建封装\tCtrl+N"));
    fileMenu->Append(wxID_OPEN, U8("打开封装...\tCtrl+O"));
    fileMenu->Append(wxID_SAVE, U8("保存\tCtrl+S"));
    fileMenu->Append(wxID_SAVEAS, U8("另存为...\tCtrl+Shift+S"));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, U8("退出"));

    auto* editMenu = new wxMenu();
    editMenu->Append(wxID_DELETE, U8("删除选中\tDel"));
    editMenu->Append(wxID_REFRESH, U8("刷新元件库\tF5"));

    auto* viewMenu = new wxMenu();
    viewMenu->Append(wxID_ZOOM_FIT, U8("适配画布\tF"));
    viewMenu->Append(wxID_ZOOM_IN, U8("放大\tCtrl++"));
    viewMenu->Append(wxID_ZOOM_OUT, U8("缩小\tCtrl+-"));

    auto* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, U8("关于"));

    auto* bar = new wxMenuBar();
    bar->Append(fileMenu, U8("文件"));
    bar->Append(editMenu, U8("编辑"));
    bar->Append(viewMenu, U8("视图"));
    bar->Append(helpMenu, U8("帮助"));
    SetMenuBar(bar);
}

void MainFrame::BuildLayout()
{
    auto* root = new wxPanel(this);
    auto* rootSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* left = BuildLibraryPanel(root);
    left->SetMinSize(wxSize(270, -1));
    left->SetMaxSize(wxSize(300, -1));

    canvas_ = new FootprintCanvas(root, this, editorService_, geometryService_);
    auto* right = BuildPropertyPanel(root);
    right->SetMinSize(wxSize(360, -1));
    right->SetMaxSize(wxSize(400, -1));

    rootSizer->Add(left, 0, wxEXPAND | wxALL, 6);
    rootSizer->Add(canvas_, 1, wxEXPAND | wxTOP | wxBOTTOM, 6);
    rootSizer->Add(right, 0, wxEXPAND | wxALL, 6);
    root->SetSizer(rootSizer);
}

wxPanel* MainFrame::BuildToolPanel(wxWindow* parent)
{
    auto* panel = new wxPanel(parent);
    auto* grid = new wxGridSizer(2, 6, 6);
    auto add = [&](const wxString& label, EditorTool tool)
    {
        auto* button = new wxButton(panel, wxID_ANY, label, wxDefaultPosition, wxSize(-1, 31));
        button->Bind(wxEVT_BUTTON, [this, tool](wxCommandEvent&) { SetTool(tool); });
        grid->Add(button, 1, wxEXPAND);
    };

    add(U8("选择"), EditorTool::Select);
    add(U8("SMD焊盘"), EditorTool::SmdPad);
    add(U8("THT引脚"), EditorTool::ThroughHolePin);
    add(U8("丝印线"), EditorTool::SilkLine);
    add(U8("矩形"), EditorTool::SilkRect);
    add(U8("圆弧"), EditorTool::SilkArc);
    add(U8("多边形"), EditorTool::SilkPolygon);
    add(U8("文本"), EditorTool::Text);
    panel->SetSizer(grid);
    return panel;
}

wxPanel* MainFrame::BuildLibraryPanel(wxWindow* parent)
{
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* toolsTitle = new wxStaticText(panel, wxID_ANY, U8("绘制工具"));
    toolsTitle->SetFont(toolsTitle->GetFont().Bold());
    sizer->Add(toolsTitle, 0, wxBOTTOM, 6);
    sizer->Add(BuildToolPanel(panel), 0, wxEXPAND | wxBOTTOM, 10);
    sizer->Add(new wxStaticLine(panel), 0, wxEXPAND | wxBOTTOM, 10);

    auto* libraryTitle = new wxStaticText(panel, wxID_ANY, U8("元件封装库"));
    libraryTitle->SetFont(libraryTitle->GetFont().Bold());
    sizer->Add(libraryTitle, 0, wxBOTTOM, 6);

    libraryList_ = new wxListBox(panel, wxID_ANY);
    libraryList_->Bind(wxEVT_LISTBOX, &MainFrame::OnLibrarySelected, this);
    sizer->Add(libraryList_, 1, wxEXPAND | wxBOTTOM, 6);

    auto* reload = new wxButton(panel, wxID_ANY, U8("刷新元件库"));
    reload->Bind(wxEVT_BUTTON, &MainFrame::OnReload, this);
    sizer->Add(reload, 0, wxEXPAND);
    panel->SetSizer(sizer);
    return panel;
}

wxWindow* MainFrame::BuildPropertyPanel(wxWindow* parent)
{
    auto* panel = new wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition,
                                       wxDefaultSize, wxVSCROLL);
    panel->SetScrollRate(0, 12);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* title = new wxStaticText(panel, wxID_ANY, U8("封装属性"));
    title->SetFont(title->GetFont().Bold());
    sizer->Add(title, 0, wxBOTTOM, 6);

    footprintName_ = Field(panel);
    footprintCategory_ = Field(panel);
    silkColor_ = Field(panel);
    footprintDescription_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition,
                                           wxSize(-1, 86), wxTE_MULTILINE);
    footprintName_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { SyncMetadata(); SetDirty(true); });
    footprintCategory_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { SyncMetadata(); SetDirty(true); });
    silkColor_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { SyncMetadata(); SetDirty(true); });
    footprintDescription_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { SyncMetadata(); SetDirty(true); });

    auto addMeta = [&](const wxString& label, wxWindow* control, int bottom = 8)
    {
        sizer->Add(new wxStaticText(panel, wxID_ANY, label), 0, wxEXPAND | wxBOTTOM, 2);
        sizer->Add(control, 0, wxEXPAND | wxBOTTOM, bottom);
    };
    addMeta(U8("名称"), footprintName_);
    addMeta(U8("分类"), footprintCategory_);
    addMeta(U8("丝印颜色"), silkColor_);
    addMeta(U8("描述"), footprintDescription_, 10);
    sizer->Add(new wxStaticLine(panel), 0, wxEXPAND | wxBOTTOM, 8);

    selectionLabel_ = new wxStaticText(panel, wxID_ANY, U8("未选中焊盘"));
    selectionLabel_->SetFont(selectionLabel_->GetFont().Bold());
    sizer->Add(selectionLabel_, 0, wxBOTTOM, 6);

    padKind_ = new wxChoice(panel, wxID_ANY);
    padKind_->Append(U8("SMD 焊盘"));
    padKind_->Append(U8("THT 引脚"));
    padNumber_ = Field(panel);
    padName_ = Field(panel);
    padX_ = Field(panel);
    padY_ = Field(panel);
    padWidth_ = Field(panel);
    padHeight_ = Field(panel);
    padRotation_ = Field(panel);
    padDrill_ = Field(panel);
    padMask_ = Field(panel);
    padClearance_ = Field(panel);

    auto* pads = new wxFlexGridSizer(2, 5, 5);
    pads->AddGrowableCol(1);
    auto rowPad = [&](const wxString& label, wxWindow* control)
    {
        pads->Add(new wxStaticText(panel, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
        pads->Add(control, 1, wxEXPAND);
    };
    rowPad(U8("类型"), padKind_);
    rowPad(U8("编号"), padNumber_);
    rowPad(U8("名称"), padName_);
    rowPad("X / mm", padX_);
    rowPad("Y / mm", padY_);
    rowPad(U8("宽 / mm"), padWidth_);
    rowPad(U8("高 / mm"), padHeight_);
    rowPad(U8("旋转 / deg"), padRotation_);
    rowPad(U8("钻孔 / mm"), padDrill_);
    rowPad(U8("阻焊扩展 / mm"), padMask_);
    rowPad(U8("安全间距 / mm"), padClearance_);
    sizer->Add(pads, 0, wxEXPAND | wxBOTTOM, 8);

    auto* apply = new wxButton(panel, wxID_ANY, U8("应用焊盘属性"));
    apply->Bind(wxEVT_BUTTON, &MainFrame::OnApplyPad, this);
    sizer->Add(apply, 0, wxEXPAND);
    panel->SetSizer(sizer);
    panel->FitInside();
    return panel;
}

void MainFrame::ReloadLibrary()
{
    libraryPaths_.clear();
    libraryList_->Clear();
    std::wstring error;
    if (!libraryService_->Reload(&error))
    {
        SetStatusText(U8("元件库尚未实现"), 0);
        return;
    }
    for (const eda::LibraryEntry& entry : libraryService_->Entries())
    {
        libraryPaths_.push_back(entry.path);
        libraryList_->Append("[" + wxString::FromUTF8(entry.category) + "] " +
                             wxString::FromUTF8(entry.name));
    }
}

void MainFrame::LoadSelectedLibraryEntry()
{
    const int selection = libraryList_->GetSelection();
    if (selection == wxNOT_FOUND || selection >= static_cast<int>(libraryPaths_.size()))
    {
        return;
    }

    std::wstring error;
    if (!libraryService_->Read(libraryPaths_[selection], currentFootprint_, &error))
    {
        ShowError(wxString(error));
        return;
    }
    currentPath_ = libraryPaths_[selection];
    canvas_->SetFootprint(&currentFootprint_);
    RefreshProperties();
    SetDirty(false);
}

void MainFrame::NewFootprint()
{
    currentFootprint_ = {};
    currentFootprint_.name = "Untitled";
    currentPath_.clear();
    canvas_->SetFootprint(&currentFootprint_);
    RefreshProperties();
    SetDirty(true);
}

bool MainFrame::SaveCurrent()
{
    if (currentPath_.empty())
    {
        return SaveAs();
    }
    SyncMetadata();
    std::wstring error;
    if (!libraryService_->Write(currentPath_, currentFootprint_, &error))
    {
        ShowError(wxString(error));
        return false;
    }
    SetDirty(false);
    return true;
}

bool MainFrame::SaveAs()
{
    wxString defaultDir(libraryService_->DefaultDirectory());
    wxFileDialog dialog(this, U8("保存封装"), defaultDir,
                        wxString::FromUTF8(currentFootprint_.name) + ".fp",
                        U8("封装文件 (*.fp)|*.fp|所有文件 (*.*)|*.*"),
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK)
    {
        return false;
    }
    currentPath_ = dialog.GetPath().ToStdWstring();
    return SaveCurrent();
}

void MainFrame::RefreshProperties()
{
    updatingControls_ = true;
    footprintName_->ChangeValue(wxString::FromUTF8(currentFootprint_.name));
    footprintCategory_->ChangeValue(wxString::FromUTF8(currentFootprint_.category));
    footprintDescription_->ChangeValue(wxString::FromUTF8(currentFootprint_.description));
    silkColor_->ChangeValue(wxString::FromUTF8(currentFootprint_.silkColor));

    const eda::PadItem* pad = canvas_->SelectedPad();
    selectionLabel_->SetLabel(pad == nullptr ? U8("未选中焊盘") : U8("已选中焊盘"));
    const bool enabled = pad != nullptr;
    padKind_->Enable(enabled);
    padNumber_->Enable(enabled);
    padName_->Enable(enabled);
    padX_->Enable(enabled);
    padY_->Enable(enabled);
    padWidth_->Enable(enabled);
    padHeight_->Enable(enabled);
    padRotation_->Enable(enabled);
    padDrill_->Enable(enabled);
    padMask_->Enable(enabled);
    padClearance_->Enable(enabled);

    if (pad != nullptr)
    {
        padKind_->SetSelection(pad->kind == eda::PadKind::Smd ? 0 : 1);
        padNumber_->ChangeValue(wxString::FromUTF8(pad->number));
        padName_->ChangeValue(wxString::FromUTF8(pad->name));
        padX_->ChangeValue(wxString::Format("%.4f", pad->x));
        padY_->ChangeValue(wxString::Format("%.4f", pad->y));
        padWidth_->ChangeValue(wxString::Format("%.4f", pad->width));
        padHeight_->ChangeValue(wxString::Format("%.4f", pad->height));
        padRotation_->ChangeValue(wxString::Format("%.2f", pad->rotation));
        padDrill_->ChangeValue(wxString::Format("%.4f", pad->drill));
        padMask_->ChangeValue(wxString::Format("%.4f", pad->maskExpansion));
        padClearance_->ChangeValue(wxString::Format("%.4f", pad->clearance));
    }
    updatingControls_ = false;
}

void MainFrame::SyncMetadata()
{
    if (updatingControls_)
    {
        return;
    }
    currentFootprint_.name = footprintName_->GetValue().utf8_string();
    currentFootprint_.category = footprintCategory_->GetValue().utf8_string();
    currentFootprint_.description = footprintDescription_->GetValue().utf8_string();
    currentFootprint_.silkColor = silkColor_->GetValue().utf8_string();
}

void MainFrame::SetDirty(bool dirty)
{
    dirty_ = dirty;
    wxString title = U8("EDA 实验一 - 课程骨架");
    if (!currentFootprint_.name.empty())
    {
        title += " | " + wxString::FromUTF8(currentFootprint_.name);
    }
    if (dirty_)
    {
        title += " *";
    }
    SetTitle(title);
}

void MainFrame::SetTool(EditorTool tool)
{
    canvas_->SetTool(tool);
}

void MainFrame::ShowError(const wxString& message)
{
    wxMessageBox(message, U8("课程骨架"), wxOK | wxICON_ERROR, this);
}

void MainFrame::OnCanvasChanged()
{
    SetDirty(true);
    Refresh();
}

void MainFrame::OnSelectionChanged()
{
    RefreshProperties();
}

void MainFrame::SetCursorPosition(double x, double y)
{
    SetStatusText(wxString::Format(U8("光标: %.3f, %.3f mm"), x, y), 1);
}

void MainFrame::OnNew(wxCommandEvent&) { NewFootprint(); }
void MainFrame::OnOpen(wxCommandEvent&) { LoadSelectedLibraryEntry(); }
void MainFrame::OnSave(wxCommandEvent&) { SaveCurrent(); }
void MainFrame::OnSaveAs(wxCommandEvent&) { SaveAs(); }
void MainFrame::OnExit(wxCommandEvent&) { Close(); }
void MainFrame::OnReload(wxCommandEvent&) { ReloadLibrary(); }
void MainFrame::OnLibrarySelected(wxCommandEvent&) { LoadSelectedLibraryEntry(); }
void MainFrame::OnDelete(wxCommandEvent&) { canvas_->DeleteSelected(); }
void MainFrame::OnFit(wxCommandEvent&) { canvas_->FitView(); canvas_->Refresh(); }
void MainFrame::OnZoomIn(wxCommandEvent&) { canvas_->ZoomIn(); }
void MainFrame::OnZoomOut(wxCommandEvent&) { canvas_->ZoomOut(); }

void MainFrame::OnApplyPad(wxCommandEvent&)
{
    const eda::PadItem* current = canvas_->SelectedPad();
    if (current == nullptr)
    {
        return;
    }
    eda::PadItem pad = *current;
    pad.kind = padKind_->GetSelection() == 1 ? eda::PadKind::ThroughHole : eda::PadKind::Smd;
    pad.number = padNumber_->GetValue().utf8_string();
    pad.name = padName_->GetValue().utf8_string();
    if (!ReadDouble(padX_, &pad.x) || !ReadDouble(padY_, &pad.y) ||
        !ReadDouble(padWidth_, &pad.width) || !ReadDouble(padHeight_, &pad.height) ||
        !ReadDouble(padRotation_, &pad.rotation) || !ReadDouble(padDrill_, &pad.drill) ||
        !ReadDouble(padMask_, &pad.maskExpansion) ||
        !ReadDouble(padClearance_, &pad.clearance))
    {
        ShowError(U8("焊盘属性格式错误"));
        return;
    }
    if (!canvas_->ApplySelectedPad(pad))
    {
        ShowError(U8("UpdatePad 尚未实现"));
    }
}

void MainFrame::OnAbout(wxCommandEvent&)
{
    wxMessageBox(
        U8("EDA 实验一课程骨架\n\n"
           "GUI 已拆出，接口已定义。\n"
           "请在 src/todo 中实现编辑、几何、编解码和元件库逻辑。"),
        U8("关于"),
        wxOK | wxICON_INFORMATION,
        this);
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    if (dirty_)
    {
        const int answer = wxMessageBox(U8("当前封装未保存，是否退出？"),
                                        U8("退出"),
                                        wxYES_NO | wxICON_QUESTION,
                                        this);
        if (answer != wxYES)
        {
            event.Veto();
            return;
        }
    }
    event.Skip();
}

