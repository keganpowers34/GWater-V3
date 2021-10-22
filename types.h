#pragma once
struct float4 {
    float x, y, z, w;
};

struct float3 {
    float x, y, z;
};

struct Particle {
    float4 pos;
    float3 vel;
};

struct Prop {
    NvFlexBuffer* verts;
    NvFlexBuffer* indices;
    NvFlexTriangleMeshId meshID;
    
    float4 pos;
    float4 lastPos;

    float4 ang;
    float4 lastAng;

    int ID;
};

