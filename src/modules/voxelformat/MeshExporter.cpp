/**
 * @file
 */

#include "MeshExporter.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ThreadPool.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VoxelUtil.h"
#include <SDL_timer.h>

namespace voxel {

void MeshExporter::subdivideTri(const Tri &tri, core::DynamicArray<Tri> &tinyTris) {
	const glm::vec3 &mins = tri.mins();
	const glm::vec3 &maxs = tri.maxs();
	const glm::vec3 size = maxs - mins;
	if (glm::any(glm::greaterThan(size, glm::vec3(1.0f)))) {
		Tri out[4];
		tri.subdivide(out);
		for (int i = 0; i < lengthof(out); ++i) {
			subdivideTri(out[i], tinyTris);
		}
		return;
	}
	tinyTris.push_back(tri);
}

void MeshExporter::voxelizeTris(voxel::RawVolume *volume, const core::DynamicArray<Tri> &subdivided) {
	core::DynamicArray<uint8_t> palette;
	palette.reserve(subdivided.size());

	const voxel::Palette &pal = voxel::getPalette();
	core::DynamicArray<glm::vec4> materialColors;
	pal.toVec4f(materialColors);

	for (const Tri &tri : subdivided) {
		const glm::vec2 &uv = tri.centerUV();
		const uint32_t rgba = tri.colorAt(uv);
		const glm::vec4 &color = core::Color::fromRGBA(rgba);
		const uint8_t index = core::Color::getClosestMatch(color, materialColors);
		palette.push_back(index);
	}

	for (size_t i = 0; i < subdivided.size(); ++i) {
		const Tri &tri = subdivided[i];
		const uint8_t index = palette[i];
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		// TODO: different tris might contribute to the same voxel - merge the color values here
		for (int v = 0; v < 3; v++) {
			const glm::ivec3 p(glm::floor(tri.vertices[v]));
			volume->setVoxel(p, voxel);
		}

		const glm::vec3 &center = tri.center();
		const glm::ivec3 p2(glm::floor(center));
		volume->setVoxel(p2, voxel);
	}
	voxelutil::fillHollow(*volume, voxel::Voxel(voxel::VoxelType::Generic, 2));
}

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
	const bool mergeQuads = core::Var::getSafe(cfg::VoxformatMergequads)->boolVal();
	const bool reuseVertices = core::Var::getSafe(cfg::VoxformatReusevertices)->boolVal();
	const bool ambientOcclusion = core::Var::getSafe(cfg::VoxformatAmbientocclusion)->boolVal();

	const float scale = core::Var::getSafe(cfg::VoxformatScale)->floatVal();

	float scaleX = core::Var::getSafe(cfg::VoxformatScaleX)->floatVal();
	float scaleY = core::Var::getSafe(cfg::VoxformatScaleY)->floatVal();
	float scaleZ = core::Var::getSafe(cfg::VoxformatScaleZ)->floatVal();

	scaleX = scaleX != 1.0f ? scaleX : scale;
	scaleY = scaleY != 1.0f ? scaleY : scale;
	scaleZ = scaleZ != 1.0f ? scaleZ : scale;

	const bool quads = core::Var::getSafe(cfg::VoxformatQuads)->boolVal();
	const bool withColor = core::Var::getSafe(cfg::VoxformatWithcolor)->boolVal();
	const bool withTexCoords = core::Var::getSafe(cfg::VoxformatWithtexcoords)->boolVal();
	const bool applyTransform = core::Var::getSafe(cfg::VoxformatTransform)->boolVal();

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
	for (;;) {
		lock.lock();
		const size_t size = meshes.size();
		lock.unlock();
		if (size < models) {
			SDL_Delay(10);
		} else {
			break;
		}
	}
	Log::debug("Save meshes");
	const bool state = saveMeshes(meshes, filename, stream, glm::vec3(scaleX, scaleY, scaleZ), quads, withColor, withTexCoords);
	for (MeshExt& meshext : meshes) {
		delete meshext.mesh;
	}
	return state;
}

}
