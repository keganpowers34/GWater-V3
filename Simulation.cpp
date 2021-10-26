#include <NvFlex.h>
#include <NvFlexExt.h>
#include <NvFlexDevice.h>

#include "declarations.h"
#include "types.h"
#include <string>
#include <random>

void flexAPI::flexSolveThread() {

	//declarations
	float simFramerate = 1.f / 60.f;	
	int simFramerateMi = static_cast<int>(simFramerate * 1000.f);

	//particle position storage
	particleBufferHost = static_cast<float4*>(malloc(sizeof(float4) * flexSolverDesc.maxParticles));;

	//runs always while sim is active
	while (simValid) {

		std::this_thread::sleep_for(std::chrono::milliseconds(simFramerateMi));

		if (numParticles < 1 && particleQueue.size() < 0) {
			continue;
		}

		bufferMutex->lock();

		//because we are in a buffer lock, the simulation might have already been shut down (even with the while loop check!)
		if (!simValid) {
			bufferMutex->unlock();
			break;

		}


		if (flexRemoveQueue) {
			particleBuffer = NvFlexAllocBuffer(flexLibrary, flexSolverDesc.maxParticles, sizeof(float4), eNvFlexBufferHost);
			velocityBuffer = NvFlexAllocBuffer(flexLibrary, flexSolverDesc.maxParticles, sizeof(float3), eNvFlexBufferHost);
			phaseBuffer = NvFlexAllocBuffer(flexLibrary, flexSolverDesc.maxParticles, sizeof(int), eNvFlexBufferHost);
			activeBuffer = NvFlexAllocBuffer(flexLibrary, flexSolverDesc.maxParticles, sizeof(int), eNvFlexBufferHost);
			particleQueue.clear();

			numParticles = 0;
			
			flexRemoveQueue = false;
		}

		// map buffers for reading / writing
		float4* particles = static_cast<float4*>(NvFlexMap(particleBuffer, eNvFlexMapWait));
		float3* velocities = static_cast<float3*>(NvFlexMap(velocityBuffer, eNvFlexMapWait));
		int* phases = static_cast<int*>(NvFlexMap(phaseBuffer, eNvFlexMapWait));
		int* activeIndices = static_cast<int*>(NvFlexMap(activeBuffer, eNvFlexMapWait));

		//loop through queue and add requested particles
		// AndrewEathan was here
		for (Particle& particle : particleQueue) {

			//apply data
			particles[numParticles] = particle.pos;
			velocities[numParticles] = particle.vel;
			phases[numParticles] = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid);
			activeIndices[numParticles] = numParticles;

			numParticles++;

		}

		//update positions of props
		float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
		float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));

		float4* prevPositions = static_cast<float4*>(NvFlexMap(geoPrevPosBuffer, 0));
		float4* prevRotations = static_cast<float4*>(NvFlexMap(geoPrevQuatBuffer, 0));

		//start at 1 because world never moves
		for (int i = 1; i < props.size(); i++) {
			positions[i] = props[i].pos;
			rotations[i] = props[i].ang;

			prevPositions[i] = props[i].lastPos;
			prevRotations[i] = props[i].lastAng;

			props[i].pos = props[i].lastPos;
			props[i].ang = props[i].lastAng;
			


		}

		NvFlexUnmap(geoPrevPosBuffer);
		NvFlexUnmap(geoPrevQuatBuffer);

		NvFlexUnmap(geoPosBuffer);
		NvFlexUnmap(geoQuatBuffer);

		particleQueue.clear();
		
		//copy particle positions (gpu memory fuckery)
		memcpy(particleBufferHost, particles, sizeof(float4) * numParticles);

		// unmap buffers
		NvFlexUnmap(particleBuffer);
		NvFlexUnmap(velocityBuffer);
		NvFlexUnmap(phaseBuffer);
		NvFlexUnmap(activeBuffer);

		// write to device (async)	
		NvFlexSetParticles(flexSolver, particleBuffer, NULL);
		NvFlexSetVelocities(flexSolver, velocityBuffer, NULL);
		NvFlexSetPhases(flexSolver, phaseBuffer, NULL);
		NvFlexSetActive(flexSolver, activeBuffer, NULL);
		NvFlexSetActiveCount(flexSolver, numParticles);
		NvFlexSetShapes(flexSolver, geometryBuffer, geoPosBuffer, geoQuatBuffer, geoPrevPosBuffer, geoPrevQuatBuffer, geoFlagsBuffer, propCount);

		// tick the solver (5 times the default looks about right -potato)
		NvFlexUpdateSolver(flexSolver, simFramerate * 8, 3, false);

		// read back (async)
		NvFlexGetParticles(flexSolver, particleBuffer, NULL);
		NvFlexGetVelocities(flexSolver, velocityBuffer, NULL);
		NvFlexGetPhases(flexSolver, phaseBuffer, NULL);
		NvFlexGetActive(flexSolver, activeBuffer, NULL);

		bufferMutex->unlock();	//dont forget to unlock our buffer

	}

}