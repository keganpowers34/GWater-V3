require("GWater_Rewrite")
print(_G.gwater)

gwater.Initialize()
gwater.Unpause()
gwater.SetRadius(10)

local m = CreateMaterial( "shitter", "UnlitGeneric", {
  ["$basetexture"] = "hunter/myplastic",
})
timer.Simple(2, function()
	timer.Create("oh my shit", 0.1, 100, function()
		gwater.SpawnCube(Vector(math.random()/10, math.random()/10, 1000), Vector(2, 2, 2))
	end)

	local data = {}

	hook.Add("PostDrawOpaqueRenderables", "bruh", function()
		data = gwater.GetData()
		render.SetMaterial(m)
		for i=1, #data do
			render.DrawSprite(data[i], 10, 10, Color( 255, 255, 255 ) )
		end
	end)
end)

timer.Simple(30, function() 
	gwater.Pause()
	gwater.DeleteSimulation()
	timer.Remove("bruh")
end)
