--
-- Game Of Life.
--
-- Take the current node as a base and create new nodes for the next generations
--
-- https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
--
-- make sure to place a few voxels in the region to start the game
-- it also helps if the region only has a height of 1 here
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'steps', desc = 'the amount of steps for the game of life', type = 'int', default = '10', min = '1', max = '255' },
	}
end

local function step(node, region, color)
	local newNode = node:clone()

	local visitor = function(volume, x, y, z)
		local aliveNeighbors = 8 - vol.countEmptyAroundOnY(volume, x, y, z, 1)
		local state
		if aliveNeighbors == 3 then
			-- born
			state = color
		else
			if aliveNeighbors == 2 then
				-- stays alive
				state = color
			else
				-- dies
				state = -1
			end
		end
		volume:setVoxel(x, y, z, state)
	end
	vol.visitYXZ(newNode:volume(), region, visitor)
	return newNode
end

function main(node, region, color, steps)
	for _ = 1, steps do
		node = step(node, region, color)
	end
end
