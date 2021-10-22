#include "declarations.h"
#include <string>

using namespace GarrysMod::Lua;

#define MAX_COLLIDERS 1000

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

		NvFlexFreeBuffer(particleBuffer);
		NvFlexFreeBuffer(velocityBuffer);
		NvFlexFreeBuffer(phaseBuffer);
		NvFlexFreeBuffer(activeBuffer);
		NvFlexFreeBuffer(geometryBuffer);
		NvFlexFreeBuffer(geoPosBuffer);
		NvFlexFreeBuffer(geoQuatBuffer);
		NvFlexFreeBuffer(geoFlagsBuffer);

		delete flexParams;

		free(particleBufferHost);

		flexLibrary = nullptr;

		bufferMutex->unlock();

		delete bufferMutex;

	}


}

void initFleX() {
	flexLibrary = NvFlexInit(120, gjelly_error);

	NvFlexSetSolverDescDefaults(&flexSolverDesc);
	flexSolverDesc.maxParticles = 65536;
	flexSolverDesc.maxDiffuseParticles = 0;

	flexParams = new NvFlexParams();
	initParams(flexParams, 10.f);

	flexSolver = NvFlexCreateSolver(flexLibrary, &flexSolverDesc);
	NvFlexSetParams(flexSolver, flexParams);

}

//generates mesh
NvFlexTriangleMeshId calcMesh(GarrysMod::Lua::ILuaBase* LUA, const float* minFloat, const float* maxFloat, size_t tableLen, Prop* p) {

	p->verts = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(float4), eNvFlexBufferHost);
	p->indices = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(int), eNvFlexBufferHost);

	float4* hostVerts = static_cast<float4*>(NvFlexMap(p->verts, eNvFlexMapWait));
	int* hostIndices = static_cast<int*>(NvFlexMap(p->indices, eNvFlexMapWait));

	//loop through map verticies
	for (int i = 0; i < tableLen; i++) {

		//lua is 1 indexed, C++ is 0 indexed
		LUA->PushNumber(i + 1);

		//gets data from table at the number ontop of the stack
		LUA->GetTable(-2);

		//returns vector at -1 stack pos
		Vector thisPos = LUA->GetVector();

		float4 vert;
		vert.x = thisPos.x;
		vert.y = thisPos.y;
		vert.z = thisPos.z;
		vert.w = 1.f;

		hostVerts[i] = vert;
		hostIndices[i] = i;

		//counter clockwise -> clockwise triangle winding
		if (i % 3 == 1) {
			int host = hostIndices[i];
			hostIndices[i] = hostIndices[i - 1];
			hostIndices[i - 1] = host;

		}

		//remove the vector from the table
		LUA->Pop();
	}

	// Pop off the table
	LUA->Pop();

	NvFlexUnmap(p->verts);
	NvFlexUnmap(p->indices);

	// create the triangle mesh
	NvFlexTriangleMeshId generatedMesh = NvFlexCreateTriangleMesh(flexLibrary);
	NvFlexUpdateTriangleMesh(flexLibrary, generatedMesh, p->verts, p->indices, tableLen, tableLen / 3, minFloat, maxFloat);

	return generatedMesh;

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

	LUA->PushNumber(static_cast<double>(numParticles));

	return 2;
}

LUA_FUNCTION(RemoveAllParticles) {
	flexRemoveQueue = true;
	return 0;

}

LUA_FUNCTION(SetRadius) {
	LUA->CheckType(-1, Type::Number);
	initParams(flexParams, static_cast<float>(LUA->GetNumber()));
	NvFlexSetParams(flexSolver, flexParams);
	return 0;

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
	Particle particle = { float4{ gmodPos.x, gmodPos.y, gmodPos.z , 0.5}, float3{gmodVel.x, gmodVel.y, gmodVel.z} };
	particleQueue.push_back(particle);

	//remove vel and pos from stack
	LUA->Pop(2);	

	return 0;
}

//the world mesh
LUA_FUNCTION(AddMesh) {

	LUA->CheckType(-1, Type::Vector); // Max
	LUA->CheckType(-2, Type::Vector); // Min
	LUA->CheckType(-3, Type::Table);  // Sorted verts

	//obbminmax
	Vector maxV = LUA->GetVector();
	Vector minV = LUA->GetVector(-2);
	float minFloat[3] = { minV.x, minV.y, minV.z };
	float maxFloat[3] = { maxV.x, maxV.y, maxV.z };
	LUA->Pop(2); //pop off min & max

	//create prop & generate mesh
	Prop newProp = Prop{};
	newProp.meshID = calcMesh(LUA, minFloat, maxFloat, LUA->ObjLen(), &newProp);
	newProp.pos = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
	newProp.ang = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
	newProp.lastPos = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
	newProp.lastAng = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
	newProp.ID = propCount;
	props.push_back(newProp);

	//map buffers
	NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
	float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
	float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
	float4* prevPositions = static_cast<float4*>(NvFlexMap(geoPrevPosBuffer, 0));
	float4* prevRotations = static_cast<float4*>(NvFlexMap(geoPrevQuatBuffer, 0));
	int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));

	// add triangle mesh
	flags[propCount] = NvFlexMakeShapeFlags(eNvFlexShapeTriangleMesh, propCount != 0);	//index 0 is ALWAYS the world
	geometry[propCount].triMesh.mesh = newProp.meshID;
	geometry[propCount].triMesh.scale[0] = 1.0f;
	geometry[propCount].triMesh.scale[1] = 1.0f;
	geometry[propCount].triMesh.scale[2] = 1.0f;
	positions[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.0f }; 
	rotations[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.01f };	//NEVER SET ROTATION TO 0,0,0,0, FLEX *HATES* IT!

	prevPositions[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
	prevRotations[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.01f };

	// unmap buffers

	NvFlexUnmap(geometryBuffer);
	NvFlexUnmap(geoPosBuffer);
	NvFlexUnmap(geoQuatBuffer);
	NvFlexUnmap(geoPrevPosBuffer);
	NvFlexUnmap(geoPrevQuatBuffer);
	NvFlexUnmap(geoFlagsBuffer);

	//other
	propCount += 1;

	printLua("[GWATER]: Added mesh " + std::to_string(propCount));

	LUA->PushNumber(propCount - 1);
	return 1;

}

LUA_FUNCTION(SetMeshPos) {

	LUA->CheckType(-1, Type::Number); // ID
	LUA->CheckType(-2, Type::Vector); // Pos

	LUA->CheckType(-3, Type::Vector); // Ang xyz
	LUA->CheckType(-4, Type::Number); // Ang w

	int id = static_cast<int>(LUA->GetNumber());
	Vector gmodPos = LUA->GetVector(-2);
	Vector gmodAng = LUA->GetVector(-3);
	float gmodAngZ = static_cast<float>(LUA->GetNumber(-4));

	//feels kinda hacky, any ideas andrew?
	for (Prop& prop : props) {
		if (prop.ID == id) {
			prop.lastPos = float4{ gmodPos.x, gmodPos.y, gmodPos.z, 1.f / 50000.f };
			prop.lastAng = float4{ gmodAng.x, gmodAng.y, gmodAng.z, gmodAngZ };
			break;

		}

	}

	LUA->Pop(4);	//pop id, pos
	return 0;

}


LUA_FUNCTION(RemoveMesh) {

	LUA->CheckType(-1, Type::Number); // ID
	int id = static_cast<int>(LUA->GetNumber());

	for (int i = 0; i < props.size(); i++) {
		if (props[i].ID == id) {

			NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
			float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
			float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
			int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));

			//positions[i] = NULL;

			NvFlexUnmap(geometryBuffer);
			NvFlexUnmap(geoPosBuffer);
			NvFlexUnmap(geoQuatBuffer);
			NvFlexUnmap(geoFlagsBuffer);
			props.erase(props.begin() + i, props.begin() + i + 1);
			break;

		}

	}

	LUA->Pop(1);	//pop id, pos
	return 0;

}




GMOD_MODULE_OPEN()
{
	GlobalLUA = LUA;
	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->CreateTable();
	ADD_GWATER_FUNC(GetData, "GetData");
	ADD_GWATER_FUNC(DeleteSimulation, "DeleteSimulation");
	ADD_GWATER_FUNC(AddMesh, "AddMesh");
	ADD_GWATER_FUNC(AddParticle, "SpawnParticle");
	ADD_GWATER_FUNC(RemoveAllParticles, "RemoveAll");
	ADD_GWATER_FUNC(SetRadius, "SetRadius");
	ADD_GWATER_FUNC(SetMeshPos, "SetMeshPos");

	LUA->SetField(-2, "gwater");
	LUA->Pop(); //remove _G

	// Initialize FleX
	initFleX();

	// Create buffers
	particleBuffer = NvFlexAllocBuffer(flexLibrary, flexSolverDesc.maxParticles, sizeof(float4), eNvFlexBufferHost);
	velocityBuffer = NvFlexAllocBuffer(flexLibrary, flexSolverDesc.maxParticles, sizeof(float3), eNvFlexBufferHost);
	phaseBuffer = NvFlexAllocBuffer(flexLibrary, flexSolverDesc.maxParticles, sizeof(int), eNvFlexBufferHost);
	activeBuffer = NvFlexAllocBuffer(flexLibrary, flexSolverDesc.maxParticles, sizeof(int), eNvFlexBufferHost);

	// Geometry buffers 
	geometryBuffer = NvFlexAllocBuffer(flexLibrary, MAX_COLLIDERS, sizeof(NvFlexCollisionGeometry), eNvFlexBufferHost);
	geoPosBuffer = NvFlexAllocBuffer(flexLibrary, MAX_COLLIDERS, sizeof(float4), eNvFlexBufferHost);
	geoFlagsBuffer = NvFlexAllocBuffer(flexLibrary, MAX_COLLIDERS, sizeof(int), eNvFlexBufferHost);
	geoQuatBuffer = NvFlexAllocBuffer(flexLibrary, MAX_COLLIDERS, sizeof(float4), eNvFlexBufferHost);

	geoPrevPosBuffer = NvFlexAllocBuffer(flexLibrary, MAX_COLLIDERS, sizeof(float4), eNvFlexBufferHost);
	geoPrevQuatBuffer = NvFlexAllocBuffer(flexLibrary, MAX_COLLIDERS, sizeof(float4), eNvFlexBufferHost);

	// Host buffer
	particleBufferHost = static_cast<float4*>(malloc(sizeof(float4) * flexSolverDesc.maxParticles));

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