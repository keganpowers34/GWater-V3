#include "GarrysMod/Lua/Interface.h"
#include "GarrysMod/Lua/SourceCompat.h"
#include "types.h"
#include "Simulation.h"

using namespace GarrysMod::Lua;

//Simulation* refSim = std::ref(sim);

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

GMOD_MODULE_OPEN() {


    // push ALL c -> lua functions
    LUA->PushSpecial(SPECIAL_GLOB); //push _G

    //LUA->PushString("GWater_GetParticleData");
    //LUA->PushCFunction(GWaterGetParticleData);
    //LUA->SetTable(-3);

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