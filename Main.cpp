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

LUA_FUNCTION(GWaterInitSim) {
    initSimulation(sim);
}

LUA_FUNCTION(GWaterStartSim) {
    sim->isRunning = true;
    return 0;
}

LUA_FUNCTION(GWaterPauseSim) {
    sim->isRunning = false;
    return 0;
}

LUA_FUNCTION(GWaterStopSim) {
    if (!sim->isValid) return 0;

    sim->stopSimulation();
    return 0;
}

LUA_FUNCTION(GWaterIsRunning) {
    LUA->PushBool(sim->isRunning);
    return 1;
}

LUA_FUNCTION(GWaterIsValid) {
    LUA->PushBool(sim->isValid);
    return 1;
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


    // push ALL c -> lua functions
    LUA->PushSpecial(SPECIAL_GLOB); //push _G

    LUA->PushString("GWater_GetParticleData");
    LUA->PushCFunction(GWaterGetParticleData);
    LUA->SetTable(-3);

    // push particle data retriever
    //LUA->PushString("GWater_SpawnParticle");
    //LUA->PushCFunction(GWaterSpawnParticle);
    //LUA->SetTable(-3);

    // push simulation init
    LUA->PushString("GWater_InitSimulation");
    LUA->PushCFunction(GWaterInitSim);
    LUA->SetTable(-3);

    // push simulation starter
    LUA->PushString("GWater_StartSimulation");
    LUA->PushCFunction(GWaterStartSim);
    LUA->SetTable(-3);

    // push simulation pauser
    LUA->PushString("GWater_PauseSimulation");
    LUA->PushCFunction(GWaterPauseSim);
    LUA->SetTable(-3);

    // push simulation destroyer
    LUA->PushString("GWater_StopSimulation");
    LUA->PushCFunction(GWaterStopSim);
    LUA->SetTable(-3);

    // push simulation isrunning
    LUA->PushString("GWater_IsRunning");
    LUA->PushCFunction(GWaterIsRunning);
    LUA->SetTable(-3);

    // push simulation isvalid
    LUA->PushString("GWater_IsValid");
    LUA->PushCFunction(GWaterIsValid);
    LUA->SetTable(-3);

    // push simulation count retriever
    LUA->PushString("GWater_ParticleCount");
    LUA->PushCFunction(GWaterParticleCount);
    LUA->SetTable(-3);

    LUA->Pop(); //pop _G


    

	return 0;
}


GMOD_MODULE_CLOSE() {


	return 0;
}