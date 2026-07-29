/**
 * @file
 */

#include "VoxFormat.h"
#include "color/ColorUtil.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"
#include "voxelformat/external/ogt_vox.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/gtc/quaternion.hpp>
#include "MagicaVoxel.h"
#include "palette/Palette.h"

namespace voxelformat {

static const uint8_t EMPTY_PALETTE = 0;

VoxFormat::VoxFormat() {
	ogt_vox_set_memory_allocator(_ogt_alloc, _ogt_free);
}

glm::ivec3 VoxFormat::maxSize() const {
	return glm::ivec3(256);
}

int VoxFormat::emptyPaletteIndex() const {
	return EMPTY_PALETTE;
}

size_t VoxFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return 0;
	}
	const size_t size = stream->size();
	uint8_t *buffer = (uint8_t *)core_malloc(size);
	if (stream->read(buffer, size) == -1) {
		core_free(buffer);
		return 0;
	}
	loadPaletteFromBuffer(buffer, size, palette);
	core_free(buffer);
	return palette.colorCount();
}

static void applyInstanceMetadata(scenegraph::SceneGraphNode &node, const ogt_vox_scene *scene,
								  const ogt_vox_instance &ogtInstance) {
	node.setColor(instanceColor(scene, ogtInstance));
	if (ogtInstance.layer_index < scene->num_layers) {
		const ogt_vox_layer &ogtLayer = scene->layers[ogtInstance.layer_index];
		if (ogtLayer.name != nullptr) {
			node.setProperty("layer", ogtLayer.name);
		}
	}
	node.setProperty("layerId", core::string::toString(ogtInstance.layer_index));
	node.setName(instanceName(scene, ogtInstance));
	node.setVisible(!instanceHidden(scene, ogtInstance));
}

bool VoxFormat::loadInstance(const ogt_vox_scene *scene, uint32_t ogt_instanceIdx, scenegraph::SceneGraph &sceneGraph,
							 int parent, core::DynamicArray<MVModelToNode> &models, const palette::Palette &palette) {
	const ogt_vox_instance &ogtInstance = scene->instances[ogt_instanceIdx];
	if (ogtInstance.model_index >= scene->num_models) {
		Log::error("Invalid model index %u for instance %u", ogtInstance.model_index, ogt_instanceIdx);
		return false;
	}
	const ogt_vox_model *ogtModel = scene->models[ogtInstance.model_index];
	if (ogtModel == nullptr) {
		Log::warn("Skipping instance %u with null model", ogt_instanceIdx);
		return true;
	}

	const bool applyTransform = core::getVar(cfg::VoxformatMVApplyTransform)->boolVal();
	const bool animAsNodes = core::getVar(cfg::VoxformatVOXAnimAsNodes)->boolVal();
	if (animAsNodes && ogtInstance.model_anim.num_keyframes > 0) {
		const char *name = instanceName(scene, ogtInstance);
		const color::RGBA color = instanceColor(scene, ogtInstance);
		const bool hidden = instanceHidden(scene, ogtInstance);

		scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
		groupNode.setName(name);
		groupNode.setVisible(!hidden);
		groupNode.setColor(color);
		if (!applyTransform) {
			loadInstanceKeyFrames(groupNode, ogtInstance, scene);
		}
		const int groupId = sceneGraph.emplace(core::move(groupNode), parent);
		if (groupId == InvalidNodeId) {
			return false;
		}

		for (uint32_t k = 0; k < ogtInstance.model_anim.num_keyframes; ++k) {
			const ogt_vox_keyframe_model &kfModel = ogtInstance.model_anim.keyframes[k];
			if (kfModel.model_index >= scene->num_models) {
				continue;
			}
			const ogt_vox_model *frameModel = scene->models[kfModel.model_index];
			if (frameModel == nullptr) {
				continue;
			}

			if (applyTransform) {
				const glm::mat4 bakeMat = ogtInstanceBakeMatrix(ogtInstance, kfModel.frame_index, scene, frameModel);
				glm::ivec3 shift(0);
				voxel::RawVolume *fv = bakeOgtModel(frameModel, bakeMat, palette, shift);
				cropOnLoad(fv);
				scenegraph::SceneGraphNode frameNode(scenegraph::SceneGraphNodeType::Model);
				scenegraph::SceneGraphTransform fTransform;
				fTransform.setWorldTranslation(glm::vec3(shift));
				frameNode.setTransform(0, fTransform);
				frameNode.setPivot(glm::vec3(0.0f));
				frameNode.setName(core::String::format("%s_frame_%u", name, kfModel.frame_index));
				frameNode.setVisible(!hidden);
				frameNode.setColor(color);
				frameNode.setVolume(fv);
				frameNode.setPalette(palette);
				if (sceneGraph.emplace(core::move(frameNode), groupId) == InvalidNodeId) {
					return false;
				}
				continue;
			}

			MVModelToNode &modelEntry = models[kfModel.model_index];
			scenegraph::SceneGraphNodeType type = scenegraph::SceneGraphNodeType::Model;
			if (modelEntry.nodeId != InvalidNodeId) {
				type = scenegraph::SceneGraphNodeType::ModelReference;
			} else if (modelEntry.volume == nullptr) {
				continue;
			}
			scenegraph::SceneGraphNode frameNode(type);
			frameNode.setName(core::String::format("%s_frame_%u", name, kfModel.frame_index));
			frameNode.setVisible(!hidden);
			frameNode.setColor(color);
			frameNode.setPalette(palette);
			frameNode.setPivot(ogtNormalizedPivot(frameModel));
			if (type == scenegraph::SceneGraphNodeType::ModelReference) {
				if (!sceneGraph.hasNode(modelEntry.nodeId) ||
					!frameNode.setReference(sceneGraph.node(modelEntry.nodeId))) {
					Log::error("Failed to reference model node %i for frame instance", modelEntry.nodeId);
					return false;
				}
			} else {
				frameNode.setVolume(modelEntry.volume);
				modelEntry.volume = nullptr;
			}
			const int nodeId = sceneGraph.emplace(core::move(frameNode), groupId);
			if (nodeId == InvalidNodeId) {
				return false;
			}
			if (type == scenegraph::SceneGraphNodeType::Model) {
				modelEntry.nodeId = nodeId;
			}
		}
		return true;
	}

	if (applyTransform) {
		const glm::mat4 bakeMat = ogtInstanceBakeMatrix(ogtInstance, 0, scene, ogtModel);
		glm::ivec3 shift(0);
		voxel::RawVolume *v = bakeOgtModel(ogtModel, bakeMat, palette, shift);
		cropOnLoad(v);

		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		scenegraph::SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3(shift));
		node.setTransform(0, transform);
		node.setPivot(glm::vec3(0.0f));
		applyInstanceMetadata(node, scene, ogtInstance);
		node.setVolume(v);
		node.setPalette(palette);
		return sceneGraph.emplace(core::move(node), parent) != InvalidNodeId;
	}

	MVModelToNode &modelEntry = models[ogtInstance.model_index];
	scenegraph::SceneGraphNodeType type = scenegraph::SceneGraphNodeType::Model;
	if (modelEntry.nodeId != InvalidNodeId) {
		type = scenegraph::SceneGraphNodeType::ModelReference;
	} else if (modelEntry.volume == nullptr) {
		Log::warn("No volume for model index %u", ogtInstance.model_index);
		return true;
	}

	scenegraph::SceneGraphNode node(type);
	loadInstanceKeyFrames(node, ogtInstance, scene);
	applyInstanceMetadata(node, scene, ogtInstance);
	node.setPivot(ogtNormalizedPivot(ogtModel));
	node.setPalette(palette);
	if (type == scenegraph::SceneGraphNodeType::ModelReference) {
		if (!sceneGraph.hasNode(modelEntry.nodeId) || !node.setReference(sceneGraph.node(modelEntry.nodeId))) {
			Log::error("Failed to reference model node %i for instance", modelEntry.nodeId);
			return false;
		}
	} else {
		node.setVolume(modelEntry.volume);
		modelEntry.volume = nullptr;
	}
	const int nodeId = sceneGraph.emplace(core::move(node), parent);
	if (nodeId == InvalidNodeId) {
		return false;
	}
	if (type == scenegraph::SceneGraphNodeType::Model) {
		modelEntry.nodeId = nodeId;
	}
	return true;
}

bool VoxFormat::loadGroup(const ogt_vox_scene *scene, uint32_t ogt_groupIdx, scenegraph::SceneGraph &sceneGraph,
						  int parent, core::DynamicArray<MVModelToNode> &models, core::Set<uint32_t> &addedInstances,
						  const palette::Palette &palette) {
	const ogt_vox_group &ogt_group = scene->groups[ogt_groupIdx];
	bool hidden = ogt_group.hidden;
	const char *name = ogt_group.name ? ogt_group.name : "<group>";
	const uint32_t layerIdx = ogt_group.layer_index;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
	if (layerIdx < scene->num_layers) {
		const ogt_vox_layer &layer = scene->layers[layerIdx];
		hidden |= layer.hidden;
		if (layer.name != nullptr) {
			node.setProperty("layer", layer.name);
		}
		node.setProperty("layerId", core::string::toString(layerIdx));
		const color::RGBA color(layer.color.r, layer.color.g, layer.color.b, layer.color.a);
		node.setColor(color);
	}
	node.setName(name);
	node.setVisible(!hidden);
	loadGroupKeyFrames(node, ogt_group.transform_anim);

	int groupId;
	if (parent == InvalidNodeId) {
		// MagicaVoxel root group maps onto the existing scenegraph root.
		groupId = sceneGraph.root().id();
		scenegraph::SceneGraphNode &root = sceneGraph.node(groupId);
		if (ogt_group.name != nullptr) {
			root.setName(name);
		}
		root.setVisible(!hidden);
		root.setColor(node.color());
		const core::String layerName = node.property("layer");
		if (!layerName.empty()) {
			root.setProperty("layer", layerName);
		}
		const core::String layerId = node.property("layerId");
		if (!layerId.empty()) {
			root.setProperty("layerId", layerId);
		}
		loadGroupKeyFrames(root, ogt_group.transform_anim);
	} else {
		groupId = sceneGraph.emplace(core::move(node), parent);
	}
	if (groupId == InvalidNodeId) {
		Log::error("Failed to add group node to the scene graph");
		return false;
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

	return true;
}

bool VoxFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
								  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const size_t size = stream->size();
	uint8_t *buffer = (uint8_t *)core_malloc(size);
	if (stream->read(buffer, size) == -1) {
		core_free(buffer);
		return false;
	}
	const uint32_t ogt_vox_flags = k_read_scene_flags_keyframes | k_read_scene_flags_groups |
								   k_read_scene_flags_keep_empty_models_instances |
								   k_read_scene_flags_keep_duplicate_models;
	const ogt_vox_scene *scene = ogt_vox_read_scene_with_flags(buffer, (uint32_t)size, ogt_vox_flags);
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
		node.setName(core::string::extractFilename(filename));
		node.createVolume(voxel::Region(0, 31));
		node.setPalette(palette);
		return sceneGraph.emplace(core::move(node), 0) != InvalidNodeId;
	}
	return true;
}

bool VoxFormat::loadScene(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph,
						  const palette::Palette &palette) {
	const bool applyTransform = core::getVar(cfg::VoxformatMVApplyTransform)->boolVal();
	core::DynamicArray<MVModelToNode> models;
	// Shared model volumes are only needed when transforms stay on nodes (references).
	// Bake path reads voxels from ogt models per instance. Still load when there are no
	// instances so orphan models can be imported.
	if (!applyTransform || scene->num_instances == 0) {
		models = loadModels(scene, palette);
	} else {
		models.resize(scene->num_models);
	}
	core::Set<uint32_t> addedInstances;
	for (uint32_t i = 0; i < scene->num_groups; ++i) {
		const ogt_vox_group &group = scene->groups[i];
		if (group.parent_group_index != k_invalid_group_index) {
			continue;
		}
		Log::debug("Add root group %u/%u", i, scene->num_groups);
		if (!loadGroup(scene, i, sceneGraph, InvalidNodeId, models, addedInstances, palette)) {
			return false;
		}
	}
	for (uint32_t n = 0; n < scene->num_instances; ++n) {
		if (addedInstances.has(n)) {
			continue;
		}
		Log::debug("Instance %u is not part of a group - attaching under root", n);
		if (!loadInstance(scene, n, sceneGraph, sceneGraph.root().id(), models, palette)) {
			return false;
		}
	}
	if (scene->num_instances == 0 && scene->num_models > 0) {
		for (MVModelToNode &m : models) {
			if (m.volume == nullptr) {
				continue;
			}
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setVolume(m.volume);
			node.setPalette(palette);
			sceneGraph.emplace(core::move(node), sceneGraph.root().id());
			m.volume = nullptr;
		}
	}

	loadCameras(scene, sceneGraph);
	loadSun(scene, sceneGraph);
	return true;
}

void VoxFormat::saveInstance(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
							 MVSceneContext &ctx, uint32_t parentGroupIdx, uint32_t layerIdx, uint32_t modelIdx) {
	ctx.nodeToModel.put(node.id(), modelIdx);
	ogt_vox_instance ogt_instance;
	core_memset(&ogt_instance, 0, sizeof(ogt_instance));
	ogt_instance.group_index = parentGroupIdx;
	ogt_instance.model_index = modelIdx;
	ogt_instance.layer_index = layerIdx;
	ogt_instance.name = node.name().c_str();
	ogt_instance.hidden = !node.visible();
	uint32_t numKeyframes = 0;
	ogt_instance.transform_anim.keyframes = saveKeyFrames(sceneGraph, node, ctx, numKeyframes);
	ogt_instance.transform_anim.num_keyframes = numKeyframes;
	ctx.instances.push_back(ogt_instance);
}

uint32_t VoxFormat::saveModel(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
							  MVSceneContext &ctx) {
	const voxel::Region region = sceneGraph.resolveRegion(node);
	ogt_vox_model ogt_model;
	core_memset(&ogt_model, 0, sizeof(ogt_model));
	ogt_model.size_x = region.getWidthInVoxels();
	ogt_model.size_y = region.getDepthInVoxels();
	ogt_model.size_z = region.getHeightInVoxels();
	const int voxelSize = (int)(ogt_model.size_x * ogt_model.size_y * ogt_model.size_z);
	uint8_t *dataptr = (uint8_t *)core_malloc(voxelSize);
	ogt_model.voxel_data = dataptr;
	auto func = [&](int, int, int, const voxel::Voxel &voxel) { *dataptr++ = voxel.getColor(); };
	voxelutil::visitVolume(*sceneGraph.resolveVolume(node), func, voxelutil::VisitAll(), voxelutil::VisitorOrder::YZmX);
	ctx.models.push_back(ogt_model);
	return (uint32_t)(ctx.models.size() - 1);
}

void VoxFormat::saveNode(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
						 MVSceneContext &ctx, uint32_t parentGroupIdx, uint32_t layerIdx) {
	Log::debug("Save node '%s' with parent group %u and layer %u", node.name().c_str(), parentGroupIdx, layerIdx);
	if (node.isRootNode() || node.isGroupNode()) {
		const bool animAsNodes = core::getVar(cfg::VoxformatVOXAnimAsNodes)->boolVal();
		if (animAsNodes && node.isGroupNode() && !node.children().empty()) {
			bool allModels = true;
			for (const core::UUID &childUUID : node.children()) {
				const scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(childUUID);
				if (child == nullptr || !child->isModelNode()) {
					allModels = false;
					break;
				}
			}
			if (allModels) {
				const uint32_t modelKeyFrameStart = (uint32_t)ctx.keyframeModels.size();
				uint32_t firstModelIdx = 0;
				bool first = true;
				for (const core::UUID &childUUID : node.children()) {
					scenegraph::SceneGraphNode *child = const_cast<scenegraph::SceneGraphNode *>(sceneGraph.findNodeByUUID(childUUID));
					if (child == nullptr) {
						continue;
					}
					const uint32_t modelIdx = saveModel(sceneGraph, *child, ctx);
					if (first) {
						firstModelIdx = modelIdx;
						first = false;
					}

					uint32_t frameIndex = (uint32_t)(ctx.keyframeModels.size() - modelKeyFrameStart);
					const core::String &childName = child->name();
					const size_t framePos = childName.rfind("_frame_");
					if (framePos != core::String::npos) {
						frameIndex = core::string::toInt(childName.substr(framePos + 7));
					}

					ogt_vox_keyframe_model kfModel;
					kfModel.model_index = modelIdx;
					kfModel.frame_index = frameIndex;
					ctx.keyframeModels.push_back(kfModel);
				}

				const uint32_t numModelKeyframes = (uint32_t)(ctx.keyframeModels.size() - modelKeyFrameStart);

				ogt_vox_instance ogt_instance;
				core_memset(&ogt_instance, 0, sizeof(ogt_instance));
				ogt_instance.group_index = parentGroupIdx;
				ogt_instance.model_index = firstModelIdx;
				ogt_instance.layer_index = layerIdx;
				ogt_instance.name = node.name().c_str();
				ogt_instance.hidden = !node.visible();
				ogt_instance.model_anim.num_keyframes = numModelKeyframes;
				ogt_instance.model_anim.keyframes = &ctx.keyframeModels[modelKeyFrameStart];

				uint32_t numKeyframes = 0;
				ogt_instance.transform_anim.keyframes = saveKeyFrames(sceneGraph, node, ctx, numKeyframes);
				ogt_instance.transform_anim.num_keyframes = numKeyframes;
				ctx.instances.push_back(ogt_instance);
				return;
			}
		}

		const bool addLayers = core::getVar(cfg::VoxformatVOXCreateLayers)->boolVal();
		uint32_t ownLayerId = layerIdx;
		if (node.isRootNode() || addLayers) {
			ogt_vox_layer ogt_layer;
			core_memset(&ogt_layer, 0, sizeof(ogt_layer));
			ogt_layer.name = node.name().c_str();
			ogt_layer.hidden = !node.visible();
			const color::RGBA layerRGBA = node.color();
			ogt_layer.color.r = layerRGBA.r;
			ogt_layer.color.g = layerRGBA.g;
			ogt_layer.color.b = layerRGBA.b;
			ogt_layer.color.a = layerRGBA.a;
			ctx.layers.push_back(ogt_layer);
			ownLayerId = (uint32_t)(ctx.layers.size() - 1);
		} else {
			const core::String layerIdProp = node.property("layerId");
			if (!layerIdProp.empty()) {
				ownLayerId = (uint32_t)core::string::toInt(layerIdProp);
			}
		}

		const bool addGroups = core::getVar(cfg::VoxformatVOXCreateGroups)->boolVal();
		uint32_t ownGroupId = parentGroupIdx;
		if (node.isRootNode() || addGroups) {
			ogt_vox_group ogt_group;
			core_memset(&ogt_group, 0, sizeof(ogt_group));
			ogt_group.hidden = !node.visible();
			ogt_group.name = node.name().c_str();
			ogt_group.layer_index = ownLayerId;
			ogt_group.parent_group_index = parentGroupIdx;
			// Groups stay identity on load (instance transforms are globals). Writing a converted
			// identity through vengiMatToOgt injects a bogus translation and breaks round-trips.
			ogt_group.transform = ogt_identity_transform;
			ogt_group.transform_anim.num_keyframes = 0;
			ogt_group.transform_anim.keyframes = nullptr;
			checkRotation(ogt_group.transform);
			ctx.groups.push_back(ogt_group);
			ownGroupId = (uint32_t)(ctx.groups.size() - 1);
		}
		for (const core::UUID &childUUID : node.children()) {
			scenegraph::SceneGraphNode *child = const_cast<scenegraph::SceneGraphNode *>(sceneGraph.findNodeByUUID(childUUID));
			if (child != nullptr) {
				saveNode(sceneGraph, *child, ctx, ownGroupId, ownLayerId);
			}
		}
	} else if (node.isCameraNode()) {
		Log::debug("Add camera node");
		const scenegraph::SceneGraphNodeCamera &camera = scenegraph::toCameraNode(node);
		const scenegraph::SceneGraphTransform &transform = camera.transform(0);
		{
			ogt_vox_cam ogt_cam;
			core_memset(&ogt_cam, 0, sizeof(ogt_cam));
			ogt_cam.camera_id = (uint32_t)ctx.cameras.size();
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
			ogt_cam.frustum = camera.propertyf(scenegraph::PropCamFrustum);
			ctx.cameras.push_back(ogt_cam);
		}
		for (const core::UUID &childUUID : node.children()) {
			scenegraph::SceneGraphNode *child = const_cast<scenegraph::SceneGraphNode *>(sceneGraph.findNodeByUUID(childUUID));
			if (child != nullptr) {
				saveNode(sceneGraph, *child, ctx, parentGroupIdx, layerIdx);
			}
		}
	} else if (node.isModelNode()) {
		Log::debug("Add model node");
		const uint32_t modelIdx = saveModel(sceneGraph, node, ctx);
		saveInstance(sceneGraph, node, ctx, parentGroupIdx, layerIdx, modelIdx);
		for (const core::UUID &childUUID : node.children()) {
			scenegraph::SceneGraphNode *child = const_cast<scenegraph::SceneGraphNode *>(sceneGraph.findNodeByUUID(childUUID));
			if (child != nullptr) {
				saveNode(sceneGraph, *child, ctx, parentGroupIdx, layerIdx);
			}
		}
	} else if (node.isReferenceNode()) {
		const scenegraph::SceneGraphNode *referencedNode = sceneGraph.findNodeByUUID(node.referenceUUID());
		auto iter = referencedNode != nullptr ? ctx.nodeToModel.find(referencedNode->id()) : ctx.nodeToModel.end();
		if (iter == ctx.nodeToModel.end()) {
			Log::error("Could not find model reference for node %i (references: %s)", node.id(),
					   node.referenceUUID().str().c_str());
		} else {
			saveInstance(sceneGraph, node, ctx, parentGroupIdx, layerIdx, iter->second);
		}
		for (const core::UUID &childUUID : node.children()) {
			scenegraph::SceneGraphNode *child = const_cast<scenegraph::SceneGraphNode *>(sceneGraph.findNodeByUUID(childUUID));
			if (child != nullptr) {
				saveNode(sceneGraph, *child, ctx, parentGroupIdx, layerIdx);
			}
		}
	} else {
		Log::error("Unhandled node type %i", (int)node.type());
	}
}

bool VoxFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &savectx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	MVSceneContext ctx;
	// Instances/groups keep pointers into these buffers - reserve so push_back never reallocates.
	size_t transformKeyFrames = 0;
	size_t modelKeyFrames = 0;
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		transformKeyFrames += node.keyFrames(sceneGraph.activeAnimation()).size();
		if (node.isModelNode()) {
			++modelKeyFrames;
		}
	}
	ctx.keyframeTransforms.reserve(transformKeyFrames + 1);
	ctx.keyframeModels.reserve(modelKeyFrames + 1);
	ctx.instances.reserve(sceneGraph.nodes().size());
	ctx.groups.reserve(sceneGraph.nodes().size());
	ctx.models.reserve(modelKeyFrames + 1);

	// worldMatrix() is required for flattened instance transforms on save.
	const_cast<scenegraph::SceneGraph &>(sceneGraph).updateTransforms();

	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	saveNode(sceneGraph, sceneGraph.node(root.id()), ctx, k_invalid_group_index, 0);

	core::Buffer<const ogt_vox_model *> modelPtr;
	modelPtr.reserve(ctx.models.size());
	for (const ogt_vox_model &mdl : ctx.models) {
		modelPtr.push_back(&mdl);
	}
	const ogt_vox_model **modelsPtr = modelPtr.data();

	ogt_vox_scene output_scene;
	core_memset(&output_scene, 0, sizeof(output_scene));
	output_scene.num_groups = (uint32_t)ctx.groups.size();
	if (output_scene.num_groups > 0) {
		output_scene.groups = &ctx.groups[0];
	}
	output_scene.num_instances = (uint32_t)ctx.instances.size();
	if (output_scene.num_instances > 0) {
		output_scene.instances = &ctx.instances[0];
	}
	output_scene.num_layers = (uint32_t)ctx.layers.size();
	if (output_scene.num_layers > 0) {
		output_scene.layers = &ctx.layers[0];
	}
	output_scene.num_models = (uint32_t)modelPtr.size();
	output_scene.models = modelsPtr;
	core_memset(&output_scene.materials, 0, sizeof(output_scene.materials));
	output_scene.num_cameras = (uint32_t)ctx.cameras.size();
	if (output_scene.num_cameras > 0) {
		output_scene.cameras = &ctx.cameras[0];
	}

	ogt_vox_palette &pal = output_scene.palette;
	ogt_vox_matl_array &mat = output_scene.materials;

	const palette::Palette &palette = sceneGraph.firstPalette();
	output_scene.num_color_names = palette.colorCount();
	core::Buffer<const char *> colorNamesPtr;
	colorNamesPtr.resize(output_scene.num_color_names);
	output_scene.color_names = &colorNamesPtr[0];

	Log::debug("vox save color count: %i (including first transparent slot)", palette.colorCount());
	for (int i = 0; i < palette.colorCount(); ++i) {
		output_scene.color_names[i] = palette.colorName(i).c_str();

		const color::RGBA &rgba = palette.color(i);
		pal.color[i].r = rgba.r;
		pal.color[i].g = rgba.g;
		pal.color[i].b = rgba.b;
		pal.color[i].a = rgba.a;

		const palette::Material &material = palette.material(i);
		palette::MaterialType type = material.type;
		if (type == palette::MaterialType::Diffuse) {
			mat.matl[i].type = ogt_matl_type_diffuse;
		} else if (type == palette::MaterialType::Metal) {
			mat.matl[i].type = ogt_matl_type_metal;
		} else if (type == palette::MaterialType::Glass) {
			mat.matl[i].type = ogt_matl_type_glass;
		} else if (type == palette::MaterialType::Emit) {
			mat.matl[i].type = ogt_matl_type_emit;
		} else if (type == palette::MaterialType::Blend) {
			mat.matl[i].type = ogt_matl_type_blend;
		} else if (type == palette::MaterialType::Media) {
			mat.matl[i].type = ogt_matl_type_media;
		} else {
			Log::error("Unknown material type %i", (int)type);
			mat.matl[i].type = ogt_matl_type_diffuse;
		}

		if (material.has(palette::MaterialProperty::MaterialMetal)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_metal;
			mat.matl[i].metal = material.value(palette::MaterialMetal);
		}
		if (material.has(palette::MaterialProperty::MaterialRoughness)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_rough;
			mat.matl[i].rough = material.value(palette::MaterialRoughness);
		}
		if (material.has(palette::MaterialProperty::MaterialSpecular)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_spec;
			mat.matl[i].spec = material.value(palette::MaterialSpecular);
		}
		if (material.has(palette::MaterialProperty::MaterialIndexOfRefraction)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_ior;
			mat.matl[i].ior = material.value(palette::MaterialIndexOfRefraction);
		}
		if (material.has(palette::MaterialProperty::MaterialAttenuation)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_att;
			mat.matl[i].att = material.value(palette::MaterialAttenuation);
		}
		if (material.has(palette::MaterialProperty::MaterialFlux)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_flux;
			mat.matl[i].flux = material.value(palette::MaterialFlux);
		}
		if (material.has(palette::MaterialProperty::MaterialEmit)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_emit;
			mat.matl[i].emit = material.value(palette::MaterialEmit);
		}
		if (material.has(palette::MaterialProperty::MaterialLowDynamicRange)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_ldr;
			mat.matl[i].ldr = material.value(palette::MaterialLowDynamicRange);
		}
		if (pal.color[i].a < 255) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_alpha;
			mat.matl[i].alpha = (float)pal.color[i].a / 255.0f;
			pal.color[i].a = 255;
		}
		if (material.has(palette::MaterialProperty::MaterialDensity)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_d;
			mat.matl[i].d = material.value(palette::MaterialDensity);
		}
		if (material.has(palette::MaterialProperty::MaterialSp)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_sp;
			mat.matl[i].sp = material.value(palette::MaterialSp);
		}
		if (material.has(palette::MaterialProperty::MaterialPhase)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_g;
			mat.matl[i].g = material.value(palette::MaterialPhase);
		}
		if (material.has(palette::MaterialProperty::MaterialMedia)) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_media;
			mat.matl[i].media = material.value(palette::MaterialMedia);
		}
	}

	ogt_vox_sun sun;
	core_memset(&sun, 0, sizeof(sun));
	const scenegraph::SceneGraphNode &rootNode = sceneGraph.root();
	const core::String &sunIntensity = rootNode.property(scenegraph::PropSunIntensity);
	if (!sunIntensity.empty()) {
		sun.intensity = core::string::toFloat(sunIntensity);
		sun.area = rootNode.propertyf(scenegraph::PropSunArea);
		sun.angle[0] = rootNode.propertyf(scenegraph::PropSunElevation);
		sun.angle[1] = rootNode.propertyf(scenegraph::PropSunAzimuth);
		const core::String &colorHex = rootNode.property(scenegraph::PropSunColor);
		if (!colorHex.empty()) {
			const color::RGBA c = color::fromHex(colorHex.c_str());
			sun.rgba = {c.r, c.g, c.b, c.a};
		}
		sun.disk = rootNode.property(scenegraph::PropSunDisk) == "true";
		output_scene.sun = &sun;
	}

	uint32_t buffersize = 0;
	uint8_t *buffer = ogt_vox_write_scene(&output_scene, &buffersize);
	if (!buffer) {
		Log::error("Failed to write the scene");
		return false;
	}
	if (stream->write(buffer, buffersize) == -1) {
		Log::error("Failed to write to the stream");
		ogt_vox_free(buffer);
		return false;
	}
	ogt_vox_free(buffer);

	for (ogt_vox_model &m : ctx.models) {
		core_free((void *)m.voxel_data);
	}

	return true;
}

} // namespace voxelformat
