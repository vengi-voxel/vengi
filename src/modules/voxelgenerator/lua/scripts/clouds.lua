function arguments()
	return {
		{ name = 'cloudcount', desc = 'number of cloud clusters to generate', type = 'int', default = '5', min = '1', max = '50' },
		{ name = 'density', desc = 'cloud density (higher = more solid)', type = 'float', default = '0.6', min = '0.1', max = '1.0' },
		{ name = 'puffiness', desc = 'cloud puffiness factor', type = 'float', default = '1.5', min = '0.5', max = '3.0' },
		{ name = 'basesize', desc = 'base size of each cloud cluster', type = 'int', default = '15', min = '5', max = '50' },
		{ name = 'sizevariance', desc = 'size variance between clouds', type = 'float', default = '0.5', min = '0.0', max = '1.0' },
		{ name = 'noise_freq', desc = 'frequency for noise-based cloud shape', type = 'float', default = '0.15', min = '0.01', max = '1.0' },
		{ name = 'vertical_squash', desc = 'vertical compression (< 1.0 = flatter clouds)', type = 'float', default = '0.6', min = '0.1', max = '2.0' },
		{ name = 'edge_softness', desc = 'softness of cloud edges (higher = wispier)', type = 'float', default = '0.3', min = '0.0', max = '0.8' },
		{ name = 'seed', desc = 'random seed for reproducible clouds', type = 'int', default = '42' }
	}
end

function description()
	return "Generates realistic fluffy voxel clouds with multiple clusters and noise-based organic shapes."
end

-- Helper function for smooth falloff
local function smoothstep(edge0, edge1, x)
	local t = math.max(0, math.min(1, (x - edge0) / (edge1 - edge0)))
	return t * t * (3 - 2 * t)
end

-- Generate a single cloud puff at a specific position
local function generateCloudPuff(volume, centerX, centerY, centerZ, radius, density, puffiness, noise_freq, vertical_squash, edge_softness, color)
	local radiusSquashed = math.floor(radius)
	local radiusVertical = math.floor(radius * vertical_squash)

	centerX = math.floor(centerX)
	centerY = math.floor(centerY)
	centerZ = math.floor(centerZ)

	-- Scan a bounding box around the cloud center
	for x = centerX - radiusSquashed - 2, centerX + radiusSquashed + 2 do
		for y = centerY - radiusVertical - 2, centerY + radiusVertical + 2 do
			for z = centerZ - radiusSquashed - 2, centerZ + radiusSquashed + 2 do
				-- Calculate distance from center with vertical squashing
				local dx = x - centerX
				local dy = (y - centerY) / vertical_squash
				local dz = z - centerZ
				local dist = math.sqrt(dx * dx + dy * dy + dz * dz)

				-- Use 3D noise to add organic variations
				local noiseValue = g_noise.noise3(x * noise_freq, y * noise_freq, z * noise_freq)

				-- Add puffiness by modulating the effective radius with noise
				local puffModulation = 1.0 + (noiseValue * puffiness * 0.3)
				local effectiveRadius = radius * puffModulation

				-- Calculate density falloff from center
				local normalizedDist = dist / effectiveRadius

				-- Create soft edges with smoothstep
				local edgeStart = 1.0 - edge_softness
				local cloudValue = 1.0 - smoothstep(edgeStart, 1.0, normalizedDist)

				-- Add additional noise-based variations
				cloudValue = cloudValue + noiseValue * 0.15

				-- Place voxel if value exceeds density threshold
				if cloudValue > (1.0 - density) and normalizedDist < 1.0 then
					-- Add some randomness to make clouds less uniform
					if math.random() < (cloudValue * 0.8 + 0.2) then
						volume:setVoxel(x, y, z, color)
					end
				end
			end
		end
	end
end

function main(node, region, color, cloudcount, density, puffiness, basesize, sizevariance, noise_freq, vertical_squash, edge_softness, seed)
	local volume = node:volume()
	local mins = region:mins()
	local maxs = region:maxs()

	local width = maxs.x - mins.x
	local height = maxs.y - mins.y
	local depth = maxs.z - mins.z

	math.randomseed(seed)

	-- Calculate maximum cloud size with variance to determine margin
	local maxCloudSize = basesize * (1.0 + sizevariance)
	-- Account for satellite puffs extending beyond main cloud (0.6 offset + 0.7 satellite size)
	local maxExtent = math.ceil(maxCloudSize * 1.3)

	-- Calculate safe region boundaries with margins
	local marginX = math.min(maxExtent, math.floor(width * 0.5))
	local marginY = math.min(math.ceil(maxExtent * vertical_squash), math.floor(height * 0.5))
	local marginZ = math.min(maxExtent, math.floor(depth * 0.5))

	-- Only generate clouds if region is large enough
	local safeWidth = width - 2 * marginX
	local safeHeight = height - 2 * marginY
	local safeDepth = depth - 2 * marginZ

	if safeWidth < 1 or safeHeight < 1 or safeDepth < 1 then
		-- Region too small for safe cloud placement, generate at center
		local centerX = mins.x + math.floor(width * 0.5)
		local centerY = mins.y + math.floor(height * 0.5)
		local centerZ = mins.z + math.floor(depth * 0.5)
		generateCloudPuff(volume, centerX, centerY, centerZ, basesize, density, puffiness,
		                  noise_freq, vertical_squash, edge_softness, color)
		return
	end

	for _ = 1, cloudcount do
		-- Random position within safe boundaries (with margins)
		local cloudX = mins.x + marginX + math.random(0, safeWidth)
		local cloudY = mins.y + math.max(marginY, math.floor(height * 0.3)) + math.random(0, math.max(0, height - math.max(marginY, math.floor(height * 0.3)) - marginY))
		local cloudZ = mins.z + marginZ + math.random(0, safeDepth)

		-- Vary the size of each cloud
		local sizeVariation = 1.0 + (math.random() - 0.5) * sizevariance * 2
		local cloudSize = basesize * sizeVariation

		-- Generate main cloud puff
		generateCloudPuff(volume, cloudX, cloudY, cloudZ, cloudSize, density, puffiness,
		                  noise_freq, vertical_squash, edge_softness, color)

		-- Add smaller satellite puffs for more complex cloud structure
		local satelliteCount = math.random(2, 4)
		for _ = 1, satelliteCount do
			local offsetDistance = math.floor(cloudSize * 0.6)
			local offsetX = cloudX + math.random(-offsetDistance, offsetDistance)
			local offsetY = cloudY + math.random(math.floor(-offsetDistance * 0.5), math.floor(offsetDistance * 0.5))
			local offsetZ = cloudZ + math.random(-offsetDistance, offsetDistance)

			-- Satellite puffs are smaller
			local satelliteSize = cloudSize * (0.4 + math.random() * 0.3)

			generateCloudPuff(volume, offsetX, offsetY, offsetZ, satelliteSize, density * 0.9,
			                  puffiness, noise_freq, vertical_squash, edge_softness, color)
		end
	end
end
