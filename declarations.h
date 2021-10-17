#pragma once
#include <NvFlex.h>
#include <thread>
#include <mutex>
#include "types.h"
#include "GarrysMod/Lua/Interface.h"
#include <vector>

extern NvFlexLibrary* flexLibrary;
extern NvFlexSolver* flexSolver;

extern NvFlexBuffer* particleBuffer;
extern NvFlexBuffer* velocityBuffer;
extern NvFlexBuffer* phaseBuffer;
extern NvFlexBuffer* activeBuffer;

extern NvFlexBuffer* geometryBuffer;
extern NvFlexBuffer* geoFlagsBuffer;
extern NvFlexBuffer* geoPosBuffer;
extern NvFlexBuffer* geoQuatBuffer;

extern NvFlexBuffer* indicesBuffer;
extern NvFlexBuffer* lengthsBuffer;
extern NvFlexBuffer* coefficientsBuffer;

extern NvFlexParams* flexParams;
extern NvFlexSolverDesc flexSolverDesc;
extern NvFlexTriangleMeshId worldMesh;

extern std::vector<Particle> particleQueue;
extern GarrysMod::Lua::ILuaBase* GlobalLUA;

extern std::mutex* bufferMutex;
extern float4* particleBufferHost;

extern int numParticles;
extern int propCount;
extern bool simValid;
extern bool flexRemoveQueue;


extern void initParams(NvFlexParams* params);
extern void flexSolveThread();
extern void printLua(std::string text);
extern void printLua(char*);