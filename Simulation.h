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
	int count = 0;
	int maxParticles = 65536;
	bool isRunning = false;
	bool isValid = false;

	float4* particles;
	float3* velocities;
	float radius = 1;
	float deltaTime = 1.f / 60.f;
	int* phases;
	NvFlexParams* g_params;
	std::thread thread;
	void initParams();
	void startSimulation();
	void pauseSimulation();
	void stopSimulation();
	void addParticle(float4 pos, float3 vel, int phase);
	void makeCube(float3 center, float3 size, int phase);

	// issue #11
	Simulation();
};

extern Simulation* sim;

void initSimulation(Simulation* sim);

