#include "pch.h"
#include "../platform_audio.h"

#include <xaudio2.h>

#include "utils/logging.h"



namespace engine {

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

    struct Platform_Audio
    {
        IXAudio2* ix_audio;
        IXAudio2MasteringVoice* master_voice;
    };

    struct Platform_Audio_Source
    {
        IXAudio2SourceVoice* source_voice;
    };


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

    Platform_Audio* initialize_audio()
    {
        print_log("Initializing audio\n");

        Platform_Audio* audio = new Platform_Audio();

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

    void terminate_audio(Platform_Audio* audio)
    {
        if (!audio)
            return;

        print_log("Terminating audio\n");

        audio->master_voice->DestroyVoice();
        audio->ix_audio->Release();

        CoUninitialize();
    }






    bool create_audio_source(const Platform_Audio* audio, Platform_Audio_Source** source, const char* path)
    {
        *source = new Platform_Audio_Source();


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

        // TODO: We might need to delete the allocated memory.
        BYTE* pDataBuffer = new BYTE[dwChunkSize];

        read_chunk_data(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

        buffer.AudioBytes = dwChunkSize;  //size of the audio buffer in bytes
        buffer.pAudioData = pDataBuffer;  //buffer containing audio data
        buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

        hr = audio->ix_audio->CreateSourceVoice(&(*source)->source_voice, &wfx.Format);
        if (FAILED(hr))
            return false;

        hr = (*source)->source_voice->SubmitSourceBuffer(&buffer);
        if (FAILED(hr))
            return false;

        return true;
    }

    void terminate_audio_source(Platform_Audio_Source* audio_source)
    {
        audio_source->source_voice->DestroyVoice();

        delete audio_source;
    }


    void start_audio_source(Platform_Audio_Source* audio_source)
    {
        HRESULT hr = audio_source->source_voice->Start(0);
        if (FAILED(hr))
            print_error("Failed to start audio.\n");

        print_log("Audio source playing sound.\n");
    }

    void stop_audio_source(Platform_Audio_Source* audio_source)
    {
        HRESULT hr = audio_source->source_voice->Stop();
        if (FAILED(hr))
            print_error("Failed to stop audio.\n");

        print_log("Audio source stopped playing sound.\n");
    }

    void set_audio_volume(Platform_Audio* audio, float volume)
    {
        const float actual_vol = std::clamp(volume, 0.0f, 100.0f) / 100.0f;
        audio->master_voice->SetVolume(actual_vol);
    }

    void set_audio_volume(Platform_Audio_Source* source, float volume)
    {
        const float actual_vol = std::clamp(volume, 0.0f, 100.0f) / 100.0f;
        source->source_voice->SetVolume(actual_vol);
    }
}
