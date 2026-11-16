#include"app.hpp"


wxIMPLEMENT_APP(MyApp); //进入主循环

// 定义的初始化函数, 用于创建主窗口
bool MyApp::OnInit()
{
    wxFrame *frame = new wxFrame(NULL, wxID_ANY, "Hello World");
    frame->Show(true);
    return true;
}