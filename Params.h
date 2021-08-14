#pragma once

NvFlexParams g_params;
g_params.gravity[0] = 0.0f;
g_params.gravity[1] = -9.8f;
g_params.gravity[2] = 0.0f;

g_params.radius = Simulation::radius;
g_params.viscosity = 0.0f;
g_params.dynamicFriction = 0.0f;
g_params.staticFriction = 0.0f;
g_params.particleFriction = 0.0f; // scale friction between particles by default
g_params.freeSurfaceDrag = 0.0f;
g_params.drag = 0.0f;
g_params.lift = 0.0f;
g_params.numIterations = 1;
g_params.fluidRestDistance = Simulation::radius / 2.f;
g_params.solidRestDistance = 0.0f;

g_params.anisotropyScale = 1.0f;
g_params.anisotropyMin = 0.1f;
g_params.anisotropyMax = 2.0f;
g_params.smoothing = 1.0f;

g_params.dissipation = 0.0f;
g_params.damping = 0.0f;
g_params.particleCollisionMargin = 0.0f;
g_params.shapeCollisionMargin = 0.0f;
g_params.collisionDistance = 0.0f;
g_params.sleepThreshold = 0.0f;
g_params.shockPropagation = 0.0f;
g_params.restitution = 0.0f;

g_params.maxSpeed = FLT_MAX;
g_params.maxAcceleration = 100.0f;    // approximately 10x gravity

g_params.relaxationMode = eNvFlexRelaxationLocal;
g_params.relaxationFactor = 1.0f;
g_params.solidPressure = 1.0f;
g_params.adhesion = 0.0f;
g_params.cohesion = 0.025f;
g_params.surfaceTension = 0.0f;
g_params.vorticityConfinement = 0.0f;
g_params.buoyancy = 1.0f;
g_params.diffuseThreshold = 100.0f;
g_params.diffuseBuoyancy = 1.0f;
g_params.diffuseDrag = 0.8f;
g_params.diffuseBallistic = 16;
g_params.diffuseLifetime = 2.0f;

// planes created after particles
g_params.planes[0][0] = 0.f;
g_params.planes[0][1] = 1.f;
g_params.planes[0][2] = 0.f;
g_params.planes[0][3] = 0.f;

g_params.numPlanes = 1;