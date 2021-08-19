#pragma once

#include "types.h"
#include "NvFlex.h"
#include "NvFlexExt.h"
#include "GarrysMod/Lua/SourceCompat.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <string>
#include <functional>
#include <mutex>

extern NvFlexParams sim_params;
extern float4* simParticles;
extern float3* simVelocities;
extern float simRadius;
extern int simCount;
extern bool simIsRunning;
extern bool simIsValid;

void simAddParticle(float4 pos, float3 vel, int phase);
void simAddCube(float3 center, float3 size, int phase);
void simSetRadius(float r);
void simStopSimulation();

void initSimulation();


