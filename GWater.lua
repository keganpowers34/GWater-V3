require("GWater_Rewrite")
print(_G.gwater)

local function triangulateWorld()
    local surfaces = game.GetWorld():GetBrushSurfaces()
	local m = {}

	local maxX = 0
	local maxY = 0
	local maxZ = 0

	local minX = 0
	local minY = 0
	local minZ = 0

	for i = 1, #surfaces do
        if surfaces[i]:IsNoDraw() then continue end

        local surface = surfaces[i]:GetVertices()
        for i = 3, #surface do
	        local len = #m
	        m[len + 1] = Vector(surface[1].x, surface[1].y, surface[1].z)
	        m[len + 2] = Vector(surface[i - 1].x, surface[i - 1].y, surface[i - 1].z)
	        m[len + 3] = Vector(surface[i].x, surface[i].y, surface[i].z)

	        --max
	        maxX = math.max(maxX, surface[1].x)
	        maxX = math.max(maxX, surface[i-1].x)
	        maxX = math.max(maxX, surface[i].x)

	        maxY = math.max(maxY, surface[1].y)
	        maxY = math.max(maxY, surface[i-1].y)
	        maxY = math.max(maxY, surface[i].y)

	        maxZ = math.max(maxZ, surface[1].z)
	        maxZ = math.max(maxZ, surface[i-1].z)
	        maxZ = math.max(maxZ, surface[i].z)

	        --min

	        minX = math.min(minX, surface[1].x)
	        minX = math.min(minX, surface[i-1].x)
	        minX = math.min(minX, surface[i].x)

	        minY = math.min(minY, surface[1].y)
	        minY = math.min(minY, surface[i-1].y)
	        minY = math.min(minY, surface[i].y)

	        minZ = math.min(minZ, surface[1].z)
	        minZ = math.min(minZ, surface[i-1].z)
	        minZ = math.min(minZ, surface[i].z)

	    end
    end
    
    return {m, Vector(minX, minY, minZ), Vector(maxX, maxY, maxZ)}
end

--[[

gwater.Initialize()
gwater.Unpause()
gwater.SetRadius(10)

local m = CreateMaterial( "shitter05", "UnlitGeneric", {
  ["$basetexture"] = "sprites/strider_blackball",
  ["$translucent"] = 1,
  ["$vertexalpha"] = 1,
})

timer.Simple(1, function()
	local worldverts = triangulateWorld()

	print(worldverts[2])
	print(worldverts[3])
	gwater.AddWorldMesh(worldverts[3], worldverts[2], worldverts[1])

	timer.Create("oh my shit", 1, 1, function()
		gwater.SpawnParticle(LocalPlayer():GetPos() + Vector(0, 0, 50), Vector(0, 0, 0))
	end)

	local data = {}

	hook.Add("PostDrawOpaqueRenderables", "bruh", function()
		data = gwater.GetData()
		//PrintTable(data)
		render.SetMaterial(m)
		for i=1, #data do
			render.DrawSprite(data[i], 10, 10, Color( 255, 255, 255 ) )
		end
	end)
end)

timer.Simple(10, function() 
	gwater.Pause()
	gwater.DeleteSimulation()
	timer.Remove("bruh")
end)]]