local MAX_TERRAIN_HEIGHT = 100
local MAX_MOUNTAIN_HEIGHT = MAX_TERRAIN_HEIGHT + 60
local MAX_WATER_HEIGHT = 10

---
-- @lower lower y (height) level for this biome
-- @upper upper y (height) level for this biome
-- @humidity [0.0,1.0]
-- @temperature [0.0,1.0]
-- @voxelType Valid values:
-- Air, Water, Generic,Grass, Wood, Leaf, LeafFir, LeafPine,
-- Flower, Bloom, Mushroom, Rock, Sand, Cloud, Dirt, Roof, Wall
-- @underGround Only used when not a surface
--
function addBiome(lower, upper, humidity, temperature, voxelType, underGround)
	return biomeMgr.addBiome(lower, upper, humidity, temperature, voxelType, underGround)
end

---
-- @biome The biome to add the tree type to
-- @tree Valid values:
-- Dome, DomeHangingLeaves, Cone, Ellipsis, BranchesEllipsis, Cube,
-- CubeSideCubes, Pine, Fir, Palm, SpaceColonization
--
function addTree(biome, tree)
	biome:addTree(tree)
end

function addSand(lower, upper, humidity, temperature, underGround)
	local sand = addBiome(lower, upper, humidity, temperature, "Sand", underGround)
	addTree(sand, "Palm")
	return sand
end

function addGrass(lower, upper, humidity, temperature, underGround)
	local grass = addBiome(lower, upper, humidity, temperature, "Grass", underGround)
	addTree(grass, "SpaceColonization")
end

---
-- Set up the biomes
--
function initBiomes()
	addSand(0, MAX_WATER_HEIGHT + 4, 0.5, 0.5, false)
	addSand(0, MAX_TERRAIN_HEIGHT - 1, 0.1, 0.9, false)
	addBiome(MAX_WATER_HEIGHT + 3, MAX_WATER_HEIGHT + 10, 1.0, 0.7, "Dirt")
	addGrass(MAX_WATER_HEIGHT + 3, MAX_TERRAIN_HEIGHT + 1, 0.5, 0.5)
	addBiome(MAX_TERRAIN_HEIGHT - 20, MAX_TERRAIN_HEIGHT + 1, 0.4, 0.5, "Rock")
	addBiome(0, MAX_TERRAIN_HEIGHT - 1, 0.4, 0.5, "Rock", true)
end
