#include "win32_audio.h"

#include "logging.h"

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

static HRESULT find_chunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType)
        {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize) return S_FALSE;

    }

    return S_OK;
}

static HRESULT read_chunk_data(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD dwRead;
    if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}



main_audio* create_audio()
{
    print_log("Initializing audio\n");

    main_audio* audio = new main_audio();

    // Initialize COM
    HRESULT hr = S_OK;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        print_error("Failed to initialize audio COM\n");

        return nullptr;
    }

    hr = XAudio2Create(&audio->ix_audio, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        print_error("Failed to create xaudio2\n");

        return nullptr;
    }

    hr = audio->ix_audio->CreateMasteringVoice(&audio->master_voice);
    if (FAILED(hr)) {
        print_error("Failed to create mastering voice\n");

        return nullptr;
    }

    return audio;
}

void destroy_windows_audio(main_audio* audio)
{
    if (!audio)
        return;

    print_log("Terminating audio\n");

    audio->master_voice->DestroyVoice();
    audio->ix_audio->Release();

    CoUninitialize();
}


void set_master_audio_volume(IXAudio2MasteringVoice* master_voice, float master_volume)
{
    const float actual_vol = std::clamp(master_volume, 0.0f, 100.0f) / 100.0f;

    master_voice->SetVolume(actual_vol);
}

bool create_audio_source(const main_audio* audio, IXAudio2SourceVoice*& out_source, const char* path)
{
    HRESULT hr = S_OK;

    WAVEFORMATEXTENSIBLE wfx = { 0 };
    XAUDIO2_BUFFER buffer = { 0 };

    // TODO: Instead of reading the entire file into memory, we should stream the data from disk
    // in chunks so that only small parts of the audio are in memory.


    // Open the file
    HANDLE hFile = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        print_error("Failed to load audio file at path %s.\n", path);
        return HRESULT_FROM_WIN32(GetLastError());
    }
        

    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());



    //
    DWORD dwChunkSize;
    DWORD dwChunkPosition;
    //check the file type, should be fourccWAVE or 'XWMA'
    find_chunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
    DWORD filetype;
    read_chunk_data(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
    if (filetype != fourccWAVE)
        return S_FALSE;

    find_chunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
    read_chunk_data(hFile, &wfx, dwChunkSize, dwChunkPosition);

    //fill out the audio data buffer with the contents of the fourccDATA chunk
    find_chunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
    BYTE* pDataBuffer = new BYTE[dwChunkSize];
    read_chunk_data(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

    buffer.AudioBytes = dwChunkSize;  //size of the audio buffer in bytes
    buffer.pAudioData = pDataBuffer;  //buffer containing audio data
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

    hr = audio->ix_audio->CreateSourceVoice(&out_source, &wfx.Format);
    if (FAILED(hr)) 
        return hr;

    hr = out_source->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
        return hr;
    
    return hr;
}

void destroy_audio_source(IXAudio2SourceVoice* source_voice)
{
    source_voice->DestroyVoice();
}


void play_audio(IXAudio2SourceVoice* source_voice)
{
    HRESULT hr = source_voice->Start(0);
    if (FAILED(hr))
        print_error("Failed to start audio.\n");

    print_log("Audio source playing sound.\n");
}

void stop_audio(IXAudio2SourceVoice* source_voice)
{
    HRESULT hr = source_voice->Stop();
    if (FAILED(hr))
        print_error("Failed to stop audio.\n");

    print_log("Audio source stopped playing sound.\n");
}

void set_audio_volume(IXAudio2SourceVoice* source_voice, float audio_volume)
{
    const float actual_vol = std::clamp(audio_volume, 0.0f, 100.0f) / 100.0f;

    source_voice->SetVolume(actual_vol);
}

void create_3d_audio(const main_audio* main, audio_3d& audio, const char* path, float speed_of_sound)
{
    HRESULT hr = S_OK;

    // Create sound source
    create_audio_source(main, audio.source_voice, path);

    DWORD dwChannelMask;
    main->master_voice->GetChannelMask(&dwChannelMask);


    hr = X3DAudioInitialize(dwChannelMask, speed_of_sound, audio.instance);
    if (FAILED(hr)) {
        print_error("Failed to initialize X3D Audio.\n");
        return;
    }

    XAUDIO2_VOICE_DETAILS voice_details{};
    main->master_voice->GetVoiceDetails(&voice_details);

    FLOAT32* matrix = new FLOAT32[voice_details.InputChannels];
    audio.dsp_settings.SrcChannelCount = 1;
    audio.dsp_settings.DstChannelCount = voice_details.InputChannels;
    audio.dsp_settings.pMatrixCoefficients = matrix;
}

void destroy_3d_audio(audio_3d& audio)
{
    delete[] audio.dsp_settings.pMatrixCoefficients;

    destroy_audio_source(audio.source_voice);
}

void update_3d_audio(const main_audio* main, audio_3d& audio, const Camera& camera)
{
    // TODO: https://github.com/microsoft/Xbox-ATG-Samples/blob/main/UWPSamples/Audio/SimplePlay3DSoundUWP/SimplePlay3DSound.cpp

    X3DAUDIO_LISTENER listener{};
    listener.OrientFront = X3DAUDIO_VECTOR(camera.front_vector.x, camera.front_vector.y, camera.front_vector.z);
    listener.OrientTop = X3DAUDIO_VECTOR(camera.up_vector.x, camera.up_vector.y, camera.up_vector.z);
    listener.Position = X3DAUDIO_VECTOR(camera.position.x, camera.position.y, camera.position.z);
    //listener.Velocity = X3DAUDIO_VECTOR(20.0f, 0.0f, 20.0f);


    float x = glm::sin(static_cast<float>(glfwGetTime())) * 100.0f;
    float y = 0.0f;
    float z = glm::cos(static_cast<float>(glfwGetTime())) * 100.0f;

    X3DAUDIO_EMITTER emitter{};
    emitter.ChannelCount = 1;
    emitter.DopplerScaler = 1.0f;
    emitter.CurveDistanceScaler = emitter.DopplerScaler;
    emitter.OrientFront = X3DAUDIO_VECTOR(0.0f, 0.0f, 1.0f);
    emitter.OrientTop = X3DAUDIO_VECTOR(0.0f, 1.0f, 0.0f);
    emitter.Position = X3DAUDIO_VECTOR(x, y, z);
    //emitter.Velocity = X3DAUDIO_VECTOR(glm::sin(glfwGetTime()) * 100.0f, 0.0f, glm::cos(glfwGetTime()) * 100.0f);

    int flags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_REVERB;
    X3DAudioCalculate(audio.instance, &listener, &emitter, flags, &audio.dsp_settings);


    audio.source_voice->SetOutputMatrix(
        main->master_voice, 
        1, 
        audio.dsp_settings.DstChannelCount, 
        audio.dsp_settings.pMatrixCoefficients
    );

    audio.source_voice->SetFrequencyRatio(audio.dsp_settings.DopplerFactor);

#if 0
    audio.source_voice->SetOutputMatrix(
        submix, 
        1, 
        1, 
        &audio.dsp_settings.ReverbLevel
    );
#endif


    XAUDIO2_FILTER_PARAMETERS FilterParameters { 
        LowPassFilter, 
        2.0f * sinf(X3DAUDIO_PI / 6.0f * audio.dsp_settings.LPFDirectCoefficient), 
        1.0f 
    };

    audio.source_voice->SetFilterParameters(&FilterParameters);

}
