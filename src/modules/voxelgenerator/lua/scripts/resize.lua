--
-- resize a volume by the given amounts of w, h and d. If you e.g. specify 1 for each of those values
-- the volume will be one unit bigger in each of the directions.
--

function arguments()
	return {
		{ name = 'w', desc = 'the amount of voxel to add for the width', type = 'int', default = '1' },
		{ name = 'h', desc = 'the amount of voxel to add for the height', type = 'int', default = '1' },
		{ name = 'd', desc = 'the amount of voxel to add for the depth', type = 'int', default = '1' },
	}
end

function main(node, region, color, w, h, d)
	node:volume():resize(w, h, d)
end
