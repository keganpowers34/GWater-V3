#include "types.h"
#include "declarations.h"

#define MAX_COLLIDERS 1024

//tysm this was very useful for debugging
void gjelly_error(NvFlexErrorSeverity type, const char* msg, const char* file, int line) {
    printLua("FLEX ERROR:");
    printLua(msg);

}

//add a particle to flex
void flexAPI::addParticle(Vector pos, Vector vel) {
    if (numParticles > flexSolverDesc.maxParticles) return;
    Particle particle = { float4{ pos.x, pos.y, pos.z , 0.5}, float3{vel.x, vel.y, vel.z} };
    particleQueue.push_back(particle);

}

void flexAPI::freeProp(int ID) {

    Prop* prop = &props[ID];

    NvFlexFreeBuffer(prop->verts);
    if (prop->isConvex) {
        NvFlexDestroyConvexMesh(flexLibrary, prop->meshID);
    }
    else {
        NvFlexFreeBuffer(prop->indices);
        NvFlexDestroyTriangleMesh(flexLibrary, prop->meshID);
    }

    NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
    float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
    float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
    float4* prevPositions = static_cast<float4*>(NvFlexMap(geoPrevPosBuffer, 0));
    float4* prevRotations = static_cast<float4*>(NvFlexMap(geoPrevQuatBuffer, 0));
    int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));
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
    NvFlexUnmap(geometryBuffer);
    NvFlexUnmap(geoPosBuffer);
    NvFlexUnmap(geoQuatBuffer);
    NvFlexUnmap(geoPrevPosBuffer);
    NvFlexUnmap(geoPrevQuatBuffer);
    NvFlexUnmap(geoFlagsBuffer);

    props.pop_back();

}

//flex startup
flexAPI::flexAPI() {
    flexLibrary = NvFlexInit(120, gjelly_error);

    NvFlexSetSolverDescDefaults(&flexSolverDesc);
    flexSolverDesc.maxParticles = 65536;
    flexSolverDesc.maxDiffuseParticles = 0;

    flexParams = new NvFlexParams();
    initParams(10.f);

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
    bufferMutex->lock();
    simValid = false;
    numParticles = 0;
    propCount = 0;

    if (flexLibrary != nullptr) {

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

        for (int i = 0; i < props.size(); i++) {
            freeProp(i);
        }
        
        delete flexParams;

        free(particleBufferHost);

        flexLibrary = nullptr;

    }

    bufferMutex->unlock();

    delete bufferMutex;
}
