
if not _G.gwater then
	require("GWater_Rewrite")
end


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
        m[len + 3] = Vector(surface[i - 1].x, surface[i - 1].y, surface[i - 1].z)
        m[len + 2] = Vector(surface[i].x, surface[i].y, surface[i].z)

	  	end

	end
	  
	  return m
end

local mblue = CreateMaterial( "shitter05", "UnlitGeneric", {
  ["$basetexture"] = "phoenix_storms/blue_steel",
})

local mwhite = CreateMaterial( "shitter04", "UnlitGeneric", {
  ["$basetexture"] = "lights/white",
})

local mgrey = CreateMaterial( "shitter03", "UnlitGeneric", {
  ["$basetexture"] = "hunter/myplastic",
  ["$translucent"] = 1,
  ["$vertexalpha"] = 1,
})


GWATER_RADIUS = 10

	gwater.AddWorldMesh(triangulateWorld(), Vector(-33000, -33000, -33000), Vector(33000, 33000, 33000))

	local data, particleCount
	hook.Add("PostDrawOpaqueRenderables", "bruh", function()

		if (LocalPlayer():KeyDown(2048)) then
			for i = 1, 10 do
				gwater.SpawnParticle(LocalPlayer():GetPos() + Vector(0, 0, 63) + EyeVector() * 100 + VectorRand(-10, 10) , EyeVector() * 100)
			end
		end

		if (LocalPlayer():KeyDown(32)) then
			gwater.RemoveAll()

		end

		data, particleCount = gwater.GetData()

		render.SetMaterial(mwhite)
		render.DrawSprite(Vector(0, 0, 0), 10, 10, Color( 255, 255, 255 ) )

		render.SetMaterial(mblue)
		for i=1, math.min(#data, 18000) do
			local particlePos = data[i]
			--if (particlePos-EyePos()):GetNormalized():Dot(EyeVector()) < 0.75 then continue end
			render.DrawSprite(particlePos, GWATER_RADIUS, GWATER_RADIUS, Color( 255, 255, 255 ) )
			//render.DrawSphere(particlePos, 10, 7, 7, Color( 255, 255, 255 ) )
		end

	end)


	hook.Add( "HUDPaint", "Particles", function() 
		draw.DrawText(tostring(particleCount), "TargetID", ScrW() * 0.99, ScrH() * 0.01, color_white, TEXT_ALIGN_RIGHT )
	end )



//timer.Simple(1000, function() 
//	print("Removing solver")
//	gwater.DeleteSimulation()
//	hook.Remove("PostDrawOpaqueRenderables", "bruh")
//end)