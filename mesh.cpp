#include "types.h"
#include "declarations.h"
#include <string>

float3 crossProduct(float3 A, float3 B) {
    return float3{
        A.y * B.z - A.z * B.y,
        A.z * B.x - A.x * B.z,
        A.x * B.y - A.y * B.x
    };
}

float3 normalize(const float3& v)
{
    float length_of_v = sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
    return float3{ v.x / length_of_v, v.y / length_of_v, v.z / length_of_v };
}

//generate a CONVEX mesh for flex
void flexAPI::calcMeshConvex(GarrysMod::Lua::ILuaBase* LUA, const float* minFloat, const float* maxFloat, size_t tableLen) {
    //prop declerations
    Prop p = Prop{};
    p.pos = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
    p.ang = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
    p.lastPos = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
    p.lastAng = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
    p.isConvex = true;

    p.verts = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(float4), eNvFlexBufferHost);
    float4* hostVerts = static_cast<float4*>(NvFlexMap(p.verts, eNvFlexMapWait));

    //loop through verticies
    for (int i = 0; i < tableLen; i += 3) {

        float3 verts[3] = { };

        for (int j = 0; j < 3; j++) {
            LUA->PushNumber(static_cast<double>(i + j) + 1.0);
            LUA->GetTable(-2);

            Vector thisPos = LUA->GetVector();
            float3 vert = { thisPos.x, thisPos.y, thisPos.z };

            verts[j] = vert;

            LUA->Pop(); //pop pos

        }

        //cross product point1 - point0 && point2 - point0
        float3 cross = crossProduct(
            float3{ verts[1].x - verts[0].x, verts[1].y - verts[0].y, verts[1].z - verts[0].z },
            float3{ verts[2].x - verts[0].x, verts[2].y - verts[0].y, verts[2].z - verts[0].z }
        );

        cross = normalize(cross);
        float d = cross.x * verts[0].x + cross.y * verts[0].y + cross.z * verts[0].z;

        //array of plane data
        hostVerts[i / 3] = float4{ cross.x, cross.y, cross.z, d };

        //debug
        printLua(std::to_string(hostVerts[i / 3].x) + ", " + std::to_string(hostVerts[i / 3].y) + ", " + std::to_string(hostVerts[i / 3].z) + ", " + std::to_string(hostVerts[i / 3].w));
    }

    LUA->Pop(); //pop table

    NvFlexUnmap(p.verts);

    p.meshID = NvFlexCreateConvexMesh(flexLibrary);
    NvFlexUpdateConvexMesh(flexLibrary, p.meshID, p.verts, tableLen / 3, minFloat, maxFloat);

    // Add data to BUFFERS
    NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
    float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
    float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
    float4* prevPositions = static_cast<float4*>(NvFlexMap(geoPrevPosBuffer, 0));
    float4* prevRotations = static_cast<float4*>(NvFlexMap(geoPrevQuatBuffer, 0));
    int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));
    flags[propCount] = NvFlexMakeShapeFlags(eNvFlexShapeConvexMesh, true);	//always dynamic (props)
        geometry[propCount].convexMesh.mesh = p.meshID;
        geometry[propCount].convexMesh.scale[0] = 1.0f;
        geometry[propCount].convexMesh.scale[1] = 1.0f;
        geometry[propCount].convexMesh.scale[2] = 1.0f;
        positions[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
        rotations[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.01f };	//NEVER SET ROTATION TO 0,0,0,0, FLEX *HATES* IT!
        prevPositions[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
        prevRotations[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
    NvFlexUnmap(geometryBuffer);
    NvFlexUnmap(geoPosBuffer);
    NvFlexUnmap(geoQuatBuffer);
    NvFlexUnmap(geoPrevPosBuffer);
    NvFlexUnmap(geoPrevQuatBuffer);
    NvFlexUnmap(geoFlagsBuffer);

    props.push_back(p);

}





//generate a TRIANGLE mesh for flex
void flexAPI::calcMeshConcave(GarrysMod::Lua::ILuaBase* LUA, const float* minFloat, const float* maxFloat, size_t tableLen) {
    //prop declerations
    Prop p = Prop{};
    p.pos = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
    p.ang = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
    p.lastPos = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
    p.lastAng = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
    p.isConvex = false;

    p.verts = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(float4), eNvFlexBufferHost);
    p.indices = NvFlexAllocBuffer(flexLibrary, tableLen, sizeof(int), eNvFlexBufferHost);

    float4* hostVerts = static_cast<float4*>(NvFlexMap(p.verts, eNvFlexMapWait));
    int* hostIndices = static_cast<int*>(NvFlexMap(p.indices, eNvFlexMapWait));

    //loop through verticies
    for (int i = 0; i < tableLen; i++) {

        //lua is 1 indexed, C++ is 0 indexed
        LUA->PushNumber(static_cast<double>(i) + 1.0);

        //gets data from table at the number ontop of the stack
        LUA->GetTable(-2);

        Vector thisPos = LUA->GetVector();
        float4 vert = { thisPos.x, thisPos.y, thisPos.z, 0.f };

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

    NvFlexUnmap(p.verts);
    NvFlexUnmap(p.indices);

    // create the triangle mesh
    p.meshID = NvFlexCreateTriangleMesh(flexLibrary);
    NvFlexUpdateTriangleMesh(flexLibrary, p.meshID, p.verts, p.indices, tableLen, tableLen / 3, minFloat, maxFloat);

    //map & unmap buffers
    NvFlexCollisionGeometry* geometry = static_cast<NvFlexCollisionGeometry*>(NvFlexMap(geometryBuffer, 0));
    float4* positions = static_cast<float4*>(NvFlexMap(geoPosBuffer, 0));
    float4* rotations = static_cast<float4*>(NvFlexMap(geoQuatBuffer, 0));
    float4* prevPositions = static_cast<float4*>(NvFlexMap(geoPrevPosBuffer, 0));
    float4* prevRotations = static_cast<float4*>(NvFlexMap(geoPrevQuatBuffer, 0));
    int* flags = static_cast<int*>(NvFlexMap(geoFlagsBuffer, 0));
    flags[propCount] = NvFlexMakeShapeFlags(eNvFlexShapeTriangleMesh, propCount != 0);	//index 0 is ALWAYS the world
        geometry[propCount].triMesh.mesh = p.meshID;
        geometry[propCount].triMesh.scale[0] = 1.0f;
        geometry[propCount].triMesh.scale[1] = 1.0f;
        geometry[propCount].triMesh.scale[2] = 1.0f;
        positions[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
        rotations[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.01f };	//NEVER SET ROTATION TO 0,0,0,0, FLEX *HATES* IT!
        prevPositions[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
        prevRotations[propCount] = float4{ 0.0f, 0.0f, 0.0f, 0.01f };
    NvFlexUnmap(geometryBuffer);
    NvFlexUnmap(geoPosBuffer);
    NvFlexUnmap(geoQuatBuffer);
    NvFlexUnmap(geoPrevPosBuffer);
    NvFlexUnmap(geoPrevQuatBuffer);
    NvFlexUnmap(geoFlagsBuffer);

    props.push_back(p);

}
