require "lua.shared"

function init()
	m = MAT.material()
	for i in range(117, 120) do
		MAT.grass(i)
	end

	for i in range(121, 122) do
		MAT.sand(i)
	end

	for i in range(128, 136) do
		MAT.leaves(i)
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
