#include "cApp.h"

wxIMPLEMENT_APP(cApp);

cApp::cApp()
{
}

cApp::~cApp()
{
}

bool cApp::OnInit()
{
    m_ChatWindow = new ChatWindow("ChatGPT");
    m_ChatWindow->Show(true);
    return true;
}