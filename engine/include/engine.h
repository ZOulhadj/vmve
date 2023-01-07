#ifndef ENGINE_H
#define ENGINE_H

struct Engine;

Engine* InitializeEngine();
bool UpdateEngine(Engine* engine);
void EnginePresent(Engine* engine);
void TerminateEngine(Engine* engine);

void SetEnviromentMap(const char* path);
void CreateCamera(Engine* engine, float fovy, float speed);

#endif