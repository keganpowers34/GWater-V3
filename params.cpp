#include <NvFlex.h>
#include <NvFlexExt.h>
#include <NvFlexDevice.h>

#include "declarations.h"
#include <float.h>

std::shared_ptr<flexAPI> flexLib;
std::vector<Particle> particleQueue;
GarrysMod::Lua::ILuaBase* GlobalLUA;

std::mutex* bufferMutex;
float4* particleBufferHost;

int numParticles = 0;
int propCount = 0;
bool simValid = true;

bool flexRemoveQueue = false;

void flexAPI::initParams(float r) {

	flexParams->gravity[0] = 0.0f;
	flexParams->gravity[1] = 0.0f;
	flexParams->gravity[2] = -9.8f;

	flexParams->wind[0] = 0.0f;
	flexParams->wind[1] = 0.0f;
	flexParams->wind[2] = 0.0f;

	flexParams->radius = r;
	flexParams->viscosity = 0.0f;
	flexParams->dynamicFriction = 0.5f;	//5/r
	flexParams->staticFriction = 0.5f;
	flexParams->particleFriction = 0.01f; // scale friction between particles by default
	flexParams->freeSurfaceDrag = 0.0f;
	flexParams->drag = 0.0f;
	flexParams->lift = 1.0f;
	flexParams->numIterations = 2;
	flexParams->fluidRestDistance = r * 0.75f;
	flexParams->solidRestDistance = r * 0.5f;

	flexParams->anisotropyScale = 0.0f;
	flexParams->anisotropyMin = 0.0f;
	flexParams->anisotropyMax = 0.0f;
	flexParams->smoothing = 0.0f;

	flexParams->dissipation = 0.01f;
	flexParams->damping = 0.0f;
	flexParams->particleCollisionMargin = r * 0.5f;
	flexParams->shapeCollisionMargin = r * 0.1f;
	flexParams->collisionDistance = r * 0.25f; // Needed for tri-particle intersection
	flexParams->sleepThreshold = 0.1f;
	flexParams->shockPropagation = 0.0f;
	flexParams->restitution = 1.0f;

	flexParams->maxSpeed = FLT_MAX;
	flexParams->maxAcceleration = 128.0f;	// approximately 10x gravity
	flexParams->relaxationMode = eNvFlexRelaxationLocal;
	flexParams->relaxationFactor = 0.0f;
	flexParams->solidPressure = 0.0f;
	flexParams->adhesion = 0.0f;
	flexParams->cohesion = 0.02f;
	flexParams->surfaceTension = 0.0f;
	flexParams->vorticityConfinement = 0.0f;
	flexParams->buoyancy = 1.0f;
	flexParams->diffuseThreshold = 100.0f;
	flexParams->diffuseBuoyancy = 1.0f;
	flexParams->diffuseDrag = 0.8f;
	flexParams->diffuseBallistic = 16;
	flexParams->diffuseLifetime = 2.0f;

	// planes created after particles
	flexParams->planes[0][0] = 0.f;
	flexParams->planes[0][1] = 0.f;
	flexParams->planes[0][2] = 1.f;
	flexParams->planes[0][3] = 13288.f;

	flexParams->numPlanes = 1;

}
