#include "declarations.h"
#include "types.h"
#include <string>
#include <NvFlexExt.h>

void flexSolveThread() {

	int simMaxParticles = 65536;
	float simFramerate = 1.f / 30.f;	//the backend framerate of the simulation, if gmods fps is lower than this, god knows what will happen
	int simFramerateMi = static_cast<int>(simFramerate * 1000.f);	//simulation framerate in milloseconds

	bool isFirst = true;	//for testing as of now

	particleBufferHost = static_cast<float4*>(malloc(sizeof(float4) * simMaxParticles));;

	while (simValid)
	{
		bufferMutex->lock();
		// If we do block--that could mean we were just shut down..
		// eg. the shutdown thread locked and exited
		if (!simValid) {
			bufferMutex->unlock();
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(simFramerateMi));

		// map buffers for reading / writing
		float4* particles = (float4*)NvFlexMap(particleBuffer, eNvFlexMapWait);
		float3* velocities = (float3*)NvFlexMap(velocityBuffer, eNvFlexMapWait);
		int* phases = (int*)NvFlexMap(phaseBuffer, eNvFlexMapWait);
		int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

		NvFlexCollisionGeometry* geometry = (NvFlexCollisionGeometry*)NvFlexMap(geometryBuffer, 0);
		float4* positions = (float4*)NvFlexMap(geoPosBuffer, 0);
		float4* rotations = (float4*)NvFlexMap(geoQuatBuffer, 0);
		int* flags = (int*)NvFlexMap(geoFlagsBuffer, 0);

		if (isFirst) {
			for (int i = 0; i < 100; i++) {
				float4 thisPos = float4{};
				phases[i] = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid);
				activeIndices[i] = i;

				thisPos.x = 0;
				thisPos.y = 0;
				thisPos.z = i * 10.f;
				thisPos.w = 1.f / 2.f;	//mass

				particles[i] = thisPos;
				velocities[i] = float3{i - 50.f, i - 50.f, 1.f};

				numParticles++;
			}

			isFirst = false;
		}

		//copy memory to be given to the client when requested
		memcpy(particleBufferHost, particles, sizeof(float4) * numParticles);	

		NvFlexUnmap(geometryBuffer);
		NvFlexUnmap(geoPosBuffer);
		NvFlexUnmap(geoQuatBuffer);
		NvFlexUnmap(geoFlagsBuffer);
		

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
		NvFlexSetShapes(flexSolver, geometryBuffer, geoPosBuffer, geoQuatBuffer, NULL, NULL, geoFlagsBuffer, propCount);	//last null is for colliders
		// tick
		
		//NvFlexSetParams(flexSolver, flexParams);	//not required
		NvFlexUpdateSolver(flexSolver, simFramerate * 5, 2, false);

		// read back (async)
		NvFlexGetParticles(flexSolver, particleBuffer, NULL);
		NvFlexGetVelocities(flexSolver, velocityBuffer, NULL);
		NvFlexGetPhases(flexSolver, phaseBuffer, NULL);

		bufferMutex->unlock();	//dont forget to unlock our buffer
	}

}