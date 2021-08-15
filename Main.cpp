#include "GarrysMod/Lua/Interface.h"
#include "GarrysMod/Lua/SourceCompat.h"
#include "types.h"
#include "Simulation.h"

using namespace GarrysMod::Lua;

Vector makeVec(float x, float y, float z) {
    Vector vec;
    vec.x = x;
    vec.y = y;
    vec.z = z;
    return vec;
}
//potato forced me to add this
#define ADD_GWATER_FUNC(funcName, tblName) LUA->PushCFunction(funcName); LUA->SetField(-2, tblName);

LUA_FUNCTION(GWaterUnpauseSim) {
    sim->isRunning = true;
    return 0;
}

LUA_FUNCTION(GWaterPauseSim) {
    sim->isRunning = false;
    return 0;
}

LUA_FUNCTION(GWaterDeleteSim) {
    if (!sim->isValid) return 0;

    sim->stopSimulation();
    return 0;
}

LUA_FUNCTION(GWaterParticleCount) {
    LUA->PushNumber(sim->count);
    return 1;
}

LUA_FUNCTION(GWaterGetParticleData) {
    int returncount = 0;

    if (!sim->isValid) return 0; //simulation doesnt exist, bail

    for (int i = 0; i < sim->count; ++i)
    {
        if (!sim->particles) continue;

        float4 pos = (float4)sim->particles[i];
        float3 vel = (float3)sim->velocities[i];

        Vector pos2 = makeVec(pos.x, pos.y, pos.z);
        Vector vel2 = makeVec(vel.x, vel.y, vel.z);

        //printLua(LUA, std::to_string(pos.y));
        //printLua(LUA, std::to_string(vel.z));

        LUA->PushVector(pos2);
        LUA->PushVector(vel2);
        returncount += 2;
    }

    //printLua(LUA, "IsValid: " + std::to_string(Simulation::isValid) + " | IsRunning: " + std::to_string(Simulation::isRunning) + " | Count: " + std::to_string(Simulation::count));
    return returncount;
}



GMOD_MODULE_OPEN() {

    initSimulation(sim);

    // push ALL c -> lua functions
    LUA->PushSpecial(SPECIAL_GLOB); //push _G

    LUA->CreateTable();

    ADD_GWATER_FUNC(GWaterUnpauseSim, "Unpause");
    ADD_GWATER_FUNC(GWaterPauseSim, "Pause");
    ADD_GWATER_FUNC(GWaterDeleteSim, "DeleteAll");
    ADD_GWATER_FUNC(GWaterGetParticleData, "GetData");

    LUA->SetField(-2, "gwater");
    LUA->Pop(); // pop _G


    

	return 0;
}


GMOD_MODULE_CLOSE() {


	return 0;
}