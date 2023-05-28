#pragma once
#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/clntdata.h>

#include "ChatGPT.hpp"
#include "ElevenLabs.hpp"
#include "IniParser.hpp"

struct OpenAI
{
    std::string apikey{};
    std::string model{};
};
struct ElevenLabs
{
    std::string apikey{};
    double stability{};
    double similarity{};
};

class ChatWindow : public wxFrame
{
public:
    ChatWindow(const wxString& title);

public:
    void OnInputEnter(wxCommandEvent& event);
    void OnSendButtonClicked(wxCommandEvent& event);
    void OnSpeakButtonClicked(wxCommandEvent& event);
    void OnClearMemoryButton(wxCommandEvent& event);
    void OnComboBoxSelection(wxCommandEvent& event);

    void SendMessage();
private:
    wxTextCtrl* wxInputTextCtrl;
    wxTextCtrl* wxOutputTextCtrl;
    wxButton*   wxSendButton;
    wxButton*   wxSpeakButton;
    wxButton*   wxClearMemory;
    wxComboBox* wxVoicesComboBox;

    ChatGPT* m_pRequestGPT;
    wxString m_sCurVoiceId;
    wxString m_sLastResponse;

    OpenAI openai;
    ElevenLabs elevenlabs;

    wxDECLARE_EVENT_TABLE();
};


//"Hello, World!", "Hh1VYuEOh2PpX6oG8nNF"
inline void ElevenLabsTTS(const std::string& api,
    const std::string& text,
    const std::string& voiceId,
    double stability = 0.1,
    double similarity = 1.0)
{
    auto mp3dir = ElevenLabsApiSpeak(api, text, voiceId, 
        stability, similarity);

    playMP3(mp3dir.c_str());

    wxRemoveFile(mp3dir);
}
