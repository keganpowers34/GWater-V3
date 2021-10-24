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
	initParams(flexLib->flexParams, static_cast<float>(LUA->GetNumber()));
	NvFlexSetParams(flexLib->flexSolver, flexLib->flexParams);
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

	//add data to PROP & generate mesh
	Prop newProp = Prop{};
	flexLib->calcMesh(LUA, minFloat, maxFloat, LUA->ObjLen(), &newProp);
	props.push_back(newProp);

	//other
	propCount += 1;
	printLua("[GWATER]: Added mesh " + std::to_string(propCount));
	LUA->PushNumber(propCount - 1);	//id of mesh

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

	LUA->Pop(4);	//pop id, pos, ang and angw
	return 0;

}

/*
LUA_FUNCTION(RemoveMesh) {

	LUA->CheckType(-1, Type::Number); // ID
	int id = static_cast<int>(LUA->GetNumber());

	for (int i = 0; i < props.size(); i++) {
		if (props[i].ID == id) {

			NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
			float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
			float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
			int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));

			//memset(positions[i], 0, 16);

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
*/



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

	// Initialize FleX api class
	flexLib = std::make_shared<flexAPI>();

	return 0;

}

// Called when the module is unloaded
GMOD_MODULE_CLOSE()
{
	//remove flexLib;

	return 0;

}