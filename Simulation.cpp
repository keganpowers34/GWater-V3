#include "Simulation.h"
#include "GarrysMod/Lua/Interface.h"
#include <sstream>

//extern GarrysMod::Lua::ILuaBase* LUA;
extern void PrintLUA(std::string text);

Simulation* sim = new Simulation();
std::mutex* bufferMutex = nullptr;

static NvFlexBuffer* particleBuffer;
static NvFlexBuffer* velocityBuffer;
static NvFlexBuffer* phaseBuffer;
static NvFlexBuffer* activeBuffer;

//issue #11
Simulation::Simulation() {
	initParams();
}

void Simulation::initParams() {
	g_params.gravity[0] = 0.0f;
	g_params.gravity[1] = 0.f;
	g_params.gravity[2] = -9.8f;		//z is down, not y

	g_params.radius = Simulation::radius;
	g_params.viscosity = 0.01f;
	g_params.dynamicFriction = 0.1f;
	g_params.staticFriction = 0.1f;
	g_params.particleFriction = 0.1f; // scale friction between particles by default
	g_params.freeSurfaceDrag = 0.1f;
	g_params.drag = 0.0f;
	g_params.lift = 0.0f;
	g_params.numIterations = 1;
	g_params.fluidRestDistance = Simulation::radius / 1.5f;
	g_params.solidRestDistance = 0.0f;

	g_params.anisotropyScale = 1.0f;
	g_params.anisotropyMin = 0.1f;
	g_params.anisotropyMax = 2.0f;
	g_params.smoothing = 1.0f;

	g_params.dissipation = 0.0f;
	g_params.damping = 0.0f;
	g_params.particleCollisionMargin = 0.0f;
	g_params.shapeCollisionMargin = 0.0f;
	g_params.collisionDistance = 0.0f;
	g_params.sleepThreshold = 0.0f;
	g_params.shockPropagation = 0.0f;
	g_params.restitution = 0.0f;

	g_params.maxSpeed = FLT_MAX;
	g_params.maxAcceleration = 100.0f;	// approximately 10x gravity

	g_params.relaxationMode = eNvFlexRelaxationLocal;
	g_params.relaxationFactor = 1.0f;
	g_params.solidPressure = 1.0f;
	g_params.adhesion = 0.005f;
	g_params.cohesion = 0.015f;
	g_params.surfaceTension = 0.0f;
	g_params.vorticityConfinement = 0.0f;
	g_params.buoyancy = 1.0f;
	g_params.diffuseThreshold = 100.0f;
	g_params.diffuseBuoyancy = 1.0f;
	g_params.diffuseDrag = 0.8f;
	g_params.diffuseBallistic = 16;
	g_params.diffuseLifetime = 2.0f;

	// planes created after particles
	g_params.planes[0][0] = 0.f;
	g_params.planes[0][1] = 0.f;
	g_params.planes[0][2] = 1.f;
	g_params.planes[0][3] = 0.f;

	g_params.numPlanes = 1;
};

void internalRun(Simulation* sim) {

	NvFlexLibrary* library = NvFlexInit();

	// create new solver
	NvFlexSolverDesc solverDesc;
	NvFlexSetSolverDescDefaults(&solverDesc);
	solverDesc.maxParticles = sim->maxParticles;
	solverDesc.maxDiffuseParticles = 0;

	NvFlexSolver* solver = NvFlexCreateSolver(library, &solverDesc);
	NvFlexSetParams(solver, &sim->g_params);

	//alocate our buffers to memory
	particleBuffer = NvFlexAllocBuffer(library, sim->maxParticles, sizeof(float4), eNvFlexBufferHost);
	velocityBuffer = NvFlexAllocBuffer(library, sim->maxParticles, sizeof(float3), eNvFlexBufferHost);
	phaseBuffer = NvFlexAllocBuffer(library, sim->maxParticles, sizeof(int), eNvFlexBufferHost);
	activeBuffer = NvFlexAllocBuffer(library, sim->maxParticles, sizeof(int), eNvFlexBufferHost);

	//idk what tf this do
	sim->particles = (float4*)malloc(sizeof(float4) * sim->maxParticles);
	sim->velocities = (float3*)malloc(sizeof(float3) * sim->maxParticles);
	sim->phases = (int*)malloc(sizeof(int) * sim->maxParticles);


	// map buffers for reading / writing
	float4* particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
	float3* velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
	int* phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);
	int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

	// spawn single particle for testing
	// this code has been moved into Simulation::addParticle by yours truly
	// :moyai:

	NvFlexUnmap(particleBuffer);
	NvFlexUnmap(velocityBuffer);
	NvFlexUnmap(phaseBuffer);
	NvFlexUnmap(activeBuffer);

	NvFlexSetParticles(solver, particleBuffer, NULL);
	NvFlexSetVelocities(solver, velocityBuffer, NULL);
	NvFlexSetPhases(solver, phaseBuffer, NULL);
	NvFlexSetActive(solver, activeBuffer, NULL);

	bufferMutex = new std::mutex();

	while (sim->isValid) {

		std::this_thread::sleep_for(std::chrono::duration<float>(sim->deltaTime));

		if (!sim->isRunning) continue;
		if (sim->count < 1) continue;

		//lock mutex
		bufferMutex->lock();

		float4* particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
		float3* velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
		int* phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);
		int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

		//do rendering here

		if (sim->particles)
			memcpy(sim->particles, particles, sizeof(float4) * sim->maxParticles);
		else {
			printf("sim->particles was NULL");
		}

		NvFlexUnmap(particleBuffer);
		NvFlexUnmap(velocityBuffer);
		NvFlexUnmap(phaseBuffer);
		NvFlexUnmap(activeBuffer);

		// write to device (async)
		NvFlexSetParticles(solver, particleBuffer, NULL);
		NvFlexSetVelocities(solver, velocityBuffer, NULL);
		NvFlexSetPhases(solver, phaseBuffer, NULL);
		NvFlexSetActive(solver, activeBuffer, NULL);
		NvFlexSetActiveCount(solver, sim->count);

		// set active count
		NvFlexSetParams(solver, &sim->g_params);
		NvFlexSetActiveCount(solver, sim->count);

		// tick
		NvFlexUpdateSolver(solver, sim->deltaTime, 1, false);

		// read back (async)
		NvFlexGetParticles(solver, particleBuffer, NULL);
		NvFlexGetVelocities(solver, velocityBuffer, NULL);
		NvFlexGetPhases(solver, phaseBuffer, NULL);

		bufferMutex->unlock();
	}

	//shutdown, free memory
	NvFlexFreeBuffer(particleBuffer);
	NvFlexFreeBuffer(velocityBuffer);
	NvFlexFreeBuffer(phaseBuffer);
	NvFlexFreeBuffer(activeBuffer);

	NvFlexDestroySolver(solver);
	NvFlexShutdown(library);
}

void initSimulation()
{
	if (sim->isValid) return;

	sim->thread = std::thread(internalRun, sim);
	sim->thread.detach();

	sim->isValid = true;
}

void Simulation::startSimulation()
{
	isRunning = true;
}

void Simulation::pauseSimulation()
{
	isRunning = false;
}

void Simulation::stopSimulation()
{
	isValid = false;
	isRunning = false;
	count = 0;
}

void Simulation::addParticle(float4 pos, float3 vel, int phase) {
	if (!sim->isValid) {
		return;
	};

	bufferMutex->lock();

	float4* particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
	float3* velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
	int* phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);
	int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

	if (!particles || !velocities || !phases || !activeIndices) {
		//fixes C26115 failing to release mutex lock
		bufferMutex->unlock();
		return;
	};

	particles[sim->count] = pos;
	velocities[sim->count] = vel;
	phases[sim->count] = phase;
	activeIndices[sim->count] = sim->count;
	sim->count++;

	NvFlexUnmap(particleBuffer);
	NvFlexUnmap(velocityBuffer);
	NvFlexUnmap(phaseBuffer);
	NvFlexUnmap(activeBuffer);

	bufferMutex->unlock();
}

void Simulation::makeCube(float3 center, float3 size, int phase) {
	if (!sim->isValid || !bufferMutex) {
		return;
	};

	bufferMutex->lock();

	float4* particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
	float3* velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
	int* phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);
	int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

	if (!particles || !velocities || !phases || !activeIndices) {
		//fixes C26115 failing to release mutex lock
		bufferMutex->unlock();
		return;
	};

	for (float z = -size.z / 2; z <= size.z / 2; z++) {
		for (float y = -size.y / 2; y <= size.y / 2; y++) {
			for (float x = -size.x / 2; x <= size.x / 2; z++) {
				float4 pos = float4{ center.x + x * radius, center.y + y * radius, center.z + z * radius, 1.f / 2.f };
				particles[sim->count] = pos;
				velocities[sim->count] = float3{0, 0, 0};
				phases[sim->count] = phase;
				activeIndices[sim->count] = sim->count;

				//if only cpp had string + number + string + etc
				std::stringstream str;
				str << sim->count << ": " << pos.x << "," << pos.y << "," << pos.z;
				PrintLUA(str.str());

				sim->count++;
			}
		}
	}

	NvFlexUnmap(particleBuffer);
	NvFlexUnmap(velocityBuffer);
	NvFlexUnmap(phaseBuffer);
	NvFlexUnmap(activeBuffer);

	bufferMutex->unlock();
}

//sets radius of particle colliders
//removed since kinda pointless sorry mee
//:(
