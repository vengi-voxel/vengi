local module = {}

function module.conditionYXZ(volume, region, visitor, condition)
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

function module.conditionXZ(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		for z = mins.z, maxs.z do
			if condition(volume, x, z) then
				visitor(volume, x, z)
			end
		end
	end
end

function module.conditionYXZDown(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	for y = maxs.y, mins.y, -1 do
		for x = mins.x, maxs.x do
			for z = mins.z, maxs.z do
				if condition(volume, x, y, z) then
					visitor(volume, x, y, z)
				end
			end
		end
	end
end

function module.visitYXZDown(volume, region, visitor)
	local condition = function (x, y, z) return true end
	module.conditionYXZDown(volume, region, visitor, condition)
end

function module.visitYXZ(volume, region, visitor)
	local condition = function (x, y, z) return true end
	module.conditionYXZ(volume, region, visitor, condition)
end

function module.visitXZ(volume, region, visitor)
	local condition = function (x, z) return true end
	module.conditionXZ(volume, region, visitor, condition)
end

return module
