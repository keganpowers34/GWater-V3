#include "declarations.h"
#include <string>

using namespace GarrysMod::Lua;

//overloaded printlua func
void printLua(std::string text)
{
	GlobalLUA->PushSpecial(SPECIAL_GLOB);
	GlobalLUA->GetField(-1, "print");
	GlobalLUA->PushString(text.c_str());
	GlobalLUA->Call(1, 0);
	GlobalLUA->Pop();
}
void printLua(char* text)
{
	GlobalLUA->PushSpecial(SPECIAL_GLOB);
	GlobalLUA->GetField(-1, "print");
	GlobalLUA->PushString(text);
	GlobalLUA->Call(1, 0);
	GlobalLUA->Pop();
}



//potato code
void gjelly_error(NvFlexErrorSeverity type, const char* msg, const char* file, int line) {
	printLua("OH NO, FLEX ERROR!!!");
	printLua(file);
	printLua(std::to_string(line));
	//printLua(std::to_string(type));
	printLua(msg);
}

void shutdownGWater() {
	simValid = false;

	GlobalLUA->PushSpecial(SPECIAL_GLOB);
	GlobalLUA->PushNil();
	GlobalLUA->SetField(-2, "gwater"); // Simply set the gjelly table to nil
	GlobalLUA->Pop(); // Pop off the global env

	if (flexLibrary != nullptr) {
		bufferMutex->lock();

		// yeesh, to-do: make buffers fall inside a map (maybe?)
		/*NvFlexFreeBuffer(particleBuffer);
		NvFlexFreeBuffer(velocityBuffer);
		NvFlexFreeBuffer(phaseBuffer);
		NvFlexFreeBuffer(activeBuffer);
		NvFlexFreeBuffer(lengthsBuffer);
		NvFlexFreeBuffer(indicesBuffer);
		NvFlexFreeBuffer(coefficientsBuffer);
		NvFlexFreeBuffer(geometryBuffer);
		NvFlexFreeBuffer(geoPosBuffer);
		NvFlexFreeBuffer(geoQuatBuffer);
		NvFlexFreeBuffer(geoFlagsBuffer);*/

		/*for (Prop thisProp : props) {
			NvFlexFreeBuffer(thisProp.verts);
			NvFlexFreeBuffer(thisProp.indices);

			NvFlexDestroyTriangleMesh(flexLib, thisProp.meshId);
		}*/

		NvFlexDestroySolver(flexSolver);
		NvFlexShutdown(flexLibrary);

		delete flexParams;
		delete bufferMutex;

		free(particleBufferHost);

		flexLibrary = nullptr;

		bufferMutex->unlock();

	}


}

#define ADD_GWATER_FUNC(funcName, tblName) GlobalLUA->PushCFunction(funcName); GlobalLUA->SetField(-2, tblName);


LUA_FUNCTION(GetData) {
	// Returns a table with all of the particles
	LUA->CreateTable();
	//printLua(std::to_string(numParticles));
	for (int i = 0; i < numParticles; i++) {
		LUA->PushNumber(i + 1);

		float4 thisPos = particleBufferHost[i];
		Vector gmodPos;
		gmodPos.x = thisPos.x;
		gmodPos.y = thisPos.y;
		gmodPos.z = thisPos.z;

		LUA->PushVector(gmodPos);
		LUA->SetTable(-3);
	}

	return 1;
}

//stops simulation
LUA_FUNCTION(DeleteSimulation) {
	shutdownGWater();

	return 0;
}

//the world mesh
LUA_FUNCTION(AddWorldMesh) {

	LUA->CheckType(-3, Type::Vector); // Max
	LUA->CheckType(-2, Type::Vector); // Min
	LUA->CheckType(-1, Type::Table); // Sorted verts

	size_t tableLen = LUA->ObjLen();
	printLua(std::to_string(tableLen));
	NvFlexBuffer* worldVerts = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(float4), eNvFlexBufferHost);
	NvFlexBuffer* worldIndices = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(int), eNvFlexBufferHost);

	float4* hostVerts = (float4*)NvFlexMap(worldVerts, eNvFlexMapWait);
	int* hostIndices = (int*)NvFlexMap(worldIndices, eNvFlexMapWait);

	for (int i = 0; i < tableLen; i++) {
		int actualIndex = i + 1;
		float4 vert;

		// Push our target index to the stack.
		LUA->PushNumber(actualIndex);
		// Get the table data at this index (and not get the table, which is what I thought this did.)
		LUA->GetTable(-2);
		// Check for the sentinel nil element.
		if (LUA->GetType(-1) == GarrysMod::Lua::Type::Nil) break;
		// Get it's value.
		Vector thisPos = LUA->GetVector();
		vert.x = thisPos.x*10;
		vert.y = thisPos.y*10;
		vert.z = thisPos.z*10;
		vert.w = 1.f / 2.f;

		printLua(std::to_string(vert.x) + "," + std::to_string(vert.y) + "," + std::to_string(vert.z));

		hostVerts[i] = vert;
		hostIndices[i] = i;

		// CCW -> CW triangle winding
		if (i % 2 == 0) {
			hostIndices[i - 1], hostIndices[i] = hostIndices[i], hostIndices[i - 1];
		}

		// Pop it off again.
		LUA->Pop(1);
	}

	// Pop off the nil
	LUA->Pop();

	// Ok, we're done, lets get our mesh id (and our min max)
	NvFlexUnmap(worldVerts);
	NvFlexUnmap(worldIndices);

	Vector minV = LUA->GetVector();
	Vector maxV = LUA->GetVector(-2);

	float minFloat[3] = { minV.x, minV.y, minV.z };
	float maxFloat[3] = { maxV.x, maxV.y, maxV.z };

	NvFlexTriangleMeshId worldMeshID = NvFlexCreateTriangleMesh(flexLibrary);

	NvFlexUpdateTriangleMesh(flexLibrary, worldMeshID, worldVerts, worldIndices, tableLen, tableLen / 3, minFloat, maxFloat);

	propCount++;

	//LUA->PushNumber(static_cast<double>(tableLen)); // The ID to the collider
	return 0;
}

GMOD_MODULE_OPEN()
{
	GlobalLUA = LUA;
	//FILE* pFile = nullptr;

	//freopen_s(&pFile, "CONOUT$", "w", stdout); // cursed way to redirect stdout to our own console
	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->CreateTable();
	ADD_GWATER_FUNC(GetData, "GetData");
	ADD_GWATER_FUNC(DeleteSimulation, "DeleteSimulation");
	ADD_GWATER_FUNC(AddWorldMesh, "AddWorldMesh");
	//ADD_GWATER_FUNC(SetColliderPos, "SetColliderPos");
	//ADD_GWATER_FUNC(SetColliderQuat, "SetColliderQuat");

	LUA->SetField(-2, "gwater");
	LUA->Pop(); // Remove global env

	// Initialize FleX
	flexLibrary = NvFlexInit(120, gjelly_error);

	NvFlexSolverDesc solverDesc;
	NvFlexSetSolverDescDefaults(&solverDesc);
	solverDesc.maxParticles = 65536;
	solverDesc.maxDiffuseParticles = 0;

	flexSolver = NvFlexCreateSolver(flexLibrary, &solverDesc);

	flexParams = new NvFlexParams();

	initParams(flexParams);

	NvFlexSetParams(flexSolver, flexParams);

	// Create buffers
	particleBuffer = NvFlexAllocBuffer(flexLibrary, solverDesc.maxParticles, sizeof(float4), eNvFlexBufferHost);
	velocityBuffer = NvFlexAllocBuffer(flexLibrary, solverDesc.maxParticles, sizeof(float3), eNvFlexBufferHost);
	phaseBuffer = NvFlexAllocBuffer(flexLibrary, solverDesc.maxParticles, sizeof(int), eNvFlexBufferHost);
	activeBuffer = NvFlexAllocBuffer(flexLibrary, solverDesc.maxParticles, sizeof(int), eNvFlexBufferHost);

	// Geometry buffers 
	geometryBuffer = NvFlexAllocBuffer(flexLibrary, 30, sizeof(NvFlexCollisionGeometry), eNvFlexBufferHost);
	geoPosBuffer = NvFlexAllocBuffer(flexLibrary, 30, sizeof(float4), eNvFlexBufferHost);
	geoFlagsBuffer = NvFlexAllocBuffer(flexLibrary, 30, sizeof(int), eNvFlexBufferHost);
	geoQuatBuffer = NvFlexAllocBuffer(flexLibrary, 30, sizeof(float4), eNvFlexBufferHost);

	// SO M ANY BUFF ER S
	indicesBuffer = NvFlexAllocBuffer(flexLibrary, 4, sizeof(int), eNvFlexBufferHost);
	lengthsBuffer = NvFlexAllocBuffer(flexLibrary, 2, sizeof(float), eNvFlexBufferHost);
	coefficientsBuffer = NvFlexAllocBuffer(flexLibrary, 2, sizeof(float), eNvFlexBufferHost);

	// Host buffer
	particleBufferHost = static_cast<float4*>(malloc(sizeof(float4) * solverDesc.maxParticles));

	bufferMutex = new std::mutex();

	// Launch our flex solver thread
	//done = false;
	std::thread flexSolverThread(flexSolveThread);

	flexSolverThread.detach(); // BE FREE

	return 0;
}

// Called when the module is unloaded
GMOD_MODULE_CLOSE()
{
	shutdownGWater();

	return 0;
}