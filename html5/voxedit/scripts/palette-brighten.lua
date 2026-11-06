function arguments()
	return {
		{ name = 'value', desc = 'brightness adjustment amount (-255 to 255, positive = brighter, negative = darker)', type = 'int', default = '20' },
	}
end

function description()
	return "Adjusts the brightness of the palette colors by a given amount. Negative values will darken the colors - positive values will brighten them."
end

function main(node, region, color, value)
	local palette = node:palette()
	local n = palette:size()
	for i = 0, n do
		local r, g, b, a = palette:rgba(i)

		local newR = math.max(0, math.min(255, r + value))
		local newG = math.max(0, math.min(255, g + value))
		local newB = math.max(0, math.min(255, b + value))

		palette:setColor(i, newR, newG, newB, a)
	end
end
