local module = {}

function module.conditionXYZ(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	for y = mins.y, maxs.y do
		for x = mins.x, maxs.x do
			for z = mins.z, maxs.z do
				if condition(volume, x, y, z) then
					visitor(volume, x, y, z)
				end
			end
		end
	end
end

function module.visitXYZ(volume, region, visitor)
	local condition = function (x, y, z) return true end
	module.conditionXYZ(volume, region, visitor, condition)
end

return module
