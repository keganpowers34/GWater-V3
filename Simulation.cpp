#include "Simulation.h"

GarrysMod::Lua::ILuaBase* GlobalLUA = nullptr;

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
static NvFlexBuffer* geometryBuffer = nullptr;
static NvFlexBuffer* geoFlagsBuffer = nullptr;
static NvFlexBuffer* geoPosBuffer = nullptr;
static NvFlexBuffer* geoQuatBuffer = nullptr;
static NvFlexBuffer* worldVerts = nullptr;
static NvFlexBuffer* worldIndices = nullptr;


NvFlexLibrary* flex_library = nullptr;
NvFlexTriangleMeshId worldMeshId = NULL;
NvFlexParams sim_params = NvFlexParams();

//max function from potatoos
#define max(a, b) a > b ? a : b

void printLua(GarrysMod::Lua::ILuaBase* LUA, std::string text)
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "print");
	LUA->PushString(text.c_str());
	LUA->Call(1, 0);
	LUA->Pop();
}

//for some reason params needs to be in a function
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
	sim_params.numIterations = 3;
	sim_params.fluidRestDistance = simRadius / 1.5f;
	sim_params.solidRestDistance = simRadius;

	sim_params.anisotropyScale = 1.0f;
	sim_params.anisotropyMin = 0.1f;
	sim_params.anisotropyMax = 2.0f;
	sim_params.smoothing = 1.0f;

	sim_params.dissipation = 0.0f;
	sim_params.damping = 0.0f;
	sim_params.particleCollisionMargin = 0.0f;
	sim_params.shapeCollisionMargin = 0.0f;
	sim_params.collisionDistance = simRadius;
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
	//sim_params.planes[0][0] = 0.f;
	//sim_params.planes[0][1] = 0.f;
	//sim_params.planes[0][2] = 1.f;
	//sim_params.planes[0][3] = 11135.5f;

	sim_params.numPlanes = 0;
};

void internalRun() {

	bufferMutex->lock();
	//init library
	flex_library = NvFlexInit();

	// create new solver
	NvFlexSolverDesc solverDesc;
	NvFlexSetSolverDescDefaults(&solverDesc);
	solverDesc.maxParticles = simMaxParticles;
	solverDesc.maxDiffuseParticles = 0;

	NvFlexSolver* solver = NvFlexCreateSolver(flex_library, &solverDesc);
	NvFlexSetParams(solver, &sim_params);

	//alocate all our buffers to memory
	particleBuffer = NvFlexAllocBuffer(flex_library, simMaxParticles, sizeof(float4), eNvFlexBufferHost);
	velocityBuffer = NvFlexAllocBuffer(flex_library, simMaxParticles, sizeof(float3), eNvFlexBufferHost);
	phaseBuffer = NvFlexAllocBuffer(flex_library, simMaxParticles, sizeof(int), eNvFlexBufferHost);
	activeBuffer = NvFlexAllocBuffer(flex_library, simMaxParticles, sizeof(int), eNvFlexBufferHost);

	//allocates how big the array of stuff is
	simParticles = (float4*)malloc(sizeof(float4) * simMaxParticles);
	simVelocities = (float3*)malloc(sizeof(float3) * simMaxParticles);
	simPhases = (int*)malloc(sizeof(int) * simMaxParticles);
	simActiveIndices = (int*)malloc(sizeof(int) * simMaxParticles);

	bufferMutex->unlock();

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
		NvFlexSetShapes(solver, geometryBuffer, geoPosBuffer, geoQuatBuffer, NULL, NULL, geoFlagsBuffer, 1);

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
	NvFlexFreeBuffer(worldVerts);
	NvFlexFreeBuffer(worldIndices);

	NvFlexDestroyTriangleMesh(flex_library, worldMeshId);

	NvFlexDestroySolver(solver);
	NvFlexShutdown(flex_library);
}

void initSimulation(){
	if (simIsValid) return;

	bufferMutex = new std::mutex();
	//bufferMutex->lock();

	simInitParams();
	simThread = std::thread(internalRun);
	simThread.detach();
	simIsValid = true;

	//bufferMutex->unlock();
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
	sim_params.solidRestDistance = r / 1.5f;

}

//adds a singular particle
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

//adds a cube of particles with a set size
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

//adds world collisions when called
void simAddWorld() {
	geometryBuffer = NvFlexAllocBuffer(flex_library, simMaxParticles, sizeof(NvFlexCollisionGeometry), eNvFlexBufferHost);
	geoPosBuffer = NvFlexAllocBuffer(flex_library, simMaxParticles, sizeof(float4), eNvFlexBufferHost);
	geoFlagsBuffer = NvFlexAllocBuffer(flex_library, simMaxParticles, sizeof(int), eNvFlexBufferHost);
	geoQuatBuffer = NvFlexAllocBuffer(flex_library, simMaxParticles, sizeof(float4), eNvFlexBufferHost);

	NvFlexCollisionGeometry* geometry = (NvFlexCollisionGeometry*)NvFlexMap(geometryBuffer, 0);
	float4* positions = (float4*)NvFlexMap(geoPosBuffer, 0);
	float4* rotations = (float4*)NvFlexMap(geoQuatBuffer, 0);
	int* flags = (int*)NvFlexMap(geoFlagsBuffer, 0);

	flags[0] = NvFlexMakeShapeFlags(eNvFlexShapeTriangleMesh, false);
	geometry[0].triMesh.mesh = worldMeshId;
	geometry[0].triMesh.scale[0] = 1.0f;
	geometry[0].triMesh.scale[1] = 1.0f;
	geometry[0].triMesh.scale[2] = 1.0f;

	positions[0] = float4{ 0.f, 0.f, 0.f, 0.f };
	rotations[0] = float4{ 0.f, 0.f, 0.f, 0.f };

	NvFlexUnmap(geometryBuffer);
	NvFlexUnmap(geoPosBuffer);
	NvFlexUnmap(geoQuatBuffer);
	NvFlexUnmap(geoFlagsBuffer);

}
