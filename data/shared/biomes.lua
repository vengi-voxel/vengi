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
function addBiome(lower, upper, humidity, temperature, voxelType, underGround, treeDistribution)
  return biomeMgr.addBiome(lower, upper, humidity, temperature, voxelType, underGround, treeDistribution)
end

---
-- The lower the value the higher the amount of trees in that biome.
--
function treeDistance(humidity, temperature)
  local dist = 30
  if temperature > 0.7 or humidity < 0.2 then
    dist = 55
  elseif temperature > 0.9 or humidity < 0.1 then
    dist = 65;
  end
  return math.floor(dist * 1.0 / temperature * humidity);
end

---
-- @biome The biome to add the tree type to
-- @tree Valid values are defined by the assets available in the data dir. The type defines the directory
-- to load the tree volumes from
--
function addTree(biome, tree)
  biome:addTree(tree)
end

function addSand(lower, upper, humidity, temperature, underGround)
  local sand = addBiome(lower, upper, humidity, temperature, "Sand", underGround, treeDistance(humidity, temperature))
  addTree(sand, "palm")
  return sand
end

function addGrass(lower, upper, humidity, temperature, underGround)
  local grass = addBiome(lower, upper, humidity, temperature, "Grass", underGround, treeDistance(humidity, temperature))
  addTree(grass, "pine")
  addTree(grass, "fir")
  return grass
end

---
-- Set up the biomes
--
function initBiomes()
  addSand(0, MAX_WATER_HEIGHT + 4, 0.5, 0.5, false)
  addSand(0, MAX_TERRAIN_HEIGHT - 1, 0.1, 0.9, false)
  addBiome(MAX_WATER_HEIGHT + 3, MAX_WATER_HEIGHT + 10, 1.0, 0.7, "Dirt")
  local biome = addGrass(MAX_WATER_HEIGHT + 3, MAX_TERRAIN_HEIGHT + 1, 0.5, 0.5)
  biomeMgr.setDefault(biome)
  addBiome(MAX_TERRAIN_HEIGHT - 20, MAX_TERRAIN_HEIGHT + 1, 0.4, 0.5, "Rock")
  addBiome(0, MAX_TERRAIN_HEIGHT - 1, 0.4, 0.5, "Rock", true)
end

function initCities()
  biomeMgr.addCity(ivec2.new(0, 0), 500.0)
end
