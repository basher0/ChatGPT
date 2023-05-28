#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <sstream>
#include <vector>
#include <thread>

#include <wx/wx.h>
#include <curl/curl.h>

#include "json.hpp"
using json = nlohmann::json;

struct Memory { std::string who; std::string data; };

// Callback function to handle the response
inline size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}


class ChatGPT
{
public:
    ChatGPT(const std::string& apiKey, const std::string& model)
        : m_ApiKey(apiKey), m_Model(model)
    {
        m_Url = { "https://api.openai.com/v1/chat/completions" };
    };
    ~ChatGPT() = default;
public:
    wxString GetResponse(const wxString& prompt)
    {
        // "Memory" for the AI with the all conversation context.
        wxString promptContext = "This is our conversation context: ";
        for(const auto& data : m_Memory)
        {
            promptContext += data.data;
        }
        promptContext += "This is what I am asking now: ";
        promptContext += prompt;

        std::string content{};

        // Construct the JSON payload
        json payload;
        payload["model"] = m_Model;
        payload["messages"] = {
            {
                {"role", "system"},
                {"content", m_Persona}
            },
            {
                {"role", "user"},
                {"content", promptContext.utf8_string()}
            }
        };
        std::string jsonPayload = payload.dump();

        // Initialize Curl
        curl_global_init(CURL_GLOBAL_DEFAULT);
        CURL* curl = curl_easy_init();
        if (curl)
        {
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("Authorization: Bearer " + m_ApiKey).c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");

            // Set Curl options
            curl_easy_setopt(curl, CURLOPT_URL, m_Url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

            curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");

            // Response container
            std::string response;

            // Set the response callback function and container
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            // Perform the request
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                wxMessageBox("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)), "cURL error", wxOK | wxICON_ERROR);
            }
            else
            {
                // Parse the response
                json jsonResponse = json::parse(response);

                // Save the message content
                content = jsonResponse["choices"][0]["message"]["content"];
            }

            // Cleanup
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        // Cleanup Curl
        curl_global_cleanup();

        // Add response to the AI memory
        m_Memory.push_back({ "Human", "I said: '" + prompt.ToStdString() + "'." });
        m_Memory.push_back({ "AI", "You said: '" + content + "'." });

        return wxString(content.c_str(), wxConvUTF8);
    }
    void SetPersona(const std::string& persona)
    {
        m_Persona = persona;
    }
    // Clear Memory
    void ClearMemory()
    {
        wxMessageBox(m_Memory.size() > 0 ? "Memory Cleared." : "No memory to clear.", "Memory", m_Memory.size() > 0 ? wxICON_INFORMATION : wxICON_WARNING);
        
        if (m_Memory.size() > 0)
            m_Memory.clear();
    }

private:
    std::string m_ApiKey;
    std::string m_Model;
    std::string m_Persona;
    std::string m_Url;
    std::vector<Memory> m_Memory;

};