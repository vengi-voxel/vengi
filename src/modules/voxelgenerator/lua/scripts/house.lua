--
-- Procedural voxel house generator with many configurable parameters and
-- seed-based randomness for varied results on each run.
--

function arguments()
	return {
		{ name = 'width',           desc = 'House width (x-axis)',                       type = 'int',        default = '12',   min = '6',   max = '64' },
		{ name = 'depth',           desc = 'House depth (z-axis)',                        type = 'int',        default = '10',   min = '6',   max = '64' },
		{ name = 'stories',         desc = 'Number of stories',                          type = 'int',        default = '1',    min = '1',   max = '5' },
		{ name = 'storyHeight',     desc = 'Wall height per story (interior)',            type = 'int',        default = '5',    min = '3',   max = '12' },
		{ name = 'roofStyle',       desc = 'Roof style',                                 type = 'enum',       default = 'gable', enum = 'gable,hip,flat' },
		{ name = 'roofOverhang',    desc = 'Roof overhang in voxels',                    type = 'int',        default = '1',    min = '0',   max = '4' },
		{ name = 'windowWidth',     desc = 'Window width',                               type = 'int',        default = '1',    min = '1',   max = '3' },
		{ name = 'windowHeight',    desc = 'Window height',                              type = 'int',        default = '2',    min = '1',   max = '4' },
		{ name = 'windowSpacing',   desc = 'Min spacing between windows',                type = 'int',        default = '3',    min = '2',   max = '8' },
		{ name = 'doorWidth',       desc = 'Door width',                                 type = 'int',        default = '2',    min = '1',   max = '4' },
		{ name = 'doorHeight',      desc = 'Door height',                                type = 'int',        default = '3',    min = '2',   max = '5' },
		{ name = 'hasChimney',      desc = 'Generate a chimney',                         type = 'bool',       default = 'true' },
		{ name = 'floorThickness',  desc = 'Floor / ceiling thickness',                  type = 'int',        default = '1',    min = '1',   max = '2' },
		{ name = 'wallColor',       desc = 'Wall color index',                           type = 'colorindex', default = '1' },
		{ name = 'roofColor',       desc = 'Roof color index',                           type = 'colorindex', default = '2' },
		{ name = 'floorColor',      desc = 'Floor color index',                          type = 'colorindex', default = '3' },
		{ name = 'doorColor',       desc = 'Door color index',                           type = 'colorindex', default = '4' },
		{ name = 'windowColor',     desc = 'Window frame color index',                   type = 'colorindex', default = '5' },
		{ name = 'chimneyColor',    desc = 'Chimney color index',                        type = 'colorindex', default = '6' },
		{ name = 'interiorColor',   desc = 'Interior furniture color index',             type = 'colorindex', default = '7' },
		{ name = 'seed',            desc = 'Random seed (0 = random)',                   type = 'int',        default = '0' }
	}
end

function description()
	return "Generates a procedural voxel house with walls, roof, floor, windows, doors, chimney, and basic interior furniture."
end

-- ---------------------------------------------------------------------------
-- Helpers
-- ---------------------------------------------------------------------------

--- Place a filled box from (x0,y0,z0) to (x1,y1,z1) inclusive.
local function box(volume, x0, y0, z0, x1, y1, z1, col)
	for y = y0, y1 do
		for x = x0, x1 do
			for z = z0, z1 do
				volume:setVoxel(x, y, z, col)
			end
		end
	end
end

--- Place a hollow box (walls only, no top/bottom).
local function hollowWalls(volume, x0, y0, z0, x1, y1, z1, col)
	for y = y0, y1 do
		for x = x0, x1 do
			volume:setVoxel(x, y, z0, col)
			volume:setVoxel(x, y, z1, col)
		end
		for z = z0 + 1, z1 - 1 do
			volume:setVoxel(x0, y, z, col)
			volume:setVoxel(x1, y, z, col)
		end
	end
end

--- Carve out (set to air = -1) a rectangular hole.
local function carve(volume, x0, y0, z0, x1, y1, z1)
	for y = y0, y1 do
		for x = x0, x1 do
			for z = z0, z1 do
				volume:setVoxel(x, y, z, -1)
			end
		end
	end
end

-- ---------------------------------------------------------------------------
-- Windows
-- ---------------------------------------------------------------------------

--- Place windows along one wall face.
--- face: "x-" | "x+" | "z-" | "z+"
local function placeWindows(volume, x0, y0, z0, x1, y1, z1, face, ww, wh, spacing, wallCol, winCol)
	-- determine the range to iterate and the fixed coordinate, and function to set a voxel
	local lo, hi, fixedCoord
	local setPixel
	if face == "z-" then
		lo = x0 + 1; hi = x1 - 1
		fixedCoord = z0
		setPixel = function(i, j) volume:setVoxel(i, j, fixedCoord, winCol) end
	elseif face == "z+" then
		lo = x0 + 1; hi = x1 - 1
		fixedCoord = z1
		setPixel = function(i, j) volume:setVoxel(i, j, fixedCoord, winCol) end
	elseif face == "x-" then
		lo = z0 + 1; hi = z1 - 1
		fixedCoord = x0
		setPixel = function(i, j) volume:setVoxel(fixedCoord, j, i, winCol) end
	elseif face == "x+" then
		lo = z0 + 1; hi = z1 - 1
		fixedCoord = x1
		setPixel = function(i, j) volume:setVoxel(fixedCoord, j, i, winCol) end
	end

	local windowBottom = y0 + math.max(1, math.floor((y1 - y0 - wh) / 2))
	local windowTop = windowBottom + wh - 1

	if windowTop > y1 - 1 then return end

	local pos = lo + 1
	while pos + ww - 1 <= hi - 1 do
		for wi = 0, ww - 1 do
			for wj = 0, wh - 1 do
				setPixel(pos + wi, windowBottom + wj)
			end
		end
		pos = pos + ww + spacing
	end
end

-- ---------------------------------------------------------------------------
-- Roofs
-- ---------------------------------------------------------------------------

local function buildGableRoof(volume, x0, y0, z0, x1, z1, overhang, roofCol)
	-- gable along z-axis (ridge runs along z)
	local halfW = math.floor((x1 - x0) / 2)
	local cx = x0 + halfW
	local isEven = ((x1 - x0) % 2 == 0)

	for layer = 0, halfW do
		local lx0 = x0 - overhang + layer
		local lx1 = x1 + overhang - layer
		if isEven and layer == halfW then
			lx0 = cx
			lx1 = cx
		end
		local ly = y0 + layer
		local lz0 = z0 - overhang
		local lz1 = z1 + overhang
		box(volume, lx0, ly, lz0, lx1, ly, lz1, roofCol)
	end
end

local function buildHipRoof(volume, x0, y0, z0, x1, z1, overhang, roofCol)
	local halfW = math.floor((x1 - x0) / 2)
	local halfD = math.floor((z1 - z0) / 2)
	local maxLayers = math.min(halfW, halfD)

	for layer = 0, maxLayers do
		local lx0 = x0 - overhang + layer
		local lx1 = x1 + overhang - layer
		local lz0 = z0 - overhang + layer
		local lz1 = z1 + overhang - layer
		if lx0 > lx1 or lz0 > lz1 then break end
		box(volume, lx0, y0 + layer, lz0, lx1, y0 + layer, lz1, roofCol)
	end
end

local function buildFlatRoof(volume, x0, y0, z0, x1, z1, overhang, roofCol)
	box(volume, x0 - overhang, y0, z0 - overhang, x1 + overhang, y0, z1 + overhang, roofCol)
	-- small parapet
	hollowWalls(volume, x0 - overhang, y0 + 1, z0 - overhang, x1 + overhang, y0 + 1, z1 + overhang, roofCol)
end

-- ---------------------------------------------------------------------------
-- Interior furniture helpers
-- ---------------------------------------------------------------------------

local function placeTable(volume, cx, y, cz, col)
	-- a small 3x1x3 table top on a 1x1 leg
	box(volume, cx - 1, y + 1, cz - 1, cx + 1, y + 1, cz + 1, col)
	volume:setVoxel(cx, y, cz, col)
end

local function placeChair(volume, x, y, z, col)
	-- seat (1 voxel) + back (1 voxel above seat)
	volume:setVoxel(x, y, z, col)
	volume:setVoxel(x, y + 1, z, col)
end

local function placeBed(volume, x0, y, z0, col)
	-- bed: 2 wide, 3 long, 1 tall with a headboard
	box(volume, x0, y, z0, x0 + 1, y, z0 + 2, col)
	box(volume, x0, y + 1, z0 + 2, x0 + 1, y + 1, z0 + 2, col)
end

local function placeShelf(volume, x, y0, z, col)
	-- a 1-wide shelf: 3 voxels tall on the wall
	for dy = 0, 2 do
		volume:setVoxel(x, y0 + dy, z, col)
	end
end

local function placeInterior(volume, x0, y0, z0, x1, y1, z1, col)
	local iw = x1 - x0 - 1  -- usable interior width
	local id = z1 - z0 - 1  -- usable interior depth
	if iw < 3 or id < 3 then return end

	local ix0 = x0 + 1
	local iz0 = z0 + 1
	local ix1 = x1 - 1
	local iz1 = z1 - 1

	-- table in center
	local tcx = math.floor((ix0 + ix1) / 2)
	local tcz = math.floor((iz0 + iz1) / 2)
	placeTable(volume, tcx, y0, tcz, col)

	-- chairs around table
	if tcx - 2 >= ix0 then placeChair(volume, tcx - 2, y0, tcz, col) end
	if tcx + 2 <= ix1 then placeChair(volume, tcx + 2, y0, tcz, col) end

	-- bed in one corner (if space)
	if iw >= 5 and id >= 5 then
		placeBed(volume, ix0, y0, iz0, col)
	end

	-- shelf on a wall
	if y1 - y0 >= 3 then
		placeShelf(volume, ix1, y0, iz0 + math.floor(id / 2), col)
	end
end

-- ---------------------------------------------------------------------------
-- Chimney
-- ---------------------------------------------------------------------------

local function placeChimney(volume, x0, y0, z0, x1, y_roof_peak, chimneyCol)
	-- chimney on one corner of the house, extends above roof peak
	local cx = x1 - 1
	local cz = z0 + 1
	local chimTop = y_roof_peak + 2
	box(volume, cx, y0, cz, cx + 1, chimTop, cz + 1, chimneyCol)
	-- hollow the inside at the top for a fireplace look
	carve(volume, cx, chimTop, cz, cx, chimTop, cz)
end

-- ---------------------------------------------------------------------------
-- Door
-- ---------------------------------------------------------------------------

local function placeDoor(volume, x0, y0, z0, x1, dw, dh, doorCol)
	-- door centered on the front wall (z = z0)
	local cx = math.floor((x0 + x1) / 2)
	local dx0 = cx - math.floor(dw / 2)
	local dx1 = dx0 + dw - 1
	-- carve opening
	carve(volume, dx0, y0, z0, dx1, y0 + dh - 1, z0)
	-- door frame
	for dy = 0, dh - 1 do
		volume:setVoxel(dx0, y0 + dy, z0, doorCol)
		if dx1 ~= dx0 then
			volume:setVoxel(dx1, y0 + dy, z0, doorCol)
		end
	end
	for dx = dx0, dx1 do
		volume:setVoxel(dx, y0 + dh - 1, z0, doorCol)
	end
	-- leave center of door as air
	carve(volume, dx0 + math.floor((dw > 1 and 1 or 0)), y0, z0, dx1 - math.floor((dw > 1 and 1 or 0)), y0 + dh - 2, z0)
end

-- ---------------------------------------------------------------------------
-- Stairs (between stories)
-- ---------------------------------------------------------------------------

local function placeStairs(volume, x0, y0, z0, x1, z1, storyH, col)
	-- simple straight staircase along z near one wall
	local sx = x0 + 1
	local sz0 = z0 + 1
	local steps = storyH
	if sz0 + steps - 1 > z1 - 1 then
		steps = z1 - 1 - sz0 + 1
	end
	for i = 0, steps - 1 do
		box(volume, sx, y0 + i, sz0 + i, sx + 1, y0 + i, sz0 + i, col)
	end
end

-- ---------------------------------------------------------------------------
-- Main
-- ---------------------------------------------------------------------------

function main(node, region, color, width, depth, stories, storyHeight, roofStyle,
              roofOverhang, windowWidth, windowHeight, windowSpacing,
              doorWidth, doorHeight, hasChimney, floorThickness,
              wallColor, roofColor, floorColor, doorColor, windowColor,
              chimneyColor, interiorColor, seed)

	-- seed
	if seed == 0 then seed = os.time() end
	math.randomseed(seed)

	local volume = node:volume()
	local mins = region:mins()

	-- base position — inset by roof overhang so the overhang stays inside the region
	local bx = mins.x + roofOverhang
	local by = mins.y
	local bz = mins.z + roofOverhang

	-- add small random variation to dimensions (up to +/-1) for organic feel
	width = width + math.random(-1, 1)
	depth = depth + math.random(-1, 1)
	if width < 5 then width = 5 end
	if depth < 5 then depth = 5 end

	-- total wall height for all stories (including floors between)
	local totalWallH = stories * storyHeight + (stories) * floorThickness

	-- build each story
	for s = 0, stories - 1 do
		local sy = by + s * (storyHeight + floorThickness)
		local sx0 = bx
		local sz0 = bz
		local sx1 = bx + width - 1
		local sz1 = bz + depth - 1

		-- floor
		box(volume, sx0, sy, sz0, sx1, sy + floorThickness - 1, sz1, floorColor)

		-- walls (hollow)
		local wallBot = sy + floorThickness
		local wallTop = sy + floorThickness + storyHeight - 1
		hollowWalls(volume, sx0, wallBot, sz0, sx1, wallTop, sz1, wallColor)

		-- windows on each face
		placeWindows(volume, sx0, wallBot, sz0, sx1, wallTop, sz1, "z-", windowWidth, windowHeight, windowSpacing, wallColor, windowColor)
		placeWindows(volume, sx0, wallBot, sz0, sx1, wallTop, sz1, "z+", windowWidth, windowHeight, windowSpacing, wallColor, windowColor)
		placeWindows(volume, sx0, wallBot, sz0, sx1, wallTop, sz1, "x-", windowWidth, windowHeight, windowSpacing, wallColor, windowColor)
		placeWindows(volume, sx0, wallBot, sz0, sx1, wallTop, sz1, "x+", windowWidth, windowHeight, windowSpacing, wallColor, windowColor)

		-- door on ground floor only (front face z-)
		if s == 0 then
			placeDoor(volume, sx0, wallBot, sz0, sx1, doorWidth, doorHeight, doorColor)
		end

		-- interior furniture (simple, varies per story via randomness)
		placeInterior(volume, sx0, wallBot, sz0, sx1, wallTop, sz1, interiorColor)

		-- stairs to next floor (except topmost story)
		if s < stories - 1 then
			-- carve stairwell opening in ceiling
			local stairX = sx0 + 1
			local stairZ = sz0 + 1
			local stairLen = math.min(storyHeight, sz1 - sz0 - 2)
			carve(volume, stairX, wallTop + 1, stairZ, stairX + 1, wallTop + floorThickness, stairZ + stairLen - 1)
			placeStairs(volume, sx0, wallBot, sz0, sx1, sz1, storyHeight, floorColor)
		end
	end

	-- roof
	local roofY = by + stories * (storyHeight + floorThickness)
	local rx0 = bx
	local rz0 = bz
	local rx1 = bx + width - 1
	local rz1 = bz + depth - 1

	if roofStyle == "gable" then
		buildGableRoof(volume, rx0, roofY, rz0, rx1, rz1, roofOverhang, roofColor)
	elseif roofStyle == "hip" then
		buildHipRoof(volume, rx0, roofY, rz0, rx1, rz1, roofOverhang, roofColor)
	elseif roofStyle == "flat" then
		buildFlatRoof(volume, rx0, roofY, rz0, rx1, rz1, roofOverhang, roofColor)
	end

	-- chimney
	if hasChimney then
		local roofPeak
		if roofStyle == "gable" then
			roofPeak = roofY + math.floor((rx1 - rx0) / 2)
		elseif roofStyle == "hip" then
			roofPeak = roofY + math.min(math.floor((rx1 - rx0) / 2), math.floor((rz1 - rz0) / 2))
		else
			roofPeak = roofY + 1
		end
		placeChimney(volume, rx0, roofY, rz0, rx1, roofPeak, chimneyColor)
	end

	-- random decorative touches: flower boxes below some windows (ground floor)
	local wallBot = by + floorThickness
	local wallTop = by + floorThickness + storyHeight - 1
	local windowBottom = wallBot + math.max(1, math.floor((wallTop - wallBot - windowHeight) / 2))
	-- flower boxes on front and back
	for _, face in ipairs({"z-", "z+"}) do
		local lo, hi, fz
		if face == "z-" then
			lo = bx + 2; hi = bx + width - 3; fz = bz - 1
		else
			lo = bx + 2; hi = bx + width - 3; fz = bz + depth
		end
		local pos = lo
		while pos + windowWidth - 1 <= hi do
			if math.random() > 0.5 and fz >= region:mins().z and fz <= region:maxs().z then
				for wi = 0, windowWidth - 1 do
					volume:setVoxel(pos + wi, windowBottom - 1, fz, interiorColor)
				end
			end
			pos = pos + windowWidth + windowSpacing
		end
	end
end
