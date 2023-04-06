#ifndef MY_ENGINE_AUDIO_H
#define MY_ENGINE_AUDIO_H

struct Engine_Audio;
struct Engine_Audio_Source;

Engine_Audio* initialize_audio();
void terminate_audio(Engine_Audio* audio);

bool create_audio_source(const Engine_Audio* audio, Engine_Audio_Source** source, const char* path);
void terminate_audio_source(Engine_Audio_Source* audio_source);

void start_audio_source(Engine_Audio_Source* audio_source);
void stop_audio_source(Engine_Audio_Source* audio_source);


void set_audio_volume(Engine_Audio* audio, float volume);
void set_audio_volume(Engine_Audio_Source* source, float volume);

#endif
