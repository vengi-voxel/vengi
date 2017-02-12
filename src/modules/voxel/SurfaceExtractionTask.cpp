#include "SurfaceExtractionTask.h"
#include "Octree.h"
#include "OctreeNode.h"
#include "core/App.h"
#include "polyvox/Region.h"
#include "polyvox/CubicSurfaceExtractor.h"
#include "polyvox/RawVolume.h"
#include "polyvox/VolumeRescaler.h"
#include "IsQuadNeeded.h"

#include <limits>

namespace voxel {

// Eliminate this
void scaleVertices(Mesh* mesh, uint32_t amount) {
	for (uint32_t ct = 0; ct < mesh->getNoOfVertices(); ct++) {
		VoxelVertex& vertex = const_cast<VoxelVertex&>(mesh->getVertex(ct));
		vertex.position *= amount;
	}
}

SurfaceExtractionTask::SurfaceExtractionTask(OctreeNode* octreeNode, PagedVolume* polyVoxVolume) :
		_node(octreeNode), _volume(polyVoxVolume) {
}

SurfaceExtractionTask::~SurfaceExtractionTask() {
	_mesh = std::shared_ptr<Mesh>();
}

void SurfaceExtractionTask::process() {
	_processingStartedTimestamp = core::App::getInstance()->timeProvider()->currentTime();

	// TODO: sizes to prevent (re-)allocations
	_mesh = std::make_shared<Mesh>(0, 0, true);
	_meshWater = std::make_shared<Mesh>(0, 0, true);
	Mesh* meshWater = _meshWater.get();
	Mesh* mesh = _mesh.get();

	uint32_t downScaleFactor = 0x0001 << _node->height();

	if (downScaleFactor == 1) {
		extractCubicMesh(_volume, _node->_region, mesh, IsQuadNeeded());
		extractCubicMesh(_volume, _node->_region, meshWater, IsWaterQuadNeeded());
	} else if (downScaleFactor == 2) {
		Region srcRegion = _node->_region;
		srcRegion.grow(2);

		glm::ivec3 lowerCorner = srcRegion.getLowerCorner();
		glm::ivec3 upperCorner = srcRegion.getUpperCorner();

		upperCorner = upperCorner - lowerCorner;
		upperCorner = upperCorner / static_cast<int32_t>(downScaleFactor);
		upperCorner = upperCorner + lowerCorner;

		Region dstRegion(lowerCorner, upperCorner);

		RawVolume resampledVolume(dstRegion);
		rescaleVolume(*_volume, srcRegion, resampledVolume, dstRegion);

		dstRegion.shrink(1);

		extractCubicMesh(&resampledVolume, dstRegion, mesh, IsQuadNeeded());
		extractCubicMesh(&resampledVolume, dstRegion, meshWater, IsWaterQuadNeeded());

		scaleVertices(mesh, downScaleFactor);
		scaleVertices(meshWater, downScaleFactor);
	} else if (downScaleFactor == 4) {
		Region srcRegion = _node->_region;
		srcRegion.grow(4);

		glm::ivec3 lowerCorner = srcRegion.getLowerCorner();
		glm::ivec3 upperCorner = srcRegion.getUpperCorner();

		upperCorner = upperCorner - lowerCorner;
		upperCorner = upperCorner / static_cast<int32_t>(2);
		upperCorner = upperCorner + lowerCorner;

		const Region dstRegion(lowerCorner, upperCorner);

		RawVolume resampledVolume(dstRegion);
		rescaleVolume(*_volume, srcRegion, resampledVolume, dstRegion);

		lowerCorner = dstRegion.getLowerCorner();
		upperCorner = dstRegion.getUpperCorner();

		upperCorner = upperCorner - lowerCorner;
		upperCorner = upperCorner / static_cast<int32_t>(2);
		upperCorner = upperCorner + lowerCorner;

		Region dstRegion2(lowerCorner, upperCorner);

		RawVolume resampledVolume2(dstRegion2);
		rescaleVolume(resampledVolume, dstRegion, resampledVolume2, dstRegion2);

		dstRegion2.shrink(1);

		extractCubicMesh(&resampledVolume2, dstRegion2, mesh, IsQuadNeeded());
		extractCubicMesh(&resampledVolume2, dstRegion2, meshWater, IsWaterQuadNeeded());

		scaleVertices(mesh, downScaleFactor);
		scaleVertices(meshWater, downScaleFactor);
	}

	_node->_octree->_finishedExtractionTasks.push(this);
}

}
