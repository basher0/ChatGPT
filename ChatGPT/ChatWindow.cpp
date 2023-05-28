#include "ChatWindow.h"

wxBEGIN_EVENT_TABLE(ChatWindow, wxFrame)
// EVENTS HANDLE MACRO
wxEND_EVENT_TABLE();


ChatWindow::ChatWindow(const wxString& title) : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1024, 768))
{
    // Get Exe path
    wxFileName path(wxStandardPaths::Get().GetExecutablePath());
    wxString appPath(path.GetPath());

    // Create a new icon bundle
    wxIconBundle iconBundle;

    // Load the icon file
    wxIcon icon(appPath + "\\ai.ico", wxBITMAP_TYPE_ICO);

    // Add the icon to the bundle
    iconBundle.AddIcon(icon);

    // Set the icon bundle as the application icon
    wxFrame::SetIcons(iconBundle);

    // Get Ini config
    IniParser ini;
    ini.parseFile(appPath.ToStdString() + "\\api.ini");

    openai.apikey         = ini["OpenAI.apikey"];
    openai.model          = ini["OpenAI.model"];

    elevenlabs.apikey     = ini["ElevenLabs.apikey"];
    elevenlabs.stability  = ini["ElevenLabs.stability"];
    elevenlabs.similarity = ini["ElevenLabs.similarity"];

    // Create ChatGPT request class
    m_pRequestGPT = new ChatGPT(openai.apikey, openai.model);

    // Set chatgpt system persona
    m_pRequestGPT->SetPersona(R"( You are a very helpful assistent. )");

    // Create the input text control
    wxInputTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    wxInputTextCtrl->SetHint("Type your message here...");

    // Create the output text control
    wxOutputTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxBORDER_SIMPLE);

    // Create the send button
    wxSendButton = new wxButton(this, wxID_ANY, "Send");

    // Create the speak button
    wxSpeakButton = new wxButton(this, wxID_ANY, "Speak");
    wxSpeakButton->Disable();

    // Create the clear memory button
    wxClearMemory = new wxButton(this, wxID_ANY, "Clear Memory");

    wxVoicesComboBox = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);

    // Bind an event handler for selection change
    wxVoicesComboBox->Bind(wxEVT_COMBOBOX, &ChatWindow::OnComboBoxSelection, this);

    // Load voices and append to the voices ComboBox
    std::thread load([&] 
    {
        // Disable current frame
        this->Disable();

        json voices = ElevenLabsGetVoices(elevenlabs.apikey);

        m_sCurVoiceId = voices["voices"][0]["voice_id"].get<json::string_t>();
        
        // Loop through the "voices" array
        for (const auto& voice : voices["voices"])
        {
            wxString voiceId = voice["voice_id"].get<json::string_t>();
            wxString voiceName = voice["name"].get<json::string_t>();

            // Save StringClientData for the voice id
            wxStringClientData* data = new wxStringClientData(voiceId);
            
            // Append voice name and data cointaining the voice id
            wxVoicesComboBox->Append(voiceName, data);
        }
        wxVoicesComboBox->SetSelection(0);

        // Re-enable current frame again
        this->Enable();
    });

    // Run "load thread" asynchronously
    load.detach();
   
    // Set up the sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->Add(wxOutputTextCtrl, 1, wxEXPAND | wxALL, 10);
    mainSizer->Add(wxInputTextCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    mainSizer->Add(wxVoicesComboBox, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);
    mainSizer->Add(wxSendButton, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);
    mainSizer->Add(wxSpeakButton, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);
    mainSizer->Add(wxClearMemory, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);

    wxInputTextCtrl->SetMinSize(wxSize(400, 30));   // Set minimum size for inputTextCtrl
    wxOutputTextCtrl->SetMinSize(wxSize(1024, 768));  // Set minimum size for outputTextCtrl
    wxSendButton->SetMinSize(wxSize(80, 30));       // Set minimum size for sendButton
    wxSpeakButton->SetMinSize(wxSize(80, 30));       // Set minimum size for sendButton
    wxClearMemory->SetMinSize(wxSize(80, 30));       // Set minimum size for clearMemory
    wxVoicesComboBox->SetMinSize(wxSize(80, 30));       // Set minimum size for clearMemory

    SetSizerAndFit(mainSizer);

    // Change font
    wxFont font(wxFontInfo(14).Family(wxFONTFAMILY_DEFAULT).FaceName("Arial Unicode MS"));
    wxOutputTextCtrl->SetFont(font);

    // Connect event handlers
    wxInputTextCtrl->Bind(wxEVT_TEXT_ENTER, &ChatWindow::OnInputEnter, this);
    wxSendButton->Bind(wxEVT_BUTTON, &ChatWindow::OnSendButtonClicked, this);
    wxSpeakButton->Bind(wxEVT_BUTTON, &ChatWindow::OnSpeakButtonClicked, this);
    wxClearMemory->Bind(wxEVT_BUTTON, &ChatWindow::OnClearMemoryButton, this);
   
}

void ChatWindow::OnComboBoxSelection(wxCommandEvent& event)
{
    // Get current selected index from ComboBox
    int selectedIndex = wxVoicesComboBox->GetSelection();

    // Get the client object data appended on creation
    wxClientData* data = wxVoicesComboBox->GetClientObject(selectedIndex);
    if (data)
    {
        // Get voice id from StringClientData
        wxString voiceId = static_cast<wxStringClientData*>(data)->GetData();

        // Set the current voice ID to the ID that we just obtained so that 
        // we can use it throughout the program.
        m_sCurVoiceId = voiceId;
    }

    event.Skip();
}

void ChatWindow::OnInputEnter(wxCommandEvent& event)
{
    SendMessage();
}
void ChatWindow::OnSendButtonClicked(wxCommandEvent& event)
{
    SendMessage();
}
void ChatWindow::OnSpeakButtonClicked(wxCommandEvent& event)
{
    // Run ElevenLabs tts thread
    std::thread speak([this] 
    {
        wxSpeakButton->Disable();

        ElevenLabsTTS(elevenlabs.apikey,
            m_sLastResponse.utf8_string(),
            m_sCurVoiceId.ToStdString(),
            elevenlabs.stability,
            elevenlabs.similarity);

        wxSpeakButton->Enable();
    });
    
    // Detach the thread to run independently
    speak.detach();
}
void ChatWindow::OnClearMemoryButton(wxCommandEvent& event)
{
    m_pRequestGPT->ClearMemory();
}
void ChatWindow::SendMessage() 
{
    // Prompt from the Input
    wxString message = wxInputTextCtrl->GetValue();
   
    if (message.empty())
        return;

    wxInputTextCtrl->Clear();

    // Perform the cURL request asynchronously
    std::thread requestThread([this, message]() 
    {
        // Disable input controls temporarily
        wxInputTextCtrl->Disable();
        wxSendButton->Disable();
        wxSpeakButton->Disable();
        wxVoicesComboBox->Disable();

        // Append your prompt to the output box
        wxOutputTextCtrl->AppendText(message + '\n');

        // Get chatgpt response for the prompt
        wxString response = m_pRequestGPT->GetResponse(message.utf8_str());

        // Append response to output box
        wxOutputTextCtrl->AppendText("AI: " + response + '\n');
        
        // Save response for the tts
        m_sLastResponse = response;

        // Enable speak button to use tts
        wxSpeakButton->Enable();

        // Update UI with response
        wxCommandEvent responseEvent(wxEVT_COMMAND_TEXT_UPDATED, wxID_ANY);
        responseEvent.SetString("Received response from ChatGPT API.\n");

        // Re-enable input controls again
        wxInputTextCtrl->Enable();
        wxSendButton->Enable();
        wxSpeakButton->Enable();
        wxVoicesComboBox->Enable();

        wxQueueEvent(this, responseEvent.Clone());
    });

    // Detach the thread to run independently
    requestThread.detach();
}
