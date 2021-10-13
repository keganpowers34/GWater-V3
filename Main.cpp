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

//tysm this was very useful for debugging
void gjelly_error(NvFlexErrorSeverity type, const char* msg, const char* file, int line) {
	printLua("FLEX ERROR:");
	printLua(msg);

}

//frees all FleX memory
void shutdownGWater(GarrysMod::Lua::ILuaBase* LUA) {
	simValid = false;

	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->PushNil();
	LUA->SetField(-2, "gwater"); 
	LUA->Pop(); // pop _G

	if (flexLibrary != nullptr) {
		bufferMutex->lock();

		delete flexParams;
		delete bufferMutex;

		free(particleBufferHost);

		flexLibrary = nullptr;

		bufferMutex->unlock();

	}


}

#define ADD_GWATER_FUNC(funcName, tblName) GlobalLUA->PushCFunction(funcName); GlobalLUA->SetField(-2, tblName);

//returns particle xyz data
LUA_FUNCTION(GetData) {
	LUA->CreateTable();

	//loop thru all particles & add to table (on stack)
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
	shutdownGWater(LUA);

	return 0;
}

LUA_FUNCTION(AddParticle) {
	//check to see if they are both vectors
	LUA->CheckType(-2, Type::Vector); // pos
	LUA->CheckType(-1, Type::Vector); // vel

	//gmod Vector and fleX float4
	Vector gmodPos = LUA->GetVector(-2);	//pos
	Vector gmodVel = LUA->GetVector(-1);	//vel

	//apply pos & vel to queue
	particleQueue.push_back(float3{ gmodPos.x, gmodPos.y, gmodPos.z });
	particleQueue.push_back(float3{ gmodVel.x, gmodVel.y, gmodVel.z });

	//remove vel and pos from stack
	LUA->Pop(2);	

	return 0;
}

//the world mesh
LUA_FUNCTION(AddWorldMesh) {

	LUA->CheckType(-3, Type::Vector); // Max
	LUA->CheckType(-2, Type::Vector); // Min
	LUA->CheckType(-1, Type::Table); // Sorted verts

	size_t tableLen = LUA->ObjLen();
	NvFlexBuffer* worldVerts = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(float4), eNvFlexBufferHost);
	NvFlexBuffer* worldIndices = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(int), eNvFlexBufferHost);

	float4* hostVerts = static_cast<float4*>(NvFlexMap(worldVerts, eNvFlexMapWait));
	int* hostIndices = static_cast<int*>(NvFlexMap(worldIndices, eNvFlexMapWait));

	//loop through map verticies
	for (int i = 0; i < tableLen; i++) {

		//lua is 1 indexed, C++ is 0 indexed
		LUA->PushNumber(i + 1);

		//gets data from table at the number ontop of the stack
		LUA->GetTable(-2);

		if (LUA->GetType(-1) == GarrysMod::Lua::Type::Nil) break;

		//returns vector at -1 stack pos
		Vector thisPos = LUA->GetVector();

		float4 vert;
		vert.x = thisPos.x;
		vert.y = thisPos.y;
		vert.z = thisPos.z;
		vert.w = 1.f / 2.f;

		hostVerts[i] = vert;
		hostIndices[i] = i;

		//counter clockwise -> clockwise triangle winding
		if (i % 2 == 0) {
			hostIndices[i - 1], hostIndices[i] = hostIndices[i], hostIndices[i - 1];
		}

		//remove the vector from the table
		LUA->Pop();
	}

	// Pop off the nil
	LUA->Pop();

	NvFlexUnmap(worldVerts);
	NvFlexUnmap(worldIndices);

	Vector minV = LUA->GetVector();
	Vector maxV = LUA->GetVector(-2);

	float minFloat[3] = { minV.x, minV.y, minV.z };
	float maxFloat[3] = { maxV.x, maxV.y, maxV.z };

	NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
	float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
	float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
	int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));

	// create the triangle mesh
	worldMesh = NvFlexCreateTriangleMesh(flexLibrary);
	NvFlexUpdateTriangleMesh(flexLibrary, worldMesh, worldVerts, worldIndices, tableLen, tableLen / 3, minFloat, maxFloat);

	// add triangle mesh
	flags[0] = NvFlexMakeShapeFlags(eNvFlexShapeTriangleMesh, false);
	geometry[0].triMesh.mesh = worldMesh;
	geometry[0].triMesh.scale[0] = 1.0f;
	geometry[0].triMesh.scale[1] = 1.0f;
	geometry[0].triMesh.scale[2] = 1.0f;
	positions[0] = float4{ 0.0f, 0.0f, -12000.f, 0.f}; //0,0,-12000 IS FLATGRASS PLANE POSITION!
	rotations[0] = float4{ 0.0f, 0.0f, 0.0f, 0.0f };

	//add sphere
	flags[1] = NvFlexMakeShapeFlags(eNvFlexShapeSphere, false);
	geometry[1].sphere.radius = 1000.f;
	positions[1] = float4{ 0.0f, 0.0f, -12288.f, 0.f };
	rotations[1] = float4{ 0.0f, 0.0f, 0.0f, 0.0f };

	// unmap buffers
	NvFlexUnmap(geometryBuffer);
	NvFlexUnmap(geoPosBuffer);
	NvFlexUnmap(geoQuatBuffer);
	NvFlexUnmap(geoFlagsBuffer);

	//world mesh and sphere
	propCount+=2;

	// send shapes to Flex
	NvFlexSetShapes(flexSolver, geometryBuffer, geoPosBuffer, geoQuatBuffer, NULL, NULL, geoFlagsBuffer, propCount);

	printLua("[GWATER]: Added world mesh");

	return 0;

}

GMOD_MODULE_OPEN()
{
	GlobalLUA = LUA;
	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->CreateTable();
	ADD_GWATER_FUNC(GetData, "GetData");
	ADD_GWATER_FUNC(DeleteSimulation, "DeleteSimulation");
	ADD_GWATER_FUNC(AddWorldMesh, "AddWorldMesh");
	ADD_GWATER_FUNC(AddParticle, "SpawnParticle");

	LUA->SetField(-2, "gwater");
	LUA->Pop(); //remove _G

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
	geoPosBuffer = NvFlexAllocBuffer(flexLibrary, 30, sizeof(float3), eNvFlexBufferHost);
	geoFlagsBuffer = NvFlexAllocBuffer(flexLibrary, 30, sizeof(int), eNvFlexBufferHost);
	geoQuatBuffer = NvFlexAllocBuffer(flexLibrary, 30, sizeof(float4), eNvFlexBufferHost);

	// Host buffer
	particleBufferHost = static_cast<float4*>(malloc(sizeof(float4) * solverDesc.maxParticles));

	//create buffer for the thread
	bufferMutex = new std::mutex();

	// Launch our flex solver thread
	std::thread flexSolverThread(flexSolveThread);
	flexSolverThread.detach();

	return 0;

}

// Called when the module is unloaded
GMOD_MODULE_CLOSE()
{
	shutdownGWater(LUA);

	return 0;

}