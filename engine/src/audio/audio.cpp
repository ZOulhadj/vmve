#include "audio.hpp"

#include "logging.hpp"

bool create_windows_audio(Audio*& out_audio)
{
    logger::info("Initializing audio");

    out_audio = new Audio();

    // Initialize COM
    HRESULT hr = S_OK;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        logger::error("Failed to initialize COM");

        return false;
    }

    hr = XAudio2Create(&out_audio->ix_audio, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        logger::error("Failed to create xaudio2");
        return false;
    }

    hr = out_audio->ix_audio->CreateMasteringVoice(&out_audio->master_voice);
    if (FAILED(hr)) {
        logger::error("Failed to create mastering voice");
        return false;
    }

    return true;
}

void destroy_windows_audio(Audio* audio)
{
    if (!audio)
        return;

    logger::info("Terminating audio");

    audio->master_voice->DestroyVoice();
    audio->ix_audio->Release();

    CoUninitialize();

    delete audio;
}
