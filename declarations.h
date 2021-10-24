#pragma once

#include <NvFlex.h>
#include <NvFlexExt.h>
#include <NvFlexDevice.h>

#include <thread>
#include <mutex>
#include "types.h"
#include <vector>

extern std::shared_ptr<flexAPI> flexLib;
extern std::vector<Particle> particleQueue;
extern std::vector<Prop> props;
extern GarrysMod::Lua::ILuaBase* GlobalLUA;

extern std::mutex* bufferMutex;
extern float4* particleBufferHost;

extern int numParticles;
extern int propCount;
extern bool simValid;

extern bool flexRemoveQueue;


extern void initParams(NvFlexParams* params, float r);
extern void printLua(std::string text);
extern void printLua(char*);