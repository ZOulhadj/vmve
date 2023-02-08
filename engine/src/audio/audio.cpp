#include "audio.hpp"

#include "logging.hpp"

bool create_windows_audio(Audio*& out_audio)
{
    print_log("Initializing audio\n");

    out_audio = new Audio();

    // Initialize COM
    HRESULT hr = S_OK;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        print_log("Failed to initialize audio COM\n");

        return false;
    }

    hr = XAudio2Create(&out_audio->ix_audio, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        print_log("Failed to create xaudio2\n");

        return false;
    }

    hr = out_audio->ix_audio->CreateMasteringVoice(&out_audio->master_voice);
    if (FAILED(hr)) {
        print_log("Failed to create mastering voice\n");

        return false;
    }

    return true;
}

void destroy_windows_audio(Audio* audio)
{
    if (!audio)
        return;

    print_log("Terminating audio\n");

    audio->master_voice->DestroyVoice();
    audio->ix_audio->Release();

    CoUninitialize();

    delete audio;
}
