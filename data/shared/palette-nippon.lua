require "lua.shared"

function init()
	m = MAT.material()

	MAT.grass(117)

	MAT.sand(102)

	MAT.leaf(133)
	MAT.leaf(134)
	MAT.leaf(135)

	MAT.leaffir(161)
	MAT.leaffir(162)
	MAT.leafpine(159)
	MAT.leafpine(160)

	for i in range(149, 152) do
		MAT.flower(i)
	end

	MAT.bloom(37) -- red
	MAT.bloom(53) -- orange
	MAT.bloom(109) -- yellow

	for i in range(237, 242) do
		MAT.mushroom(i)
	end

	MAT.water(197)

	MAT.cloud(191)
	MAT.cloud(192)

	MAT.wood(67)
	MAT.wood(81)

	MAT.rock(248)

	MAT.dirt(65)
end
