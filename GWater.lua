
if not _G.gwater then
	require("GWater_Rewrite")
end


local math_sin = math.sin
local math_cos = math.cos
local math_rad = math.rad


local function quatUnpack(q)
	return q[1], q[2], q[3], q[4]
end

-- We're gonna make this one not self-modify for the sake of sanity
local function getQuatMul(lhs, rhs)
	local lhs1, lhs2, lhs3, lhs4 = quatUnpack(lhs)
	local rhs1, rhs2, rhs3, rhs4 = quatUnpack(rhs)
	return {
		lhs1 * rhs1 - lhs2 * rhs2 - lhs3 * rhs3 - lhs4 * rhs4,
		lhs1 * rhs2 + lhs2 * rhs1 + lhs3 * rhs4 - lhs4 * rhs3,
		lhs1 * rhs3 + lhs3 * rhs1 + lhs4 * rhs2 - lhs2 * rhs4,
		lhs1 * rhs4 + lhs4 * rhs1 + lhs2 * rhs3 - lhs3 * rhs2
	}
end

local function quatFromAngleComponents(p, y, r)
	p = math_rad(p) * 0.5
	y = math_rad(y) * 0.5
	r = math_rad(r) * 0.5

	return getQuatMul( { math_cos(y), 0, 0, math_sin(y) }, getQuatMul( { math_cos(p), 0, math_sin(p), 0 }, { math_cos(r), math_sin(r), 0, 0 } ) )
end

local function quatFromAngle(ang)
	return quatFromAngleComponents(ang[1], ang[2], ang[3])
end

local function unfuckQuat(q)
    return { q[2], q[3], q[4], q[1] }
end


local function triangulateWorld()
  local surfaces = game.GetWorld():GetBrushSurfaces()
	local m = {}

	for i = 1, #surfaces do
      if surfaces[i]:IsNoDraw() then continue end

      local surface = surfaces[i]:GetVertices()
      for i = 3, #surface do
        local len = #m
        m[len + 1] = Vector(surface[1].x, surface[1].y, surface[1].z)
        m[len + 2] = Vector(surface[i - 1].x, surface[i - 1].y, surface[i - 1].z)
        m[len + 3] = Vector(surface[i].x, surface[i].y, surface[i].z)

	  	end

	end
	  
	  return m
end

--potatofunc
local function addPropMesh(prop)
    local model = prop:GetModel()
		if not util.GetModelMeshes(model) then print("INVALID MESH!") return end

    local pos = prop:GetPos()

    local chonker_mesh = {}

    for _, mesh in pairs( util.GetModelMeshes(model) ) do -- combine all model meshes into the chonker mesh
        for _, tri in pairs( mesh.triangles ) do
            table.insert( chonker_mesh, tri.pos )
        end
    end

    return gwater.AddMesh(chonker_mesh, prop:OBBMins() * 2, prop:OBBMaxs() * 2)

end


--materials--
local mblue = CreateMaterial( "shitter05", "UnlitGeneric", {
  ["$basetexture"] = "phoenix_storms/blue_steel",
})

local mwhite = CreateMaterial( "shitter04", "UnlitGeneric", {
  ["$basetexture"] = "lights/white",
})


GWATER_RADIUS = 10
--world mesh--
gwater.AddMesh(triangulateWorld(), Vector(-33000, -33000, -33000), Vector(33000, 33000, 33000))

--loop through props--
local meshes = {}
local props = ents.GetAll()
for i = 1, #props do
		if props[i]:GetClass() == "prop_physics" then
			table.insert(meshes, {props[i], addPropMesh(props[i])})

		end

end



local data, particleCount
local gwaterMat = Material("phoenix_storms/blue_steel")		--phoenix_storms/blue_steel

local lp = LocalPlayer()

hook.Add("PostDrawOpaqueRenderables", "bruh", function()

	if(lp:Alive() and lp:GetActiveWeapon():GetClass() == "weapon_crowbar") then
		if lp:KeyDown(1) then
			for i = 1, 10 do
				gwater.SpawnParticle(LocalPlayer():GetPos() + Vector(0, 0, 63) + EyeVector() * 100 + VectorRand(-15, 15) , EyeVector() * 100)
			end
		end

		if lp:KeyDown(2048) then
			gwater.RemoveAll()
		end
	end

	for _, mesh in ipairs(meshes) do
				print("anything at all")
			if not mesh[1] then 
				gwater.RemoveMesh(mesh[2])
				continue 
			end

			local newQuat = unfuckQuat( quatFromAngle( mesh[1]:GetAngles() ) )
			//gwater.SetMeshPos(newQuat[4], Vector(newQuat[1], newQuat[2], newQuat[3]), mesh[1]:GetPos(), mesh[2]);

	end

	data, particleCount = gwater.GetData()

	render.SetMaterial(mwhite)
	render.DrawSprite(Vector(0, 0, 0), 100, 100, Color( 255, 255, 255 ) )

	render.SetMaterial(gwaterMat)
	for i=1, math.min(#data, 18000) do
		local particlePos = data[i]
		--if (particlePos-EyePos()):GetNormalized():Dot(EyeVector()) < 0.75 then continue end
		--render.DrawSprite(particlePos, GWATER_RADIUS, GWATER_RADIUS, Color( 255, 255, 255 ) )
		render.DrawSphere(particlePos, GWATER_RADIUS / 1.5, 4, 3, Color( 255, 255, 255 ) )
	end

end)


hook.Add( "HUDPaint", "Particles", function() 
	draw.DrawText(tostring(particleCount), "TargetID", ScrW() * 0.99, ScrH() * 0.01, color_white, TEXT_ALIGN_RIGHT )
end )
