#include "MainFrame.h"

#include <wx/wx.h>

class EdaLab1App final : public wxApp
{
public:
    bool OnInit() override
    {
        auto* frame = new MainFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(EdaLab1App);
