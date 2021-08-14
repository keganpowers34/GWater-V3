#include "Simulation.h"
#include "GarrysMod/Lua/Interface.h"

//extern GarrysMod::Lua::ILuaBase* GlobalLUA;
//extern void printLua(GarrysMod::Lua::ILuaBase* LUA, std::string text);
Simulation* sim = new Simulation();
sim->radius = 1.f;

NvFlexLibrary* Simulation::library = nullptr;
NvFlexBuffer* Simulation::particleBuffer = nullptr;
NvFlexBuffer* Simulation::velocityBuffer = nullptr;
NvFlexBuffer* Simulation::phaseBuffer = nullptr;
NvFlexSolver* Simulation::solver = nullptr;
