/**
 * @file
 */

#include "MeshFormat.h"
#include "app/App.h"
#include "app/Async.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "core/concurrent/Lock.h"
#include "io/Archive.h"
#include "io/FormatDescription.h"
#include "palette/NormalPalette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/ChunkMesh.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/Voxel.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>

namespace voxelformat {

#define AlphaThreshold 0

MeshFormat::MeshFormat() {
	_flattenFactor = core::Var::getSafe(cfg::VoxformatRGBFlattenFactor)->intVal();
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

void MeshFormat::subdivideTri(const voxelformat::MeshTri &tri, TriCollection &tinyTris) {
	if (stopExecution()) {
		return;
	}
	const glm::vec3 &mins = tri.mins();
	const glm::vec3 &maxs = tri.maxs();
	const glm::vec3 size = maxs - mins;
	if (glm::any(glm::greaterThan(size, glm::vec3(1.0f)))) {
		voxelformat::MeshTri out[4];
		tri.subdivide(out);
		for (int i = 0; i < lengthof(out); ++i) {
			subdivideTri(out[i], tinyTris);
		}
		return;
	}
	tinyTris.push_back(tri);
}

uint8_t MeshFormat::PosSampling::getNormal() const {
	if (entries[1].area == 0) {
		return entries[0].normal;
	}
	uint8_t normal = 0;
	uint32_t area = 0;
	for (const PosSamplingEntry &pe : entries) {
		if (pe.area > area) {
			area = pe.area;
			normal = pe.normal;
		}
	}
	return normal;
}

bool MeshFormat::PosSampling::add(uint32_t area, core::RGBA color, uint8_t normal) {
	// TODO: VOXELFORMAT: why?
	if (entries[0].color == color) {
		return false;
	}
	if (area == 0) {
		// nothing to contribute
		return false;
	}
#if 0
	for (int i = 0; i < MaxTriangleColorContributions; ++i) {
		// same values only increase the area of contribution and thus the weighting influence for this color
		if (entries[i].area > 0 && entries[i].color == color && entries[i].normal == normal) {
			entries[i].area += area;
			return true;
		}
	}
#endif

#if 1
	for (int i = 0; i < MaxTriangleColorContributions; ++i) {
		// free slot
		if (entries[i].area == 0) {
			entries[i].area = area;
			entries[i].color = color;
			entries[i].normal = normal;
			return true;
		}
	}
#else
	int smallestArea = 0xffffffff;
	int index = 0;
	for (int i = 0; i < MaxTriangleColorContributions; ++i) {
		// free slot
		if (entries[i].area == 0) {
			entries[i].area = area;
			entries[i].color = color;
			entries[i].normal = normal;
			return true;
		}
		if (smallestArea > entries[i].area) {
			smallestArea = entries[i].area;
			index = i;
		}
	}
	// check if this contribution should have a higher impact
	if (entries[index].area < area) {
		entries[index].area = area;
		entries[index].color = color;
		entries[index].normal = normal;
		return true;
	}
#endif

	return false;
}

core::RGBA MeshFormat::PosSampling::getColor(uint8_t flattenFactor, bool weightedAverage) const {
	if (entries[1].area == 0) {
		return core::Color::flattenRGB(entries[0].color.r, entries[0].color.g, entries[0].color.b, entries[0].color.a,
									   flattenFactor);
	}
	if (weightedAverage) {
		uint32_t sumArea = 0;
		for (const PosSamplingEntry &pe : entries) {
			sumArea += pe.area;
		}
		core::RGBA color(0, 0, 0, 255);
		if (sumArea == 0) {
			return color;
		}
		for (const PosSamplingEntry &pe : entries) {
			if (pe.area == 0) {
				break;
			}
			color = core::RGBA::mix(color, pe.color, (float)pe.area / (float)sumArea);
		}
		return core::Color::flattenRGB(color.r, color.g, color.b, color.a, flattenFactor);
	}
	core::RGBA color(0, 0, 0, AlphaThreshold);
	uint32_t area = 0;
	for (const PosSamplingEntry &pe : entries) {
		if (pe.area == 0) {
			break;
		}
		if (pe.area > area) {
			area = pe.area;
			color = pe.color;
		}
	}
	return core::Color::flattenRGB(color.r, color.g, color.b, color.a, flattenFactor);
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

void MeshFormat::addToPosMap(PosMap &posMap, core::RGBA rgba, uint32_t area, uint8_t normalIdx, const glm::ivec3 &pos) {
	auto iter = posMap.find(pos);
	if (iter == posMap.end()) {
		posMap.emplace(pos, {area, rgba, normalIdx});
	} else {
		PosSampling &posSampling = iter->value;
		posSampling.add(area, rgba, normalIdx);
	}
}

void MeshFormat::transformTris(const voxel::Region &region, const TriCollection &tris, PosMap &posMap,
							   const palette::NormalPalette &normalPalette) {
	Log::debug("subdivided into %i triangles", (int)tris.size());
	for (const voxelformat::MeshTri &tri : tris) {
		if (stopExecution()) {
			return;
		}
		const core::RGBA rgba = tri.centerColor();
		if (rgba.a <= AlphaThreshold) {
			continue;
		}
		const uint32_t area = (uint32_t)(tri.area() * 1000.0f);
		glm::vec3 c = tri.center();
		convertToVoxelGrid(c);

		const uint8_t normalIdx = normalPalette.getClosestMatch(tri.normal());

		const glm::ivec3 p(c);
		core_assert_msg(region.containsPoint(p), "Failed to transform tri %i:%i:%i (region: %s)", p.x, p.y, p.z,
						region.toString().c_str());
		addToPosMap(posMap, rgba, area, normalIdx, p);
	}
}

void MeshFormat::transformTrisAxisAligned(const voxel::Region &region, const TriCollection &tris, PosMap &posMap, const palette::NormalPalette &normalPalette) {
	Log::debug("axis aligned %i triangles", (int)tris.size());
	for (const voxelformat::MeshTri &tri : tris) {
		if (stopExecution()) {
			return;
		}
		const core::RGBA rgba = tri.centerColor();
		if (rgba.a <= AlphaThreshold) {
			continue;
		}
		const uint32_t area = (uint32_t)(tri.area() * 1000.0f);
		const glm::vec3 &normal = glm::normalize(tri.normal());
		const glm::ivec3 sideDelta(normal.x <= 0 ? 0 : -1, normal.y <= 0 ? 0 : -1, normal.z <= 0 ? 0 : -1);
		const glm::ivec3 mins = tri.roundedMins();
		const glm::ivec3 maxs = tri.roundedMaxs() + glm::ivec3(glm::round(glm::abs(normal)));
		Log::trace("mins: %i:%i:%i", mins.x, mins.y, mins.z);
		Log::trace("maxs: %i:%i:%i", maxs.x, maxs.y, maxs.z);
		Log::trace("normal: %f:%f:%f", normal.x, normal.y, normal.z);
		Log::trace("sideDelta: %i:%i:%i", sideDelta.x, sideDelta.y, sideDelta.z);
		const uint8_t normalIdx = normalPalette.getClosestMatch(normal);
		for (int x = mins.x; x < maxs.x; x++) {
			for (int y = mins.y; y < maxs.y; y++) {
				for (int z = mins.z; z < maxs.z; z++) {
					const glm::ivec3 p(x + sideDelta.x, y + sideDelta.y, z + sideDelta.z);
					if (!region.containsPoint(p)) {
						Log::debug("Failed to transform tri %i:%i:%i (region: %s), (sideDelta: %i:%i:%i)", p.x, p.y,
								   p.z, region.toString().c_str(), sideDelta.x, sideDelta.y, sideDelta.z);
						continue;
					}
					addToPosMap(posMap, rgba, area, normalIdx, p);
				}
			}
		}
	}
}

bool MeshFormat::isVoxelMesh(const TriCollection &tris) {
	for (const voxelformat::MeshTri &tri : tris) {
		if (!tri.flat()) {
			Log::debug("No axis aligned mesh found");
#ifdef DEBUG
			for (int i = 0; i < 3; ++i) {
				const glm::vec3 &v = tri.vertices[i];
				Log::debug("tri.vertices[%i]: %f:%f:%f", i, v.x, v.y, v.z);
			}
			const glm::vec3 &n = tri.normal();
			Log::debug("tri.normal: %f:%f:%f", n.x, n.y, n.z);
#endif
			return false;
		}
	}
	Log::debug("Found axis aligned mesh");
	return true;
}

template<class FUNC>
static void voxelizeTriangle(const glm::vec3 &trisMins, const voxelformat::MeshTri &tri, FUNC &&func) {
	const glm::vec3 voxelHalf(0.5f);
	const glm::vec3 shiftedTrisMins = trisMins + voxelHalf;
	const glm::vec3 &v0 = tri.vertices[0];
	const glm::vec3 &v1 = tri.vertices[1];
	const glm::vec3 &v2 = tri.vertices[2];
	const glm::vec3 mins = tri.mins();
	const glm::vec3 maxs = tri.maxs();
	const glm::ivec3 imins(glm::floor(mins - shiftedTrisMins));
	const glm::ivec3 size(glm::round(maxs - mins));
	const glm::ivec3 imaxs = 2 + imins + size;

	glm::vec3 center {};
	for (int x = imins.x; x < imaxs.x; x++) {
		center.x = trisMins.x + x;
		for (int y = imins.y; y < imaxs.y; y++) {
			center.y = trisMins.y + y;
			for (int z = imins.z; z < imaxs.z; z++) {
				center.z = trisMins.z + z;
				if (glm::intersectTriangleAABB(center, voxelHalf, v0, v1, v2)) {
					glm::vec2 uv;
					if (!tri.calcUVs(center, uv)) {
						continue;
					}
					func(tri, uv, shiftedTrisMins.x + x, shiftedTrisMins.y + y, shiftedTrisMins.z + z);
				}
			}
		}
	}
}

int MeshFormat::voxelizeNode(const core::String &uuid, const core::String &name, scenegraph::SceneGraph &sceneGraph, const TriCollection &tris,
							 int parent, bool resetOrigin) const {
	if (tris.empty()) {
		Log::warn("Empty volume - no triangles given");
		return InvalidNodeId;
	}

	const bool axisAligned = isVoxelMesh(tris);

	glm::vec3 trisMins;
	glm::vec3 trisMaxs;
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

	const glm::ivec3 &vdim = region.getDimensionsInVoxels();
	if (glm::any(glm::greaterThan(vdim, glm::ivec3(512)))) {
		Log::warn("Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh! (%i:%i:%i)",
				  vdim.x, vdim.y, vdim.z);
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, uuid);
	node.setVolume(new voxel::RawVolume(region), true);
	node.setName(name);
	palette::NormalPalette normalPalette;
	normalPalette.redAlert2();
	node.setNormalPalette(normalPalette);

	const int voxelizeMode = core::Var::getSafe(cfg::VoxformatVoxelizeMode)->intVal();
	const bool fillHollow = core::Var::getSafe(cfg::VoxformatFillHollow)->boolVal();
	if (axisAligned) {
		const int maxVoxels = vdim.x * vdim.y * vdim.z;
		Log::debug("max voxels: %i (%i:%i:%i)", maxVoxels, vdim.x, vdim.y, vdim.z);
		PosMap posMap(maxVoxels);
		transformTrisAxisAligned(region, tris, posMap, normalPalette);
		voxelizeTris(node, posMap, fillHollow);
	} else if (voxelizeMode == VoxelizeMode::Fast) {
		voxel::RawVolumeWrapper wrapper(node.volume());
		palette::Palette palette;

		const bool shouldCreatePalette = core::Var::getSafe(cfg::VoxelCreatePalette)->boolVal();
		if (shouldCreatePalette) {
			RGBAMap colors;
			Log::debug("create palette");
			for (const voxelformat::MeshTri &triangle : tris) {
#if 1
				voxelizeTriangle(trisMins, triangle, [this, &colors] (const voxelformat::MeshTri &tri, const glm::vec2 &uv, int x, int y, int z) {
					const core::RGBA rgba = flattenRGB(tri.colorAt(uv));
					colors.put(rgba, true);
				});
#else
				const core::RGBA rgba = flattenRGB(triangle.centerColor());
				colors.put(rgba, true);
#endif
			}
			createPalette(colors, palette);
		} else {
			palette = voxel::getPalette();
		}

		Log::debug("create voxels from %i tris", (int)tris.size());
		palette::PaletteLookup palLookup(palette);
		for (const voxelformat::MeshTri &triangle : tris) {
			voxelizeTriangle(trisMins, triangle, [&] (const voxelformat::MeshTri &tri, const glm::vec2 &uv, int x, int y, int z) {
				const core::RGBA color = flattenRGB(tri.colorAt(uv));
				const glm::vec3 &normal = tri.normal();
				const uint8_t normalIndex = normalPalette.getClosestMatch(normal);
				const voxel::Voxel voxel = voxel::createVoxel(palette, palLookup.findClosestIndex(color), normalIndex);
				wrapper.setVoxel(x, y, z, voxel);
			});
		}

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
		Log::debug("Subdivide %i triangles", (int)tris.size());
		core::DynamicArray<std::future<TriCollection>> futures;
		futures.reserve(tris.size());
		for (const voxelformat::MeshTri &tri : tris) {
			futures.emplace_back(app::async([tri]() {
				TriCollection subdivided;
				subdivideTri(tri, subdivided);
				return subdivided;
			}));
		}
		TriCollection subdivided;
		for (std::future<TriCollection> &future : futures) {
			const TriCollection &sub = future.get();
			subdivided.append(sub);
		}

		if (subdivided.empty()) {
			Log::warn("Empty volume - could not subdivide");
			return InvalidNodeId;
		}

		PosMap posMap((int)subdivided.size() * 3);
		transformTris(region, subdivided, posMap, normalPalette);
		voxelizeTris(node, posMap, fillHollow);
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

bool MeshFormat::calculateAABB(const TriCollection &tris, glm::vec3 &mins, glm::vec3 &maxs) {
	if (tris.empty()) {
		mins = maxs = glm::vec3(0.0f);
		return false;
	}

	maxs = tris[0].mins();
	mins = tris[0].maxs();

	for (const voxelformat::MeshTri &tri : tris) {
		maxs = glm::max(maxs, tri.maxs());
		mins = glm::min(mins, tri.mins());
	}
	return true;
}

void MeshFormat::voxelizeTris(scenegraph::SceneGraphNode &node, const PosMap &posMap, bool fillHollow) const {
	voxel::RawVolumeWrapper wrapper(node.volume());
	palette::Palette palette;
	const bool shouldCreatePalette = core::Var::getSafe(cfg::VoxelCreatePalette)->boolVal();
	if (shouldCreatePalette) {
		RGBAMap colors;
		Log::debug("create palette");
		for (const auto &entry : posMap) {
			if (stopExecution()) {
				return;
			}
			const PosSampling &pos = entry->second;
			const core::RGBA rgba = pos.getColor(_flattenFactor, _weightedAverage);
			if (rgba.a <= AlphaThreshold) {
				continue;
			}
			colors.put(rgba, true);
		}
		createPalette(colors, palette);
	} else {
		palette = voxel::getPalette();
	}

	Log::debug("create voxels for %i positions", (int)posMap.size());
	for (const auto &entry : posMap) {
		if (stopExecution()) {
			return;
		}
		const PosSampling &pos = entry->second;
		const core::RGBA rgba = pos.getColor(_flattenFactor, _weightedAverage);
		if (rgba.a <= AlphaThreshold) {
			continue;
		}
		const voxel::Voxel voxel = voxel::createVoxel(palette, palette.getClosestMatch(rgba), pos.getNormal());
		wrapper.setVoxel(entry->first, voxel);
	}
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
		voxelutil::fillHollow(wrapper, voxel);
	}
}

MeshFormat::MeshExt::MeshExt(voxel::ChunkMesh *_mesh, const scenegraph::SceneGraphNode &node, bool _applyTransform)
	: mesh(_mesh), name(node.name()), applyTransform(_applyTransform), size(node.region().getDimensionsInVoxels()),
	  pivot(node.pivot()), nodeId(node.id()) {
}

bool MeshFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
							scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	const bool retVal = voxelizeGroups(filename, archive, sceneGraph, ctx);
	sceneGraph.updateTransforms();
	return retVal;
}

bool MeshFormat::voxelizePointCloud(const core::String &filename, scenegraph::SceneGraph &sceneGraph, const core::DynamicArray<PointCloudVertex> &vertices) const {
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
	for (const PointCloudVertex &vertex : vertices) {
		const glm::ivec3 pos = glm::round(vertex.position);
		const voxel::Voxel voxel = voxel::createVoxel(palette, palette.getClosestMatch(vertex.color));
		for (int x = 0; x < pointSize; ++x) {
			for (int y = 0; y < pointSize; ++y) {
				for (int z = 0; z < pointSize; ++z) {
					v->setVoxel(pos + glm::ivec3(x, y, z), voxel);
				}
			}
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(v, true);
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool MeshFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &, scenegraph::SceneGraph &,
								const LoadContext &) {
	Log::debug("Mesh %s can't get voxelized yet", filename.c_str());
	return false;
}

// TODO: VOXELFORMAT: use io::Archive here, too
core::String MeshFormat::lookupTexture(const core::String &meshFilename, const core::String &in) {
	const core::String &meshPath = core::string::extractDir(meshFilename);
	core::String name = in;
	io::normalizePath(name);
	if (!core::string::isAbsolutePath(name)) {
		name = core::string::path(meshPath, name);
	}
	if (io::filesystem()->exists(name)) {
		Log::debug("Found image %s in path %s", in.c_str(), name.c_str());
		return name;
	}

	if (!meshPath.empty()) {
		io::filesystem()->sysPushDir(meshPath);
	}
	core::String filename = core::string::extractFilenameWithExtension(name);
	const core::String &path = core::string::extractDir(name);
	core::String fullpath = io::searchPathFor(io::filesystem(), path, filename);
	if (fullpath.empty() && path != meshPath) {
		fullpath = io::searchPathFor(io::filesystem(), meshPath, filename);
	}
	if (fullpath.empty()) {
		fullpath = io::searchPathFor(io::filesystem(), "texture", filename);
	}
	if (fullpath.empty()) {
		fullpath = io::searchPathFor(io::filesystem(), "textures", filename);
	}

	// if not found, loop over all supported image formats and repeat the search
	if (fullpath.empty()) {
		const core::String &baseFilename = core::string::extractFilename(name);
		if (!baseFilename.empty()) {
			for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
				for (const core::String &ext : desc->exts) {
					const core::String &f = core::string::format("%s.%s", baseFilename.c_str(), ext.c_str());
					if (f == filename) {
						continue;
					}
					fullpath = io::searchPathFor(io::filesystem(), path, f);
					if (fullpath.empty() && path != meshPath) {
						fullpath = io::searchPathFor(io::filesystem(), meshPath, f);
					}
					if (fullpath.empty()) {
						fullpath = io::searchPathFor(io::filesystem(), "texture", f);
					}
					if (fullpath.empty()) {
						fullpath = io::searchPathFor(io::filesystem(), "textures", f);
					}
					if (!fullpath.empty()) {
						if (!meshPath.empty()) {
							io::filesystem()->sysPopDir();
						}
						return fullpath;
					}
				}
			}
		}
	}

	if (fullpath.empty()) {
		Log::error("Failed to perform texture lookup for '%s' (filename: '%s')", name.c_str(), filename.c_str());
	}
	if (!meshPath.empty()) {
		io::filesystem()->sysPopDir();
	}
	return fullpath;
}

bool MeshFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive, const SaveContext &saveCtx) {
	const bool mergeQuads = core::Var::getSafe(cfg::VoxformatMergequads)->boolVal();
	const bool reuseVertices = core::Var::getSafe(cfg::VoxformatReusevertices)->boolVal();
	const bool ambientOcclusion = core::Var::getSafe(cfg::VoxformatAmbientocclusion)->boolVal();
	const bool quads = core::Var::getSafe(cfg::VoxformatQuads)->boolVal();
	const bool withColor = core::Var::getSafe(cfg::VoxformatWithColor)->boolVal();
	const bool withNormals = core::Var::getSafe(cfg::VoxformatWithNormals)->boolVal();
	const bool withTexCoords = core::Var::getSafe(cfg::VoxformatWithtexcoords)->boolVal();
	const bool applyTransform = core::Var::getSafe(cfg::VoxformatTransform)->boolVal();
	const bool optimizeMesh = core::Var::getSafe(cfg::VoxformatOptimize)->boolVal();

	const voxel::SurfaceExtractionType type = (voxel::SurfaceExtractionType)core::Var::getSafe(cfg::VoxelMeshMode)->intVal();

	const size_t models = sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels);
	Meshes meshes;
	core::Map<int, int> meshIdxNodeMap;
	core_trace_mutex(core::Lock, lock, "MeshFormat");
	// TODO: VOXELFORMAT: this could get optimized by re-using the same mesh for multiple nodes (in case of reference nodes)
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		app::async([&, volume = sceneGraph.resolveVolume(node), region = sceneGraph.resolveRegion(node)]() {
			voxel::ChunkMesh *mesh = new voxel::ChunkMesh();
			voxel::Region regionExt = region;
			// we are increasing the region by one voxel to ensure the inclusion of the boundary voxels in this mesh
			regionExt.shiftUpperCorner(1, 1, 1);
			voxel::SurfaceExtractionContext ctx =
				voxel::createContext(type, volume, regionExt, node.palette(), *mesh, {0, 0, 0}, mergeQuads,
									 reuseVertices, ambientOcclusion);
			voxel::extractSurface(ctx);
			if (withNormals) {
				Log::debug("Calculate normals");
				mesh->calculateNormals();
			}
			if (optimizeMesh) {
				mesh->optimize();
			}

			core::ScopedLock scoped(lock);
			meshes.emplace_back(mesh, node, applyTransform);
		});
	}
	for (;;) {
		lock.lock();
		const size_t size = meshes.size();
		lock.unlock();
		if (size < models) {
			app::App::getInstance()->wait(10);
		} else {
			break;
		}
	}
	Meshes nonEmptyMeshes;
	nonEmptyMeshes.reserve(meshes.size());

	// filter out empty meshes
	for (auto iter = meshes.begin(); iter != meshes.end(); ++iter) {
		if (iter->mesh->isEmpty()) {
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
