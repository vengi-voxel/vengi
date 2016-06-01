#include "PlantGenerator.h"
#include "voxel/polyvox/RawVolume.h"
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
	volume.setBorderValue(createVoxel(Air));
	glm::ivec3 pos = region.getCentre();
	pos.y = 0;
	switch (type) {
	case Flower: {
		const Voxel stalk = createVoxel(Grass1);
		for (int i = 0; i < size - 2; ++i) {
			volume.setVoxel(pos, stalk);
			++pos.y;
		}
		volume.setVoxel(pos, createVoxel(Leaves10));
		--pos.x;
		volume.setVoxel(pos, stalk);
		--pos.z;
		++pos.x;
		volume.setVoxel(pos, stalk);
		pos.z += 2;
		volume.setVoxel(pos, stalk);
		--pos.z;
		++pos.x;
		volume.setVoxel(pos, stalk);
		}
		break;
	case Mushroom:
		return false;
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
		generatePlant(5, (PlantType)t, _meshes[t]);
	}
}

}
