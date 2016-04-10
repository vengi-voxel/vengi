#include "ColoredCubicSurfaceExtractionTask.h"

#include "Color.h"
#include "OctreeNode.h"

#include "PolyVox/CubicSurfaceExtractor.h"
#include "PolyVox/RawVolume.h"
#include "PolyVox/PagedVolume.h"

#include <limits>

using namespace PolyVox;

namespace Cubiquity {

// Eliminate this
void scaleVertices(ColoredCubesMesh* mesh, uint32_t amount) {
	for (uint32_t ct = 0; ct < mesh->getNoOfVertices(); ct++) {
		ColoredCubesVertex& vertex = const_cast<ColoredCubesVertex&>(mesh->getVertex(ct));
		vertex.encodedPosition *= amount;
	}
}

ColoredCubicSurfaceExtractionTask::ColoredCubicSurfaceExtractionTask(OctreeNode<Color>* octreeNode, ::PolyVox::PagedVolume<Color>* polyVoxVolume) :
		Task(), _octreeNode(octreeNode), _polyVoxVolume(polyVoxVolume), _polyVoxMesh(0), _processingStartedTimestamp((std::numeric_limits<Timestamp>::max)()), _ownMesh(false) {
}

ColoredCubicSurfaceExtractionTask::~ColoredCubicSurfaceExtractionTask() {
	if (_ownMesh) {
		delete _polyVoxMesh;
		_polyVoxMesh = 0;
		_ownMesh = false;
	}
}

void ColoredCubicSurfaceExtractionTask::process(void) {
	_processingStartedTimestamp = Clock::getTimestamp();

	//Extract the surface
	_polyVoxMesh = new ColoredCubesMesh();
	_ownMesh = true;

	uint32_t downScaleFactor = 0x0001 << _octreeNode->_height;

	ColoredCubesIsQuadNeeded isQuadNeeded;

	if (downScaleFactor == 1) {
		extractCubicMeshCustom(_polyVoxVolume, _octreeNode->_region, _polyVoxMesh, isQuadNeeded, true);
	} else if (downScaleFactor == 2) {
		Region srcRegion = _octreeNode->_region;

		srcRegion.grow(2);

		Vector3I lowerCorner = srcRegion.getLowerCorner();
		Vector3I upperCorner = srcRegion.getUpperCorner();

		upperCorner = upperCorner - lowerCorner;
		upperCorner = upperCorner / static_cast<int32_t>(downScaleFactor);
		upperCorner = upperCorner + lowerCorner;

		Region dstRegion(lowerCorner, upperCorner);

		::PolyVox::RawVolume<Color> resampledVolume(dstRegion);
		rescaleCubicVolume(_polyVoxVolume, srcRegion, &resampledVolume, dstRegion);

		dstRegion.shrink(1);

		//dstRegion.shiftLowerCorner(-1, -1, -1);

		extractCubicMeshCustom(&resampledVolume, dstRegion, _polyVoxMesh, isQuadNeeded, true);

		scaleVertices(_polyVoxMesh, downScaleFactor);
		//translateVertices(mPolyVoxMesh, Vector3DFloat(0.5f, 0.5f, 0.5f)); // Removed when going from float positions to uin8_t. Do we need this?
	} else if (downScaleFactor == 4) {
		Region srcRegion = _octreeNode->_region;

		srcRegion.grow(4);

		Vector3I lowerCorner = srcRegion.getLowerCorner();
		Vector3I upperCorner = srcRegion.getUpperCorner();

		upperCorner = upperCorner - lowerCorner;
		upperCorner = upperCorner / static_cast<int32_t>(2);
		upperCorner = upperCorner + lowerCorner;

		Region dstRegion(lowerCorner, upperCorner);

		::PolyVox::RawVolume<Color> resampledVolume(dstRegion);
		rescaleCubicVolume(_polyVoxVolume, srcRegion, &resampledVolume, dstRegion);

		lowerCorner = dstRegion.getLowerCorner();
		upperCorner = dstRegion.getUpperCorner();

		upperCorner = upperCorner - lowerCorner;
		upperCorner = upperCorner / static_cast<int32_t>(2);
		upperCorner = upperCorner + lowerCorner;

		Region dstRegion2(lowerCorner, upperCorner);

		::PolyVox::RawVolume<Color> resampledVolume2(dstRegion2);
		rescaleCubicVolume(&resampledVolume, dstRegion, &resampledVolume2, dstRegion2);

		dstRegion2.shrink(1);

		//dstRegion.shiftLowerCorner(-1, -1, -1);

		extractCubicMeshCustom(&resampledVolume2, dstRegion2, _polyVoxMesh, isQuadNeeded, true);

		scaleVertices(_polyVoxMesh, downScaleFactor);
		//translateVertices(mPolyVoxMesh, Vector3DFloat(1.5f, 1.5f, 1.5f)); // Removed when going from float positions to uin8_t. Do we need this?
	}

	_octreeNode->_octree->_finishedSurfaceExtractionTasks.push(this);
}

}
