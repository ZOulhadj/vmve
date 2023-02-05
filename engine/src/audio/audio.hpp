#ifndef MY_ENGINE_AUDIO_HPP
#define MY_ENGINE_AUDIO_HPP


struct Audio {
    IXAudio2* ix_audio;
    IXAudio2MasteringVoice* master_voice;
};


bool create_windows_audio(Audio*& out_audio);
void destroy_windows_audio(Audio* audio);

#endif