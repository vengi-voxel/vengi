--
-- move the voxels in a volume by the given offsets.
--

function arguments()
	return {
		{ name = 'x', desc = 'the amount of voxel to move in x direction (right)', type = 'int', default = '1' },
		{ name = 'y', desc = 'the amount of voxel to move in y direction (up)', type = 'int', default = '0' },
		{ name = 'z', desc = 'the amount of voxel to move in z direction (backward)', type = 'int', default = '0' },
	}
end

function main(node, region, color, x, y, z)
	node:volume():move(x, y, z)
end
