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

class Simulation
{
public:
	int count = 0;
	int maxParticles = 65536;
	bool isRunning = false;
	bool isValid = false;

	float4* particles;
	float3* velocities;
	float radius = 1.f;
	float deltaTime = 1.0f / 60.0f;
	int* phases;
	NvFlexParams g_params;
	void initParams();
	void startSimulation();
	void pauseSimulation();
	void stopSimulation();
	void setRadius(float r);

	Simulation() {
		initParams();
	};

};

extern Simulation* sim;

void initSimulation(Simulation* sim);

