#include "Simulation.h"
#include "GarrysMod/Lua/Interface.h"

int simCount = 0;
int simMaxParticles = 65536;
bool simIsRunning = false;
bool simIsValid = false;
float simRadius = 1.f;
float simDeltaTime = 1.f / 60.f;
int* simActiveIndices = nullptr;
int* simPhases = nullptr;
float4* simParticles = nullptr;
float3* simVelocities = nullptr;
std::thread simThread = std::thread();
std::mutex* bufferMutex = nullptr;

static NvFlexBuffer* particleBuffer = nullptr;
static NvFlexBuffer* velocityBuffer = nullptr;
static NvFlexBuffer* phaseBuffer = nullptr;
static NvFlexBuffer* activeBuffer = nullptr;

NvFlexParams sim_params = NvFlexParams();

//for some reason it needs to be in a function, could not understand why
void simInitParams() {
	sim_params.gravity[0] = 0.0f;
	sim_params.gravity[1] = 0.f;
	sim_params.gravity[2] = -9.8f;		//z is down, not y

	sim_params.radius = simRadius;
	sim_params.viscosity = 0.01f;
	sim_params.dynamicFriction = 0.001f;
	sim_params.staticFriction = 0.001f;
	sim_params.particleFriction = 0.001f; // scale friction between particles by default
	sim_params.freeSurfaceDrag = 0.001f;
	sim_params.drag = 0.0f;
	sim_params.lift = 0.0f;
	sim_params.numIterations = 1;
	sim_params.fluidRestDistance = simRadius / 1.5f;
	sim_params.solidRestDistance = 0.0f;

	sim_params.anisotropyScale = 1.0f;
	sim_params.anisotropyMin = 0.1f;
	sim_params.anisotropyMax = 2.0f;
	sim_params.smoothing = 1.0f;

	sim_params.dissipation = 0.0f;
	sim_params.damping = 0.0f;
	sim_params.particleCollisionMargin = 0.0f;
	sim_params.shapeCollisionMargin = 0.0f;
	sim_params.collisionDistance = 0.0f;
	sim_params.sleepThreshold = 0.0f;
	sim_params.shockPropagation = 0.0f;
	sim_params.restitution = 0.0f;

	sim_params.maxSpeed = FLT_MAX;
	sim_params.maxAcceleration = 100.0f;	// approximately 10x gravity

	sim_params.relaxationMode = eNvFlexRelaxationLocal;
	sim_params.relaxationFactor = 1.0f;
	sim_params.solidPressure = 1.0f;
	sim_params.adhesion = 0.008f;
	sim_params.cohesion = 0.019f;
	sim_params.surfaceTension = 0.0f;
	sim_params.vorticityConfinement = 0.0f;
	sim_params.buoyancy = 1.0f;
	sim_params.diffuseThreshold = 100.0f;
	sim_params.diffuseBuoyancy = 1.0f;
	sim_params.diffuseDrag = 0.8f;
	sim_params.diffuseBallistic = 16;
	sim_params.diffuseLifetime = 2.0f;

	// planes created after particles
	sim_params.planes[0][0] = 0.f;
	sim_params.planes[0][1] = 0.f;
	sim_params.planes[0][2] = 1.f;
	sim_params.planes[0][3] = 0.f;

	sim_params.numPlanes = 1;
};

void internalRun() {

	NvFlexLibrary* library = NvFlexInit();

	// create new solver
	NvFlexSolverDesc solverDesc;
	NvFlexSetSolverDescDefaults(&solverDesc);
	solverDesc.maxParticles = simMaxParticles;
	solverDesc.maxDiffuseParticles = 0;

	NvFlexSolver* solver = NvFlexCreateSolver(library, &solverDesc);
	NvFlexSetParams(solver, &sim_params);

	//alocate our buffers to memory
	particleBuffer = NvFlexAllocBuffer(library, simMaxParticles, sizeof(float4), eNvFlexBufferHost);
	velocityBuffer = NvFlexAllocBuffer(library, simMaxParticles, sizeof(float3), eNvFlexBufferHost);
	phaseBuffer = NvFlexAllocBuffer(library, simMaxParticles, sizeof(int), eNvFlexBufferHost);
	activeBuffer = NvFlexAllocBuffer(library, simMaxParticles, sizeof(int), eNvFlexBufferHost);

	//allocates how big the array of stuff is
	simParticles = (float4*)malloc(sizeof(float4) * simMaxParticles);
	simVelocities = (float3*)malloc(sizeof(float3) * simMaxParticles);
	simPhases = (int*)malloc(sizeof(int) * simMaxParticles);
	simActiveIndices = (int*)malloc(sizeof(int) * simMaxParticles);

	bufferMutex = new std::mutex();

	while (simIsValid) {

		std::this_thread::sleep_for(std::chrono::duration<float>(simDeltaTime));

		if (!simIsRunning) continue;
		if (simCount < 1) continue;

		//lock mutex
		bufferMutex->lock();

		if (!simIsValid) {
			bufferMutex->unlock();
			break;
		}
	

		float4* particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
		float3* velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
		int* phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);
		int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

		//do rendering here

		if (simParticles)
			memcpy(simParticles, particles, sizeof(float4) * simMaxParticles);
		else {
			printf("Simulation::particles was NULL");
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
		NvFlexSetActiveCount(solver, simCount);

		// tick
		NvFlexSetParams(solver, &sim_params);
		NvFlexUpdateSolver(solver, simDeltaTime * 8.f, 1, false);

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

void initSimulation(){
	if (simIsValid) return;

	simInitParams();
	simThread = std::thread(internalRun);
	simThread.detach();

	simIsValid = true;
}

void simStopSimulation(){
	simIsValid = false;
	simIsRunning = false;
	simCount = 0;
}

void simSetRadius(float r) {
	simRadius = r;
	sim_params.radius = r;
	sim_params.fluidRestDistance = r / 1.5f;

}

void simAddParticle(float4 pos, float3 vel, int phase) {
	if (!simIsValid) {
		return;
	};


	//we need to lock
	bufferMutex->lock();

	float4* particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
	float3* velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
	int* phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);
	int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

	if (!particles || !velocities || !phases || !activeIndices) {
		bufferMutex->unlock();
		return;
	};

	particles[simCount] = pos;
	velocities[simCount] = vel;
	phases[simCount] = phase;
	activeIndices[simCount] = simCount;
	simCount++;

	NvFlexUnmap(particleBuffer);
	NvFlexUnmap(velocityBuffer);
	NvFlexUnmap(phaseBuffer);
	NvFlexUnmap(activeBuffer);

	bufferMutex->unlock();
}

void simAddCube(float3 center, float3 size, int phase) {
	if (!simIsValid || !bufferMutex) {
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
			for (float x = -size.x / 2; x <= size.x / 2; x++) {
				//it appears you are out of particles
				if (simCount >= simMaxParticles) {
					bufferMutex->unlock();
					return;
				}

				float4 pos = float4{ center.x + x * simRadius, center.y + y * simRadius, center.z + z * simRadius, 1.f / 2.f };
				particles[simCount] = pos;
				velocities[simCount] = float3{0.f, 0.f, 0.f};
				phases[simCount] = phase;
				activeIndices[simCount] = simCount;
				simCount++;

			}
		}
	}

	NvFlexUnmap(particleBuffer);
	NvFlexUnmap(velocityBuffer);
	NvFlexUnmap(phaseBuffer);
	NvFlexUnmap(activeBuffer);

	bufferMutex->unlock();
}
