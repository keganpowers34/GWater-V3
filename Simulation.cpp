#include "declarations.h"
#include "types.h"
#include <string>
#include <NvFlex.h>
#include <NvFlexExt.h>
#include <random>

void flexSolveThread() {

	//declarations
	int simMaxParticles = 65536;
	float simFramerate = 1.f / 60.f;	
	int simFramerateMi = static_cast<int>(simFramerate * 1000.f);

	//particle position storage
	particleBufferHost = static_cast<float4*>(malloc(sizeof(float4) * simMaxParticles));;

	//runs always while sim is active
	while (simValid) {

		bufferMutex->lock();

		//because we are in a buffer lock, the simulation might have already been shut down (even with the while loop check!)
		if (!simValid) {
			bufferMutex->unlock();
			break;

		}

		std::this_thread::sleep_for(std::chrono::milliseconds(simFramerateMi));

		// map buffers for reading / writing
		float4* particles = static_cast<float4*>(NvFlexMap(particleBuffer, eNvFlexMapWait));
		float3* velocities = static_cast<float3*>(NvFlexMap(velocityBuffer, eNvFlexMapWait));
		int* phases = static_cast<int*>(NvFlexMap(phaseBuffer, eNvFlexMapWait));
		int* activeIndices = static_cast<int*>(NvFlexMap(activeBuffer, eNvFlexMapWait));

		//loop through queue and add requested particles
		int size = particleQueue.size() - 1;
		for (int i = 0; i < size; i+=2) {

			//0:pos 1:vel
			float3 particlePos = particleQueue[0];
			
			//apply data
			phases[numParticles] = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid);
			activeIndices[numParticles] = numParticles;
			particles[numParticles] = float4{ particlePos.x, particlePos.y, particlePos.z, 0.5f };
			velocities[numParticles] = particleQueue[1];

			//remove vel and pos instance from queue
			particleQueue.erase(particleQueue.begin(), particleQueue.begin() + 2);
			numParticles++;
				
		}
		
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
		NvFlexSetShapes(flexSolver, geometryBuffer, geoPosBuffer, geoQuatBuffer, NULL, NULL, geoFlagsBuffer, propCount);	//doesnt work??

		// tick the solver (5 times the default looks about right -potato)
		NvFlexUpdateSolver(flexSolver, simFramerate * 5, 4, false);

		// read back (async)
		NvFlexGetParticles(flexSolver, particleBuffer, NULL);
		NvFlexGetVelocities(flexSolver, velocityBuffer, NULL);
		NvFlexGetPhases(flexSolver, phaseBuffer, NULL);

		bufferMutex->unlock();	//dont forget to unlock our buffer

	}

}