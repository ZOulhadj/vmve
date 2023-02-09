#include "audio.hpp"

#include "logging.hpp"

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


void set_master_audio_volume(IXAudio2MasteringVoice* master_voice, int master_volume)
{
    const float actual_vol = master_volume / 100.0f;

    master_voice->SetVolume(actual_vol);
}

bool create_audio_source(Audio* audio, IXAudio2SourceVoice*& out_source, const char* path)
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

    if (INVALID_HANDLE_VALUE == hFile)
        return HRESULT_FROM_WIN32(GetLastError());

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

    hr = audio->ix_audio->CreateSourceVoice(&out_source, (WAVEFORMATEX*)&wfx);
    if (FAILED(hr)) 
        return hr;

    hr = out_source->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
        return hr;
    
    return hr;
}

void destroy_audio_source(IXAudio2SourceVoice* source_voice)
{
    if (!source_voice)
        return;

    source_voice->DestroyVoice();
}


void play_audio(IXAudio2SourceVoice* source_voice)
{
    if (!source_voice)
        return;

    HRESULT hr = source_voice->Start(0);
    if (FAILED(hr))
        print_log("Failed to start audio.\n");

    print_log("Audio source playing sound.\n");
}

void stop_audio(IXAudio2SourceVoice* source_voice)
{
    if (!source_voice)
        return;

    HRESULT hr = source_voice->Stop();
    if (FAILED(hr))
        print_log("Failed to stop audio.\n");

    print_log("Audio source stopped playing sound.\n");
}

void set_audio_volume(IXAudio2SourceVoice* source_voice, int audio_volume)
{
    if (!source_voice)
        return;

    const float actual_vol = audio_volume / 100.0f;

    source_voice->SetVolume(actual_vol);
}
