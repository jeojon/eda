#include "MainFrame.h"

#include "FootprintCanvas.h"

#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/scrolwin.h>
#include <wx/treectrl.h>
#include <wx/statline.h>
#include <wx/statusbr.h>
#include <wx/stdpaths.h>

#include <algorithm>
#include <functional>
#include <map>

namespace
{
class LibraryItemData final : public wxTreeItemData
{
public:
    explicit LibraryItemData(const wxString& path)
        : path(path)
    {
    }

    wxString path;
};

wxString U8(const char* text)
{
    return wxString::FromUTF8(text);
}

wxTextCtrl* MakeValueField(wxWindow* parent)
{
    auto* control = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxSize(100, -1));
    control->SetMinSize(wxSize(100, -1));
    return control;
}

wxString NormalizeFileName(const std::string& name)
{
    wxString value = wxString::FromUTF8(name);
    const wxString forbidden = "\\/:*?\"<>|";
    for (wxUniChar ch : forbidden)
    {
        value.Replace(wxString(ch), "_");
    }
    if (value.empty())
    {
        value = "footprint";
    }
    return value;
}

} // namespace

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, U8("EDA 实验一 - 封装编辑器"),
              wxDefaultPosition, wxSize(1360, 860)),
      library_(FootprintLibrary::DefaultDirectory())
{
    SetMinSize(wxSize(1120, 700));
    BuildMenu();
    BuildLayout();

    CreateStatusBar(3);
    const int widths[] = {-1, 230, 280};
    SetStatusWidths(3, widths);
    SetStatusText(U8("就绪"), 0);
    SetStatusText(U8("光标: -- mm"), 1);
    SetStatusText(U8("左键编辑 | 中键平移 | 滚轮缩放 | Delete 删除"), 2);

    Bind(wxEVT_MENU, &MainFrame::OnNew, this, ID_New);
    Bind(wxEVT_MENU, &MainFrame::OnOpen, this, ID_Open);
    Bind(wxEVT_MENU, &MainFrame::OnSave, this, ID_Save);
    Bind(wxEVT_MENU, &MainFrame::OnSaveAs, this, ID_SaveAs);
    Bind(wxEVT_MENU, &MainFrame::OnReloadLibrary, this, ID_ReloadLibrary);
    Bind(wxEVT_MENU, &MainFrame::OnDeleteFootprint, this, ID_DeleteFootprint);
    Bind(wxEVT_MENU, &MainFrame::OnFitView, this, ID_FitView);
    Bind(wxEVT_MENU, &MainFrame::OnZoomIn, this, ID_ZoomIn);
    Bind(wxEVT_MENU, &MainFrame::OnZoomOut, this, ID_ZoomOut);
    Bind(wxEVT_MENU, &MainFrame::OnZoomReset, this, ID_ZoomReset);
    Bind(wxEVT_MENU, &MainFrame::OnToggleGrid, this, ID_ToggleGrid);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

    library_.EnsureSamples();
    LoadLibrary();
    if (libraryTree_->GetRootItem().IsOk())
    {
        const wxTreeItemId root = libraryTree_->GetRootItem();
        wxTreeItemIdValue cookie;
        const wxTreeItemId category = libraryTree_->GetFirstChild(root, cookie);
        if (category.IsOk())
        {
            const wxTreeItemId firstEntry = libraryTree_->GetFirstChild(category, cookie);
            if (firstEntry.IsOk())
            {
                libraryTree_->SelectItem(firstEntry);
            }
            else
            {
                StartNewFootprint();
            }
        }
        else
        {
            StartNewFootprint();
        }
    }
    else
    {
        StartNewFootprint();
    }

    Centre();
}

void MainFrame::BuildMenu()
{
    auto* fileMenu = new wxMenu();
    fileMenu->Append(ID_New, U8("新建封装\tCtrl+N"));
    fileMenu->Append(ID_Open, U8("打开封装...\tCtrl+O"));
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_Save, U8("保存\tCtrl+S"));
    fileMenu->Append(ID_SaveAs, U8("另存为...\tCtrl+Shift+S"));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, U8("退出"));

    auto* editMenu = new wxMenu();
    editMenu->Append(ID_DeleteFootprint, U8("删除所选封装文件"));
    editMenu->Append(ID_ReloadLibrary, U8("刷新元件库\tF5"));

    auto* viewMenu = new wxMenu();
    viewMenu->Append(ID_FitView, U8("适配画布\tF"));
    viewMenu->Append(ID_ZoomIn, U8("放大\tCtrl++"));
    viewMenu->Append(ID_ZoomOut, U8("缩小\tCtrl+-"));
    viewMenu->Append(ID_ZoomReset, U8("实际大小\tCtrl+0"));
    viewMenu->AppendCheckItem(ID_ToggleGrid, U8("显示网格\tG"));
    viewMenu->Check(ID_ToggleGrid, true);

    auto* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, U8("关于实验一"));

    auto* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, U8("文件"));
    menuBar->Append(editMenu, U8("编辑"));
    menuBar->Append(viewMenu, U8("视图"));
    menuBar->Append(helpMenu, U8("帮助"));
    SetMenuBar(menuBar);

    wxAcceleratorEntry entries[4];
    entries[0].Set(wxACCEL_NORMAL, WXK_F5, ID_ReloadLibrary);
    entries[1].Set(wxACCEL_NORMAL, 'F', ID_FitView);
    entries[2].Set(wxACCEL_CTRL, 'N', ID_New);
    entries[3].Set(wxACCEL_CTRL, 'S', ID_Save);
    SetAcceleratorTable(wxAcceleratorTable(4, entries));
}

void MainFrame::BuildLayout()
{
    auto* root = new wxPanel(this);
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* libraryPanel = BuildLibraryPanel(root);
    libraryPanel->SetMinSize(wxSize(260, -1));
    libraryPanel->SetMaxSize(wxSize(300, -1));

    canvas_ = new FootprintCanvas(root, this);

    auto* propertyPanel = BuildPropertyPanel(root);
    propertyPanel->SetMinSize(wxSize(360, -1));
    propertyPanel->SetMaxSize(wxSize(400, -1));

    contentSizer->Add(libraryPanel, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 6);
    contentSizer->Add(canvas_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    contentSizer->Add(propertyPanel, 0, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 6);
    rootSizer->Add(contentSizer, 1, wxEXPAND);
    root->SetSizer(rootSizer);
}

wxPanel* MainFrame::BuildToolPanel(wxWindow* parent)
{
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxGridSizer(2, 6, 6);

    auto addTool = [&](const wxString& label, EditorTool tool)
    {
        auto* button = new wxButton(panel, wxID_ANY, label, wxDefaultPosition, wxSize(-1, 31));
        button->Bind(wxEVT_BUTTON, [this, tool](wxCommandEvent&) { SetTool(tool); });
        sizer->Add(button, 1, wxEXPAND);
    };

    addTool(U8("选择"), EditorTool::Select);
    addTool(U8("SMD焊盘"), EditorTool::SmdPad);
    addTool(U8("THT引脚"), EditorTool::ThroughHolePin);
    addTool(U8("丝印线"), EditorTool::SilkLine);
    addTool(U8("矩形"), EditorTool::SilkRect);
    addTool(U8("圆弧"), EditorTool::SilkArc);
    addTool(U8("多边形"), EditorTool::SilkPolygon);
    addTool(U8("文本"), EditorTool::Text);

    auto* deleteButton = new wxButton(panel, wxID_ANY, U8("删除选中"), wxDefaultPosition, wxSize(-1, 31));
    deleteButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        if (canvas_ != nullptr)
        {
            canvas_->DeleteSelected();
        }
    });
    sizer->Add(deleteButton, 1, wxEXPAND);
    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::BuildLibraryPanel(wxWindow* parent)
{
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* toolsTitle = new wxStaticText(panel, wxID_ANY, U8("绘制工具"));
    toolsTitle->SetFont(toolsTitle->GetFont().Bold());
    sizer->Add(toolsTitle, 0, wxEXPAND | wxBOTTOM, 6);
    sizer->Add(BuildToolPanel(panel), 0, wxEXPAND | wxBOTTOM, 10);
    sizer->Add(new wxStaticLine(panel), 0, wxEXPAND | wxBOTTOM, 10);

    auto* title = new wxStaticText(panel, wxID_ANY, U8("元件封装库"));
    title->SetFont(title->GetFont().Bold());
    sizer->Add(title, 0, wxEXPAND | wxBOTTOM, 8);

    libraryTree_ = new wxTreeCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_SINGLE);
    libraryTree_->Bind(wxEVT_TREE_SEL_CHANGED, &MainFrame::OnLibrarySelected, this);
    sizer->Add(libraryTree_, 1, wxEXPAND | wxBOTTOM, 8);

    auto* buttonSizer = new wxGridSizer(2, 6, 6);
    auto* newButton = new wxButton(panel, wxID_ANY, U8("新建"));
    auto* reloadButton = new wxButton(panel, wxID_ANY, U8("刷新"));
    auto* saveButton = new wxButton(panel, wxID_ANY, U8("保存"));
    auto* saveAsButton = new wxButton(panel, wxID_ANY, U8("另存为"));

    newButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { StartNewFootprint(); });
    reloadButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { LoadLibrary(); });
    saveButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SaveCurrent(); });
    saveAsButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SaveCurrentAs(); });

    buttonSizer->Add(newButton, 1, wxEXPAND);
    buttonSizer->Add(reloadButton, 1, wxEXPAND);
    buttonSizer->Add(saveButton, 1, wxEXPAND);
    buttonSizer->Add(saveAsButton, 1, wxEXPAND);
    sizer->Add(buttonSizer, 0, wxEXPAND | wxBOTTOM, 8);

    auto* libraryHint = new wxStaticText(panel, wxID_ANY,
        U8("封装文件目录:\n") + library_.Directory(), wxDefaultPosition, wxDefaultSize,
        wxST_ELLIPSIZE_MIDDLE);
    libraryHint->SetForegroundColour(wxColour(96, 101, 108));
    libraryHint->SetToolTip(library_.Directory());
    sizer->Add(libraryHint, 0, wxEXPAND);

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
    sizer->Add(title, 0, wxEXPAND | wxBOTTOM, 6);

    footprintName_ = MakeValueField(panel);
    footprintName_->Bind(wxEVT_TEXT, &MainFrame::OnMetadataChanged, this);
    footprintCategory_ = MakeValueField(panel);
    footprintCategory_->Bind(wxEVT_TEXT, &MainFrame::OnMetadataChanged, this);
    footprintDescription_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition,
                                           wxSize(-1, 86), wxTE_MULTILINE);
    footprintDescription_->Bind(wxEVT_TEXT, &MainFrame::OnMetadataChanged, this);
    silkColor_ = MakeValueField(panel);
    silkColor_->Bind(wxEVT_TEXT, &MainFrame::OnMetadataChanged, this);

    auto addMeta = [&](const wxString& label, wxWindow* control, int bottom = 8)
    {
        sizer->Add(new wxStaticText(panel, wxID_ANY, label), 0, wxEXPAND | wxBOTTOM, 2);
        sizer->Add(control, 0, wxEXPAND | wxBOTTOM, bottom);
    };
    addMeta(U8("名称"), footprintName_);
    addMeta(U8("分类"), footprintCategory_);
    addMeta(U8("描述"), footprintDescription_, 10);
    addMeta(U8("丝印颜色"), silkColor_);

    sizer->Add(new wxStaticLine(panel), 0, wxEXPAND | wxBOTTOM, 8);

    selectionLabel_ = new wxStaticText(panel, wxID_ANY, U8("未选中对象"));
    selectionLabel_->SetFont(selectionLabel_->GetFont().Bold());
    sizer->Add(selectionLabel_, 0, wxEXPAND | wxBOTTOM, 8);

    padKind_ = new wxChoice(panel, wxID_ANY);
    padKind_->Append(U8("SMD 焊盘"));
    padKind_->Append(U8("THT 引脚"));

    padNumber_ = MakeValueField(panel);
    padName_ = MakeValueField(panel);
    padX_ = MakeValueField(panel);
    padY_ = MakeValueField(panel);
    padWidth_ = MakeValueField(panel);
    padHeight_ = MakeValueField(panel);
    padRotation_ = MakeValueField(panel);
    padDrill_ = MakeValueField(panel);
    padMask_ = MakeValueField(panel);
    padClearance_ = MakeValueField(panel);

    auto* padGrid = new wxFlexGridSizer(2, 5, 5);
    padGrid->AddGrowableCol(1);

    auto addRow = [&](const wxString& label, wxWindow* control)
    {
        padGrid->Add(new wxStaticText(panel, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
        padGrid->Add(control, 1, wxEXPAND);
    };

    addRow(U8("类型"), padKind_);
    addRow(U8("编号"), padNumber_);
    addRow(U8("名称"), padName_);
    addRow(U8("X / mm"), padX_);
    addRow(U8("Y / mm"), padY_);
    addRow(U8("宽 / mm"), padWidth_);
    addRow(U8("高 / mm"), padHeight_);
    addRow(U8("旋转 / deg"), padRotation_);
    addRow(U8("钻孔 / mm"), padDrill_);
    addRow(U8("阻焊扩展 / mm"), padMask_);
    addRow(U8("安全间距 / mm"), padClearance_);
    sizer->Add(padGrid, 0, wxEXPAND | wxBOTTOM, 8);

    auto* applyButton = new wxButton(panel, wxID_ANY, U8("应用焊盘属性"));
    applyButton->Bind(wxEVT_BUTTON, &MainFrame::OnApplyProperties, this);
    sizer->Add(applyButton, 0, wxEXPAND | wxBOTTOM, 6);

    auto* fitButton = new wxButton(panel, wxID_ANY, U8("适配画布"));
    fitButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { canvas_->FitView(); canvas_->Refresh(); });
    sizer->Add(fitButton, 0, wxEXPAND | wxBOTTOM, 8);

    auto* hint = new wxStaticText(panel, wxID_ANY,
        U8("图元说明\n"
           "SMD 焊盘: 贴片铜箔\n"
           "THT 引脚: 焊盘 + 钻孔\n"
           "线段/矩形/圆弧/多边形: 丝印\n"
           "多边形的最后一个点可右键结束"));
    hint->SetForegroundColour(wxColour(96, 101, 108));
    sizer->Add(hint, 0, wxEXPAND);
    sizer->AddStretchSpacer();
    panel->SetSizer(sizer);
    panel->FitInside();
    return panel;
}

void MainFrame::LoadLibrary()
{
    wxString selectedPath;
    if (libraryTree_ != nullptr)
    {
        const wxTreeItemId selected = libraryTree_->GetSelection();
        if (selected.IsOk())
        {
            if (auto* data = dynamic_cast<LibraryItemData*>(libraryTree_->GetItemData(selected)))
            {
                selectedPath = data->path;
            }
        }
        libraryTree_->DeleteAllItems();
    }

    wxString error;
    if (!library_.Reload(&error))
    {
        wxMessageBox(U8("刷新元件库失败:\n") + error, U8("实验一"), wxOK | wxICON_ERROR, this);
        return;
    }
    const wxTreeItemId root = libraryTree_->AddRoot(U8("封装库"));
    std::map<std::string, wxTreeItemId> categories;
    for (const FootprintLibraryEntry& entry : library_.Entries())
    {
        wxString categoryName = wxString::FromUTF8(entry.category);
        if (categoryName.empty())
        {
            categoryName = U8("未分类");
        }
        const std::string key = entry.category.empty() ? "Uncategorized" : entry.category;
        auto found = categories.find(key);
        wxTreeItemId categoryItem;
        if (found == categories.end())
        {
            categoryItem = libraryTree_->AppendItem(root, categoryName);
            categories.emplace(key, categoryItem);
        }
        else
        {
            categoryItem = found->second;
        }

        const wxTreeItemId item = libraryTree_->AppendItem(
            categoryItem,
            wxString::FromUTF8(entry.name));
        libraryTree_->SetItemData(item, new LibraryItemData(entry.path));
    }
    libraryTree_->ExpandAll();
    if (!selectedPath.empty())
    {
        SelectLibraryItem(selectedPath);
    }

    SetStatusText(
        wxString::Format(U8("元件库: %zu 个封装 | "), library_.Entries().size()) +
            library_.Directory(),
        0);
}

void MainFrame::SelectLibraryItem(const wxString& path)
{
    if (libraryTree_ == nullptr)
    {
        return;
    }

    std::function<bool(const wxTreeItemId&)> visit = [&](const wxTreeItemId& item) -> bool
    {
        if (!item.IsOk())
        {
            return false;
        }
        if (auto* data = dynamic_cast<LibraryItemData*>(libraryTree_->GetItemData(item)))
        {
            if (data->path == path)
            {
                libraryTree_->SelectItem(item);
                return true;
            }
        }
        wxTreeItemIdValue cookie;
        wxTreeItemId child = libraryTree_->GetFirstChild(item, cookie);
        while (child.IsOk())
        {
            if (visit(child))
            {
                return true;
            }
            child = libraryTree_->GetNextChild(item, cookie);
        }
        return false;
    };
    visit(libraryTree_->GetRootItem());
}

bool MainFrame::LoadFootprint(const wxString& path)
{
    Footprint footprint;
    wxString error;
    if (!library_.Read(path, footprint, &error))
    {
        wxMessageBox(U8("读取失败:\n") + error, U8("实验一"), wxOK | wxICON_ERROR, this);
        return false;
    }

    currentFootprint_ = footprint;
    currentPath_ = path;
    canvas_->SetFootprint(&currentFootprint_);
    RefreshProperties();
    dirty_ = false;
    UpdateTitle();
    SetStatusText(U8("已加载: ") + path, 0);
    return true;
}

void MainFrame::StartNewFootprint()
{
    if (!ConfirmDiscardChanges())
    {
        return;
    }

    currentFootprint_.Clear();
    currentFootprint_.name = "Untitled";
    currentFootprint_.category = "Uncategorized";
    currentFootprint_.description = "New footprint";
    currentPath_.clear();
    canvas_->SetFootprint(&currentFootprint_);
    RefreshProperties();
    SetTool(EditorTool::SmdPad);
    dirty_ = true;
    UpdateTitle();
    SetStatusText(U8("已新建空白封装"), 0);
}

bool MainFrame::SaveCurrent()
{
    if (currentPath_.empty())
    {
        return SaveCurrentAs();
    }

    SyncMetadataFromControls();
    wxString error;
    if (!library_.Write(currentPath_, currentFootprint_, &error))
    {
        wxMessageBox(U8("保存失败:\n") + error, U8("实验一"), wxOK | wxICON_ERROR, this);
        return false;
    }

    dirty_ = false;
    UpdateTitle();
    LoadLibrary();
    SelectLibraryItem(currentPath_);
    SetStatusText(U8("已保存: ") + currentPath_, 0);
    return true;
}

bool MainFrame::SaveCurrentAs()
{
    SyncMetadataFromControls();
    const wxString suggested = NormalizeFileName(currentFootprint_.name) + ".fp";
    wxFileDialog dialog(this, U8("保存封装"), library_.Directory(), suggested,
                        U8("EDA Lab1 封装 (*.fp)|*.fp|所有文件 (*.*)|*.*"),
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK)
    {
        return false;
    }

    const wxString path = dialog.GetPath();
    wxString error;
    if (!library_.Write(path, currentFootprint_, &error))
    {
        wxMessageBox(U8("保存失败:\n") + error, U8("实验一"), wxOK | wxICON_ERROR, this);
        return false;
    }

    currentPath_ = path;
    dirty_ = false;
    UpdateTitle();
    LoadLibrary();
    SelectLibraryItem(path);
    SetStatusText(U8("已保存: ") + path, 0);
    return true;
}

bool MainFrame::ConfirmDiscardChanges()
{
    if (!dirty_)
    {
        return true;
    }

    const int result = wxMessageBox(
        U8("当前封装有未保存修改，是否先保存？"),
        U8("实验一"),
        wxYES_NO | wxCANCEL | wxICON_QUESTION,
        this);

    if (result == wxCANCEL)
    {
        return false;
    }
    if (result == wxYES)
    {
        return SaveCurrent();
    }
    return true;
}

void MainFrame::SyncMetadataFromControls()
{
    if (updatingControls_)
    {
        return;
    }
    currentFootprint_.name = footprintName_->GetValue().utf8_string();
    currentFootprint_.category = footprintCategory_->GetValue().utf8_string();
    currentFootprint_.description = footprintDescription_->GetValue().utf8_string();
    currentFootprint_.silkColor = silkColor_->GetValue().utf8_string();
    if (currentFootprint_.name.empty())
    {
        currentFootprint_.name = "Untitled";
    }
    if (currentFootprint_.category.empty())
    {
        currentFootprint_.category = "Uncategorized";
    }
}

void MainFrame::RefreshProperties()
{
    updatingControls_ = true;
    footprintName_->ChangeValue(wxString::FromUTF8(currentFootprint_.name));
    footprintCategory_->ChangeValue(wxString::FromUTF8(currentFootprint_.category));
    footprintDescription_->ChangeValue(wxString::FromUTF8(currentFootprint_.description));
    wxString silkColor = wxString::FromUTF8(currentFootprint_.silkColor);
    if (!wxColour(silkColor).IsOk())
    {
        silkColor = "#1A6984";
    }
    silkColor_->ChangeValue(silkColor);

    const PadItem* pad = canvas_ != nullptr ? canvas_->GetSelectedPad() : nullptr;
    if (canvas_ != nullptr)
    {
        selectionLabel_->SetLabel(canvas_->GetSelectionDescription());
    }

    if (pad == nullptr)
    {
        SelectAllPadFields(false);
        padKind_->SetSelection(wxNOT_FOUND);
        padNumber_->ChangeValue("");
        padName_->ChangeValue("");
        padX_->ChangeValue("");
        padY_->ChangeValue("");
        padWidth_->ChangeValue("");
        padHeight_->ChangeValue("");
        padRotation_->ChangeValue("");
        padDrill_->ChangeValue("");
        padMask_->ChangeValue("");
        padClearance_->ChangeValue("");
    }
    else
    {
        SelectAllPadFields(true);
        padKind_->SetSelection(pad->kind == PadKind::Smd ? 0 : 1);
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

void MainFrame::SetDirty(bool dirty)
{
    dirty_ = dirty;
    UpdateTitle();
}

void MainFrame::UpdateTitle()
{
    wxString title = U8("EDA 实验一 - 封装编辑器");
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
    if (canvas_ != nullptr)
    {
        canvas_->SetTool(tool);
    }
}

void MainFrame::SelectAllPadFields(bool enabled)
{
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
}

bool MainFrame::ReadDoubleControl(wxTextCtrl* control, double* value) const
{
    return control->GetValue().ToDouble(value);
}

void MainFrame::OnCanvasSelectionChanged()
{
    RefreshProperties();
}

void MainFrame::OnCanvasDocumentChanged()
{
    SetDirty(true);
}

void MainFrame::UpdateCursorPosition(double x, double y)
{
    SetStatusText(wxString::Format(U8("光标: %.3f, %.3f mm"), x, y), 1);
}

void MainFrame::OnNew(wxCommandEvent&)
{
    StartNewFootprint();
}

void MainFrame::OnOpen(wxCommandEvent&)
{
    if (!ConfirmDiscardChanges())
    {
        return;
    }

    wxFileDialog dialog(this, U8("打开封装"), library_.Directory(), "",
                        U8("EDA Lab1 封装 (*.fp)|*.fp|所有文件 (*.*)|*.*"),
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK)
    {
        return;
    }
    LoadFootprint(dialog.GetPath());
}

void MainFrame::OnSave(wxCommandEvent&)
{
    SaveCurrent();
}

void MainFrame::OnSaveAs(wxCommandEvent&)
{
    SaveCurrentAs();
}

void MainFrame::OnExit(wxCommandEvent&)
{
    Close();
}

void MainFrame::OnReloadLibrary(wxCommandEvent&)
{
    LoadLibrary();
}

void MainFrame::OnDeleteFootprint(wxCommandEvent&)
{
    const wxTreeItemId selection = libraryTree_->GetSelection();
    if (!selection.IsOk())
    {
        return;
    }

    auto* itemData = dynamic_cast<LibraryItemData*>(libraryTree_->GetItemData(selection));
    if (itemData == nullptr)
    {
        return;
    }
    const wxString path = itemData->path;
    if (wxMessageBox(U8("确定删除封装文件？\n") + path,
                     U8("删除封装"),
                     wxYES_NO | wxNO_DEFAULT | wxICON_WARNING,
                     this) != wxYES)
    {
        return;
    }

    wxString error;
    if (!library_.Delete(path, &error))
    {
        wxMessageBox(U8("删除失败:\n") + error, U8("实验一"), wxOK | wxICON_ERROR, this);
        return;
    }

    if (path == currentPath_)
    {
        currentPath_.clear();
        dirty_ = true;
    }
    LoadLibrary();
}

void MainFrame::OnLibrarySelected(wxTreeEvent&)
{
    const wxTreeItemId selection = libraryTree_->GetSelection();
    if (!selection.IsOk())
    {
        return;
    }
    auto* itemData = dynamic_cast<LibraryItemData*>(libraryTree_->GetItemData(selection));
    if (itemData == nullptr)
    {
        return;
    }
    if (!ConfirmDiscardChanges())
    {
        return;
    }
    LoadFootprint(itemData->path);
}

void MainFrame::OnApplyProperties(wxCommandEvent&)
{
    const PadItem* selected = canvas_->GetSelectedPad();
    if (selected == nullptr)
    {
        return;
    }

    PadItem updated = *selected;
    updated.kind = padKind_->GetSelection() == 1 ? PadKind::ThroughHole : PadKind::Smd;
    updated.number = padNumber_->GetValue().utf8_string();
    updated.name = padName_->GetValue().utf8_string();

    double value = 0.0;
    if (!ReadDoubleControl(padX_, &value)) { wxMessageBox(U8("X 坐标格式错误。")); return; }
    updated.x = value;
    if (!ReadDoubleControl(padY_, &value)) { wxMessageBox(U8("Y 坐标格式错误。")); return; }
    updated.y = value;
    if (!ReadDoubleControl(padWidth_, &value) || value <= 0.0) { wxMessageBox(U8("焊盘宽度无效。")); return; }
    updated.width = value;
    if (!ReadDoubleControl(padHeight_, &value) || value <= 0.0) { wxMessageBox(U8("焊盘高度无效。")); return; }
    updated.height = value;
    if (!ReadDoubleControl(padRotation_, &value)) { wxMessageBox(U8("旋转角格式错误。")); return; }
    updated.rotation = value;
    if (!ReadDoubleControl(padDrill_, &value) || value < 0.0) { wxMessageBox(U8("钻孔直径无效。")); return; }
    updated.drill = value;
    if (!ReadDoubleControl(padMask_, &value) || value < 0.0) { wxMessageBox(U8("阻焊扩展无效。")); return; }
    updated.maskExpansion = value;
    if (!ReadDoubleControl(padClearance_, &value) || value < 0.0) { wxMessageBox(U8("安全间距无效。")); return; }
    updated.clearance = value;

    if (updated.kind == PadKind::ThroughHole && updated.shape == PadShape::RoundedRect)
    {
        updated.shape = PadShape::Circle;
    }
    if (updated.kind == PadKind::Smd && updated.shape == PadShape::Circle)
    {
        updated.shape = PadShape::RoundedRect;
    }

    canvas_->ApplySelectedPad(updated);
}

void MainFrame::OnFitView(wxCommandEvent&)
{
    if (canvas_ != nullptr)
    {
        canvas_->FitView();
        canvas_->Refresh();
    }
}

void MainFrame::OnZoomIn(wxCommandEvent&)
{
    canvas_->ZoomIn();
}

void MainFrame::OnZoomOut(wxCommandEvent&)
{
    canvas_->ZoomOut();
}

void MainFrame::OnZoomReset(wxCommandEvent&)
{
    canvas_->ZoomReset();
}

void MainFrame::OnToggleGrid(wxCommandEvent& event)
{
    canvas_->ToggleGrid();
    if (wxMenuBar* menuBar = GetMenuBar())
    {
        if (wxMenuItem* item = menuBar->FindItem(ID_ToggleGrid))
        {
            item->Check(event.IsChecked());
        }
    }
}

void MainFrame::OnMetadataChanged(wxCommandEvent&)
{
    if (updatingControls_)
    {
        return;
    }
    SyncMetadataFromControls();
    SetDirty(true);
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    if (ConfirmDiscardChanges())
    {
        event.Skip();
    }
    else
    {
        event.Veto();
    }
}

void MainFrame::OnAbout(wxCommandEvent&)
{
    wxMessageBox(
        U8("EDA 软件设计 I - 实验一\n"
           "PCB 元件封装设计与元件库管理\n\n"
           "wxWidgets 3.2.2 / C++17\n"
           "支持 SMD 焊盘、THT 引脚、丝印图元、自定义封装文件和元件库。"),
        U8("关于实验一"),
        wxOK | wxICON_INFORMATION,
        this);
}

