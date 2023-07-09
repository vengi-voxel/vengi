/**
 * @file
 */

#include "VoxFormat.h"
#include "core/Color.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Var.h"
#include "private/MagicaVoxel.h"
#include "scenegraph/CoordinateSystemUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxelformat/external/ogt_vox.h"
#include "voxelutil/VolumeVisitor.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace voxelformat {

VoxFormat::VoxFormat() {
	ogt_vox_set_memory_allocator(_ogt_alloc, _ogt_free);
}

size_t VoxFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
							  const LoadContext &ctx) {
	const size_t size = stream.size();
	uint8_t *buffer = (uint8_t *)core_malloc(size);
	if (stream.read(buffer, size) == -1) {
		core_free(buffer);
		return 0;
	}
	loadPaletteFromBuffer(buffer, size, palette);
	core_free(buffer);
	return palette.colorCount();
}

bool VoxFormat::loadInstance(const ogt_vox_scene *scene, uint32_t ogt_instanceIdx, scenegraph::SceneGraph &sceneGraph,
							 int parent, core::DynamicArray<MVModelToNode> &models, const voxel::Palette &palette) {
	const ogt_vox_instance &ogtInstance = scene->instances[ogt_instanceIdx];
	const ogt_vox_model *ogtModel = scene->models[ogtInstance.model_index];
	const glm::mat4 ogtMat = ogtTransformToMat(ogtInstance, 0, scene, ogtModel);
	const glm::vec4 &ogtPivot = ogtVolumePivot(ogtModel);
	const glm::ivec3 &ogtMins = calcTransform(ogtMat, glm::ivec3(0), ogtPivot);
	const glm::ivec3 &ogtMaxs = calcTransform(ogtMat, ogtVolumeSize(ogtModel), ogtPivot);
	const glm::ivec3 mins(-(ogtMins.x + 1), ogtMins.z, ogtMins.y);
	const glm::ivec3 maxs(-(ogtMaxs.x + 1), ogtMaxs.z, ogtMaxs.y);
	voxel::Region region(glm::min(mins, maxs), glm::max(mins, maxs));
	const glm::ivec3 shift = region.getLowerCorner();
	region.shift(-shift);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	scenegraph::SceneGraphTransform transform;
	transform.setWorldTranslation(shift);

	const uint8_t *ogtVoxel = ogtModel->voxel_data;
	for (uint32_t k = 0; k < ogtModel->size_z; ++k) {
		for (uint32_t j = 0; j < ogtModel->size_y; ++j) {
			for (uint32_t i = 0; i < ogtModel->size_x; ++i, ++ogtVoxel) {
				if (ogtVoxel[0] == 0) {
					continue;
				}
				const voxel::Voxel voxel = voxel::createVoxel(palette, ogtVoxel[0] - 1);
				const glm::ivec3 &ogtPos = calcTransform(ogtMat, glm::ivec3(i, j, k), ogtPivot);
				const glm::ivec3 pos(-(ogtPos.x + 1), ogtPos.z, ogtPos.y);
				v->setVoxel(pos - shift, voxel);
			}
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	loadKeyFrames(sceneGraph, node, ogtInstance, scene);
	// TODO: we are overriding the keyframe data here
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	node.setColor(instanceColor(scene, ogtInstance));
	node.setName(instanceName(scene, ogtInstance));
	node.setVisible(!instanceHidden(scene, ogtInstance));
	node.setVolume(v, true);
	// TODO: use already loaded models and create a model reference if needed
	// TODO: node.setVolume(new voxel::RawVolume(models[ogtInstance.model_index].volume), true);
	// TODO: set correct pivot
	// TODO: node.setPivot({ogtPivot.x / (float)ogtModel->size_x, ogtPivot.z / (float)ogtModel->size_z, ogtPivot.y / (float)ogtModel->size_y});
	// TODO: node.setPivot({(ogtPivot.x + 0.5f) / (float)ogtModel->size_x, (ogtPivot.z + 0.5f) / (float)ogtModel->size_z, (ogtPivot.y + 0.5f) / (float)ogtModel->size_y});
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node), parent) != -1;
}

bool VoxFormat::loadGroup(const ogt_vox_scene *scene, uint32_t ogt_groupIdx, scenegraph::SceneGraph &sceneGraph,
						  int parent, core::DynamicArray<MVModelToNode> &models, core::Set<uint32_t> &addedInstances,
						  const voxel::Palette &palette) {
	const ogt_vox_group &ogt_group = scene->groups[ogt_groupIdx];
	bool hidden = ogt_group.hidden;
	const char *name = ogt_group.name ? ogt_group.name : "Group";
	const uint32_t layerIdx = ogt_group.layer_index;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
	if (layerIdx < scene->num_layers) {
		const ogt_vox_layer &layer = scene->layers[layerIdx];
		hidden |= layer.hidden;
		if (layer.name != nullptr) {
			name = layer.name;
		}
		const core::RGBA color(layer.color.r, layer.color.g, layer.color.b, layer.color.a);
		node.setColor(color);
	}
	node.setName(name);
	node.setVisible(!hidden);
	const int groupId = parent == -1 ? sceneGraph.root().id() : sceneGraph.emplace(core::move(node), parent);
	if (groupId == -1) {
		Log::error("Failed to add group node to the scene graph");
		return false;
	}

	for (uint32_t n = 0; n < scene->num_instances; ++n) {
		const ogt_vox_instance &ogtInstance = scene->instances[n];
		if (ogtInstance.group_index != ogt_groupIdx) {
			continue;
		}
		if (!addedInstances.insert(n)) {
			continue;
		}
		if (!loadInstance(scene, n, sceneGraph, groupId, models, palette)) {
			return false;
		}
	}

	for (uint32_t groupIdx = 0; groupIdx < scene->num_groups; ++groupIdx) {
		const ogt_vox_group &group = scene->groups[groupIdx];
		Log::debug("group %u with parent: %u (searching for %u)", groupIdx, group.parent_group_index, ogt_groupIdx);
		if (group.parent_group_index != ogt_groupIdx) {
			continue;
		}
		Log::debug("Found matching group (%u) with scene graph parent: %i", groupIdx, groupId);
		if (!loadGroup(scene, groupIdx, sceneGraph, groupId, models, addedInstances, palette)) {
			return false;
		}
	}

	return true;
}

bool VoxFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette, const LoadContext &ctx) {
	const size_t size = stream.size();
	uint8_t *buffer = (uint8_t *)core_malloc(size);
	if (stream.read(buffer, size) == -1) {
		core_free(buffer);
		return false;
	}
	const uint32_t ogt_vox_flags = k_read_scene_flags_keyframes | k_read_scene_flags_groups |
								   k_read_scene_flags_keep_empty_models_instances |
								   k_read_scene_flags_keep_duplicate_models;
	const ogt_vox_scene *scene = ogt_vox_read_scene_with_flags(buffer, size, ogt_vox_flags);
	core_free(buffer);
	if (scene == nullptr) {
		Log::error("Could not load scene %s", filename.c_str());
		return false;
	}

	printDetails(scene);
	loadPaletteFromScene(scene, palette);
	if (!loadScene(scene, sceneGraph, palette)) {
		ogt_vox_destroy_scene(scene);
		return false;
	}

	ogt_vox_destroy_scene(scene);

	if (sceneGraph.empty() && palette.colorCount() > 0) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName(filename);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 31)), true);
		node.setPalette(palette);
		return sceneGraph.emplace(core::move(node), 0) != InvalidNodeId;
	}
	return true;
}

bool VoxFormat::loadScene(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph,
						  const voxel::Palette &palette) {
	core::DynamicArray<MVModelToNode> models = loadModels(scene, palette);
	core::Set<uint32_t> addedInstances;
	for (uint32_t i = 0; i < scene->num_groups; ++i) {
		const ogt_vox_group &group = scene->groups[i];
		// find the main group nodes
		if (group.parent_group_index != k_invalid_group_index) {
			continue;
		}
		Log::debug("Add root group %u/%u", i, scene->num_groups);
		if (!loadGroup(scene, i, sceneGraph, -1, models, addedInstances, palette)) {
			return false;
		}
		break;
	}
	for (uint32_t n = 0; n < scene->num_instances; ++n) {
		if (addedInstances.has(n)) {
			continue;
		}
		// TODO: the parent is wrong
		if (!loadInstance(scene, n, sceneGraph, sceneGraph.root().id(), models, palette)) {
			return false;
		}
	}

	loadCameras(scene, sceneGraph);
	return true;
}

void VoxFormat::saveInstance(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
							 MVSceneContext &ctx, uint32_t parentGroupIdx, uint32_t layerIdx, uint32_t modelIdx) {
	const scenegraph::SceneGraphKeyFrames &keyFrames = node.keyFrames(sceneGraph.activeAnimation());
	ctx.nodeToModel.put(node.id(), modelIdx);
	{
		ogt_vox_instance ogt_instance;
		core_memset(&ogt_instance, 0, sizeof(ogt_instance));
		ogt_instance.group_index = parentGroupIdx;
		ogt_instance.model_index = modelIdx;
		ogt_instance.layer_index = layerIdx;
		ogt_instance.name = node.name().c_str();
		ogt_instance.hidden = !node.visible();
		ogt_instance.transform_anim.num_keyframes = keyFrames.size();
		// set this to the start pointer of the ctx.keyframeTransforms array - the array is filled below
		ogt_instance.transform_anim.keyframes =
			ogt_instance.transform_anim.num_keyframes ? &ctx.keyframeTransforms[ctx.transformKeyFrameIdx] : nullptr;
		ctx.instances.push_back(ogt_instance);
	}

	const voxel::Region region = sceneGraph.resolveRegion(node);
	const glm::vec3 width(region.getDimensionsInVoxels());
	const glm::vec3 mins(region.getLowerCornerf());
	for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
		ogt_vox_keyframe_transform ogt_keyframe;
		core_memset(&ogt_keyframe, 0, sizeof(ogt_keyframe));
		ogt_keyframe.frame_index = kf.frameIdx;
		ogt_keyframe.transform = ogt_identity_transform;
		// y and z are flipped here
		const glm::vec3 kftransform = mins + kf.transform().worldTranslation() + width / 2.0f;
		ogt_keyframe.transform.m30 = -glm::floor(kftransform.x + 0.5f);
		ogt_keyframe.transform.m31 = kftransform.z;
		ogt_keyframe.transform.m32 = kftransform.y;
		checkRotation(ogt_keyframe.transform);
		// TODO: apply rotation - but rotations are not interpolated - they must be aligned here somehow...
		ctx.keyframeTransforms[ctx.transformKeyFrameIdx++] = ogt_keyframe;
	}
}

void VoxFormat::saveNode(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
						 MVSceneContext &ctx, uint32_t parentGroupIdx, uint32_t layerIdx, const voxel::Palette &palette,
						 uint8_t replacement) {
	Log::debug("Save node '%s' with parent group %u and layer %u", node.name().c_str(), parentGroupIdx, layerIdx);
	if (node.type() == scenegraph::SceneGraphNodeType::Root || node.type() == scenegraph::SceneGraphNodeType::Group) {
		if (node.type() == scenegraph::SceneGraphNodeType::Root) {
			Log::debug("Add root node");
		} else {
			Log::debug("Add group node");
		}
		const bool addLayers = core::Var::getSafe(cfg::VoxformatVOXCreateLayers)->boolVal();
		if (node.type() == scenegraph::SceneGraphNodeType::Root || addLayers) {
			// TODO: only add the layer if there are models in this group?
			// https://github.com/mgerhardy/vengi/issues/186
			ogt_vox_layer ogt_layer;
			core_memset(&ogt_layer, 0, sizeof(ogt_layer));
			ogt_layer.name = node.name().c_str();
			ogt_layer.hidden = !node.visible();
			const core::RGBA layerRGBA = node.color();
			ogt_layer.color.r = layerRGBA.r;
			ogt_layer.color.g = layerRGBA.g;
			ogt_layer.color.b = layerRGBA.b;
			ogt_layer.color.a = layerRGBA.a;
			ctx.layers.push_back(ogt_layer);
		}
		const uint32_t ownLayerId = (int)ctx.layers.size() - 1;
		const bool addGroups = core::Var::getSafe(cfg::VoxformatVOXCreateGroups)->boolVal();
		if (node.type() == scenegraph::SceneGraphNodeType::Root || addGroups) {
			ogt_vox_group ogt_group;
			core_memset(&ogt_group, 0, sizeof(ogt_group));
			ogt_group.hidden = !node.visible();
			ogt_group.name = node.name().c_str();
			ogt_group.layer_index = ownLayerId;
			ogt_group.parent_group_index = parentGroupIdx;
			ogt_group.transform = ogt_identity_transform;
			checkRotation(ogt_group.transform);
			ctx.groups.push_back(ogt_group);
		}
		const uint32_t ownGroupId = (int)ctx.groups.size() - 1;
		for (int childId : node.children()) {
			saveNode(sceneGraph, sceneGraph.node(childId), ctx, ownGroupId, ownLayerId, palette, replacement);
		}
	} else if (node.type() == scenegraph::SceneGraphNodeType::Camera) {
		Log::debug("Add camera node");
		const scenegraph::SceneGraphNodeCamera &camera = toCameraNode(node);
		const scenegraph::SceneGraphTransform &transform = camera.transform(0);
		{
			ogt_vox_cam ogt_cam;
			core_memset(&ogt_cam, 0, sizeof(ogt_cam));
			ogt_cam.camera_id = ctx.cameras.size();
			const glm::vec3 &euler = glm::eulerAngles(transform.worldOrientation());
			ogt_cam.angle[0] = euler[0];
			ogt_cam.angle[1] = euler[2];
			ogt_cam.angle[2] = euler[1];
			const glm::vec3 &pos = transform.worldTranslation();
			ogt_cam.focus[0] = pos[0];
			ogt_cam.focus[1] = pos[2];
			ogt_cam.focus[2] = pos[1];
			ogt_cam.mode = camera.isPerspective() ? ogt_cam_mode_perspective : ogt_cam_mode_orthographic;
			ogt_cam.radius = (int)camera.farPlane();
			ogt_cam.fov = camera.fieldOfView();
			ogt_cam.frustum = camera.propertyf("frustum"); // TODO:
			ctx.cameras.push_back(ogt_cam);
		}
		for (int childId : node.children()) {
			saveNode(sceneGraph, sceneGraph.node(childId), ctx, parentGroupIdx, layerIdx, palette, replacement);
		}
	} else if (node.type() == scenegraph::SceneGraphNodeType::Model) {
		Log::debug("Add model node");
		const voxel::Region region = node.region();
		const voxel::Palette &nodePalette = node.palette();
		{
			ogt_vox_model ogt_model;
			core_memset(&ogt_model, 0, sizeof(ogt_model));
			// flip y and z here
			ogt_model.size_x = region.getWidthInVoxels();
			ogt_model.size_y = region.getDepthInVoxels();
			ogt_model.size_z = region.getHeightInVoxels();
			const int voxelSize = (int)(ogt_model.size_x * ogt_model.size_y * ogt_model.size_z);
			uint8_t *dataptr = (uint8_t *)core_malloc(voxelSize);
			ogt_model.voxel_data = dataptr;
			voxelutil::visitVolume(
				*node.volume(),
				[&](int, int, int, const voxel::Voxel &voxel) {
					const core::RGBA rgba = nodePalette.color(voxel.getColor());
					if (rgba.a == 0 || isAir(voxel.getMaterial())) {
						*dataptr++ = 0;
					} else {
						const uint8_t palIndex = palette.getClosestMatch(rgba, nullptr, 0);
						if (palIndex == 0u && !ctx.paletteErrorPrinted) {
							Log::debug("palette index %u: %s mapped to %s", voxel.getColor(),
									   core::Color::print(rgba).c_str(), core::Color::print(palette.color(0)).c_str());
							Log::error("Could not find a valid color for %u", voxel.getColor());
							ctx.paletteErrorPrinted = true;
						}
						*dataptr++ = palIndex;
					}
				},
				voxelutil::VisitAll(), voxelutil::VisitorOrder::YZmX);

			ctx.models.push_back(ogt_model);
		}
		saveInstance(sceneGraph, node, ctx, parentGroupIdx, layerIdx, ctx.models.size() - 1);
		for (int childId : node.children()) {
			saveNode(sceneGraph, sceneGraph.node(childId), ctx, parentGroupIdx, layerIdx, palette, replacement);
		}
	} else if (node.type() == scenegraph::SceneGraphNodeType::ModelReference) {
		auto iter = ctx.nodeToModel.find(node.reference());
		if (iter == ctx.nodeToModel.end()) {
			Log::error("Could not find model reference for node %i (references: %i)", node.id(), node.reference());
		} else {
			saveInstance(sceneGraph, node, ctx, parentGroupIdx, layerIdx, iter->second);
		}
		for (int childId : node.children()) {
			saveNode(sceneGraph, sceneGraph.node(childId), ctx, parentGroupIdx, layerIdx, palette, replacement);
		}
	} else {
		Log::error("Unhandled node type %i", (int)node.type());
	}
}

bool VoxFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   io::SeekableWriteStream &stream, const SaveContext &savectx) {
	voxel::Palette palette = sceneGraph.mergePalettes(true, 0);
	if (palette.colorCount() <= 0) {
		Log::error("Could not find any colors in the merged palette");
		return false;
	}
	int palReplacement = findClosestPaletteIndex(palette);

	core_assert(palReplacement != 0u);
	Log::debug("Found closest palette slot %i as replacement", palReplacement);

	MVSceneContext ctx;
	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	saveNode(sceneGraph, sceneGraph.node(root.id()), ctx, k_invalid_group_index, 0, palette, palReplacement);

	core::Buffer<const ogt_vox_model *> modelPtr;
	modelPtr.reserve(ctx.models.size());
	for (const ogt_vox_model &mdl : ctx.models) {
		modelPtr.push_back(&mdl);
	}
	const ogt_vox_model **modelsPtr = modelPtr.data();

	ogt_vox_scene output_scene;
	core_memset(&output_scene, 0, sizeof(output_scene));
	output_scene.num_groups = ctx.groups.size();
	if (output_scene.num_groups > 0) {
		output_scene.groups = &ctx.groups[0];
	}
	output_scene.num_instances = ctx.instances.size();
	if (output_scene.num_instances > 0) {
		output_scene.instances = &ctx.instances[0];
	}
	output_scene.num_layers = ctx.layers.size();
	if (output_scene.num_layers > 0) {
		output_scene.layers = &ctx.layers[0];
	}
	output_scene.num_models = modelPtr.size();
	output_scene.models = modelsPtr;
	core_memset(&output_scene.materials, 0, sizeof(output_scene.materials));
	output_scene.num_cameras = ctx.cameras.size();
	if (output_scene.num_cameras > 0) {
		output_scene.cameras = &ctx.cameras[0];
	}

	ogt_vox_palette &pal = output_scene.palette;
	ogt_vox_matl_array &mat = output_scene.materials;

	core_assert(palette.colorCount() > 0);
	Log::debug("vox save color count: %i (including first transparent slot)", palette.colorCount());
	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::RGBA &rgba = palette.color(i);
		pal.color[i].r = rgba.r;
		pal.color[i].g = rgba.g;
		pal.color[i].b = rgba.b;
		pal.color[i].a = rgba.a;

		const core::RGBA &glowColor = palette.glowColor(i);
		if (glowColor.rgba != 0) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_emit;
			mat.matl[i].type = ogt_matl_type::ogt_matl_type_emit;
			mat.matl[i].emit = 1.0f;
		}

		if (pal.color[i].a < 255) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_alpha;
			mat.matl[i].alpha = (float)pal.color[i].a / 255.0f;
		}
	}

	uint32_t buffersize = 0;
	uint8_t *buffer = ogt_vox_write_scene(&output_scene, &buffersize);
	if (!buffer) {
		Log::error("Failed to write the scene");
		return false;
	}
	if (stream.write(buffer, buffersize) == -1) {
		Log::error("Failed to write to the stream");
		return false;
	}
	ogt_vox_free(buffer);

	for (ogt_vox_model &m : ctx.models) {
		core_free((void *)m.voxel_data);
	}

	return true;
}

} // namespace voxelformat
