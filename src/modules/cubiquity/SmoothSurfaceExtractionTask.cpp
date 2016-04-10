#include "SmoothSurfaceExtractionTask.h"

#include "MaterialSet.h"

#include "PolyVox/MarchingCubesSurfaceExtractor.h"
#include "PolyVox/RawVolume.h"
#include "PolyVox/PagedVolume.h"

#include <limits>

using namespace PolyVox;

namespace Cubiquity {
// Eliminate this
void scaleVertices(TerrainMesh* mesh, uint32_t amount) {
	for (uint32_t ct = 0; ct < mesh->getNoOfVertices(); ct++) {
		TerrainVertex& vertex = const_cast<TerrainVertex&>(mesh->getVertex(ct));
		vertex.encodedPosition *= amount;
	}
}

SmoothSurfaceExtractionTask::SmoothSurfaceExtractionTask(OctreeNode<MaterialSet>* octreeNode, ::PolyVox::PagedVolume<MaterialSet>* polyVoxVolume) :
		Task(), _octreeNode(octreeNode), _polyVoxVolume(polyVoxVolume), _polyVoxMesh(0), _processingStartedTimestamp((std::numeric_limits<Timestamp>::max)()), _ownMesh(false) {
}

SmoothSurfaceExtractionTask::~SmoothSurfaceExtractionTask() {
	if (_ownMesh) {
		delete _polyVoxMesh;
		_polyVoxMesh = 0;
		_ownMesh = false;
	}
}

void SmoothSurfaceExtractionTask::process(void) {
	_processingStartedTimestamp = Clock::getTimestamp();
	//Extract the surface
	_polyVoxMesh = new TerrainMesh();
	_ownMesh = true;

	generateSmoothMesh(_octreeNode->_region, _octreeNode->_height, _polyVoxMesh);

	_octreeNode->_octree->_finishedSurfaceExtractionTasks.push(this);
}

void SmoothSurfaceExtractionTask::generateSmoothMesh(const Region& region, uint32_t lodLevel, TerrainMesh* resultMesh) {
	MaterialSetMarchingCubesController controller;

	if (lodLevel == 0) {
		extractMarchingCubesMeshCustom(_polyVoxVolume, region, resultMesh, controller);
	} else {
		uint32_t downSampleFactor = 0x0001 << lodLevel;

		int crackHidingFactor = 5; //This should probably be configurable?
		controller.setThreshold(controller.getThreshold() + (downSampleFactor * crackHidingFactor));

		Region highRegion = region;
		highRegion.grow(downSampleFactor, downSampleFactor, downSampleFactor);

		Region lowRegion = highRegion;
		Vector3I lowerCorner = lowRegion.getLowerCorner();
		Vector3I upperCorner = lowRegion.getUpperCorner();

		upperCorner = upperCorner - lowerCorner;
		upperCorner = upperCorner / static_cast<int32_t>(downSampleFactor);
		upperCorner = upperCorner + lowerCorner;
		lowRegion.setUpperCorner(upperCorner);

		::PolyVox::RawVolume<MaterialSet> resampledVolume(lowRegion);

		resampleVolume(downSampleFactor, _polyVoxVolume, highRegion, &resampledVolume, lowRegion);

		lowRegion.shrink(1, 1, 1);

		extractMarchingCubesMeshCustom(&resampledVolume, lowRegion, resultMesh, controller);

		scaleVertices(resultMesh, downSampleFactor);

		recalculateMaterials(resultMesh, static_cast<Vector3F>(_octreeNode->_region.getLowerCorner()), _polyVoxVolume);
	}
}

void recalculateMaterials(TerrainMesh* mesh, const Vector3F& meshOffset, ::PolyVox::PagedVolume<MaterialSet>* volume) {
	for (uint32_t ct = 0; ct < mesh->getNoOfVertices(); ct++) {
		// Nasty casting away of constness so we can tweak the material values.
		TerrainVertex& vertex = const_cast<TerrainVertex&>(mesh->getVertex(ct));
		const Vector3DFloat& vertexPos = decodeVertex(vertex).position + meshOffset;
		MaterialSet value = getInterpolatedValue(volume, vertexPos);

		// It seems that sometimes the vertices can fall in an empty cell. The reason for this
		// isn't clear but it might be inaccuraceies in the lower LOD mesh. It also might only
		// happen right on the edge of the volume so wrap modes might help. Hopefully we can
		// remove this hack in the future.
		Vector<8, float> matAsVec = value;
		if (matAsVec.length() < 0.001f) {
			value = MaterialSet(0);
			value.setMaterial(0, 255);
		}

		vertex.data = value;
	}
}

MaterialSet getInterpolatedValue(::PolyVox::PagedVolume<MaterialSet>* volume, const Vector3F& position) {
	::PolyVox::PagedVolume<MaterialSet>::Sampler sampler(volume);

	int32_t iLowerX = ::PolyVox::roundTowardsNegInf(position.getX());
	int32_t iLowerY = ::PolyVox::roundTowardsNegInf(position.getY());
	int32_t iLowerZ = ::PolyVox::roundTowardsNegInf(position.getZ());

	float fOffsetX = position.getX() - iLowerX;
	float fOffsetY = position.getY() - iLowerY;
	float fOffsetZ = position.getZ() - iLowerZ;

	/*int32_t iCeilX = iFloorX + 1;
	 int32_t iCeilY = iFloorY + 1;
	 int32_t iCeilZ = iFloorZ + 1;*/

	sampler.setPosition(iLowerX, iLowerY, iLowerZ);

	MaterialSet v000 = sampler.peekVoxel0px0py0pz();
	MaterialSet v100 = sampler.peekVoxel1px0py0pz();
	MaterialSet v010 = sampler.peekVoxel0px1py0pz();
	MaterialSet v110 = sampler.peekVoxel1px1py0pz();
	MaterialSet v001 = sampler.peekVoxel0px0py1pz();
	MaterialSet v101 = sampler.peekVoxel1px0py1pz();
	MaterialSet v011 = sampler.peekVoxel0px1py1pz();
	MaterialSet v111 = sampler.peekVoxel1px1py1pz();

	MaterialSet result = trilerp(v000, v100, v010, v110, v001, v101, v011, v111, fOffsetX, fOffsetY, fOffsetZ);

	return result;
}
}
