local function range(a, b, step)
	if not b then
		b = a
		a = 1
	end
	step = step or 1
	local f =
		step > 0 and
		function(_, lastvalue)
			local nextvalue = lastvalue + step
			if nextvalue <= b then return nextvalue end
		end or
		step < 0 and
		function(_, lastvalue)
			local nextvalue = lastvalue + step
			if nextvalue >= b then return nextvalue end
		end or
		function(_, lastvalue) return lastvalue end
	return f, nil, a - step
end

function init()
	m = MAT.material()

	MAT.Grass(117)

	MAT.Sand(102)

	MAT.Leaf(133)
	MAT.Leaf(134)
	MAT.Leaf(135)

	MAT.LeafFir(161)
	MAT.LeafFir(162)
	MAT.LeafPine(159)
	MAT.LeafPine(160)

	for i in range(149, 152) do
		MAT.Flower(i)
	end

	MAT.Bloom(37) -- red
	MAT.Bloom(53) -- orange
	MAT.Bloom(109) -- yellow

	for i in range(237, 242) do
		MAT.Mushroom(i)
	end

	MAT.Water(197)
	MAT.Transparent(0)

	MAT.Cloud(191)
	MAT.Cloud(192)

	MAT.Wood(67)
	MAT.Wood(81)

	MAT.Rock(248)

	MAT.Roof(25)
	MAT.Roof(29)
	MAT.Wall(241)

	MAT.Dirt(65)
end
