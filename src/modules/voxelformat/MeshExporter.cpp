/**
 * @file
 */

#include "MeshExporter.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ThreadPool.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/RawVolume.h"
#include <SDL_timer.h>

namespace voxel {

MeshExporter::MeshExt::MeshExt(voxel::Mesh *_mesh, const SceneGraphNode& node, bool _applyTransform) :
		mesh(_mesh), name(node.name()), applyTransform(_applyTransform) {
	transform = node.transform();
	size = node.region().getDimensionsInVoxels();
}

bool MeshExporter::loadGroups(const core::String &filename, io::SeekableReadStream& file, SceneGraph& sceneGraph) {
	Log::debug("Meshes can't get voxelized yet");
	return false;
}

bool MeshExporter::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	const bool mergeQuads = core::Var::get(cfg::VoxformatMergequads, "true", core::CV_NOPERSIST)->boolVal();
	const bool reuseVertices = core::Var::get(cfg::VoxformatReusevertices, "true", core::CV_NOPERSIST)->boolVal();
	const bool ambientOcclusion = core::Var::get(cfg::VoxformatAmbientocclusion, "false", core::CV_NOPERSIST)->boolVal();
	const float scale = core::Var::get(cfg::VoxformatScale, "1.0", core::CV_NOPERSIST)->floatVal();
	const bool quads = core::Var::get(cfg::VoxformatQuads, "true", core::CV_NOPERSIST)->boolVal();
	const bool withColor = core::Var::get(cfg::VoxformatWithcolor, "true", core::CV_NOPERSIST)->boolVal();
	const bool withTexCoords = core::Var::get(cfg::VoxformatWithtexcoords, "true", core::CV_NOPERSIST)->boolVal();
	const bool applyTransform = core::Var::get(cfg::VoxformatTransform, "false", core::CV_NOPERSIST)->boolVal();

	const size_t models = sceneGraph.size();
	core::ThreadPool& threadPool = app::App::getInstance()->threadPool();
	Meshes meshes;
	core_trace_mutex(core::Lock, lock, "MeshExporter");
	for (const SceneGraphNode& node : sceneGraph) {
		auto lambda = [&] () {
			voxel::Mesh *mesh = new voxel::Mesh();
			voxel::Region region = node.region();
			region.shiftUpperCorner(1, 1, 1);
			voxel::extractCubicMesh(node.volume(), region, mesh, voxel::IsQuadNeeded(), glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
			core::ScopedLock scoped(lock);
			meshes.emplace_back(mesh, node, applyTransform);
		};
		threadPool.enqueue(lambda);
	}
	while (meshes.size() < models) {
		SDL_Delay(10);
	}
	Log::debug("Save meshes");
	const bool state = saveMeshes(meshes, filename, stream, scale, quads, withColor, withTexCoords);
	for (MeshExt& meshext : meshes) {
		delete meshext.mesh;
	}
	return state;
}

}
