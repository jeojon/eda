#include <wx/wx.h>


class MainFrame : public wxFrame
{
    public:
        MainFrame(const wxString& title); // 构造函数,初始化主窗口


    void OnExit(wxCommandEvent& event); // 退出事件处理函数
    void OnAbout(wxCommandEvent& event); // 关于事件处理函数
    
};