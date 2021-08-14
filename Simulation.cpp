#include "Simulation.h"
#include "GarrysMod/Lua/Interface.h"

//extern GarrysMod::Lua::ILuaBase* GlobalLUA;
//extern void printLua(GarrysMod::Lua::ILuaBase* LUA, std::string text);
Simulation* sim = new Simulation();

void Simulation::initParams() {

	g_params.gravity[0] = 0.0f;
	g_params.gravity[1] = 0.f;
	g_params.gravity[2] = -9.8f;		//z is down, not y

	g_params.radius = Simulation::radius;
	g_params.viscosity = 0.0f;
	g_params.dynamicFriction = 0.0f;
	g_params.staticFriction = 0.0f;
	g_params.particleFriction = 0.0f; // scale friction between particles by default
	g_params.freeSurfaceDrag = 0.0f;
	g_params.drag = 0.0f;
	g_params.lift = 0.0f;
	g_params.numIterations = 1;
	g_params.fluidRestDistance = Simulation::radius / 2.f;
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
	g_params.maxAcceleration = 100.0f;    // approximately 10x gravity

	g_params.relaxationMode = eNvFlexRelaxationLocal;
	g_params.relaxationFactor = 1.0f;
	g_params.solidPressure = 1.0f;
	g_params.adhesion = 0.0f;
	g_params.cohesion = 0.025f;
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
	g_params.planes[0][1] = 1.f;
	g_params.planes[0][2] = 0.f;
	g_params.planes[0][3] = 0.f;

	g_params.numPlanes = 1;

};

void internalRun(Simulation sim) {

    NvFlexLibrary* library = NvFlexInit();

    // create new solver
    NvFlexSolverDesc solverDesc;
    NvFlexSetSolverDescDefaults(&solverDesc);
    solverDesc.maxParticles = sim.maxParticles;
    solverDesc.maxDiffuseParticles = 0;

    NvFlexSolver* solver = NvFlexCreateSolver(library, &solverDesc);
    NvFlexSetParams(solver, &sim.g_params);

	//alocate our buffers to memory
    NvFlexBuffer* particleBuffer = NvFlexAllocBuffer(library, sim.maxParticles, sizeof(float4), eNvFlexBufferHost);
    NvFlexBuffer* velocityBuffer = NvFlexAllocBuffer(library, sim.maxParticles, sizeof(float3), eNvFlexBufferHost);
    NvFlexBuffer* phaseBuffer = NvFlexAllocBuffer(library, sim.maxParticles, sizeof(int), eNvFlexBufferHost);
    NvFlexBuffer* activeBuffer = NvFlexAllocBuffer(library, sim.maxParticles, sizeof(int), eNvFlexBufferHost);


    // map buffers for reading / writing
    float4* particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
    float3* velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
    int* phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);

    // spawn single particle for testin'
    particles[0] = float4{ 0, 0, 1, 1.f / 2.f };
    velocities[0] = float3{ 0, 1, 0 };
    phases[0] = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid);

	sim.count++;

    // unmap buffers
    NvFlexUnmap(particleBuffer);
    NvFlexUnmap(velocityBuffer);
    NvFlexUnmap(phaseBuffer);
	NvFlexUnmap(activeBuffer);

    //NvFlexSetParams(solver, &g_params);
    NvFlexSetParticles(solver, particleBuffer, NULL);
    NvFlexSetVelocities(solver, velocityBuffer, NULL);
    NvFlexSetPhases(solver, phaseBuffer, NULL);

    while (sim.isValid) {
        if (!sim.isRunning) continue;

        // write to device (async)
        NvFlexSetParticles(solver, particleBuffer, NULL);
        NvFlexSetVelocities(solver, velocityBuffer, NULL);
        NvFlexSetPhases(solver, phaseBuffer, NULL);
        NvFlexSetActive(solver, activeBuffer, NULL);
        NvFlexSetActiveCount(solver, sim.maxParticles);

        // set active count
        NvFlexSetParams(solver, &sim.g_params);
        NvFlexSetActiveCount(solver, sim.maxParticles);

        // tick
        NvFlexUpdateSolver(solver, sim.deltaTime, 1, false);

        // read back (async)
        NvFlexGetParticles(solver, particleBuffer, NULL);
        NvFlexGetVelocities(solver, velocityBuffer, NULL);
        NvFlexGetPhases(solver, phaseBuffer, NULL);

		
    }

	//shutdown, free memory
    NvFlexFreeBuffer(particleBuffer);
    NvFlexFreeBuffer(velocityBuffer);
    NvFlexFreeBuffer(phaseBuffer);
    NvFlexFreeBuffer(activeBuffer);
    NvFlexDestroySolver(solver);
    NvFlexShutdown(library);
}

void initSimulation(Simulation sim)
{
	if (sim.isValid) {
		return;
	}

	//broken as of now
	std::thread simThread = std::thread(sim.internalRun());
	simThread.detach();

	sim.isValid = true;
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

//sets radius of particle colliders
void Simulation::setRadius(float r) {
	radius = r;

};

/* //depricated function
void Simulation::SpawnParticle(Vector pos, Vector vel) {
	if (!Simulation::isValid || solver == nullptr) {
		return;
	}

	count++;

	float4 posconv = { pos.x, pos.y, pos.z, 0 };
	float3 velconv = { vel.x, vel.y, vel.z };

	// map buffers for reading / writing
	if (particles == nullptr) particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
	if (velocities == nullptr) velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
	if (phases == nullptr) phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);

	particles[count] = posconv;
	velocities[count] = velconv;
	phases[count] = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid);    //make fluid phase, not cloth


	NvFlexBuffer* activeBuffer = NvFlexAllocBuffer(library, maxParticles, sizeof(int), eNvFlexBufferHost);

	int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

	activeIndices[count] = count;
	NvFlexUnmap(activeBuffer);

	NvFlexSetActive(solver, activeBuffer, NULL);
	NvFlexSetActiveCount(solver, count);
}
*/
