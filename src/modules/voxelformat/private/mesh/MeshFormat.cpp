/**
 * @file
 */

#include "MeshFormat.h"
#include "app/App.h"
#include "app/Async.h"
#include "core/Color.h"
#include "core/ConfigVar.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Lock.h"
#include "io/Archive.h"
#include "palette/NormalPalette.h"
#include "palette/NormalPaletteLookup.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/ChunkMesh.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelutil/FillHollow.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>

namespace voxelformat {

MeshFormat::MeshFormat() {
	_weightedAverage = core::Var::getSafe(cfg::VoxformatRGBWeightedAverage)->boolVal();
}

MeshFormat::MeshExt *MeshFormat::getParent(const scenegraph::SceneGraph &sceneGraph, MeshFormat::Meshes &meshes,
										   int nodeId) {
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

glm::vec3 MeshFormat::getInputScale() {
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

void MeshFormat::subdivideTri(const voxelformat::MeshTri &meshTri, MeshTriCollection &tinyTris) {
	if (stopExecution()) {
		return;
	}
	const glm::vec3 &mins = meshTri.mins();
	const glm::vec3 &maxs = meshTri.maxs();
	const glm::vec3 size = maxs - mins;
	if (glm::any(glm::greaterThan(size, glm::vec3(1.0f)))) {
		voxelformat::MeshTri out[4];
		subdivide(meshTri, out);
		for (int i = 0; i < lengthof(out); ++i) {
			subdivideTri(out[i], tinyTris);
		}
		return;
	}
	tinyTris.push_back(meshTri);
}

static void convertToVoxelGrid(glm::vec3 &v) {
	// convert into voxel grid coordinates
	if (v.x < 0.0f) {
		v.x -= 1.0f;
	}
	if (v.y < 0.0f) {
		v.y -= 1.0f;
	}
	if (v.z < 0.0f) {
		v.z -= 1.0f;
	}
}

glm::vec2 MeshFormat::paletteUV(int colorIndex) {
	// 1 x 256 is the texture format that we are using for our palette
	// sample the center of the palette pixels
	// see https://github.com/vengi-voxel/vengi/issues/403
	const float u = (((float)colorIndex) + 0.5f) / (float)palette::PaletteMaxColors;
	const float v = 0.5f;
	return {u, v};
}

void MeshFormat::addToPosMap(PosMap &posMap, const voxel::Region &region, core::RGBA rgba, uint32_t area, uint8_t normalIdx, const glm::ivec3 &pos,
							 MeshMaterialIndex materialIdx) const {
	if (rgba.a <= AlphaThreshold) {
		return;
	}
	core::ScopedLock lock(_mutex);
	const int idx = region.index(pos);
	auto iter = posMap.find(idx);
	if (iter == posMap.end()) {
		posMap.emplace(idx, {area, rgba, normalIdx, materialIdx});
	} else {
		PosSampling &posSampling = iter->value;
		posSampling.add(area, rgba, normalIdx, materialIdx);
	}
}

void MeshFormat::transformTris(const voxel::Region &region, const MeshTriCollection &tris, PosMap &posMap,
							   const MeshMaterialArray &meshMaterialArray,
							   const palette::NormalPalette &normalPalette) const {
	Log::debug("subdivided into %i triangles", (int)tris.size());
	palette::NormalPaletteLookup normalLookup(normalPalette);
	auto fn = [&tris, &region, &normalLookup, &posMap, &meshMaterialArray, this](int start, int end) {
		for (int i = start; i < end; ++i) {
			if (stopExecution()) {
				return;
			}
			const voxelformat::MeshTri &meshTri = tris[i];
			const glm::vec2 &uv = meshTri.centerUV();
			const core::RGBA rgba = colorAt(meshTri, meshMaterialArray, uv);
			if (rgba.a <= AlphaThreshold) {
				continue;
			}
			const uint32_t area = (uint32_t)(meshTri.area() * 1000.0f);
			glm::vec3 c = meshTri.center();
			convertToVoxelGrid(c);

			int normalIdx = normalLookup.getClosestMatch(meshTri.normal());
			if (normalIdx == palette::PaletteNormalNotFound) {
				normalIdx = NO_NORMAL;
			}

			const glm::ivec3 p(c);
			addToPosMap(posMap, region, rgba, area, normalIdx, p, meshTri.materialIdx);
		}
	};
	app::for_parallel(0, tris.size(), fn);
}

void MeshFormat::transformTrisAxisAligned(const voxel::Region &region, const MeshTriCollection &tris, PosMap &posMap,
										  const MeshMaterialArray &meshMaterialArray,
										  const palette::NormalPalette &normalPalette) const {
	Log::debug("axis aligned %i triangles", (int)tris.size());
	palette::NormalPaletteLookup normalLookup(normalPalette);
	auto fn = [&tris, &normalLookup, region, &posMap, &meshMaterialArray, this](int start, int end) {
		for (int i = start; i < end; ++i) {
			const voxelformat::MeshTri &meshTri = tris[i];
			if (stopExecution()) {
				break;
			}
			const glm::vec2 &uv = meshTri.centerUV();
			const core::RGBA rgba = colorAt(meshTri, meshMaterialArray, uv);
			if (rgba.a <= AlphaThreshold) {
				continue;
			}
			const uint32_t area = (uint32_t)(meshTri.area() * 1000.0f);
			const glm::vec3 &normal = glm::normalize(meshTri.normal());
			const glm::ivec3 sideDelta(normal.x <= 0 ? 0 : -1, normal.y <= 0 ? 0 : -1, normal.z <= 0 ? 0 : -1);
			const glm::ivec3 mins = meshTri.roundedMins();
			const glm::ivec3 maxs = meshTri.roundedMaxs() + glm::ivec3(glm::round(glm::abs(normal)));
			Log::trace("mins: %i:%i:%i", mins.x, mins.y, mins.z);
			Log::trace("maxs: %i:%i:%i", maxs.x, maxs.y, maxs.z);
			Log::trace("normal: %f:%f:%f", normal.x, normal.y, normal.z);
			Log::trace("sideDelta: %i:%i:%i", sideDelta.x, sideDelta.y, sideDelta.z);
			int normalIdx = normalLookup.getClosestMatch(normal);
			if (normalIdx == palette::PaletteNormalNotFound) {
				normalIdx = NO_NORMAL;
			}
			for (int x = mins.x; x < maxs.x; x++) {
				if (!region.containsPointInX(x + sideDelta.x)) {
					continue;
				}
				for (int y = mins.y; y < maxs.y; y++) {
					if (!region.containsPointInY(y + sideDelta.y)) {
						continue;
					}
					for (int z = mins.z; z < maxs.z; z++) {
						if (!region.containsPointInZ(z + sideDelta.z)) {
							continue;
						}
						const glm::ivec3 p(x + sideDelta.x, y + sideDelta.y, z + sideDelta.z);
						addToPosMap(posMap, region, rgba, area, normalIdx, p, meshTri.materialIdx);
					}
				}
			}
		}
	};
	app::for_parallel(0, tris.size(), fn);
}

bool MeshFormat::isVoxelMesh(const MeshTriCollection &tris) {
	for (const voxelformat::MeshTri &meshTri : tris) {
		if (!meshTri.flat()) {
			Log::debug("No axis aligned mesh found");
			return false;
		}
	}
	Log::debug("Found axis aligned mesh");
	return true;
}

template<class FUNC>
static void voxelizeTriangle(const glm::vec3 &trisMins, const voxelformat::MeshTri &meshTri, FUNC &&func) {
	const glm::vec3 voxelHalf(0.5f);
	const glm::vec3 shiftedTrisMins = trisMins + voxelHalf;
	const glm::vec3 &v0 = meshTri.vertex0();
	const glm::vec3 &v1 = meshTri.vertex1();
	const glm::vec3 &v2 = meshTri.vertex2();
	const glm::vec3 mins = meshTri.mins();
	const glm::vec3 maxs = meshTri.maxs();
	const glm::ivec3 imins(glm::floor(mins - shiftedTrisMins));
	const glm::ivec3 size(glm::round(maxs - mins));
	const glm::ivec3 imaxs = 2 + imins + size;

	glm::vec3 center{};
	for (int x = imins.x; x < imaxs.x; x++) {
		center.x = trisMins.x + x;
		for (int y = imins.y; y < imaxs.y; y++) {
			center.y = trisMins.y + y;
			for (int z = imins.z; z < imaxs.z; z++) {
				center.z = trisMins.z + z;
				if (glm::intersectTriangleAABB(center, voxelHalf, v0, v1, v2)) {
					glm::vec2 uv;
					if (!meshTri.calcUVs(center, uv)) {
						continue;
					}
					func(meshTri, uv, shiftedTrisMins.x + x, shiftedTrisMins.y + y, shiftedTrisMins.z + z);
				}
			}
		}
	}
}

int MeshFormat::voxelizeNode(const core::String &uuid, const core::String &name, scenegraph::SceneGraph &sceneGraph,
							 MeshTriCollection &&tris, const MeshMaterialArray &meshMaterialArray, int parent, bool resetOrigin) const {
	if (tris.empty()) {
		Log::warn("Empty volume - no triangles given");
		return InvalidNodeId;
	}

	const bool axisAligned = isVoxelMesh(tris);

	glm::vec3 trisMins{0};
	glm::vec3 trisMaxs{0};
	core_assert_always(calculateAABB(tris, trisMins, trisMaxs));
	Log::debug("mins: %f:%f:%f, maxs: %f:%f:%f", trisMins.x, trisMins.y, trisMins.z, trisMaxs.x, trisMaxs.y,
			   trisMaxs.z);

	trisMins = glm::floor(trisMins);
	trisMaxs = glm::ceil(trisMaxs);

	if (!axisAligned) {
		convertToVoxelGrid(trisMins);
		convertToVoxelGrid(trisMaxs);
	}

	const voxel::Region region(trisMins, trisMaxs);
	if (!region.isValid()) {
		Log::error("Invalid region: %s", region.toString().c_str());
		return InvalidNodeId;
	}

	const int voxelizeMode = core::Var::getSafe(cfg::VoxformatVoxelizeMode)->intVal();
	const glm::ivec3 &vdim = region.getDimensionsInVoxels();
	if (glm::any(glm::greaterThan(vdim, glm::ivec3(512)))) {
		Log::warn("Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh! (%i:%i:%i)",
				  vdim.x, vdim.y, vdim.z);
		if (voxelizeMode != VoxelizeMode::Fast) {
			Log::warn("Another option when using very large meshes is to use the fast voxelization mode (%s)",
					  cfg::VoxformatVoxelizeMode);
		}
	}

	const size_t bytes = voxel::RawVolume::size(region);
	if (!app::App::getInstance()->hasEnoughMemory(bytes)) {
		const core::String &neededMem = core::string::humanSize(bytes);
		Log::error("Not enough memory to create a volume of size %i:%i:%i (would need %s)", vdim.x, vdim.y, vdim.z, neededMem.c_str());
		return InvalidNodeId;
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, uuid);
	node.setName(name);
	palette::NormalPalette normalPalette;
	const core::VarPtr &normalPaletteVar = core::Var::getSafe(cfg::NormalPalette);
	if (!normalPalette.load(normalPaletteVar->strVal().c_str())) {
		Log::debug("Failed to load normal palette %s - use redalert2 as default", normalPaletteVar->strVal().c_str());
		normalPalette.redAlert2();
	} else {
		Log::debug("Loaded normal palette %s", normalPaletteVar->strVal().c_str());
	}
	// TODO: VOXELFORMAT: auto generate the normal palette from the input tris?
	node.setNormalPalette(normalPalette);

	const bool fillHollow = core::Var::getSafe(cfg::VoxformatFillHollow)->boolVal();
	const int maxVoxels = vdim.x * vdim.y * vdim.z;
	if (axisAligned) {
		Log::debug("max voxels: %i (%i:%i:%i)", maxVoxels, vdim.x, vdim.y, vdim.z);
		PosMap posMap(maxVoxels);
		transformTrisAxisAligned(region, tris, posMap, meshMaterialArray, normalPalette);
		tris.release();
		node.setVolume(new voxel::RawVolume(region), true);
		voxelizeTris(node, posMap, meshMaterialArray, fillHollow);
	} else if (voxelizeMode == VoxelizeMode::Fast) {
		palette::Palette palette;

		const bool shouldCreatePalette = core::Var::getSafe(cfg::VoxelCreatePalette)->boolVal();
		if (shouldCreatePalette) {
			RGBAMaterialMap colorMaterials;
			Log::debug("create palette");
			for (const voxelformat::MeshTri &meshTri : tris) {
#if 1
				voxelizeTriangle(
					trisMins, meshTri,
					[this, &colorMaterials, &meshMaterialArray](const voxelformat::MeshTri &tri, const glm::vec2 &uv, int x, int y, int z) {
						const core::RGBA rgba = flattenRGB(colorAt(tri, meshMaterialArray, uv));
						colorMaterials.put(rgba, tri.materialIdx > 0 && tri.materialIdx < (int)meshMaterialArray.size() ? &meshMaterialArray[tri.materialIdx]->material : nullptr);
					});
#else
				const glm::vec2 &uv = meshTri.centerUV();
				const core::RGBA rgba = flattenRGB(colorAt(meshTri, materials, uv));
				colorMaterials.put(rgba, meshTri.material > 0 && meshTri.material < (int)materials.size() ? &materials[meshTri.material]->material : nullptr);
#endif
			}
			createPalette(colorMaterials, palette);
		} else {
			palette = voxel::getPalette();
		}

		Log::debug("create voxels from %i tris", (int)tris.size());
		palette::PaletteLookup palLookup(palette);
		node.setVolume(new voxel::RawVolume(region), true);
		voxel::RawVolumeWrapper wrapper(node.volume());
		palette::NormalPaletteLookup normalLookup(normalPalette);
		for (const voxelformat::MeshTri &meshTri : tris) {
			auto fn = [&](const voxelformat::MeshTri &tri, const glm::vec2 &uv, int x, int y, int z) {
				const core::RGBA color = flattenRGB(colorAt(tri, meshMaterialArray, uv));
				const glm::vec3 &normal = tri.normal();
				int normalIdx = normalLookup.getClosestMatch(normal);
				if (normalIdx == palette::PaletteNormalNotFound) {
					normalIdx = NO_NORMAL;
				}
				const voxel::Voxel voxel = voxel::createVoxel(palette, palLookup.findClosestIndex(color), normalIdx);
				wrapper.setVoxel(x, y, z, voxel);
			};
			voxelizeTriangle(trisMins, meshTri, fn);
		}
		tris.release();

		if (palette.colorCount() == 1) {
			core::RGBA c = palette.color(0);
			if (c.a == 0) {
				c.a = 255;
				palette.setColor(0, c);
			}
		}
		node.setPalette(palette);
		if (fillHollow && !stopExecution()) {
			Log::debug("fill hollows");
			const voxel::Voxel voxel = voxel::createVoxel(palette, FillColorIndex);
			voxelutil::fillHollow(wrapper, voxel);
		}
	} else {
		const size_t parallel = app::for_parallel_size(0, tris.size());
		Log::debug("Subdivide %i triangles (%i parallel)", (int)tris.size(), (int)parallel);
		core::DynamicArray<MeshTriCollection> meshTriCollections;
		meshTriCollections.resize(parallel);
		core::AtomicInt currentIdx(0);
		app::for_parallel(0, tris.size(), [&meshTriCollections, &tris, &currentIdx] (int start, int end) {
			MeshTriCollection &subdivided = meshTriCollections[currentIdx.increment()];
			subdivided.reserve(end - start);
			for (int i = start; i < end; ++i) {
				if (stopExecution()) {
					return;
				}
				subdivideTri(tris[i], subdivided);
			}
		});
		Log::debug("Subdivision done");
		tris.release();
		size_t cnt = 0;
		for (const MeshTriCollection &e : meshTriCollections) {
			cnt += e.size();
		}

		MeshTriCollection subdivided;
		subdivided.reserve(cnt);
		for (MeshTriCollection &e : meshTriCollections) {
			if (e.empty()) {
				continue;
			}
			subdivided.append(core::move(e));
			e.release();
		}
		meshTriCollections.release();

		if (subdivided.empty()) {
			Log::warn("Empty volume - could not subdivide");
			return InvalidNodeId;
		}

		PosMap posMap(maxVoxels);
		transformTris(region, subdivided, posMap, meshMaterialArray, normalPalette);
		subdivided.release();
		node.setVolume(new voxel::RawVolume(region), true);
		voxelizeTris(node, posMap, meshMaterialArray, fillHollow);
	}

	if (resetOrigin) {
		scenegraph::SceneGraphTransform transform;
		transform.setLocalTranslation(region.getLowerCornerf());
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);

		node.volume()->translate(-region.getLowerCorner());
	}

	return sceneGraph.emplace(core::move(node), parent);
}

bool MeshFormat::calculateAABB(const MeshTriCollection &tris, glm::vec3 &mins, glm::vec3 &maxs) {
	if (tris.empty()) {
		mins = maxs = glm::vec3(0.0f);
		return false;
	}

	maxs = tris[0].mins();
	mins = tris[0].maxs();

	for (const voxelformat::MeshTri &meshTri : tris) {
		maxs = glm::max(maxs, meshTri.maxs());
		mins = glm::min(mins, meshTri.mins());
	}
	return true;
}

void MeshFormat::voxelizeTris(scenegraph::SceneGraphNode &node, const PosMap &posMap, const MeshMaterialArray &meshMaterialArray, bool fillHollow) const {
	if (posMap.empty()) {
		Log::debug("Empty volume - no positions given");
		return;
	}
	palette::Palette palette;
	const bool shouldCreatePalette = core::Var::getSafe(cfg::VoxelCreatePalette)->boolVal();
	if (shouldCreatePalette) {
		RGBAMaterialMap colorMaterials;
		Log::debug("create palette");
		for (const auto &entry : posMap) {
			if (stopExecution()) {
				return;
			}
			const PosSampling &pos = entry->second;
			// TODO: PERF: don't do pos.getColor call twice
			const core::RGBA rgba = pos.getColor(_flattenFactor, _weightedAverage);
			if (rgba.a <= AlphaThreshold) {
				continue;
			}
			MeshMaterialIndex materialIdx = pos.getMaterialIndex();
			colorMaterials.put(rgba, materialIdx > 0 && materialIdx < (int)meshMaterialArray.size() ? &meshMaterialArray[materialIdx]->material : nullptr);
		}
		createPalette(colorMaterials, palette);
	} else {
		palette = voxel::getPalette();
	}

	Log::debug("create voxels for %i positions", (int)posMap.size());
	voxel::RawVolume *volume = node.volume();
	palette::PaletteLookup palLookup(palette);
	auto fn = [&palette, volume, &palLookup, this](int idx, const PosSampling &posSampling) {
		if (stopExecution()) {
			return;
		}
		const core::RGBA rgba = posSampling.getColor(_flattenFactor, _weightedAverage);
		if (rgba.a <= AlphaThreshold) {
			return;
		}
		uint8_t colorIndex;
		{
			core::ScopedLock lock(_mutex);
			colorIndex = palLookup.findClosestIndex(rgba);
		}
		const voxel::Voxel voxel = voxel::createVoxel(palette, colorIndex, posSampling.getNormal());
		volume->setVoxel(idx, voxel);
	};
	posMap.for_parallel(fn);
	if (palette.colorCount() == 1) {
		core::RGBA c = palette.color(0);
		if (c.a == 0) {
			c.a = 255;
			palette.setColor(0, c);
		}
	}
	node.setPalette(palette);
	if (fillHollow) {
		if (stopExecution()) {
			return;
		}
		Log::debug("fill hollows");
		const voxel::Voxel voxel = voxel::createVoxel(palette, FillColorIndex);
		voxelutil::fillHollow(*volume, voxel);
	}
}

MeshFormat::MeshExt::MeshExt(voxel::ChunkMesh *_mesh, const scenegraph::SceneGraphNode &node, bool _applyTransform)
	: mesh(_mesh), name(node.name()), applyTransform(_applyTransform), size(node.region().getDimensionsInVoxels()),
	  pivot(node.pivot()), nodeId(node.id()) {
}

void MeshFormat::MeshExt::visitByMaterial(
	int materialIndex,
	const std::function<void(const voxel::Mesh &, voxel::IndexType, voxel::IndexType, voxel::IndexType)> &callback)
	const {
	if (!mesh) {
		return;
	}
	for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
		const voxel::Mesh &vmesh = mesh->mesh[i];
		if (vmesh.isEmpty()) {
			continue;
		}
		const int ni = vmesh.getNoOfIndices();
		const voxel::IndexArray &indices = vmesh.getIndexVector();
		for (int j = 0; j < ni; j += 3) {
			const voxel::IndexType i0 = indices[j + 0];
			if (vmesh.getVertex(i0).colorIndex != materialIndex) {
				continue;
			}
			const voxel::IndexType i1 = indices[j + 1];
			const voxel::IndexType i2 = indices[j + 2];
			callback(vmesh, i0, i1, i2);
		}
	}
}

bool MeshFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
							scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	const bool retVal = voxelizeGroups(filename, archive, sceneGraph, ctx);
	sceneGraph.updateTransforms();
	return retVal;
}

int MeshFormat::voxelizePointCloud(const core::String &filename, scenegraph::SceneGraph &sceneGraph,
									const PointCloud &vertices) const {
	glm::vec3 mins{std::numeric_limits<float>::max()};
	glm::vec3 maxs{std::numeric_limits<float>::min()};
	const glm::vec3 scale = getInputScale();
	for (PointCloudVertex &v : vertices) {
		v.position *= scale;
		mins = glm::min(mins, v.position);
		maxs = glm::max(maxs, v.position);
	}

	const int pointSize = core_max(1, core::Var::getSafe(cfg::VoxformatPointCloudSize)->intVal());
	const voxel::Region region(glm::floor(mins), glm::ceil(maxs) + glm::vec3((float)(pointSize - 1)));
	voxel::RawVolume *v = new voxel::RawVolume(region);
	const palette::Palette &palette = voxel::getPalette();
	auto fn = [&vertices, &palette, pointSize, v](int start, int end) {
		palette::PaletteLookup palLookup(palette);
		for (int i = start; i < end; ++i) {
			if (stopExecution()) {
				return;
			}
			const PointCloudVertex &vertex = vertices[i];
			const glm::ivec3 pos = glm::round(vertex.position);
			const voxel::Voxel voxel = voxel::createVoxel(palette, palLookup.findClosestIndex(vertex.color));
			if (pointSize == 1) {
				v->setVoxel(pos, voxel);
				return;
			}
			voxel::RawVolume::Sampler sampler(v);
			sampler.setPosition(pos);
			for (int z = 0; z < pointSize; ++z) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (int y = 0; y < pointSize; ++y) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int x = 0; x < pointSize; ++x) {
						sampler3.setVoxel(voxel);
						sampler3.movePositiveX();
					}
					sampler2.movePositiveY();
				}
				sampler.movePositiveZ();
			}
		}
	};
	app::for_parallel(0, vertices.size(), fn);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(v, true);
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node));
}

bool MeshFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &, scenegraph::SceneGraph &,
								const LoadContext &) {
	Log::debug("Mesh %s can't get voxelized yet", filename.c_str());
	return false;
}

bool MeshFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive, const SaveContext &saveCtx) {
	const bool quads = core::Var::getSafe(cfg::VoxformatQuads)->boolVal();
	const bool withColor = core::Var::getSafe(cfg::VoxformatWithColor)->boolVal();
	const bool withTexCoords = core::Var::getSafe(cfg::VoxformatWithtexcoords)->boolVal();
	const voxel::SurfaceExtractionType type =
		(voxel::SurfaceExtractionType)core::Var::getSafe(cfg::VoxelMeshMode)->intVal();

	Meshes meshes;
	meshes.resize(sceneGraph.nodes().size());
	// TODO: VOXELFORMAT: this could get optimized by re-using the same mesh for multiple nodes (in case of reference
	// nodes)
	app::for_parallel(0, sceneGraph.nodes().size(), [&sceneGraph, type, &meshes] (int start, int end) {
		const bool withNormals = core::Var::getSafe(cfg::VoxformatWithNormals)->boolVal();
		const bool optimizeMesh = core::Var::getSafe(cfg::VoxformatOptimize)->boolVal();
		const bool mergeQuads = core::Var::getSafe(cfg::VoxformatMergequads)->boolVal();
		const bool reuseVertices = core::Var::getSafe(cfg::VoxformatReusevertices)->boolVal();
		const bool ambientOcclusion = core::Var::getSafe(cfg::VoxformatAmbientocclusion)->boolVal();
		const bool applyTransform = core::Var::getSafe(cfg::VoxformatTransform)->boolVal();
		for (int i = start; i < end; ++i) {
			const scenegraph::SceneGraphNode &node = sceneGraph.node(i);
			if (!node.isAnyModelNode()) {
				continue;
			}
			auto volume = sceneGraph.resolveVolume(node);
			auto region = sceneGraph.resolveRegion(node);
			voxel::ChunkMesh *mesh = new voxel::ChunkMesh();
			voxel::Region regionExt = region;
			// we are increasing the region by one voxel to ensure the inclusion of the boundary voxels in this mesh
			regionExt.shiftUpperCorner(1, 1, 1);
			voxel::SurfaceExtractionContext ctx = voxel::createContext(
				type, volume, regionExt, node.palette(), *mesh, {0, 0, 0}, mergeQuads, reuseVertices, ambientOcclusion);
			voxel::extractSurface(ctx);
			if (withNormals) {
				Log::debug("Calculate normals");
				mesh->calculateNormals();
			}
			if (optimizeMesh) {
				mesh->optimize();
			}

			meshes[i] = core::move(MeshExt(mesh, node, applyTransform));
		}
	});
	Meshes nonEmptyMeshes;
	nonEmptyMeshes.reserve(meshes.size());

	core::Map<int, int> meshIdxNodeMap;
	// filter out empty meshes
	for (auto iter = meshes.begin(); iter != meshes.end(); ++iter) {
		if (!iter->mesh || iter->mesh->isEmpty()) {
			continue;
		}
		nonEmptyMeshes.emplace_back(*iter);
		meshIdxNodeMap.put(iter->nodeId, (int)nonEmptyMeshes.size() - 1);
	}
	bool state;
	if (nonEmptyMeshes.empty() && sceneGraph.empty(scenegraph::SceneGraphNodeType::Point)) {
		Log::warn("Empty scene can't get saved as mesh");
		state = false;
	} else {
		Log::debug("Save meshes");
		state = saveMeshes(meshIdxNodeMap, sceneGraph, nonEmptyMeshes, filename, archive, {1.0f, 1.0f, 1.0f},
						   type == voxel::SurfaceExtractionType::Cubic ? quads : false, withColor, withTexCoords);
	}
	for (MeshExt &meshext : meshes) {
		delete meshext.mesh;
	}
	return state;
}

} // namespace voxelformat
