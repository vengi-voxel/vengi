local MAX_TERRAIN_HEIGHT = 100
local MAX_MOUNTAIN_HEIGHT = MAX_TERRAIN_HEIGHT + 60
local MAX_WATER_HEIGHT = 10

function initBiomes()
	BiomeManager.addBiome(0, MAX_WATER_HEIGHT + 4, 0.5, 0.5, "Sand");
	BiomeManager.addBiome(0, MAX_TERRAIN_HEIGHT - 1, 0.1, 0.9, "Sand");
	BiomeManager.addBiome(MAX_WATER_HEIGHT + 3, MAX_WATER_HEIGHT + 10, 1.0, 0.7, "Dirt");
	BiomeManager.addBiome(MAX_WATER_HEIGHT + 3, MAX_TERRAIN_HEIGHT + 1, 0.5, 0.5, "Grass");
	BiomeManager.addBiome(MAX_TERRAIN_HEIGHT - 20, MAX_TERRAIN_HEIGHT + 1, 0.4, 0.5, "Rock");
	BiomeManager.addBiome(0, MAX_TERRAIN_HEIGHT - 1, 0.4, 0.5, "Rock", true);
end