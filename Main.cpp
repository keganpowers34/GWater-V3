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


#define ADD_GWATER_FUNC(funcName, tblName) GlobalLUA->PushCFunction(funcName); GlobalLUA->SetField(-2, tblName);



float dot(float3 a, float3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float distance2(float4 a, float3 b) {
	float x = b.x - a.x;
	float y = b.y - a.y;
	float z = b.z - a.z;

	return (x * x + y * y + z * z);
}

float3 subtractFloat34(float4 a, float3 b) {
	return float3{ a.x - b.x, a.y - b.y, a.z - b.z };
}

//returns particle xyz data
LUA_FUNCTION(GetData) {

	LUA->CheckType(-1, Type::Vector);
	LUA->CheckType(-2, Type::Vector);

	Vector gmodDir = LUA->GetVector();
	float3 dir = float3{ gmodDir.x, gmodDir.y, gmodDir.z };

	Vector gmodPos = LUA->GetVector(-2);
	float3 pos = float3{ gmodPos.x, gmodPos.y, gmodPos.z };

	LUA->Pop(2);

	LUA->CreateTable();

	//loop thru all particles & add to table (on stack)
	int addedIndex = 0;
	for (int i = 0; i < numParticles; i++) {
		float4 thisPos = particleBufferHost[i];

		if (dot(subtractFloat34(thisPos, pos), dir) < 0 || distance2(thisPos, pos) > 25000000) {
			continue;
		}

		Vector gmodPos;
		gmodPos.x = thisPos.x;
		gmodPos.y = thisPos.y;
		gmodPos.z = thisPos.z;

		addedIndex++;
		LUA->PushNumber(addedIndex);
		LUA->PushVector(gmodPos);
		LUA->SetTable(-3);
	}

	LUA->PushNumber(static_cast<double>(addedIndex));

	return 2;
}

LUA_FUNCTION(RemoveAllParticles) {
	flexRemoveQueue = true;
	return 0;

}

LUA_FUNCTION(SetRadius) {
	LUA->CheckType(-1, Type::Number);
	flexLib->initParams(static_cast<float>(LUA->GetNumber()));
	return 0;

}

//stops simulation (doesnt work as of now!)
LUA_FUNCTION(DeleteSimulation) {
	//shutdownGWater(LUA);
	return 0;
}

LUA_FUNCTION(AddParticle) {
	//check to see if they are both vectors
	LUA->CheckType(-2, Type::Vector); // pos
	LUA->CheckType(-1, Type::Vector); // vel

	//gmod Vector and fleX float4
	Vector gmodPos = LUA->GetVector(-2);	//pos
	Vector gmodVel = LUA->GetVector(-1);	//vel

	flexLib->addParticle(gmodPos, gmodVel);

	//remove vel and pos from stack
	LUA->Pop(2);	

	return 0;
}

//the world mesh
LUA_FUNCTION(AddConvexMesh) {

	LUA->CheckType(-1, Type::Vector); // Max
	LUA->CheckType(-2, Type::Vector); // Min
	LUA->CheckType(-3, Type::Table);  // Sorted verts

	//obbminmax
	Vector maxV = LUA->GetVector();
	Vector minV = LUA->GetVector(-2);
	float minFloat[3] = { minV.x, minV.y, minV.z };
	float maxFloat[3] = { maxV.x, maxV.y, maxV.z };
	LUA->Pop(2); //pop off min & max

	//lock buffer
	bufferMutex->lock();

	size_t len = LUA->ObjLen();

	//check to make sure the mesh is even valid (error models)
	if (len < 1 || len % 3 != 0) {
		bufferMutex->unlock();
		printLua("[GWATER]: Invalid mesh given");
		LUA->PushBool(false);
		return 1;
	}

	if (len / 3 <= 64) {
		flexLib->addMeshConvex(LUA, minFloat, maxFloat, LUA->ObjLen());
		printLua("[GWATER]: Added convex mesh " + std::to_string(propCount));
	}
	else {
		flexLib->addMeshConcave(LUA, minFloat, maxFloat, LUA->ObjLen());
		printLua("[GWATER]: Too many tris for mesh " + std::to_string(propCount) + ", adding as concave mesh!");
	}

	bufferMutex->unlock();

	//other
	LUA->PushBool(true);

	return 1;

}


LUA_FUNCTION(AddConcaveMesh) {

	LUA->CheckType(-1, Type::Vector); // Max
	LUA->CheckType(-2, Type::Vector); // Min
	LUA->CheckType(-3, Type::Table);  // Sorted verts

	//obbminmax
	Vector maxV = LUA->GetVector();
	Vector minV = LUA->GetVector(-2);
	float minFloat[3] = { minV.x, minV.y, minV.z };
	float maxFloat[3] = { maxV.x, maxV.y, maxV.z };
	LUA->Pop(2); //pop off min & max

	//lock buffer
	bufferMutex->lock();

	size_t len = LUA->ObjLen();

	//check to make sure the mesh is even valid (error models)
	if (len < 1 || len % 3 != 0) {
		bufferMutex->unlock();
		printLua("[GWATER]: Invalid mesh given");
		LUA->PushBool(false);
		return 1;
	}

	flexLib->addMeshConcave(LUA, minFloat, maxFloat, LUA->ObjLen());
	printLua("[GWATER]: Added concave mesh " + std::to_string(propCount));


	bufferMutex->unlock();

	//other
	LUA->PushBool(true);

	return 1;

}

LUA_FUNCTION(SetMeshPos) {

	LUA->CheckType(-1, Type::Number); // ID
	LUA->CheckType(-2, Type::Vector); // Pos

	LUA->CheckType(-3, Type::Vector); // Ang xyz
	LUA->CheckType(-4, Type::Number); // Ang w

	int id = static_cast<int>(LUA->GetNumber());	//lua is 1 indexed
	Vector gmodPos = LUA->GetVector(-2);
	Vector gmodAng = LUA->GetVector(-3);
	float gmodAngW = static_cast<float>(LUA->GetNumber(-4));

	flexLib->updateMeshPos(float4{ gmodPos.x, gmodPos.y, gmodPos.z, 1.f / 50000.f }, float4{ gmodAng.x, gmodAng.y, gmodAng.z, gmodAngW }, id);

	LUA->Pop(4);	//pop id, pos, ang and angw
	return 0;

}


LUA_FUNCTION(RemoveMesh) {

	LUA->CheckType(-1, Type::Number); // ID
	int id = static_cast<int>(LUA->GetNumber());	

	bufferMutex->lock();

	flexLib->freeProp(id);

	bufferMutex->unlock();

	LUA->Pop();	//pop id

	return 0;

}




GMOD_MODULE_OPEN()
{
	GlobalLUA = LUA;
	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->CreateTable();
	ADD_GWATER_FUNC(GetData, "GetData");
	ADD_GWATER_FUNC(DeleteSimulation, "DeleteSimulation");
	ADD_GWATER_FUNC(AddConvexMesh, "AddConvexMesh");
	ADD_GWATER_FUNC(AddConcaveMesh, "AddConcaveMesh");
	ADD_GWATER_FUNC(AddParticle, "SpawnParticle");
	ADD_GWATER_FUNC(RemoveAllParticles, "RemoveAll");
	ADD_GWATER_FUNC(SetMeshPos, "SetMeshPos");
	ADD_GWATER_FUNC(RemoveMesh, "RemoveMesh");

	//param funcs
	ADD_GWATER_FUNC(SetRadius, "SetRadius");

	LUA->SetField(-2, "gwater");
	LUA->Pop(); //remove _G

	// Initialize FleX api class
	flexLib = std::make_shared<flexAPI>();

	return 0;

}

// Called when the module is unloaded
GMOD_MODULE_CLOSE()
{

	flexLib.reset();

	//set .gwater to nil
	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->PushNil();
	LUA->SetField(-2, "gwater");
	LUA->Pop(); // pop _G

	return 0;

}