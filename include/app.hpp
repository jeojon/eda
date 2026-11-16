#include <iostream>
#include <wx/wx.h>

// 定义应用程序类
class MyApp : public wxApp
{
    public:
        virtual bool OnInit(); //初始化函数
};

wxDECLARE_APP(MyApp);