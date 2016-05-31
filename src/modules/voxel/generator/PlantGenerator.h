#pragma once

#include "voxel/polyvox/Mesh.h"

namespace voxel {

enum PlantType {
	Flower,

	MaxPlantTypes
};

class PlantGenerator {
private:
	Mesh* _meshes[MaxPlantTypes];
public:
	PlantGenerator();
	~PlantGenerator();

	void shutdown();

	bool generatePlant(int size, PlantType type, Mesh *result);

	Mesh* getMesh(PlantType type);

	void generateAll();
};

}
