#ifndef MY_ENGINE_AUDIO_HPP
#define MY_ENGINE_AUDIO_HPP


struct Audio {
    IXAudio2* ix_audio;
    IXAudio2MasteringVoice* master_voice;
};


bool create_windows_audio(Audio*& out_audio);
void destroy_windows_audio(Audio* audio);

void set_master_audio_volume(IXAudio2MasteringVoice* master_voice, int master_volume);
bool create_audio_source(Audio* audio, IXAudio2SourceVoice*& out_source, const char* path);
void destroy_audio_source(IXAudio2SourceVoice* source_voice);
void play_audio(IXAudio2SourceVoice* source_voice);
void stop_audio(IXAudio2SourceVoice* source_voice);
void set_audio_volume(IXAudio2SourceVoice* source_voice, int audio_volume);

#endif