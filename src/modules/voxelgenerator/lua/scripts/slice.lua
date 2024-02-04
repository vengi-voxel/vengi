--
-- cuts a node into smaller pieces. If you e.g. specify 1 for each of width, height and depth, you will get one node (or model) per voxel.
--

function arguments()
	return {
		{ name = 'width', desc = 'the amount of voxels for the width', type = 'int', default = '32' },
		{ name = 'height', desc = 'the amount of voxels for the height', type = 'int', default = '32' },
		{ name = 'depth', desc = 'the amount of voxels for the depth', type = 'int', default = '32' },
	}
end

function main(node, region, color, width, height, depth)
	local volume = node:volume()
	local mins = region:mins()
	local size = region:size()

	local volumeWidth = size.x
	local volumeHeight = size.y
	local volumeDepth = size.z

	local numCols = math.ceil(volumeWidth / width)
	local numRows = math.ceil(volumeHeight / height)
	local numSlices = math.ceil(volumeDepth / depth)

	for slice = 1, numSlices do
		for row = 1, numRows do
			for col = 1, numCols do
				local minsx = mins.x + (col - 1) * width
				local minsy = mins.y + (row - 1) * height
				local minsz = mins.z + (slice - 1) * depth
				local maxsx = minsx + width - 1
				local maxsy = minsy + height - 1
				local maxsz = minsz + depth - 1
				local slicedRegion = g_region.new(minsx, minsy, minsz, maxsx, maxsy, maxsz)
				local slicedNode = g_scenegraph.new('slice', slicedRegion)
				local v = slicedNode:volume()
				for d = 1, depth do
					local z = mins.z + (slice - 1) * depth + d - 1
					for h = 1, height do
						local y = mins.y + (row - 1) * height + h - 1
						for w = 1, width do
							local x = mins.x + (col - 1) * width + w - 1
							local voxel = volume:voxel(x, y, z)
							v:setVoxel(x, y, z, voxel)
						end
					end
				end
			end
		end
	end
end
