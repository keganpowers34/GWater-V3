#include "types.h"
#include "declarations.h"

#define MAX_COLLIDERS 1024

//tysm this was very useful for debugging
void gjelly_error(NvFlexErrorSeverity type, const char* msg, const char* file, int line) {
    printLua("FLEX ERROR:");
    printLua(msg);

}

//generate a mesh for flex
void flexAPI::calcMesh(GarrysMod::Lua::ILuaBase* LUA, const float* minFloat, const float* maxFloat, size_t tableLen, Prop* p) {

    p->verts = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(float4), eNvFlexBufferHost);
    p->indices = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(int), eNvFlexBufferHost);

    float4* hostVerts = static_cast<float4*>(NvFlexMap(p->verts, eNvFlexMapWait));
    int* hostIndices = static_cast<int*>(NvFlexMap(p->indices, eNvFlexMapWait));

    //loop through map verticies
    for (int i = 0; i < tableLen; i++) {

        //lua is 1 indexed, C++ is 0 indexed
        LUA->PushNumber(static_cast<double>(i) + 1.0);

        //gets data from table at the number ontop of the stack
        LUA->GetTable(-2);

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

        LUA->Pop(); //pop pos
    }

    LUA->Pop(); //pop table

    NvFlexUnmap(p->verts);
    NvFlexUnmap(p->indices);

    // create the triangle mesh
    NvFlexTriangleMeshId generatedMesh = NvFlexCreateTriangleMesh(flexLib->flexLibrary);
    NvFlexUpdateTriangleMesh(flexLib->flexLibrary, generatedMesh, p->verts, p->indices, tableLen, tableLen / 3, minFloat, maxFloat);

    //prop declarations
    p->pos = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
    p->ang = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
    p->lastPos = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
    p->lastAng = float4{ 0.0f, 0.0f, 0.0f, 0.01f };

    // Add data to BUFFERS
    NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
    float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
    float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
    float4* prevPositions = static_cast<float4*>(NvFlexMap(geoPrevPosBuffer, 0));
    float4* prevRotations = static_cast<float4*>(NvFlexMap(geoPrevQuatBuffer, 0));
    int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));

    // add triangle mesh
    flags[propCount] = NvFlexMakeShapeFlags(eNvFlexShapeTriangleMesh, propCount != 0);	//index 0 is ALWAYS the world
    geometry[propCount].triMesh.mesh = generatedMesh;
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

}

//add a particle to flex
void flexAPI::addParticle(Vector pos, Vector vel) {
    if (numParticles > flexSolverDesc.maxParticles) return;
    Particle particle = { float4{ pos.x, pos.y, pos.z , 0.5}, float3{vel.x, vel.y, vel.z} };
    particleQueue.push_back(particle);

}

void flexAPI::freeProp(int ID) {

    NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
    float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
    float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
    int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));
    float4* prevPositions = static_cast<float4*>(NvFlexMap(geoPrevPosBuffer, 0));
    float4* prevRotations = static_cast<float4*>(NvFlexMap(geoPrevQuatBuffer, 0));

    for (int i = ID; i < propCount; i++) {
        int nextIndex = i + 1;

        geometry[i] = geometry[nextIndex];
        positions[i] = positions[nextIndex];
        prevPositions[i] = prevPositions[nextIndex];
        rotations[i] = rotations[nextIndex];
        prevRotations[i] = prevRotations[nextIndex];
        flags[i] = flags[nextIndex];

        props[i] = props[nextIndex];
    }

    NvFlexUnmap(geoPrevPosBuffer);
    NvFlexUnmap(geoPrevQuatBuffer);
    NvFlexUnmap(geometryBuffer);
    NvFlexUnmap(geoPosBuffer);
    NvFlexUnmap(geoQuatBuffer);
    NvFlexUnmap(geoFlagsBuffer);

}

//flex startup
flexAPI::flexAPI() {
    flexLibrary = NvFlexInit(120, gjelly_error);

    NvFlexSetSolverDescDefaults(&flexSolverDesc);
    flexSolverDesc.maxParticles = 65536;
    flexSolverDesc.maxDiffuseParticles = 0;

    flexParams = new NvFlexParams();
    initParams(flexParams, 10.f);

    flexSolver = NvFlexCreateSolver(flexLibrary, &flexSolverDesc);
    NvFlexSetParams(flexSolver, flexParams);


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
    std::thread(&flexAPI::flexSolveThread, this).detach();
}

//flex shutdown
flexAPI::~flexAPI() {
    simValid = false;
    numParticles = 0;
    propCount = 0;

    //GlobalLUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
    //GlobalLUA->PushNil();
    //GlobalLUA->SetField(-2, "gwater");
    //GlobalLUA->Pop(); // pop _G

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
        NvFlexFreeBuffer(geoPrevPosBuffer);
        NvFlexFreeBuffer(geoPrevQuatBuffer);

        for (Prop prop : props) {
            NvFlexFreeBuffer(prop.verts);
            NvFlexFreeBuffer(prop.indices);

            NvFlexDestroyTriangleMesh(flexLibrary, prop.meshID);
        }
        
        delete flexParams;

        free(particleBufferHost);

        flexLibrary = nullptr;

        bufferMutex->unlock();

        delete bufferMutex;

    }
}
