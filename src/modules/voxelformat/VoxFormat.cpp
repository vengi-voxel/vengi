/**
 * @file
 */

#include "VoxFormat.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "math/Math.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VolumeVisitor.h"

#define OGT_VOX_IMPLEMENTATION
#include "external/ogt_vox.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

struct ogt_SceneContext {
	core::Buffer<ogt_vox_group> groups;
	core::Buffer<ogt_vox_model> models;
	core::Buffer<ogt_vox_layer> layers;
	core::Buffer<ogt_vox_instance> instances;
	int transformKeyFrameIdx = 0;
	core::Array<ogt_vox_keyframe_transform, 4096> keyframeTransforms;
	core::Buffer<ogt_vox_cam> cameras;
};

static void *_ogt_alloc(size_t size) {
	return core_malloc(size);
}

static void _ogt_free(void *mem) {
	core_free(mem);
}

static const ogt_vox_transform ogt_identity_transform {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

VoxFormat::VoxFormat() {
	ogt_vox_set_memory_allocator(_ogt_alloc, _ogt_free);
}

size_t VoxFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette) {
	const size_t size = stream.size();
	uint8_t *buffer = (uint8_t *)core_malloc(size);
	if (stream.read(buffer, size) == -1) {
		core_free(buffer);
		return 0;
	}
	const ogt_vox_scene *scene = ogt_vox_read_scene_with_flags(buffer, size, 0);
	core_free(buffer);
	if (scene == nullptr) {
		Log::error("Could not load scene %s", filename.c_str());
		return 0;
	}
	palette.colorCount = 0;
	int palIdx = 0;
	for (int i = 0; i < lengthof(scene->palette.color); ++i) {
		const ogt_vox_rgba color = scene->palette.color[(i + 1) & 255];
		palette.colors[palIdx] = core::RGBA(color.r, color.g, color.b, color.a);
		const ogt_vox_matl &matl = scene->materials.matl[(i + 1) & 255];
		if (matl.type == ogt_matl_type_emit) {
			palette.glowColors[palIdx] = palette.colors[palIdx];
		}
		++palIdx;
	}
	palette.colorCount = palIdx;
	Log::debug("vox load color count: %i", palette.colorCount);
	ogt_vox_destroy_scene(scene);
	return palette.size();
}

/**
 * @brief Calculate the scene graph object transformation. Used for the voxel and the AABB of the volume.
 *
 * @param mat The world space model matrix (rotation and translation) for the chunk
 * @param pos The position inside the untransformed chunk (local position)
 * @param pivot The pivot to do the rotation around. This is the @code chunk_size - 1 + 0.5 @endcode. Please
 * note that the @c w component must be @c 0.0
 * @return glm::vec4 The transformed world position
 */
static inline glm::vec4 calcTransform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec4 &pivot) {
	return glm::floor(mat * (glm::vec4((float)pos.x + 0.5f, (float)pos.y + 0.5f, (float)pos.z + 0.5f, 1.0f) - pivot));
}

static bool loadKeyFrames(voxelformat::SceneGraph &sceneGraph, voxelformat::SceneGraphNode& node, const ogt_vox_keyframe_transform* transformKeyframes, uint32_t numKeyframes) {
	SceneGraphKeyFrames kf;
	Log::debug("Load %d keyframes", numKeyframes);
	kf.reserve(numKeyframes);
	for (uint32_t keyFrameIdx = 0; keyFrameIdx < numKeyframes; ++keyFrameIdx) {
		const ogt_vox_keyframe_transform& transform_keyframe = transformKeyframes[keyFrameIdx];
		const ogt_vox_transform& keyframeTransform = transform_keyframe.transform;
		const glm::vec4 ogtKeyFrameCol0(keyframeTransform.m00, keyframeTransform.m01, keyframeTransform.m02, keyframeTransform.m03);
		const glm::vec4 ogtKeyFrameCol1(keyframeTransform.m10, keyframeTransform.m11, keyframeTransform.m12, keyframeTransform.m13);
		const glm::vec4 ogtKeyFrameCol2(keyframeTransform.m20, keyframeTransform.m21, keyframeTransform.m22, keyframeTransform.m23);
		const glm::vec4 ogtKeyFrameCol3(keyframeTransform.m30, keyframeTransform.m31, keyframeTransform.m32, keyframeTransform.m33);
		const glm::mat4 ogtKeyFrameMat = glm::mat4{ogtKeyFrameCol0, ogtKeyFrameCol1, ogtKeyFrameCol2, ogtKeyFrameCol3};
		SceneGraphKeyFrame sceneGraphKeyFrame;
		sceneGraphKeyFrame.frameIdx = transform_keyframe.frame_index;
		sceneGraphKeyFrame.interpolation = InterpolationType::Linear;
		sceneGraphKeyFrame.longRotation = false;
		sceneGraphKeyFrame.transform().setWorldMatrix(ogtKeyFrameMat);
		kf.push_back(sceneGraphKeyFrame);
	}
	return node.setKeyFrames(kf);
}

bool VoxFormat::addInstance(const ogt_vox_scene *scene, uint32_t ogt_instanceIdx, SceneGraph &sceneGraph, int parent, const glm::mat4 &zUpMat, const voxel::Palette &palette, bool groupHidden) {
	const ogt_vox_instance& ogtInstance = scene->instances[ogt_instanceIdx];
	const ogt_vox_transform& ogtTransform = ogtInstance.transform;
	const glm::vec4 ogtCol0(ogtTransform.m00, ogtTransform.m01, ogtTransform.m02, ogtTransform.m03);
	const glm::vec4 ogtCol1(ogtTransform.m10, ogtTransform.m11, ogtTransform.m12, ogtTransform.m13);
	const glm::vec4 ogtCol2(ogtTransform.m20, ogtTransform.m21, ogtTransform.m22, ogtTransform.m23);
	const glm::vec4 ogtCol3(ogtTransform.m30, ogtTransform.m31, ogtTransform.m32, ogtTransform.m33);
	const glm::mat4 ogtMat = glm::mat4{ogtCol0, ogtCol1, ogtCol2, ogtCol3};
	const ogt_vox_model *ogtModel = scene->models[ogtInstance.model_index];
	const uint8_t *ogtVoxels = ogtModel->voxel_data;
	const uint8_t *ogtVoxel = ogtVoxels;
	const glm::ivec3 maxs(ogtModel->size_x - 1, ogtModel->size_y - 1, ogtModel->size_z - 1);
	const glm::vec4 pivot(glm::floor((float)ogtModel->size_x / 2.0f), glm::floor((float)ogtModel->size_y / 2.0f), glm::floor((float)ogtModel->size_z / 2.0f), 0.0f);
	const glm::ivec3& transformedMins = calcTransform(ogtMat, glm::ivec3(0), pivot);
	const glm::ivec3& transformedMaxs = calcTransform(ogtMat, maxs, pivot);
	const glm::ivec3& zUpMins = calcTransform(zUpMat, transformedMins, glm::ivec4(0));
	const glm::ivec3& zUpMaxs = calcTransform(zUpMat, transformedMaxs, glm::ivec4(0));
	voxel::Region region(glm::min(zUpMins, zUpMaxs), glm::max(zUpMins, zUpMaxs));
	const glm::ivec3 shift = region.getLowerCorner();
	region.shift(-shift);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	SceneGraphTransform transform;
	transform.setWorldTranslation(shift);

	for (uint32_t k = 0; k < ogtModel->size_z; ++k) {
		for (uint32_t j = 0; j < ogtModel->size_y; ++j) {
			for (uint32_t i = 0; i < ogtModel->size_x; ++i, ++ogtVoxel) {
				if (ogtVoxel[0] == 0) {
					continue;
				}
				const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, ogtVoxel[0] - 1);
				const glm::ivec3 &pos = calcTransform(ogtMat, glm::ivec3(i, j, k), pivot);
				const glm::ivec3 &poszUp = calcTransform(zUpMat, pos, glm::ivec4(0));
				const glm::ivec3 &regionPos = poszUp - shift;
				v->setVoxel(regionPos, voxel);
			}
		}
	}

	SceneGraphNode node(SceneGraphNodeType::Model);
	const char *name = ogtInstance.name;
	if (name == nullptr) {
		const ogt_vox_layer &layer = scene->layers[ogtInstance.layer_index];
		name = layer.name;
		core::RGBA col;
		col.r = layer.color.r;
		col.g = layer.color.g;
		col.b = layer.color.b;
		col.a = layer.color.a;
		node.setColor(col);
		if (name == nullptr) {
			name = "";
		}
	}
	loadKeyFrames(sceneGraph, node, ogtInstance.transform_anim.keyframes, ogtInstance.transform_anim.num_keyframes);
	// TODO: we are overriding the keyframe data here
	const KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	node.setName(name);
	node.setVisible(!ogtInstance.hidden && !groupHidden);
	node.setVolume(v, true);
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node), parent) != -1;
}

bool VoxFormat::addGroup(const ogt_vox_scene *scene, uint32_t ogt_groupIdx, SceneGraph &sceneGraph, int parent, const glm::mat4 &zUpMat, core::Set<uint32_t> &addedInstances, const voxel::Palette &palette) {
	// TODO: with each save/load there is one root group more in some situations
	const ogt_vox_group &ogt_group = scene->groups[ogt_groupIdx];
	bool hidden = ogt_group.hidden;
	const char *name = "Group";
	const uint32_t layerIdx = ogt_group.layer_index;
	// TODO: group transforms are not handled correctly
	SceneGraphNode node(SceneGraphNodeType::Group);
	if (layerIdx < scene->num_layers) {
		const ogt_vox_layer &layer = scene->layers[layerIdx];
		hidden |= layer.hidden;
		if (layer.name != nullptr) {
			name = layer.name;
		}
		core::RGBA color;
		color.r = layer.color.r;
		color.g = layer.color.g;
		color.b = layer.color.b;
		color.a = layer.color.a;
		node.setColor(color);
	}
	loadKeyFrames(sceneGraph, node, ogt_group.transform_anim.keyframes, ogt_group.transform_anim.num_keyframes);
	node.setName(name);
	node.setVisible(!hidden);
	const int groupId = sceneGraph.emplace(core::move(node), parent);
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
		if (!addInstance(scene, n, sceneGraph, groupId, zUpMat, palette, hidden)) {
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
		if (!addGroup(scene, groupIdx, sceneGraph, groupId, zUpMat, addedInstances, palette)) {
			return false;
		}
	}

	return true;
}

bool VoxFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, voxel::Palette &palette) {
	const size_t size = stream.size();
	uint8_t *buffer = (uint8_t *)core_malloc(size);
	if (stream.read(buffer, size) == -1) {
		core_free(buffer);
		return false;
	}
	const uint32_t ogt_vox_flags = k_read_scene_flags_groups | k_read_scene_flags_keyframes | k_read_scene_flags_keep_empty_models_instances;
	const ogt_vox_scene *scene = ogt_vox_read_scene_with_flags(buffer, size, ogt_vox_flags);
	core_free(buffer);
	if (scene == nullptr) {
		Log::error("Could not load scene %s", filename.c_str());
		return false;
	}

	palette.colorCount = 0;
	int palIdx = 0;
	for (int i = 0; i < lengthof(scene->palette.color); ++i) {
		const ogt_vox_rgba color = scene->palette.color[(i + 1) & 255];
		palette.colors[palIdx] = core::RGBA(color.r, color.g, color.b, color.a);
		const ogt_vox_matl &matl = scene->materials.matl[(i + 1) & 255];
		if (matl.type == ogt_matl_type_emit) {
			palette.glowColors[palIdx] = palette.colors[palIdx];
		}
		++palIdx;
	}
	palette.colorCount = palIdx;
	Log::debug("vox load color count: %i", palette.colorCount);

	// rotation matrix to convert into our coordinate system (mv has z pointing upwards)
	const glm::mat4 zUpMat{
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};
	// glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	core::Set<uint32_t> addedInstances;

	for (uint32_t i = 0; i < scene->num_groups; ++i) {
		const ogt_vox_group &group = scene->groups[i];
		// find the main group nodes
		if (group.parent_group_index != k_invalid_group_index) {
			continue;
		}
		Log::debug("Add root group %u/%u", i, scene->num_groups);
		if (!addGroup(scene, i, sceneGraph, sceneGraph.root().id(), zUpMat, addedInstances, palette)) {
			ogt_vox_destroy_scene(scene);
			return false;
		}
		break;
	}
	for (uint32_t n = 0; n < scene->num_instances; ++n) {
		if (addedInstances.has(n)) {
			continue;
		}
		// TODO: the parent is wrong
		if (!addInstance(scene, n, sceneGraph, sceneGraph.root().id(), zUpMat, palette)) {
			ogt_vox_destroy_scene(scene);
			return false;
		}
	}

	for (uint32_t n = 0; n < scene->num_cameras; ++n) {
		const ogt_vox_cam& c = scene->cameras[n];
		const glm::vec3 target(c.focus[0], c.focus[1], c.focus[2]);
		const glm::quat quat(glm::vec3(c.angle[0], c.angle[1], c.angle[2]));
		const float distance = (float)c.radius;
		const glm::vec3& forward = glm::conjugate(quat) * glm::forward;
		const glm::vec3& backward = -forward;
		const glm::vec3& newPosition = target + backward * distance;
		const glm::mat4& orientation = glm::mat4_cast(quat);
		const glm::mat4& viewMatrix = glm::translate(orientation, -newPosition);
		// if (c.mode == ogt_cam_mode_perspective) {
		//  const float fieldOfView = glm::radians((float)c.fov);
		// }

		{
			SceneGraphNodeCamera camNode;
			camNode.setName(core::String::format("Camera %u", c.camera_id));
			SceneGraphTransform transform;
			transform.setWorldMatrix(viewMatrix);
			const KeyFrameIndex keyFrameIdx = 0;
			camNode.setTransform(keyFrameIdx, transform);
			camNode.setFieldOfView(c.fov);
			camNode.setFarPlane((float)c.radius);
			camNode.setProperty("frustum", core::String::format("%f", c.frustum)); // TODO:
			if (c.mode == ogt_cam_mode_perspective) {
				camNode.setPerspective();
			} else if (c.mode == ogt_cam_mode_orthographic) {
				camNode.setOrthographic();
			}
			sceneGraph.emplace(core::move(camNode), sceneGraph.root().id());
		}
	}

	ogt_vox_destroy_scene(scene);
	return true;
}

int VoxFormat::findClosestPaletteIndex(const voxel::Palette &palette) {
	// we have to find a replacement for the first palette entry - as this is used
	// as the empty voxel in magicavoxel
	core::DynamicArray<glm::vec4> materialColors;
	palette.toVec4f(materialColors);
	const glm::vec4 first = materialColors[0];
	materialColors.erase(materialColors.begin());
	return core::Color::getClosestMatch(first, materialColors) + 1;
}

void VoxFormat::addNodeToScene(const SceneGraph &sceneGraph, SceneGraphNode &node, ogt_SceneContext &ctx, uint32_t parentGroupIdx, uint32_t layerIdx, const voxel::Palette &palette, uint8_t replacement) {
	Log::debug("Save node '%s' with parent group %u and layer %u", node.name().c_str(), parentGroupIdx, layerIdx);
	if (node.type() == SceneGraphNodeType::Root || node.type() == SceneGraphNodeType::Group) {
		if (node.type() == SceneGraphNodeType::Root) {
			Log::debug("Add root node");
		} else {
			Log::debug("Add group node");
		}
		{
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
		{
			ogt_vox_group ogt_group;
			core_memset(&ogt_group, 0, sizeof(ogt_group));
			ogt_group.hidden = !node.visible();
			ogt_group.name = node.name().c_str();
			ogt_group.layer_index = ownLayerId;
			ogt_group.parent_group_index = parentGroupIdx;
			ogt_group.transform = ogt_identity_transform;
			ctx.groups.push_back(ogt_group);
		}
		const uint32_t ownGroupId = (int)ctx.groups.size() - 1;
		for (int childId : node.children()) {
			addNodeToScene(sceneGraph, sceneGraph.node(childId), ctx, ownGroupId, ownLayerId, palette, replacement);
		}
	} else if (node.type() == SceneGraphNodeType::Camera) {
		Log::debug("Add camera node");
		const SceneGraphNodeCamera &camera = toCameraNode(node);
		const SceneGraphTransform &transform = camera.transform(0);
		{
			ogt_vox_cam ogt_cam;
			core_memset(&ogt_cam, 0, sizeof(ogt_cam));
			ogt_cam.camera_id = ctx.cameras.size();
			const glm::vec3 &euler = glm::eulerAngles(transform.worldOrientation());
			ogt_cam.angle[0] = euler[0];
			ogt_cam.angle[1] = euler[1];
			ogt_cam.angle[2] = euler[2];
			const glm::vec3 &pos = transform.worldTranslation();
			ogt_cam.focus[0] = pos[0];
			ogt_cam.focus[1] = pos[1];
			ogt_cam.focus[2] = pos[2];
			ogt_cam.mode = camera.isPerspective() ? ogt_cam_mode_perspective : ogt_cam_mode_orthographic;
			ogt_cam.radius = (int)camera.farPlane();
			ogt_cam.fov = camera.fieldOfView();
			ogt_cam.frustum = camera.propertyf("frustum"); // TODO:
			ctx.cameras.push_back(ogt_cam);
		}
		for (int childId : node.children()) {
			addNodeToScene(sceneGraph, sceneGraph.node(childId), ctx, parentGroupIdx, layerIdx, palette, replacement);
		}
	} else if (node.type() == SceneGraphNodeType::Model) {
		Log::debug("Add model node");
		const voxel::Region region = node.region();
		const voxel::Palette &nodePalette = node.palette();
		const bool needsRemapping = palette.hash() != nodePalette.hash();
		{
			ogt_vox_model ogt_model;
			core_memset(&ogt_model, 0, sizeof(ogt_model));
			// flip y and z here
			ogt_model.size_x = region.getWidthInVoxels();
			ogt_model.size_y = region.getDepthInVoxels();
			ogt_model.size_z = region.getHeightInVoxels();
			const int voxelSize = (int)(ogt_model.size_x * ogt_model.size_y * ogt_model.size_z);
			uint8_t *dataptr = (uint8_t*)core_malloc(voxelSize);
			ogt_model.voxel_data = dataptr;
			voxelutil::visitVolume(*node.volume(), [&] (int, int, int, const voxel::Voxel& voxel) {
				if (voxel.getColor() == 0) {
					if (isAir(voxel.getMaterial())) {
						*dataptr++ = 0;
					} else {
						*dataptr++ = replacement;
					}
				} else if (needsRemapping) {
					const uint8_t palIndex = palette.getClosestMatch(nodePalette.colors[voxel.getColor()]);
					if (palIndex == 0) {
						*dataptr++ = replacement;
					} else {
						*dataptr++ = palIndex;
					}
				} else {
					*dataptr++ = voxel.getColor();
				}
			}, voxelutil::VisitAll(), voxelutil::VisitorOrder::YZmX);

			ctx.models.push_back(ogt_model);
		}
		{
			{
				ogt_vox_instance ogt_instance;
				core_memset(&ogt_instance, 0, sizeof(ogt_instance));
				ogt_instance.group_index = parentGroupIdx;
				ogt_instance.model_index = ctx.models.size() - 1;
				ogt_instance.layer_index = layerIdx;
				ogt_instance.name = node.name().c_str();
				ogt_instance.hidden = !node.visible();
				ogt_instance.transform_anim.num_keyframes = node.keyFrames().size();
				ogt_instance.transform_anim.keyframes = ogt_instance.transform_anim.num_keyframes ? &ctx.keyframeTransforms[ctx.transformKeyFrameIdx] : nullptr;
				ctx.instances.push_back(ogt_instance);
			}

			const glm::vec3 &mins = region.getLowerCornerf();
			const glm::vec3 &maxs = region.getUpperCornerf();
			const glm::vec3 width = maxs - mins + 1.0f;
			for (const SceneGraphKeyFrame& kf : node.keyFrames()) {
				ogt_vox_keyframe_transform ogt_keyframe;
				core_memset(&ogt_keyframe, 0, sizeof(ogt_keyframe));
				ogt_keyframe.frame_index = kf.frameIdx;
				ogt_keyframe.transform = ogt_identity_transform;
				// y and z are flipped here
				const glm::vec3 kftransform = mins + kf.transform().worldTranslation() + width / 2.0f;
				ogt_keyframe.transform.m30 = -glm::floor(kftransform.x + 0.5f);
				ogt_keyframe.transform.m31 = kftransform.z;
				ogt_keyframe.transform.m32 = kftransform.y;
				// TODO: apply rotation - but rotations are not interpolated - they must be aligned here somehow...
				ctx.keyframeTransforms[ctx.transformKeyFrameIdx++] = ogt_keyframe;
			}
		}
		for (int childId : node.children()) {
			addNodeToScene(sceneGraph, sceneGraph.node(childId), ctx, parentGroupIdx, layerIdx, palette, replacement);
		}
	} else {
		Log::error("Unhandled node type %i", (int)node.type());
	}
}

glm::ivec3 VoxFormat::maxSize() const {
	const glm::ivec3 maxSize = glm::ivec3(256);
	return maxSize;
}

bool VoxFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream &stream) {
	voxel::Palette palette = sceneGraph.mergePalettes(true);
	int palReplacement = findClosestPaletteIndex(palette);

	core_assert(palReplacement != 0u);
	Log::debug("Found closest palette slot %i as replacement", palReplacement);

	ogt_SceneContext ctx;
	const SceneGraphNode &root = sceneGraph.root();
	addNodeToScene(sceneGraph, sceneGraph.node(root.id()), ctx, k_invalid_group_index, 0, palette, palReplacement);

	core::Buffer<const ogt_vox_model *> modelPtr;
	modelPtr.reserve(ctx.models.size());
	for (const ogt_vox_model &mdl : ctx.models) {
		modelPtr.push_back(&mdl);
	}
	const ogt_vox_model **modelsPtr = modelPtr.data();

	ogt_vox_scene output_scene;
	core_memset(&output_scene, 0, sizeof(output_scene));
	output_scene.groups = &ctx.groups[0];
	output_scene.num_groups = ctx.groups.size();
	output_scene.instances = &ctx.instances[0];
	output_scene.num_instances = ctx.instances.size();
	output_scene.layers = &ctx.layers[0];
	output_scene.num_layers = ctx.layers.size();
	output_scene.models = modelsPtr;
	output_scene.num_models = modelPtr.size();
	core_memset(&output_scene.materials, 0, sizeof(output_scene.materials));
	output_scene.num_cameras = ctx.cameras.size();
	if (output_scene.num_cameras > 0) {
		output_scene.cameras = &ctx.cameras[0];
	}

	ogt_vox_palette &pal = output_scene.palette;
	ogt_vox_matl_array &mat = output_scene.materials;

	core_assert(palette.colorCount > 0);
	for (int i = 0; i < palette.colorCount; ++i) {
		const core::RGBA &rgba = palette.colors[i];
		pal.color[i].r = rgba.r;
		pal.color[i].g = rgba.g;
		pal.color[i].b = rgba.b;
		pal.color[i].a = rgba.a;

		const core::RGBA &glowColor = palette.glowColors[i];
		if (glowColor.rgba != 0) {
			mat.matl[i].content_flags |= k_ogt_vox_matl_have_emit;
			mat.matl[i].type = ogt_matl_type::ogt_matl_type_emit;
			mat.matl[i].emit = 1.0f;
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

} // namespace voxel
