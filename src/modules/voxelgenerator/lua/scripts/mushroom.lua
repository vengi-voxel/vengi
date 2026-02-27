--
-- Mushroom generator
-- Creates various mushroom types: classic toadstool, flat shelf/bracket,
-- tall thin mushroom, puffball, and morel. Each with characteristic shapes,
-- spots, gills, and coloring.
--

function arguments()
	return {
		{ name = 'variety', desc = 'Mushroom type', type = 'enum', default = 'toadstool', enum = 'toadstool,shelf,tall,puffball,morel' },
		{ name = 'height', desc = 'Overall height', type = 'int', default = '10', min = '4', max = '30' },
		{ name = 'capRadius', desc = 'Cap radius', type = 'int', default = '6', min = '2', max = '16' },
		{ name = 'stemThickness', desc = 'Stem thickness', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'spots', desc = 'Show spots on the cap', type = 'bool', default = 'true' },
		{ name = 'ring', desc = 'Show stem ring (skirt)', type = 'bool', default = 'true' },
		{ name = 'capColor', desc = 'Cap top color', type = 'hexcolor', default = '#CC2222' },
		{ name = 'capUnderColor', desc = 'Cap underside / gill color', type = 'hexcolor', default = '#F5DEB3' },
		{ name = 'spotColor', desc = 'Spot color', type = 'hexcolor', default = '#FFFFF0' },
		{ name = 'stemColor', desc = 'Stem color', type = 'hexcolor', default = '#F5F5DC' },
		{ name = 'ringColor', desc = 'Stem ring color', type = 'hexcolor', default = '#EEEED1' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates mushrooms in various styles: toadstool, shelf, tall, puffball, or morel'
end

-- Get center bottom of region
local function getCenterBottom(region)
	local mins = region:mins()
	local maxs = region:maxs()
	return g_ivec3.new(
		math.floor((mins.x + maxs.x) / 2),
		mins.y,
		math.floor((mins.z + maxs.z) / 2)
	)
end

-- Classic toadstool / fly agaric style
local function buildToadstool(volume, pos, height, capRadius, stemThick, showSpots, showRing,
	capColor, capUnderColor, spotColor, stemColor, ringColor)

	-- Stem — slightly tapered cylinder
	local stemTop = g_ivec3.new(pos.x, pos.y + height, pos.z)
	g_shape.line(volume, pos, stemTop, stemColor, stemThick)

	-- Slight base bulge
	g_shape.dome(volume, pos, 'y', false, (stemThick + 1) * 2, math.max(1, stemThick), (stemThick + 1) * 2, stemColor)

	-- Stem ring / skirt
	if showRing then
		local ringY = pos.y + math.floor(height * 0.55)
		local ringPos = g_ivec3.new(pos.x, ringY, pos.z)
		local ringW = stemThick + 2
		g_shape.dome(volume, ringPos, 'y', false, ringW * 2, 1, ringW * 2, ringColor)
	end

	-- Cap — dome on top, inverted dome underneath for gills
	local capCenter = g_ivec3.new(pos.x, stemTop.y, pos.z)
	local capH = math.max(2, math.floor(capRadius * 0.6))

	-- Cap top (convex dome)
	g_shape.dome(volume, capCenter, 'y', false, capRadius * 2, capH, capRadius * 2, capColor)

	-- Cap underside (flat disc / gills)
	local gillPos = g_ivec3.new(pos.x, stemTop.y - 1, pos.z)
	g_shape.dome(volume, gillPos, 'y', true, capRadius * 2, math.max(1, math.floor(capH * 0.3)), capRadius * 2, capUnderColor)

	-- Spots on top of the cap
	if showSpots then
		local numSpots = math.random(4, 8 + capRadius)
		for _ = 1, numSpots do
			local angle = math.random() * 2 * math.pi
			local dist = math.random(1, capRadius - 1)
			local sx = math.floor(capCenter.x + math.cos(angle) * dist)
			local sz = math.floor(capCenter.z + math.sin(angle) * dist)
			-- Place spot on top of the dome surface
			local hFrac = 1.0 - (dist / capRadius)
			local sy = capCenter.y + math.max(0, math.floor(capH * hFrac * 0.7))
			g_shape.line(volume, g_ivec3.new(sx, sy, sz), g_ivec3.new(sx, sy, sz), spotColor, 1)
			-- Some spots are 2 voxels
			if math.random() > 0.5 and dist < capRadius - 2 then
				local sx2 = sx + math.random(-1, 1)
				local sz2 = sz + math.random(-1, 1)
				g_shape.line(volume, g_ivec3.new(sx2, sy, sz2), g_ivec3.new(sx2, sy, sz2), spotColor, 1)
			end
		end
	end
end

-- Shelf / bracket fungus — grows sideways from a surface
local function buildShelf(volume, pos, height, capRadius, stemThick,
	capColor, capUnderColor, stemColor)

	-- Vertical trunk / host surface that all shelves attach to
	local stemTop = g_ivec3.new(pos.x, pos.y + height, pos.z)
	g_shape.line(volume, pos, stemTop, stemColor, stemThick)

	-- Base bulge
	g_shape.dome(volume, pos, 'y', false, (stemThick + 1) * 2, math.max(1, stemThick), (stemThick + 1) * 2, stemColor)

	-- Multiple shelf layers attached directly to the trunk
	local numShelves = math.max(1, math.floor(height / 3))
	for s = 1, numShelves do
		local shelfY = pos.y + math.max(1, (s - 1) * 3 + 1)
		if shelfY >= pos.y + height then break end

		local shelfW = capRadius - math.random(0, 2)
		shelfW = math.max(2, shelfW)
		local shelfH = math.max(1, math.floor(shelfW * 0.25))

		-- Shelf extends to one side but stays connected to the trunk
		-- Use a small offset so the dome overlaps with the stem
		local offsetX = math.floor(shelfW * 0.3)
		local shelfPos = g_ivec3.new(pos.x + offsetX, shelfY, pos.z)

		-- Connection bridge from trunk to shelf center
		g_shape.line(volume, g_ivec3.new(pos.x, shelfY, pos.z), shelfPos, stemColor, 1)

		g_shape.dome(volume, shelfPos, 'y', false, shelfW * 2, shelfH, shelfW * 2, capColor)
		-- Underside
		g_shape.dome(volume, shelfPos, 'y', true, math.floor(shelfW * 1.5), math.max(1, shelfH - 1),
			math.floor(shelfW * 1.5), capUnderColor)
	end
end

-- Tall thin mushroom — long delicate stem with a small cap
local function buildTall(volume, pos, height, capRadius, stemThick, showRing,
	capColor, capUnderColor, stemColor, ringColor)

	-- Long thin stem
	local stemTop = g_ivec3.new(pos.x, pos.y + height, pos.z)
	local thinStem = math.max(1, stemThick - 1)
	g_shape.line(volume, pos, stemTop, stemColor, thinStem)

	-- Slight lean via a kink
	local kinkY = pos.y + math.floor(height * 0.6)
	local kinkOffset = math.random(-1, 1)
	if kinkOffset ~= 0 then
		local kinkPos = g_ivec3.new(pos.x + kinkOffset, kinkY, pos.z)
		g_shape.line(volume, kinkPos, stemTop, stemColor, thinStem)
	end

	-- Small delicate ring
	if showRing then
		local ringY = pos.y + math.floor(height * 0.65)
		local ringPos = g_ivec3.new(pos.x, ringY, pos.z)
		g_shape.dome(volume, ringPos, 'y', false, (thinStem + 1) * 2, 1, (thinStem + 1) * 2, ringColor)
	end

	-- Small conical cap
	local smallCap = math.max(2, math.floor(capRadius * 0.6))
	local capH = math.max(2, smallCap)
	local capCenter = g_ivec3.new(pos.x, stemTop.y, pos.z)
	g_shape.cone(volume, g_ivec3.new(capCenter.x, capCenter.y - 1, capCenter.z),
		'y', false, smallCap * 2, capH, smallCap * 2, capColor)

	-- Underside gills
	g_shape.dome(volume, g_ivec3.new(capCenter.x, capCenter.y - 1, capCenter.z),
		'y', true, smallCap * 2, 1, smallCap * 2, capUnderColor)
end

-- Puffball — round ball sitting on the ground, no real stem
local function buildPuffball(volume, pos, height, capRadius, showSpots,
	capColor, spotColor, stemColor)

	local radius = math.max(2, capRadius)
	local ballCenter = g_ivec3.new(pos.x, pos.y + radius, pos.z)

	-- Solid stem column connecting base to ball center
	g_shape.line(volume, pos, ballCenter, stemColor, math.max(1, math.floor(radius * 0.5)))

	-- Main ball body — solid dome on top
	g_shape.dome(volume, pos, 'y', false, radius * 2, radius * 2, radius * 2, capColor)

	-- Upper hemisphere for roundness
	g_shape.dome(volume, ballCenter, 'y', false, radius * 2, radius, radius * 2, capColor)

	-- Base attachment
	g_shape.dome(volume, pos, 'y', false, math.floor(radius * 1.0),
		math.max(1, math.floor(radius * 0.4)), math.floor(radius * 1.0), stemColor)

	-- Surface texture — scattered spots / patches
	if showSpots then
		local numPatches = math.random(5, 10 + radius)
		for _ = 1, numPatches do
			local angle1 = math.random() * 2 * math.pi
			local angle2 = math.random() * math.pi * 0.5 -- upper hemisphere only
			local dist = radius
			local px = math.floor(ballCenter.x + math.cos(angle1) * math.sin(angle2) * dist)
			local py = math.floor(ballCenter.y + math.cos(angle2) * dist * 0.8)
			local pz = math.floor(ballCenter.z + math.sin(angle1) * math.sin(angle2) * dist)
			g_shape.line(volume, g_ivec3.new(px, py, pz), g_ivec3.new(px, py, pz), spotColor, 1)
		end
	end

	-- Opening at the top (mature puffball)
	if height > 8 then
		local openPos = g_ivec3.new(ballCenter.x, ballCenter.y + radius - 1, ballCenter.z)
		g_shape.dome(volume, openPos, 'y', false, 3, 1, 3, spotColor)
	end
end

-- Morel — honeycomb-like pitted cap on a pale stem
local function buildMorel(volume, pos, height, capRadius, stemThick,
	capColor, capUnderColor, stemColor)

	-- Pale hollow stem
	local stemHeight = math.max(2, math.floor(height * 0.4))
	local stemTop = g_ivec3.new(pos.x, pos.y + stemHeight, pos.z)
	g_shape.line(volume, pos, stemTop, stemColor, stemThick)

	-- Stem base widens slightly
	g_shape.dome(volume, pos, 'y', false, (stemThick + 1) * 2, math.max(1, stemThick - 1),
		(stemThick + 1) * 2, stemColor)

	-- Morel cap — elongated conical/ellipsoid shape with pits
	local capH = height - stemHeight
	capH = math.max(3, capH)
	local capW = math.max(2, capRadius)

	-- Main cap body (elongated ellipsoid via cone + dome)
	g_shape.cone(volume, stemTop, 'y', false, capW * 2, capH, capW * 2, capColor)
	g_shape.dome(volume, g_ivec3.new(pos.x, stemTop.y + capH - 2, pos.z), 'y', false,
		math.floor(capW * 1.2), math.max(1, math.floor(capW * 0.5)), math.floor(capW * 1.2), capColor)

	-- Pits / honeycomb texture — erase small holes in the cap surface
	local numPits = math.random(6, 12 + capW * 2)
	for _ = 1, numPits do
		local pitAngle = math.random() * 2 * math.pi
		local pitH = stemTop.y + math.random(1, capH - 1)
		-- Distance from center decreases as we go up (cone shape)
		local heightFrac = (pitH - stemTop.y) / capH
		local maxDist = math.max(1, math.floor(capW * (1.0 - heightFrac * 0.7)))
		local pitDist = math.random(1, maxDist)
		local pitX = math.floor(pos.x + math.cos(pitAngle) * pitDist)
		local pitZ = math.floor(pos.z + math.sin(pitAngle) * pitDist)
		-- Use the underside color for the pits to create depth illusion
		g_shape.line(volume, g_ivec3.new(pitX, pitH, pitZ), g_ivec3.new(pitX, pitH, pitZ), capUnderColor, 1)
	end
end

function main(node, region, color, variety, height, capRadius, stemThickness, spots, ring,
	capColor, capUnderColor, spotColor, stemColor, ringColor, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local pos = getCenterBottom(region)

	if variety == 'toadstool' then
		buildToadstool(volume, pos, height, capRadius, stemThickness, spots, ring,
			capColor, capUnderColor, spotColor, stemColor, ringColor)
	elseif variety == 'shelf' then
		buildShelf(volume, pos, height, capRadius, stemThickness,
			capColor, capUnderColor, stemColor)
	elseif variety == 'tall' then
		buildTall(volume, pos, height, capRadius, stemThickness, ring,
			capColor, capUnderColor, stemColor, ringColor)
	elseif variety == 'puffball' then
		buildPuffball(volume, pos, height, capRadius, spots,
			capColor, spotColor, stemColor)
	elseif variety == 'morel' then
		buildMorel(volume, pos, height, capRadius, stemThickness,
			capColor, capUnderColor, stemColor)
	end
end
