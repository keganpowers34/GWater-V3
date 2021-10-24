#include <NvFlex.h>
#include <NvFlexExt.h>
#include <NvFlexDevice.h>

#include "declarations.h"
#include <float.h>

std::shared_ptr<flexAPI> flexLib;
std::vector<Particle> particleQueue;
std::vector<Prop> props;
GarrysMod::Lua::ILuaBase* GlobalLUA;

std::mutex* bufferMutex;
float4* particleBufferHost;

int numParticles = 0;
int propCount = 0;
bool simValid = true;

bool flexRemoveQueue = false;

void initParams(NvFlexParams* params, float r) {
	params->gravity[0] = 0.0f;
	params->gravity[1] = 0.0f;
	params->gravity[2] = -9.8f;

	params->wind[0] = 0.0f;
	params->wind[1] = 0.0f;
	params->wind[2] = 0.0f;

	params->radius = r;
	params->viscosity = 0.0f;
	params->dynamicFriction = 5 / r;	//5/r
	params->staticFriction = 5 / r;
	params->particleFriction = 0.01f; // scale friction between particles by default
	params->freeSurfaceDrag = 0.0f;
	params->drag = 0.0f;
	params->lift = 1.0f;
	params->numIterations = 3;
	params->fluidRestDistance = r * 0.75f;
	params->solidRestDistance = r * 0.5f;

	params->anisotropyScale = 0.0f;
	params->anisotropyMin = 0.0f;
	params->anisotropyMax = 0.0f;
	params->smoothing = 0.0f;

	params->dissipation = 0.01f;
	params->damping = 0.0f;
	params->particleCollisionMargin = 0.f;
	params->shapeCollisionMargin = r * 0.1;
	params->collisionDistance = r * 0.25f; // Needed for tri-particle intersection
	params->sleepThreshold = 0.1f;
	params->shockPropagation = 0.0f;
	params->restitution = 1.0f;

	params->maxSpeed = FLT_MAX;
	params->maxAcceleration = 100.0f;	// approximately 10x gravity
	params->relaxationMode = eNvFlexRelaxationLocal;
	params->relaxationFactor = 1.0f;
	params->solidPressure = 1.0f;
	params->adhesion = 0.0f;
	params->cohesion = 0.005f;
	params->surfaceTension = 0.0f;
	params->vorticityConfinement = 0.0f;
	params->buoyancy = 1.0f;
	params->diffuseThreshold = 100.0f;
	params->diffuseBuoyancy = 1.0f;
	params->diffuseDrag = 0.8f;
	params->diffuseBallistic = 16;
	params->diffuseLifetime = 2.0f;

	// planes created after particles
	params->planes[0][0] = 0.f;
	params->planes[0][1] = 0.f;
	params->planes[0][2] = 1.f;
	params->planes[0][3] = 13288.f;

	params->numPlanes = 1;

	
}
