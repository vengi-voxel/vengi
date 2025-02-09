local vol = require "modules.volume"

function arguments()
	return {
		{name = "coils", desc = "Number of coils for the stair", type = "int", default = "2"},
		{name = "width", desc = "Width of the steps", type = "int", default = "4"},
		{name = "height", desc = "Height of the steps", type = "int", default = "1"}
	}
end

function main(node, region, color, coils, width, height)
	local regionCenter = region:center()
	local regionSize = region:size()
	local halfWidth = 0.5 * width
	local halfHeight = 0.5 * height
	local regionHeight = regionSize.y
	local fullCoils = coils / (regionHeight / math.pi)
	local regionHalfY = regionHeight / 2
	local minRegionXY = math.min(regionSize.x, regionSize.y)
	local adjustedRadiusBase = (minRegionXY - width) / 2

	local visitor = function(volume, x, y, z)
		local localPosX = x - regionCenter.x
		local localPosY = y - regionCenter.y
		local localPosZ = z - regionCenter.z

		local horizontalDist = math.sqrt(localPosX * localPosX + localPosZ * localPosZ)
		if horizontalDist == 0 then
			return
		end
		local adjustedRadius = adjustedRadiusBase - math.sqrt(localPosY + regionHalfY) / 2
		local invHorizontalDist = 1 / horizontalDist
		local projectedPosX = localPosX * invHorizontalDist * adjustedRadius
		local projectedPosZ = localPosZ * invHorizontalDist * adjustedRadius
		local projectedPosY = math.min(math.max(localPosY, -regionHalfY), regionHalfY)

		-- Calculate squared distances
		local distToCylinderX2 = (localPosX - projectedPosX) ^ 2
		local distToCylinderY2 = (localPosY - projectedPosY) ^ 2
		local distToCylinderZ2 = (localPosZ - projectedPosZ) ^ 2
		local distToCylinder = math.sqrt(distToCylinderX2 + distToCylinderY2 + distToCylinderZ2)

		local coilOffset = math.asin(math.sin(localPosY * fullCoils + 0.5 * math.atan(localPosX, localPosZ))) / fullCoils
		local displacementX = distToCylinder - halfWidth
		local displacementY = math.abs(coilOffset) - halfHeight
		local displacementXPos = math.max(displacementX, 0)
		local displacementYPos = math.max(displacementY, 0)

		local edgeDist =
			math.sqrt(displacementXPos * displacementXPos + displacementYPos * displacementYPos) +
			math.min(math.max(displacementX, displacementY), 0)

		local boundaryFactor = math.max(adjustedRadius / 2 - math.abs(horizontalDist - adjustedRadius), 0) * 0.4
		local isInside = 1.0 - (edgeDist * boundaryFactor) > 1.0

		if isInside then
			volume:setVoxel(x, y, z, color)
		end
	end

	vol.visitYXZ(node:volume(), region, visitor)
end
