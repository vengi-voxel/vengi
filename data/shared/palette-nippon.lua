require "lua.shared"

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

	MAT.Cloud(191)
	MAT.Cloud(192)

	MAT.Wood(67)
	MAT.Wood(81)

	MAT.Rock(248)

	MAT.Dirt(65)
end
