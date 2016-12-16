#include "PlantGenerator.h"

#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"

namespace voxel {

PlantGenerator::PlantGenerator() {
	for (int i = 0; i < MaxPlantTypes; ++i) {
		_meshes[i] = nullptr;
	}
}

PlantGenerator::~PlantGenerator() {
	shutdown();
}

void PlantGenerator::shutdown() {
	for (int i = 0; i < MaxPlantTypes; ++i) {
		delete _meshes[i];
		_meshes[i] = nullptr;
	}
}

bool PlantGenerator::generatePlant(int size, PlantType type, Mesh *result) {
	const Region region(0, 0, 0, size, size, size);
	RawVolume volume(region);
	volume.setBorderValue(Voxel());
	glm::ivec3 pos = region.getCentre();
	pos.y = 0;
	switch (type) {
	case Flower:
		createFlower(size, pos, volume);
		break;
	case Mushroom:
		createMushroom(size, pos, volume);
		break;
	case Grass:
		createGrass(size, pos, volume);
		break;
	default:
		return false;
	}

	extractCubicMesh(&volume, region, result, IsQuadNeeded(false));

	return true;
}

Mesh* PlantGenerator::getMesh(PlantType type) {
	return _meshes[type];
}

void PlantGenerator::generateAll() {
	for (int t = 0; t < MaxPlantTypes; ++t) {
		_meshes[t] = new Mesh(1000, 1000);
		int size;
		switch (t) {
		case Grass:
			size = 10;
			break;
		case Mushroom:
			size = 7;
			break;
		default:
			size = 5;
			break;
		}
		generatePlant(size, (PlantType)t, _meshes[t]);
	}
}

}
