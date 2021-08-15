#include "GarrysMod/Lua/Interface.h"
#include "GarrysMod/Lua/SourceCompat.h"
#include "types.h"
#include "Simulation.h"

using namespace GarrysMod::Lua;

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

//potatoos function
LUA_FUNCTION(GWaterGetParticleData) {

    LUA->CreateTable();

    for (int i = 0; i < sim->count; i++) {
        LUA->PushNumber(i + 1);

        float4 thisPos = sim->particles[i];
        Vector gmodPos;
        gmodPos.x = thisPos.x;
        gmodPos.y = thisPos.y;
        gmodPos.z = thisPos.z;

        LUA->PushVector(gmodPos);
        LUA->SetTable(-3);
    }

    return 1;

}



GMOD_MODULE_OPEN() {

    initSimulation(sim);
    printf("SIMULATION INITED");
    // push ALL c -> lua functions
    LUA->PushSpecial(SPECIAL_GLOB); //push _G

    LUA->CreateTable();

    ADD_GWATER_FUNC(GWaterUnpauseSim, "Unpause");
    ADD_GWATER_FUNC(GWaterPauseSim, "Pause");
    ADD_GWATER_FUNC(GWaterDeleteSim, "DeleteSimulation");
    ADD_GWATER_FUNC(GWaterGetParticleData, "GetData");

    LUA->SetField(-2, "gwater");
    LUA->Pop(); // pop _G


    

	return 0;
}


GMOD_MODULE_CLOSE() {
    sim->isValid = false;
    sim->isRunning = false;
    sim->count = 0;

	return 0;
}