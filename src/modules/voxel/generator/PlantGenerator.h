#pragma once

#include "voxel/polyvox/Mesh.h"
#include "voxel/polyvox/RawVolume.h"

namespace voxel {

enum PlantType {
	Flower,
	Mushroom,
	Grass,

	MaxPlantTypes
};

class PlantGenerator {
private:
	Mesh* _meshes[MaxPlantTypes];

	void createFlower(int size, glm::ivec3 pos, RawVolume& volume) const;
	void createGrass(int size, glm::ivec3 pos, RawVolume& volume) const;
	void createMushroom(int size, glm::ivec3 pos, RawVolume& volume) const;

public:
	PlantGenerator();
	~PlantGenerator();

	void shutdown();

	bool generatePlant(int size, PlantType type, Mesh *result);

	Mesh* getMesh(PlantType type);

	void generateAll();
};

}
