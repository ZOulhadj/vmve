#ifndef MY_ENGINE_AUDIO_H
#define MY_ENGINE_AUDIO_H

struct engine_audio;
struct engine_audio_source;

engine_audio* initialize_audio();
void terminate_audio(engine_audio* audio);

bool create_audio_source(const engine_audio* audio, engine_audio_source** source, const char* path);
void terminate_audio_source(engine_audio_source* audio_source);

void start_audio_source(engine_audio_source* audio_source);
void stop_audio_source(engine_audio_source* audio_source);


void set_audio_volume(engine_audio* audio, float volume);
void set_audio_volume(engine_audio_source* source, float volume);

#endif
