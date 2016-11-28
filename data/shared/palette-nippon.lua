function init()
	m = MAT.material()
	-- todo implement me properly
	for colorindex, color in ipairs(m) do
		if colorindex ~= 0 then
			if color.b > 0.8 then
				MAT.water(colorindex)
				MAT.cloud(colorindex)
			end
			if color.g > 0.8 then
				MAT.grass(colorindex)
				MAT.leaves(colorindex)
			end
			MAT.wood(colorindex)
			MAT.rock(colorindex)
			MAT.sand(colorindex)
			MAT.dirt(colorindex)
		end
	end
end
