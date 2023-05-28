#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <curl/curl.h>

#include <SDL.h>
#include <SDL_mixer.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "json.hpp"

using json = nlohmann::json;

// Callback function to handle the response and directly write file
inline size_t FileWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    std::ofstream* outputFile = static_cast<std::ofstream*>(userp);
    outputFile->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Callback function to handle the raw response
inline size_t NormalWriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
// Get voices from ElevenLabs
inline json ElevenLabsGetVoices(const std::string& api)
{
    std::string url = "https://api.elevenlabs.io/v1/voices";
    std::string response;

    // Initialize cURL
    CURL* curl = curl_easy_init();
    if (curl)
    {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Set the request headers
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "accept: application/json");
        headers = curl_slist_append(headers, ("xi-api-key: " + api).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the write callback function to receive the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NormalWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "cURL request failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    else
    {
        std::cerr << "cURL initialization failed." << std::endl;
    }

    // Parse the response string as json
    json jsonResponse;
    try
    {
        jsonResponse = json::parse(response);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to parse API response as JSON: " << e.what() << std::endl;
    }

    return jsonResponse;
}

// TTS itself using cURL and writing direct to a file output
inline std::string ElevenLabsApiSpeak(const std::string& api, 
    const std::string& text, 
    const std::string& voiceId,
    double stability,
    double similarity)
{
    // Set up the cURL handle
    CURL* curl = curl_easy_init();
    if (!curl) 
    {
        std::cerr << "Failed to initialize cURL" << std::endl;
    }

    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    wxString appPath(f.GetPath());

    // Set the cURL options
    std::string url = "https://api.elevenlabs.io/v1/text-to-speech/" + voiceId + "/stream";
    std::ofstream outputFile(wxString(appPath + "\\output.mp3").ToStdString(), std::ios::binary);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outputFile);
   
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: audio/mpeg");
    headers = curl_slist_append(headers, ("xi-api-key: " + api).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);


    // Prepare the JSON payload
    json payload;
    payload["text"] = text;
    payload["model_id"] = "eleven_multilingual_v1";
    payload["voice_settings"]["stability"] = stability;
    payload["voice_settings"]["similarity_boost"] = similarity;

    // Get the payload string
    const std::string jsonString = payload.dump();

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());
    curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");

    // Perform the cURL request
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) 
    {
        std::cerr << "cURL request failed: " << curl_easy_strerror(res) << std::endl;
    }
    outputFile.close();

    return wxString(appPath + "\\output.mp3").ToStdString();
}

// Play a mp3 file using SDL2 to play the TTS mp3 output
inline void playMP3(const char* filePath)
{
    // Initialize SDL and SDL_mixer
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        // Handle initialization error
        return;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        // Handle audio initialization error
        return;
    }

    // Load and play the MP3 file
    Mix_Music* music = Mix_LoadMUS(filePath);
    if (music == nullptr) {
        // Handle loading error
        return;
    }

    // Play
    Mix_PlayMusic(music, 0);

    // Wait for the audio to finish playing (optional)
    while (Mix_PlayingMusic())
    {
        SDL_Delay(100); // Adjust the delay time as needed
    }

    // Clean up and quit
    Mix_FreeMusic(music);
    Mix_CloseAudio();
    SDL_Quit();
}