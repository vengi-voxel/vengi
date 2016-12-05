require "lua.shared"

function init()
	m = MAT.material()
	for i in range(117, 120) do
		MAT.grass(i)
	end

	for i in range(121, 122) do
		MAT.sand(i)
	end

	for i in range(133, 135) do
		MAT.leaf(i)
	end

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

	for i in range(197, 198) do
		MAT.water(i)
	end

	for i in range(191, 193) do
		MAT.cloud(i)
	end

	MAT.wood(67)
	MAT.wood(81)

	for i in range(235, 241) do
		MAT.rock(i)
	end
	for i in range(249, 252) do
		MAT.rock(i)
	end

	for i in range(65, 71) do
		MAT.dirt(i)
	end
end
