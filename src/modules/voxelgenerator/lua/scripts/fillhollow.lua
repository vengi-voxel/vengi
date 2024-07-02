--
-- fill hollows in a model
--
-- This script iterates over the voxel volume and identifies hollows that are totally enclosed by existing voxels.
-- It then fills these hollow spaces with a specified fillvoxel.
--

function arguments()
	return {
		{ name = 'fillvoxel', desc = 'the palette index of the color to use', type = 'colorindex' },
	}
end

function main(node, region, color, fillvoxel)
	local volume = node:volume()
	volume:fillHollow(fillvoxel)
end
