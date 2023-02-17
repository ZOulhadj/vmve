#ifndef MY_ENGINE_AUDIO_HPP
#define MY_ENGINE_AUDIO_HPP

#include "rendering/camera.h"


struct main_audio
{
    IXAudio2* ix_audio;
    IXAudio2MasteringVoice* master_voice;
};

struct audio_3d
{
    X3DAUDIO_HANDLE instance;
    X3DAUDIO_DSP_SETTINGS dsp_settings;

    IXAudio2SourceVoice* source_voice;
};


main_audio* create_windows_audio();
void destroy_windows_audio(main_audio* audio);

// 2d audio
void set_master_audio_volume(IXAudio2MasteringVoice* master_voice, int master_volume);
bool create_audio_source(const main_audio* audio, IXAudio2SourceVoice*& out_source, const char* path);
void destroy_audio_source(IXAudio2SourceVoice* source_voice);
void play_audio(IXAudio2SourceVoice* source_voice);
void stop_audio(IXAudio2SourceVoice* source_voice);
void set_audio_volume(IXAudio2SourceVoice* source_voice, int audio_volume);


// 3d audio
void create_3d_audio(const main_audio* main, audio_3d& audio, const char* path, float speed_of_sound = X3DAUDIO_SPEED_OF_SOUND);
void destroy_3d_audio(audio_3d& audio);
void update_3d_audio(const main_audio* main, audio_3d& audio, const Camera& camera);

#endif