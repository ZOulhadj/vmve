#ifndef ENGINE_H
#define ENGINE_H


// TODO: Find a better way of obtaining engine specific details without having
// everything be a function. Maybe a single struct with everything combined?
// The main problem is that I do not want to include various dependencies.


// TODO: Append the engine name for all function calls so it acts as a namespace
// without using namespaces directly as this should be C compatible.

// 
//
//
//
struct EngineInfo
{
    const char* iconPath;

    int windowWidth;
    int windowHeight;
};


//
//
//
//
struct Engine;


// Core

//
//
//
//
Engine* EngineInitialize(EngineInfo info);

//
//
//
//
void EngineTerminate(Engine* engine);

// Rendering

//
//
//
//
bool EngineUpdate(Engine* engine);

//
//
//
//
void EngineBeginRender(Engine* engine);

//
//
//
//
void EngineRender(Engine* engine);

//
//
//
//
void EnginePresent(Engine* engine);

// Enviroment

//
//
//
//
void EngineSetEnvironmentMap(const char* path);


// Models

//
//
//
//
void EngineAddModel(Engine* engine, const char* path, bool flipUVs);

//
//
//
//
void EngineRemoveModel(Engine* engine, int modelID);

//
//
//
//
int EngineGetModelCount(Engine* engine);

//
//
//
//
const char* EngineGetModelName(Engine* engine, int modelID);

// Instances

//
//
//
//
int EngineGetInstanceCount(Engine* engine);

//
//
//
//
void EngineAddInstance(Engine* engine, int modelID, float x, float y, float z);

//
//
//
//
void EngineRemoveInstance(Engine* engine, int instanceID);

//
//
//
//
void EngineGetInstanceID(Engine* engine, int instanceIndex, int* instanceID);

//
//
//
//
const char* EngineGetInstanceName(Engine* engine, int instanceIndex);

//
//
//
//
void EngineGetInstancePosition(Engine* engine, int instanceIndex, float* position);

//
//
//
//
void EngineGetInstanceRotation(Engine* engine, int instanceIndex, float* rotation);

//
//
//
//
void EngineGetInstanceScale(Engine* engine, int instanceIndex, float* scale);

// Timing

//
//
//
//
double EngineGetDeltaTime(Engine* engine);

//
//
//
//
void EngineGetUptime(Engine* engine, int* hours, int* minutes, int* seconds);


// Memory

//
//
//
//
void EngineGetMemoryStats(Engine* engine, float* memoryUsage, unsigned int* maxMemory);

// Filesystem

//
//
//
//
const char* EngineDisplayFileExplorer(Engine* engine, const char* path); // TEMP: Must be moved to VMVE

//
//
//
//
const char* EngineGetExecutableDirectory(Engine* engine);

// Input

//
//
//
//
void EngineUpdateInput(Engine* engine);

// Camera

//
//
//
//
void EngineCreateCamera(Engine* engine, float fovy, float speed);

//
//
//
//
void EngineUpdateCameraView(Engine* engine);

//
//
//
//
void EngineUpdateCameraProjection(Engine* engine, int width, int height);

//
//
//
//
void EngineGetCameraPosition(Engine* engine, float* x, float* y, float* z);

//
//
//
//
void EngineGetCameraFrontVector(Engine* engine, float* x, float* y, float* z);

//
//
//
//
float* EngineGetCameraFOV(Engine* engine);

//
//
//
//
float* EngineGetCameraSpeed(Engine* engine);

//
//
//
//
float* EngineGetCameraNear(Engine* engine);

//
//
//
//
float* EngineGetCameraFar(Engine* engine);

//
//
//
//
void EngineSetCameraPosition(Engine* engine, float x, float y, float z);

// Logs

//
//
//
//
void EngineClearLogs(Engine* engine);

//
//
//
//
void EngineExportLogsToFile(Engine* engine, const char* path);

//
//
//
//
int EngineGetLogCount(Engine* engine);

//
//
//
//
int EngineGetLogType(Engine* engine, int logIndex);

//
//
//
//
const char* EngineGetLog(Engine* engine, int logIndex);


// UI

//
//
//
//
void EngineEnableUIPass(Engine* engine);

//
//
//
//
void EngineBeginUIPass();

//
//
//
//
void EngineEndUIPass();

//
//
//
//
void EngineRenderViewportUI(int width, int height);


#endif