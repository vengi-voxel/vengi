/**
 * @file
 */

#include "MeshFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ThreadPool.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/private/PaletteLookup.h"
#include "voxelutil/VoxelUtil.h"
#include <SDL_timer.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>

namespace voxelformat {

MeshFormat::MeshExt* MeshFormat::getParent(const voxelformat::SceneGraph &sceneGraph, MeshFormat::Meshes &meshes, int nodeId) {
	if (!sceneGraph.hasNode(nodeId)) {
		return nullptr;
	}
	const int parent = sceneGraph.node(nodeId).parent();
	for (MeshExt &me : meshes) {
		if (me.nodeId == parent) {
			return &me;
		}
	}
	return nullptr;
}

glm::vec3 MeshFormat::getScale() {
	const float scale = core::Var::getSafe(cfg::VoxformatScale)->floatVal();

	float scaleX = core::Var::getSafe(cfg::VoxformatScaleX)->floatVal();
	float scaleY = core::Var::getSafe(cfg::VoxformatScaleY)->floatVal();
	float scaleZ = core::Var::getSafe(cfg::VoxformatScaleZ)->floatVal();

	scaleX = glm::epsilonNotEqual(scaleX, 1.0f, glm::epsilon<float>()) ? scaleX : scale;
	scaleY = glm::epsilonNotEqual(scaleY, 1.0f, glm::epsilon<float>()) ? scaleY : scale;
	scaleZ = glm::epsilonNotEqual(scaleZ, 1.0f, glm::epsilon<float>()) ? scaleZ : scale;
	Log::debug("scale: %f:%f:%f", scaleX, scaleY, scaleZ);
	return {scaleX, scaleY, scaleZ};
}

bool MeshFormat::flipWinding(const glm::vec3 &scale) {
	return (scale.x * scale.y * scale.z) < 0.0f ? true : false;
}

void MeshFormat::subdivideTri(const Tri &tri, TriCollection &tinyTris) {
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

void MeshFormat::voxelizeTris(voxelformat::SceneGraphNode &node, const TriCollection &subdivided) {
	struct PosSamplingEntry {
		inline PosSamplingEntry(float _area, const glm::vec4 &_color) : area(_area), color(_color) {
		}
		float area;
		glm::vec4 color;
	};

	struct PosSampling {
		core::DynamicArray<PosSamplingEntry> entries;
		inline PosSampling(float area, const glm::vec4 &color) {
			entries.emplace_back(area, color);
		}
		glm::vec4 avgColor() const {
			if (entries.size() == 1) {
				return entries[0].color;
			}
			float sumArea = 0.0f;
			for (const PosSamplingEntry& pe : entries) {
				sumArea += pe.area;
			}
			glm::vec4 color{0.0f};
			for (const PosSamplingEntry& pe : entries) {
				color += pe.color * pe.area / sumArea;
			}
			color[3] = 1.0f;
			return color;
		}
	};

	typedef core::Map<glm::ivec3, PosSampling, 64, glm::hash<glm::ivec3>> PosMap;
	PosMap posMap((int)subdivided.size() * 3);
	Log::debug("subdivided into %i triangles", (int)subdivided.size());
	for (const Tri &tri : subdivided) {
		const glm::vec2 &uv = tri.centerUV();
		const core::RGBA rgba = tri.colorAt(uv);
		const float area = tri.area();
		const glm::vec4 &color = core::Color::fromRGBA(rgba);
		for (int v = 0; v < 3; v++) {
			const glm::ivec3 p(glm::round(tri.vertices[v]));
			auto iter = posMap.find(p);
			if (iter == posMap.end()) {
				posMap.emplace(p, {area, color});
			} else {
				PosSampling &pos = iter->value;
				pos.entries.emplace_back(area, color);
			}
		}
	}
	Log::debug("create voxels");
	voxel::RawVolume *volume = node.volume();
	PaletteLookup palLookup;
	for (const auto &entry : posMap) {
		const PosSampling &pos = entry->second;
		const glm::vec4 &color = pos.avgColor();
		const uint8_t index = palLookup.findClosestIndex(color);
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		volume->setVoxel(entry->first, voxel);
	}
	node.setPalette(palLookup.palette());
	Log::debug("fill hollows");
	voxelutil::fillHollow(*volume, voxel::Voxel(voxel::VoxelType::Generic, 2));
}

MeshFormat::MeshExt::MeshExt(voxel::Mesh *_mesh, const SceneGraphNode& node, bool _applyTransform) :
		mesh(_mesh), name(node.name()), applyTransform(_applyTransform) {
	transform = node.transform();
	size = node.region().getDimensionsInVoxels();
	nodeId = node.id();
}

bool MeshFormat::loadGroups(const core::String &filename, io::SeekableReadStream& file, SceneGraph& sceneGraph) {
	Log::debug("Meshes can't get voxelized yet");
	return false;
}

bool MeshFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	const bool mergeQuads = core::Var::getSafe(cfg::VoxformatMergequads)->boolVal();
	const bool reuseVertices = core::Var::getSafe(cfg::VoxformatReusevertices)->boolVal();
	const bool ambientOcclusion = core::Var::getSafe(cfg::VoxformatAmbientocclusion)->boolVal();

	const glm::vec3 &scale = getScale();

	const bool quads = core::Var::getSafe(cfg::VoxformatQuads)->boolVal();
	const bool withColor = core::Var::getSafe(cfg::VoxformatWithcolor)->boolVal();
	const bool withTexCoords = core::Var::getSafe(cfg::VoxformatWithtexcoords)->boolVal();
	const bool applyTransform = core::Var::getSafe(cfg::VoxformatTransform)->boolVal();

	const size_t models = sceneGraph.size();
	core::ThreadPool& threadPool = app::App::getInstance()->threadPool();
	Meshes meshes;
	core::Map<int, int> meshIdxNodeMap;
	core_trace_mutex(core::Lock, lock, "MeshFormat");
	for (const SceneGraphNode& node : sceneGraph) {
		auto lambda = [&] () {
			voxel::Mesh *mesh = new voxel::Mesh();
			voxel::Region region = node.region();
			region.shiftUpperCorner(1, 1, 1);
			voxel::extractCubicMesh(node.volume(), region, mesh, voxel::IsQuadNeeded(), glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
			core::ScopedLock scoped(lock);
			meshes.emplace_back(mesh, node, applyTransform);
			meshIdxNodeMap.put(node.id(), (int)meshes.size() - 1);
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
	const bool state = saveMeshes(meshIdxNodeMap, sceneGraph, meshes, filename, stream, scale, quads, withColor, withTexCoords);
	for (MeshExt& meshext : meshes) {
		delete meshext.mesh;
	}
	return state;
}

}
