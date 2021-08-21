#include "types.h"
#include "Simulation.h"

using namespace GarrysMod::Lua;

//potato forced me to add this
#define ADD_GWATER_FUNC(funcName, tblName) LUA->PushCFunction(funcName); LUA->SetField(-2, tblName);

LUA_FUNCTION(GWaterInitSim) {
	initSimulation();
	return 0;
}

LUA_FUNCTION(GWaterUnpauseSim) {
	simIsRunning = true;
	return 0;
}

LUA_FUNCTION(GWaterPauseSim) {
	simIsRunning = false;
	return 0;
}

LUA_FUNCTION(GWaterDeleteSim) {
	if (!simIsValid) return 0;

	simStopSimulation();
	return 0;
}

LUA_FUNCTION(GWaterParticleCount) {
	LUA->PushNumber(simCount);
	return 1;
}

//potatoos function
LUA_FUNCTION(GWaterGetParticleData) {

	LUA->CreateTable();

	for (int i = 0; i < simCount; i++) {
		LUA->PushNumber(static_cast<double>(i) + 1); //double cast to avoid C26451 arithmetic overflow
		//add one because lua is 1 indexed

		float4 thisPos = simParticles[i];
		Vector gmodPos;
		gmodPos.x = thisPos.x;
		gmodPos.y = thisPos.y;
		gmodPos.z = thisPos.z;

		LUA->PushVector(gmodPos);
		LUA->SetTable(-3);
	}

	return 1;

}

LUA_FUNCTION(GWaterSpawnParticle) {
	if (!simIsValid) return 0;

	Vector pos = LUA->GetVector(-2);
	Vector vel = LUA->GetVector(-1);

	simAddParticle(float4{ pos.x, pos.y, pos.z, 1.f / 2.f }, float3{ vel.x, vel.y, vel.z }, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid));

	return 0;
}

LUA_FUNCTION(GWaterMakeWaterCube) {
	if (!simIsValid) return 0;

	Vector center = LUA->GetVector(-2);
	Vector size = LUA->GetVector(-1);

	simAddCube(float3{ center.x, center.y, center.z }, float3{ size.x, size.y, size.z }, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid));

	return 0;
}

LUA_FUNCTION(GWaterSetRadius) {
	if (!simIsValid) return 0;

	float r = static_cast<float>(LUA->GetNumber(-1));
	simSetRadius(r);

	return 0;
}

LUA_FUNCTION(GWaterUpdateParams) {
	if (!simIsValid) return 0;

	sim_params.adhesion = (float)LUA->GetNumber(-4);
	sim_params.cohesion = (float)LUA->GetNumber(-3);
	sim_params.vorticityConfinement = (float)LUA->GetNumber(-2);
	sim_params.viscosity = (float)LUA->GetNumber(-1);

	return 0;
}

LUA_FUNCTION(AddWorldMesh) {

	LUA->CheckType(-3, Type::Vector); // Max
	LUA->CheckType(-2, Type::Vector); // Min
	LUA->CheckType(-1, Type::Table); // Sorted verts

	size_t tableLen = LUA->ObjLen();
	NvFlexBuffer* worldVerts = NvFlexAllocBuffer(flex_library, tableLen, sizeof(float4), eNvFlexBufferHost);
	NvFlexBuffer* worldIndices = NvFlexAllocBuffer(flex_library, tableLen, sizeof(int), eNvFlexBufferHost);

	float4* hostVerts = (float4*)NvFlexMap(worldVerts, eNvFlexMapWait);
	int* hostIndices = (int*)NvFlexMap(worldIndices, eNvFlexMapWait);

	for (int i = 0; i < tableLen; i++) {
		float4 vert;

		// Push our target index to the stack.
		//add 1 because lua is 1 indexed
		LUA->PushNumber(i + 1);
		// Get the table data at this index (and not get the table, which is what I thought this did.)
		LUA->GetTable(-2);
		// Check for the sentinel nil element.
		if (LUA->GetType(-1) == Type::Nil) break;

		Vector thisPos = LUA->GetVector();
		vert.x = thisPos.x;
		vert.y = thisPos.y;
		vert.z = thisPos.z;
		vert.w = 1.f / 2.f;

		hostVerts[i] = vert;
		hostIndices[i] = i;

		// Pop it off again.
		LUA->Pop(1);
	}

	// Pop off the nil

	LUA->Pop();

	// Ok, we're done, lets get our mesh id (and our min max)

	NvFlexUnmap(worldVerts);
	NvFlexUnmap(worldIndices);

	worldMeshId = NvFlexCreateTriangleMesh(flex_library);

	Vector minV = LUA->GetVector(-1);
	Vector maxV = LUA->GetVector(-2);

	float minFloat[3] = { minV.x, minV.y, minV.z };
	float maxFloat[3] = { maxV.x, maxV.y, maxV.z };

	NvFlexUpdateTriangleMesh(flex_library, worldMeshId, worldVerts, worldIndices, tableLen, tableLen / 3, minFloat, maxFloat);

	//map world collider
	simAddWorld();

	return 0;
}



GMOD_MODULE_OPEN() {
	printf("gWater v1.5");
	GlobalLUA = LUA;
	// push ALL c -> lua functions
	LUA->PushSpecial(SPECIAL_GLOB); //push _G

	LUA->CreateTable();

	ADD_GWATER_FUNC(GWaterInitSim, "Initialize");
	ADD_GWATER_FUNC(GWaterUnpauseSim, "Unpause");
	ADD_GWATER_FUNC(GWaterPauseSim, "Pause");
	ADD_GWATER_FUNC(GWaterDeleteSim, "DeleteSimulation");
	ADD_GWATER_FUNC(GWaterGetParticleData, "GetData");
	ADD_GWATER_FUNC(GWaterSpawnParticle, "SpawnParticle");
	ADD_GWATER_FUNC(GWaterMakeWaterCube, "SpawnCube");
	ADD_GWATER_FUNC(GWaterSetRadius, "SetRadius");
	ADD_GWATER_FUNC(GWaterUpdateParams, "UpdateParams");
	ADD_GWATER_FUNC(AddWorldMesh, "AddWorldMesh");

	LUA->SetField(-2, "gwater");
	LUA->Pop(); // pop _G


	

	return 0;
}


GMOD_MODULE_CLOSE() {
	simIsValid = false;
	simIsRunning = false;
	simCount = 0;

	return 0;
}