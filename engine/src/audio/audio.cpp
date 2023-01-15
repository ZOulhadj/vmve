#include "audio.hpp"

#include "logging.hpp"

Audio* CreateAudio()
{
    Logger::Info("Initializing audio");

    auto audio = new Audio();

    // Initialize COM
    HRESULT hr = S_OK;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        Logger::Error("Failed to initialize COM");

        return nullptr;
    }

    hr = XAudio2Create(&audio->ixAudio, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        Logger::Error("Failed to create xaudio2");
        return nullptr;
    }

    hr = audio->ixAudio->CreateMasteringVoice(&audio->masterVoice);
    if (FAILED(hr))
    {
        Logger::Error("Failed to create mastering voice");
        return nullptr;
    }

    return audio;
}

void DestroyAudio(Audio* audio)
{
    Logger::Info("Terminating audio");

    audio->masterVoice->DestroyVoice();
    audio->ixAudio->Release();

    CoUninitialize();
}
