/**
 * @file
 */

enum VoxelType {
	Air = 0,
	Water,
	Generic,
	Grass,
	Wood,
	Leaf,
	LeafFir,
	LeafPine,
	Flower,
	Bloom,
	Mushroom,
	Rock,
	Sand,
	Cloud,
	Dirt,
	Roof,
	Wall,

	Max
};

bool isBlocked(VoxelType material) {
	return material != VoxelType::Air;
}

bool isWater(VoxelType material) {
	return material == VoxelType::Water;
}

bool isLeaves(VoxelType material) {
	return material == VoxelType::Leaf;
}

bool isAir(VoxelType material) {
	return material == VoxelType::Air;
}

bool isWood(VoxelType material) {
	return material == VoxelType::Wood;
}

bool isGrass(VoxelType material) {
	return material == VoxelType::Grass;
}

bool isRock(VoxelType material) {
	return material == VoxelType::Rock;
}

bool isSand(VoxelType material) {
	return material == VoxelType::Sand;
}

bool isDirt(VoxelType material) {
	return material == VoxelType::Dirt;
}

bool isFloor(VoxelType material) {
	return isRock(material) || isDirt(material) || isSand(material) || isGrass(material);
}
