#ifndef MY_ENGINE_AUDIO_HPP
#define MY_ENGINE_AUDIO_HPP


struct Audio
{
    IXAudio2* ixAudio;
    IXAudio2MasteringVoice* masterVoice;
};


Audio* CreateAudio();
void DestroyAudio(Audio* audio);

#endif