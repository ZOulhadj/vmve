#include "audio.hpp"

#include "logging.hpp"

Audio* create_windows_audio() {
    Logger::info("Initializing audio");

    auto audio = new Audio();

    // Initialize COM
    HRESULT hr = S_OK;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        Logger::error("Failed to initialize COM");

        return nullptr;
    }

    hr = XAudio2Create(&audio->ix_audio, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        Logger::error("Failed to create xaudio2");
        return nullptr;
    }

    hr = audio->ix_audio->CreateMasteringVoice(&audio->master_voice);
    if (FAILED(hr)) {
        Logger::error("Failed to create mastering voice");
        return nullptr;
    }

    return audio;
}

void destroy_windows_audio(Audio* audio) {
    if (!audio)
        return;

    Logger::info("Terminating audio");

    audio->master_voice->DestroyVoice();
    audio->ix_audio->Release();

    CoUninitialize();
}
