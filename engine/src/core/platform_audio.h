#ifndef MY_ENGINE_AUDIO_H
#define MY_ENGINE_AUDIO_H

namespace engine {
    struct Platform_Audio;
    struct Platform_Audio_Source;

    Platform_Audio* initialize_audio();
    void terminate_audio(Platform_Audio* audio);
    bool create_audio_source(const Platform_Audio* audio, Platform_Audio_Source** source, const char* path);
    void terminate_audio_source(Platform_Audio_Source* audio_source);
    void start_audio_source(Platform_Audio_Source* audio_source);
    void stop_audio_source(Platform_Audio_Source* audio_source);
    void set_audio_volume(Platform_Audio* audio, float volume);
    void set_audio_volume(Platform_Audio_Source* source, float volume);
}


#endif
