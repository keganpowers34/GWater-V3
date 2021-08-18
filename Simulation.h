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

class Simulation
{
public:
	static int count;
	static int maxParticles;
	static bool isRunning;
	static bool isValid;

	static float4* particles;
	static float3* velocities;
	static int* activeIndices;
	static float radius;
	static float deltaTime;
	static int* phases;
	static NvFlexParams g_params;
	static std::thread thread;
	static void initParams();
	static void startSimulation();
	static void pauseSimulation();
	static void stopSimulation();
	static void setRadius(float r);
	static void addParticle(float4 pos, float3 vel, int phase);
	static void makeCube(float3 center, float3 size, int phase);

	// issue #11
	Simulation();
};

extern Simulation* sim;

void initSimulation();

