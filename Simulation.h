#pragma once

#include "types.h"
#include "NvFlex.h"
#include "NvFlexExt.h"
#include "GarrysMod/Lua/SourceCompat.h"
#include "GarrysMod/Lua/Interface.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <string>
#include <functional>
#include <mutex>


extern GarrysMod::Lua::ILuaBase* GlobalLUA;
extern NvFlexLibrary* flex_library;

extern NvFlexTriangleMeshId worldMeshId;
extern NvFlexBuffer* worldVerts;
extern NvFlexBuffer* worldIndices;

extern float4* simParticles;
extern float3* simVelocities;
extern NvFlexParams sim_params;
extern float simRadius;
extern int simCount;
extern bool simIsRunning;
extern bool simIsValid;

void printLua(GarrysMod::Lua::ILuaBase* LUA, std::string text);
void simAddParticle(float4 pos, float3 vel, int phase);
void simAddCube(float3 center, float3 size, int phase);
void simSetRadius(float r);
void simStopSimulation();
void simAddWorld();

void initSimulation();

