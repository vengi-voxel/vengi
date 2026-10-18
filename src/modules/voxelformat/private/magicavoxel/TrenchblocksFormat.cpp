/**
 * @file
 */

#include "TrenchblocksFormat.h"
#include "MagicaVoxel.h"
#include "VoxFormat.h"
#include "color/ColorUtil.h"
#include "color/Distance.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxelutil/VolumeVisitor.h"
#include <limits.h>

namespace voxelformat {

static const char *TileNames[] = {"T1", "T2", "T3", "T4"};
struct MarkerType {
	const char *name;
	ogt_vox_rgba color;
};
static const MarkerType MarkerTypes[] = {
	{"FlagRed", {255, 66, 66, 255}},	   {"SpawnRed", {123, 0, 0, 255}},
	{"FlagBlue", {25, 148, 255, 255}},	   {"SpawnBlue", {0, 91, 172, 255}},
	{"PickupHealth", {237, 79, 255, 255}}, {"PickupAmmo", {255, 182, 0, 255}},
	{"PickupSatchel", {0, 0, 0, 255}},	   {"SpectatorSpawnPoint", {255, 255, 255, 255}}};

bool TrenchblocksFormat::isTrenchblocksScene(const ogt_vox_scene *scene) {
	if (scene->num_models != lengthof(TileNames) || scene->num_instances != lengthof(TileNames) ||
		scene->num_groups != 1 || scene->groups[0].parent_group_index != k_invalid_group_index) {
		return false;
	}
	for (int i = 0; i < (int)lengthof(MarkerTypes); ++i) {
		const ogt_vox_rgba &c = scene->palette.color[i + 1];
		const ogt_vox_rgba &expected = MarkerTypes[i].color;
		if (c.r != expected.r || c.g != expected.g || c.b != expected.b || c.a != expected.a) {
			return false;
		}
	}
	glm::mat4 first(1.0f);
	for (uint32_t i = 0; i < 4; ++i) {
		const ogt_vox_instance &instance = scene->instances[i];
		const ogt_vox_model *model = scene->models[i];
		if (instance.group_index != 0 || instance.transform_anim.num_keyframes > 1 ||
			scene->groups[0].transform_anim.num_keyframes > 1) {
			return false;
		}
		if (model == nullptr || model->size_x != 256 || model->size_y != 256 || model->size_z == 0 ||
			model->size_z > 256 || model->size_z != scene->models[0]->size_z || instance.model_index != i ||
			instance.name == nullptr || core::String(instance.name) != TileNames[i]) {
			return false;
		}
		const glm::mat4 transform = ogtInstanceBakeMatrix(instance, 0, scene, model);
		// Leave room for tile extents before converting world coordinates to integers.
		const glm::vec3 translation(transform[3]);
		if (glm::mat3(transform) != glm::mat3(1.0f) ||
			!glm::all(glm::lessThan(glm::abs(translation), glm::vec3((float)(INT_MAX - 1024))))) {
			return false;
		}
		if (i == 0) {
			first = transform;
		} else if (transform[3].x != first[3].x + (i % 2) * 256 || transform[3].y != first[3].y - (i >= 2 ? 256 : 0) ||
				   transform[3].z != first[3].z) {
			return false;
		}
	}
	return true;
}

bool TrenchblocksFormat::loadScene(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph) {
	palette::Palette palette;
	loadPaletteFromScene(scene, palette);
	core::DynamicArray<MVModelToNode> models = loadModels(scene, palette);
	for (uint32_t i = 0; i < lengthof(TileNames); ++i) {
		if (stopExecution()) {
			return false;
		}
		const ogt_vox_instance &instance = scene->instances[i];
		const glm::mat4 bake = ogtInstanceBakeMatrix(instance, 0, scene, scene->models[i]);
		const glm::ivec3 corner = calcTransform(bake, glm::vec3(255, 0, 0));
		const glm::ivec3 shift(-1 - corner.x, corner.z, corner.y);
		const bool visible = !instanceHidden(scene, instance);
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName(instance.name);
		node.setPalette(palette);
		node.setVisible(visible);
		node.setPivot(glm::vec3(0.0f));
		node.transform().setLocalTranslation(glm::vec3(shift));
		voxel::RawVolume *volume = models[i].volume;
		node.setVolume(volume);
		models[i].volume = nullptr;
		if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
			return false;
		}
		bool success = true;
		voxelutil::visitVolume(*volume, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			// MagicaVoxel indices 1..8 become native palette indices 0..7.
			if (voxel.getColor() >= lengthof(MarkerTypes) || !success) {
				return;
			}
			scenegraph::SceneGraphNode point(scenegraph::SceneGraphNodeType::Point);
			point.setName(MarkerTypes[voxel.getColor()].name);
			point.setVisible(visible);
			point.transform().setLocalTranslation(glm::vec3(shift + glm::ivec3(x, y, z)));
			success = sceneGraph.emplace(core::move(point)) != InvalidNodeId;
			volume->setVoxel(x, y, z, voxel::Voxel());
		});
		if (!success) {
			return false;
		}
	}
	sceneGraph.updateTransforms();
	return true;
}

bool TrenchblocksFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
									scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	VoxFormat format;
	return format.load(filename, archive, sceneGraph, ctx);
}

bool TrenchblocksFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
									const io::ArchivePtr &archive, const SaveContext &ctx) {
	palette::Palette palette;
	core::ScopedPtr<voxel::RawVolume> volume(mergeVolumes(sceneGraph, glm::ivec3(512, 256, 512), palette));
	if (!volume) {
		return false;
	}
	const voxel::Region &bounds = volume->region();
	const glm::ivec3 size = bounds.getDimensionsInVoxels();
	const glm::ivec3 mins = bounds.getLowerCorner();
	const glm::ivec3 maxs = bounds.getUpperCorner();
	// MagicaVoxel uses Z up and mirrors vengi X.
	const glm::ivec3 origin(-1 - maxs.x, mins.z, mins.y);
	int height = size.y;
	core::DynamicArray<core::String> spawnLayers;
	core::string::splitString(sceneGraph.root().property("LayersPlayerSpawn"), spawnLayers, ",");
	for (core::String &name : spawnLayers) {
		const size_t first = name.find_first_not_of(" \t");
		size_t end = name.size();
		while (end > 0 && (name[end - 1] == ' ' || name[end - 1] == '\t')) {
			--end;
		}
		name = first == core::String::npos ? "" : name.substr(first, end - first);
	}
	auto spawnLayer = [&](const scenegraph::SceneGraphNode &node) {
		const core::String layer = node.property("layer");
		for (const core::String &name : spawnLayers) {
			if (!name.empty() && (name == node.name() || name == layer)) {
				return true;
			}
		}
		return false;
	};
	bool filterSpawns = false;
	for (const auto &entry : sceneGraph.nodes()) {
		if (entry->second.isAnyModelNode() && spawnLayer(entry->second)) {
			filterSpawns = true;
			break;
		}
	}
	voxel::SparseVolume blocking(bounds);
	if (filterSpawns) {
		for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::AllModels); iter != sceneGraph.end();
			 ++iter) {
			const scenegraph::SceneGraphNode &node = *iter;
			if (node.visible() && !spawnLayer(node)) {
				sceneGraph.bakeIntoSparse(0, blocking, node, palette);
			}
		}
	}
	bool used[256]{};
	core::Buffer<int> heights;
	heights.resize(size.x * size.z);
	for (int &top : heights) {
		top = mins.y;
	}
	const int solid = voxelutil::visitVolume(*volume, bounds, [&](int x, int y, int z, const voxel::Voxel &voxel) {
		used[voxel.getColor()] = true;
		int &top = heights[x - mins.x + (z - mins.z) * size.x];
		top = glm::max(top, y + 1);
	});
	if (solid == 0) {
		Log::error("Trenchblocks maps require at least one solid terrain voxel");
		return false;
	}
	struct Stamp {
		glm::ivec3 pos;
		uint8_t index;
	};
	core::Buffer<Stamp> stamps;
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Point); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (!node.visible()) {
			continue;
		}
		const scenegraph::SceneGraphNode *parent = sceneGraph.findNodeByUUID(node.parentUUID());
		const core::String &name = parent != nullptr && parent->isGroupNode() ? parent->name() : node.name();
		for (int i = 0; i < (int)lengthof(MarkerTypes); ++i) {
			if (name != MarkerTypes[i].name) {
				continue;
			}
			glm::dvec3 position = glm::floor(glm::dvec3(node.transform().worldTranslation()));
			if (glm::any(glm::isnan(position)) || glm::any(glm::isinf(position))) {
				Log::error("Invalid Trenchblocks marker position");
				return false;
			}
			if (position.x < mins.x || position.x > maxs.x || position.z < mins.z || position.z > maxs.z) {
				continue;
			}
			const bool point2D = node.property("trenchblocks_2d") == "true";
			if (point2D) {
				position.y = heights[(int)position.x - mins.x + ((int)position.z - mins.z) * size.x];
			}
			if (position.y < mins.y || position.y - mins.y >= 256) {
				Log::error("Trenchblocks marker exceeds map height");
				return false;
			}
			const glm::ivec3 p(position);
			bool blocked = false;
			if (filterSpawns && (i == 1 || i == 3)) {
				if (point2D) {
					for (int y = mins.y; y <= maxs.y; ++y) {
						if (!voxel::isAir(blocking.voxel(p.x, y, p.z).getMaterial())) {
							blocked = true;
							break;
						}
					}
				} else {
					blocked = !voxel::isAir(blocking.voxel(p).getMaterial());
				}
			}
			if (blocked) {
				continue;
			}
			const glm::ivec3 local(maxs.x - p.x, p.z - mins.z, p.y - mins.y);
			height = glm::max(height, local.z + 1);
			stamps.push_back({local, (uint8_t)(i + 1)});
			break;
		}
	}

	ogt_vox_scene scene{};
	scene.file_version = 200;
	for (int i = 0; i < (int)lengthof(MarkerTypes); ++i) {
		scene.palette.color[i + 1] = MarkerTypes[i].color;
	}
	// Keep terrain separate from markers, even for identical RGB values.
	uint8_t remap[256]{};
	int nextColor = 17;
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (!used[i]) {
			continue;
		}
		const color::RGBA c = palette.color(i);
		int best = 17;
		float distance = color::ApproximationDistanceMax;
		for (int j = 17; j < nextColor; ++j) {
			const ogt_vox_rgba &other = scene.palette.color[j];
			const float d =
				color::getDistance(c, color::RGBA(other.r, other.g, other.b, 255), color::Distance::Approximation);
			if (d < distance) {
				distance = d;
				best = j;
			}
		}
		if (distance > 0.0f && nextColor < 256) {
			best = nextColor++;
			scene.palette.color[best] = {c.r, c.g, c.b, 255};
		}
		remap[i] = (uint8_t)best;
	}

	ogt_vox_model models[4]{};
	const ogt_vox_model *modelPtrs[4];
	ogt_vox_instance instances[4]{};
	core::Buffer<uint8_t> data[4];
	for (int i = 0; i < 4; ++i) {
		data[i].resize(256 * 256 * height);
		models[i].size_x = models[i].size_y = 256;
		models[i].size_z = height;
		models[i].voxel_data = data[i].data();
		modelPtrs[i] = &models[i];
		instances[i].name = TileNames[i];
		instances[i].model_index = i;
		instances[i].transform = ogt_identity_transform;
		instances[i].transform.m30 = origin.x + (i % 2) * 256 + 128;
		instances[i].transform.m31 = origin.y + (i < 2 ? 256 : 0) + 128;
		instances[i].transform.m32 = origin.z + height / 2;
	}
	auto put = [&](const glm::ivec3 &p, uint8_t index) {
		const int tile = (p.y >= 256 ? 0 : 2) + p.x / 256;
		data[tile][p.x % 256 + (p.y % 256) * 256 + p.z * 256 * 256] = index;
	};
	voxelutil::visitVolume(*volume, bounds, [&](int x, int y, int z, const voxel::Voxel &voxel) {
		put(glm::ivec3(maxs.x - x, z - mins.z, y - mins.y), remap[voxel.getColor()]);
	});
	for (const Stamp &stamp : stamps) {
		put(stamp.pos, stamp.index);
	}
	ogt_vox_group group{};
	group.transform = ogt_identity_transform;
	group.parent_group_index = k_invalid_group_index;
	ogt_vox_layer layer{};
	layer.color = {255, 255, 255, 255};
	scene.num_layers = 1;
	scene.layers = &layer;
	scene.num_groups = 1;
	scene.groups = &group;
	scene.num_models = scene.num_instances = 4;
	scene.models = modelPtrs;
	scene.instances = instances;
	uint32_t length = 0;
	uint8_t *buffer = ogt_vox_write_scene(&scene, &length);
	if (buffer == nullptr) {
		return false;
	}
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	const bool success = stream && stream->write(buffer, length) == (int)length;
	ogt_vox_free(buffer);
	ctx.setProgress(1.0f);
	return success;
}

} // namespace voxelformat
