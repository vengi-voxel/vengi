--
-- This script iterates over the voxel volume and identifies hollows that are totally enclosed by existing voxels.
-- It then fills these hollow spaces with a selected color.
--

function description()
	return "Fills hollows in a model with the selected color."
end

function main(node, _, color)
	local volume = node:volume()
	volume:fillHollow(color)
end
