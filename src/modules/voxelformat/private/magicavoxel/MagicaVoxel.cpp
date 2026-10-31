/**
 * @file
 */

#include "MagicaVoxel.h"
#include "app/Async.h"
#include "color/Color.h"
#include "core/ConfigVar.h"
#include "core/GLMConst.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "core/Endian.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include <limits>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#define OGT_VOX_BIGENDIAN_SWAP32 core_swap32le
#define OGT_VOX_IMPLEMENTATION
#define ogt_assert(x, msg) core_assert_msg(x, "%s", msg)
#define ogt_assert_warn(x, msg)                                                                                                                                                                                        \
	if (!(x)) {                                                                                                                                                                                                        \
		Log::warn("%s", msg);                                                                                                                                                                                          \
	}
#include "voxelformat/external/ogt_vox.h"

namespace voxelformat {

glm::mat4 ogtToMat(const ogt_vox_transform &t) {
	const glm::vec4 col0(t.m00, t.m01, t.m02, t.m03);
	const glm::vec4 col1(t.m10, t.m11, t.m12, t.m13);
	const glm::vec4 col2(t.m20, t.m21, t.m22, t.m23);
	const glm::vec4 col3(t.m30, t.m31, t.m32, t.m33);
	return glm::mat4{col0, col1, col2, col3};
}

/**
 * Snap a 3x3 basis to a MagicaVoxel-compatible signed permutation.
 * Independent per-column snapping can map two columns onto the same axis (e.g. 45-degree
 * rotations); ogt then writes an invalid packed _r (e.g. 15) and crashes on reload.
 */
static void snapToSignedPermutation(glm::vec3 &x, glm::vec3 &y, glm::vec3 &z) {
	const glm::vec3 cols[3] = {x, y, z};
	// clang-format off
	const int perms[6][3] = {
		{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
		{1, 2, 0}, {2, 0, 1}, {2, 1, 0}
	};
	// clang-format on
	int best[3] = {0, 1, 2};
	float bestScore = -1.0f;
	for (int p = 0; p < 6; ++p) {
		const float score = glm::abs(cols[0][perms[p][0]]) + glm::abs(cols[1][perms[p][1]]) +
							glm::abs(cols[2][perms[p][2]]);
		if (score > bestScore) {
			bestScore = score;
			best[0] = perms[p][0];
			best[1] = perms[p][1];
			best[2] = perms[p][2];
		}
	}
	auto axis = [](int index, float signedComponent) {
		glm::vec3 v(0.0f);
		// Zero/NaN columns still need a cardinal axis so packing never sees an all-zero row.
		const float sign = signedComponent < 0.0f ? -1.0f : 1.0f;
		v[index] = sign;
		return v;
	};
	x = axis(best[0], cols[0][best[0]]);
	y = axis(best[1], cols[1][best[1]]);
	z = axis(best[2], cols[2][best[2]]);
}

ogt_vox_transform matToOgt(const glm::mat4 &mat) {
	// MagicaVoxel / ogt only support signed-permutation rotations (see ogt_vox_transform).
	glm::vec3 x(mat[0]);
	glm::vec3 y(mat[1]);
	glm::vec3 z(mat[2]);
	snapToSignedPermutation(x, y, z);
	ogt_vox_transform t = ogt_identity_transform;
	// ogt stores columns: (m00,m01,m02)=x, (m10,m11,m12)=y, (m20,m21,m22)=z
	t.m00 = x.x;
	t.m01 = x.y;
	t.m02 = x.z;
	t.m10 = y.x;
	t.m11 = y.y;
	t.m12 = y.z;
	t.m20 = z.x;
	t.m21 = z.y;
	t.m22 = z.z;
	t.m30 = glm::round(mat[3].x);
	t.m31 = glm::round(mat[3].y);
	t.m32 = glm::round(mat[3].z);
	t.m33 = 1.0f;
	checkRotation(t);
	return t;
}

static glm::mat4 computeBakeMatrix(const glm::mat4 &ogtTransform, const glm::vec3 &pivot) {
	const glm::mat4 shiftMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f));
	const glm::mat4 pivotMatrix = glm::translate(glm::mat4(1.0f), -pivot);
	return ogtTransform * shiftMatrix * pivotMatrix;
}

static ogt_vox_transform instanceTransform(const ogt_vox_instance &instance, uint32_t frameIdx,
										 const ogt_vox_scene *scene) {
	// A single frame can differ from MagicaVoxel's static base pose (_frm_t/_frm_r).
	// Use the base pose for static nodes, and sample nodes with multiple animation frames.
	ogt_vox_transform transform = instance.transform_anim.num_keyframes > 1
		? ogt_vox_sample_instance_transform_local(&instance, frameIdx) : instance.transform;
	uint32_t groupIdx = instance.group_index;
	while (groupIdx != k_invalid_group_index) {
		const ogt_vox_group &group = scene->groups[groupIdx];
		const ogt_vox_transform groupTransform = group.transform_anim.num_keyframes > 1
			? ogt_vox_sample_group_transform_local(&group, frameIdx) : group.transform;
		transform = ogt_vox_transform_multiply(transform, groupTransform);
		groupIdx = group.parent_group_index;
	}
	return transform;
}

glm::mat4 ogtInstanceBakeMatrix(const ogt_vox_instance &instance, uint32_t frameIdx, const ogt_vox_scene *scene,
								const ogt_vox_model *model) {
	const ogt_vox_transform t = instanceTransform(instance, frameIdx, scene);
	return computeBakeMatrix(ogtToMat(t), glm::vec3(ogtVolumePivot(model)));
}

voxel::RawVolume *bakeOgtModel(const ogt_vox_model *model, const glm::mat4 &bakeMat, const palette::Palette &palette,
							   glm::ivec3 &outShift) {
	const glm::vec3 volSize = ogtVolumeSize(model);
	const glm::vec3 corners[8] = {glm::vec3(0),
								  glm::vec3(volSize.x, 0, 0),
								  glm::vec3(0, volSize.y, 0),
								  glm::vec3(volSize.x, volSize.y, 0),
								  glm::vec3(0, 0, volSize.z),
								  glm::vec3(volSize.x, 0, volSize.z),
								  glm::vec3(0, volSize.y, volSize.z),
								  volSize};
	glm::ivec3 mins(std::numeric_limits<int>::max());
	glm::ivec3 maxs(std::numeric_limits<int>::min());
	for (int c = 0; c < 8; ++c) {
		const glm::ivec3 ogtCorner = calcTransform(bakeMat, corners[c]);
		const glm::ivec3 pos(-(ogtCorner.x + 1), ogtCorner.z, ogtCorner.y);
		mins = glm::min(mins, pos);
		maxs = glm::max(maxs, pos);
	}
	voxel::Region region(mins, maxs);
	outShift = region.getLowerCorner();
	region.shift(-outShift);
	voxel::RawVolume *v = new voxel::RawVolume(region);

	auto fn = [model, v, &palette, bakeMat, outShift](int start, int end) {
		const uint8_t *ogtVoxel = model->voxel_data + start * model->size_x * model->size_y;
		for (int k = start; k < end; ++k) {
			for (uint32_t j = 0; j < model->size_y; ++j) {
				for (uint32_t i = 0; i < model->size_x; ++i, ++ogtVoxel) {
					if (ogtVoxel[0] == 0) {
						continue;
					}
					const voxel::Voxel voxel = voxel::createVoxel(palette, ogtVoxel[0] - 1);
					const glm::ivec3 &ogtPos = calcTransform(bakeMat, glm::vec3(i, j, k));
					const glm::ivec3 pos(-(ogtPos.x + 1), ogtPos.z, ogtPos.y);
					v->setVoxel(pos - outShift, voxel);
				}
			}
		}
	};
	app::for_parallel(0, (int)model->size_z, fn);
	return v;
}

glm::mat4 ogtMatToVengi(const glm::mat4 &ogtMat) {
	// loadModels stores MagicaVoxel (x,y,z) at vengi (sx-1-x, z, y). The previous bake path used
	// transform * translate(0.5) * translate(-pivot) then remapped with (-(x+1), z, y) and floor().
	// With pivot handled by SceneGraphTransform, match that placement with A-conjugation, the -1 X
	// bias, the half-voxel shift, then snap translation to the integer grid MagicaVoxel uses.
	// clang-format off
	const glm::mat4 A(
		-1.0f, 0.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 1.0f, 0.0f,
		 0.0f, 1.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 0.0f, 1.0f);
	// clang-format on
	const glm::mat4 offset = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	const glm::mat4 halfVoxel = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f));
	return offset * A * (ogtMat * halfVoxel) * A;
}

glm::mat4 vengiMatToOgt(const glm::mat4 &vengiMat) {
	// Inverse: remove A/offset, then undo the half-voxel translation on the MagicaVoxel side.
	// clang-format off
	const glm::mat4 A(
		-1.0f, 0.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 1.0f, 0.0f,
		 0.0f, 1.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 0.0f, 1.0f);
	// clang-format on
	const glm::mat4 offsetInv = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	const glm::mat4 halfVoxelInv = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f));
	return (A * offsetInv * vengiMat * A) * halfVoxelInv;
}

glm::vec3 ogtNormalizedPivot(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ) {
	// MagicaVoxel pivot is floor(size/2) in model space.
	// loadModels maps MagicaVoxel (x,y,z) -> vengi (sx-1-x, z, y).
	const float sx = (float)sizeX;
	const float sy = (float)sizeY;
	const float sz = (float)sizeZ;
	const float px = (float)(int)(sizeX / 2);
	const float py = (float)(int)(sizeY / 2);
	const float pz = (float)(int)(sizeZ / 2);
	return glm::vec3((sx - 1.0f - px) / sx, pz / sz, py / sy);
}

glm::vec3 ogtNormalizedPivot(const ogt_vox_model *model) {
	return ogtNormalizedPivot(model->size_x, model->size_y, model->size_z);
}

void *_ogt_alloc(size_t size) {
	return core_malloc(size);
}

void _ogt_free(void *mem) {
	core_free(mem);
}

bool loadInstanceKeyFrames(scenegraph::SceneGraphNode &node, const ogt_vox_instance &ogtInstance,
						   const ogt_vox_scene *scene) {
	const ogt_vox_anim_transform &transformAnim = ogtInstance.transform_anim;
	if (transformAnim.num_keyframes == 0) {
		scenegraph::SceneGraphKeyFrames kf;
		scenegraph::SceneGraphKeyFrame sceneGraphKeyFrame;
		sceneGraphKeyFrame.frameIdx = 0;
		sceneGraphKeyFrame.interpolation = scenegraph::InterpolationType::Linear;
		sceneGraphKeyFrame.transform().setWorldMatrix(
			ogtMatToVengi(ogtToMat(instanceTransform(ogtInstance, 0, scene))));
		kf.push_back(core::move(sceneGraphKeyFrame));
		return node.setKeyFrames(kf);
	}
	// Sample each keyframe as a global transform so group hierarchy is flattened into the instance.
	// Groups remain in the scenegraph for structure/visibility but keep identity transforms.
	scenegraph::SceneGraphKeyFrames kf;
	kf.reserve(transformAnim.num_keyframes);
	for (uint32_t keyFrameIdx = 0; keyFrameIdx < transformAnim.num_keyframes; ++keyFrameIdx) {
		const ogt_vox_keyframe_transform &keyFrameTransform = transformAnim.keyframes[keyFrameIdx];
		const uint32_t frameIdx = keyFrameTransform.frame_index;
		scenegraph::SceneGraphKeyFrame sceneGraphKeyFrame;
		sceneGraphKeyFrame.frameIdx = (scenegraph::FrameIndex)frameIdx;
		sceneGraphKeyFrame.interpolation = scenegraph::InterpolationType::Linear;
		sceneGraphKeyFrame.longRotation = false;
		sceneGraphKeyFrame.transform().setWorldMatrix(
			ogtMatToVengi(ogtToMat(instanceTransform(ogtInstance, frameIdx, scene))));
		kf.push_back(core::move(sceneGraphKeyFrame));
	}
	return node.setKeyFrames(kf);
}

bool loadGroupKeyFrames(scenegraph::SceneGraphNode &node, const ogt_vox_anim_transform &transformAnim) {
	// Group transforms are baked into instance globals; keep groups as identity for placement.
	(void)transformAnim;
	scenegraph::SceneGraphKeyFrames kf;
	scenegraph::SceneGraphKeyFrame sceneGraphKeyFrame;
	sceneGraphKeyFrame.frameIdx = 0;
	sceneGraphKeyFrame.interpolation = scenegraph::InterpolationType::Linear;
	sceneGraphKeyFrame.transform().setWorldMatrix(glm::mat4(1.0f));
	kf.push_back(core::move(sceneGraphKeyFrame));
	return node.setKeyFrames(kf);
}

const ogt_vox_keyframe_transform *saveKeyFrames(const scenegraph::SceneGraph &sceneGraph,
												const scenegraph::SceneGraphNode &node, MVSceneContext &ctx,
												uint32_t &numKeyframes) {
	const scenegraph::SceneGraphKeyFrames &keyFrames = node.keyFrames(sceneGraph.activeAnimation());
	numKeyframes = (uint32_t)keyFrames.size();
	if (numKeyframes == 0) {
		return nullptr;
	}
	const uint32_t start = (uint32_t)ctx.keyframeTransforms.size();
	ctx.keyframeTransforms.reserve(start + numKeyframes);

	const bool applyTransform = core::getVar(cfg::VoxformatMVApplyTransform)->boolVal();
	if (applyTransform || !node.isAnyModelNode()) {
		glm::vec3 width(0.0f);
		glm::vec3 mins(0.0f);
		if (node.isAnyModelNode()) {
			const voxel::Region region = sceneGraph.resolveRegion(node);
			width = glm::vec3(region.getDimensionsInVoxels());
			mins = region.getLowerCornerf();
		}
		for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
			ogt_vox_keyframe_transform ogt_keyframe;
			core_memset(&ogt_keyframe, 0, sizeof(ogt_keyframe));
			ogt_keyframe.frame_index = kf.frameIdx;
			ogt_keyframe.transform = ogt_identity_transform;
			const glm::vec3 kftransform =
				mins + kf.transform().worldTranslation() - node.pivot() * width + width / 2.0f;
			ogt_keyframe.transform.m30 = -glm::floor(kftransform.x + 0.5f);
			ogt_keyframe.transform.m31 = kftransform.z;
			ogt_keyframe.transform.m32 = kftransform.y;
			checkRotation(ogt_keyframe.transform);
			ctx.keyframeTransforms.push_back(ogt_keyframe);
		}
	} else {
		glm::mat4 pivotAdjust(1.0f);
		const voxel::Region region = sceneGraph.resolveRegion(node);
		const glm::vec3 dims(region.getDimensionsInVoxels());
		const glm::vec3 mvPivot = ogtNormalizedPivot((uint32_t)dims.x, (uint32_t)dims.z, (uint32_t)dims.y);
		pivotAdjust = glm::translate(glm::mat4(1.0f), (mvPivot - node.pivot()) * dims);
		for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
			ogt_vox_keyframe_transform ogt_keyframe;
			core_memset(&ogt_keyframe, 0, sizeof(ogt_keyframe));
			ogt_keyframe.frame_index = kf.frameIdx;
			const glm::mat4 worldForMv = kf.transform().worldMatrix() * pivotAdjust;
			ogt_keyframe.transform = matToOgt(vengiMatToOgt(worldForMv));
			ctx.keyframeTransforms.push_back(ogt_keyframe);
		}
	}
	return &ctx.keyframeTransforms[start];
}

void loadPaletteFromScene(const ogt_vox_scene *scene, palette::Palette &palette) {
	palette.setSize(0);
	int palIdx = 0;
	for (int i = 0; i < lengthof(scene->palette.color) - 1; ++i) {
		if (i < (int)scene->num_color_names) {
			const char *name = scene->color_names[i];
			if (name != nullptr) {
				palette.setColorName(palIdx, name);
			}
		}
		const ogt_vox_rgba color = scene->palette.color[(i + 1) & 255];
		palette.setColor(palIdx, color::RGBA(color.r, color.g, color.b, color.a));
		const ogt_vox_matl &matl = scene->materials.matl[(i + 1) & 255];
		if (matl.type == ogt_matl_type_diffuse) {
			palette.setMaterialType(palIdx, palette::MaterialType::Diffuse);
		} else if (matl.type == ogt_matl_type_metal) {
			palette.setMaterialType(palIdx, palette::MaterialType::Metal);
		} else if (matl.type == ogt_matl_type_glass) {
			palette.setMaterialType(palIdx, palette::MaterialType::Glass);
		} else if (matl.type == ogt_matl_type_emit) {
			palette.setMaterialType(palIdx, palette::MaterialType::Emit);
		} else if (matl.type == ogt_matl_type_blend) {
			palette.setMaterialType(palIdx, palette::MaterialType::Blend);
		} else if (matl.type == ogt_matl_type_media) {
			palette.setMaterialType(palIdx, palette::MaterialType::Media);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_metal) {
			palette.setMetal(palIdx, matl.metal);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_rough) {
			palette.setRoughness(palIdx, matl.rough);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_spec) {
			palette.setSpecular(palIdx, matl.spec);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_ior) {
			const float n = matl.ior < 1.0f ? (1.0f + matl.ior) : matl.ior;
			palette.setIndexOfRefraction(palIdx, n);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_ri) {
			palette.setIndexOfRefraction(palIdx, matl.ri);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_att) {
			palette.setAttenuation(palIdx, matl.att);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_flux) {
			palette.setFlux(palIdx, matl.flux);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_emit) {
			palette.setEmit(palIdx, matl.emit);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_ldr) {
			palette.setLowDynamicRange(palIdx, matl.ldr);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_trans) {
			palette.setAlpha(palIdx, matl.trans);
		} else if (matl.content_flags & k_ogt_vox_matl_have_alpha) {
			palette.setAlpha(palIdx, matl.alpha);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_d) {
			palette.setDensity(palIdx, matl.d);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_sp) {
			palette.setSp(palIdx, matl.sp);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_g) {
			palette.setPhase(palIdx, matl.g);
		}
		if (matl.content_flags & k_ogt_vox_matl_have_media) {
			palette.setMedia(palIdx, matl.media);
		}
		if (matl.media_type != ogt_media_type_absorb || matl.type == ogt_matl_type_media) {
			palette.setMedia(palIdx, (float)matl.media_type);
		}
		++palIdx;
	}
	int n = 0;
	for (int i = 0; i < palette::PaletteMaxColors; ++i) {
		if (palette.color(i).a > 0) {
			n = i + 1;
		}
	}
	if (n > 0) {
		palette.setSize(n);
	}
	Log::debug("vox load color count: %i", palette.colorCount());
}

bool loadPaletteFromBuffer(const uint8_t *buffer, size_t size, palette::Palette &palette) {
	const ogt_vox_scene *scene = ogt_vox_read_scene_with_flags(buffer, size, 0);
	if (scene == nullptr) {
		Log::error("Could not load scene");
		return false;
	}
	loadPaletteFromScene(scene, palette);
	ogt_vox_destroy_scene(scene);
	return true;
}

void printDetails(const ogt_vox_scene *scene) {
	Log::debug("vox groups: %u", scene->num_groups);
	for (uint32_t i = 0; i < scene->num_groups; ++i) {
		if (scene->groups[i].name) {
			Log::debug(" %u: %s", i, scene->groups[i].name);
		}
	}
	Log::debug("vox instances: %u", scene->num_instances);
	for (uint32_t i = 0; i < scene->num_instances; ++i) {
		if (scene->instances[i].name) {
			Log::debug(" %u: %s", i, scene->instances[i].name);
		}
	}
	Log::debug("vox layers: %u", scene->num_layers);
	for (uint32_t i = 0; i < scene->num_layers; ++i) {
		if (scene->layers[i].name) {
			Log::debug(" %u: %s", i, scene->layers[i].name);
		}
	}
	Log::debug("vox models: %u", scene->num_models);
	Log::debug("vox cameras: %u", scene->num_cameras);
}

#ifdef DEBUG
static bool checkRotationRow(const glm::vec3 &vec) {
	int nonZero = 0;
	for (int i = 0; i < 3; i++) {
		if (vec[i] == 1.0f || vec[i] == -1.0f) {
			++nonZero;
			continue;
		}
		core_assert_msg(vec[i] == 0.0f, "rotation vector should contain only 0.0f, 1.0f, or -1.0f");
	}
	return nonZero == 1;
}
#endif

void checkRotation(const ogt_vox_transform &transform) {
#ifdef DEBUG
	// Columns must be cardinal axes (ogt column-major layout).
	core_assert(checkRotationRow({transform.m00, transform.m01, transform.m02}));
	core_assert(checkRotationRow({transform.m10, transform.m11, transform.m12}));
	core_assert(checkRotationRow({transform.m20, transform.m21, transform.m22}));
	// MagicaVoxel packed _r encodes rows (m00,m10,m20) etc. - must also be a permutation.
	core_assert(checkRotationRow({transform.m00, transform.m10, transform.m20}));
	core_assert(checkRotationRow({transform.m01, transform.m11, transform.m21}));
	core_assert(checkRotationRow({transform.m02, transform.m12, transform.m22}));
#endif
}

void loadCameras(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph) {
	for (uint32_t n = 0; n < scene->num_cameras; ++n) {
		const ogt_vox_cam &c = scene->cameras[n];
		const glm::vec3 target(c.focus[0], c.focus[2], c.focus[1]);
		const glm::vec3 angles(c.angle[0], -c.angle[1], c.angle[2]);
		const glm::vec3 rangles = glm::radians(angles);
		const glm::quat quat(rangles);
		const float distance = (float)c.radius;
		const glm::vec3 &forward = glm::conjugate(quat) * glm::forward();
		const glm::vec3 &backward = -forward;
		const glm::vec3 &newPosition = target + backward * distance;
		const glm::mat4 &orientation = glm::mat4_cast(quat);
		const glm::mat4 &viewMatrix = glm::translate(orientation, -newPosition);

		{
			scenegraph::SceneGraphNodeCamera camNode;
			camNode.setName(core::String::format("Camera %u", c.camera_id));
			scenegraph::SceneGraphTransform transform;
			transform.setWorldMatrix(viewMatrix);
			const scenegraph::KeyFrameIndex keyFrameIdx = 0;
			camNode.setTransform(keyFrameIdx, transform);
			camNode.setFieldOfView(c.fov);
			camNode.setFarPlane((float)c.radius);
			camNode.setProperty(scenegraph::PropCamFrustum, core::String::format("%f", c.frustum));
			if (c.mode == ogt_cam_mode_perspective) {
				camNode.setPerspective();
			} else if (c.mode == ogt_cam_mode_orthographic) {
				camNode.setOrthographic();
			}
			sceneGraph.emplace(core::move(camNode), sceneGraph.root().id());
		}
	}
}

void loadSun(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph) {
	if (scene->sun == nullptr) {
		return;
	}
	const ogt_vox_sun &sun = *scene->sun;
	scenegraph::SceneGraphNode &root = sceneGraph.node(sceneGraph.root().id());
	root.setProperty(scenegraph::PropSunIntensity, sun.intensity);
	root.setProperty(scenegraph::PropSunArea, sun.area);
	root.setProperty(scenegraph::PropSunElevation, sun.angle[0]);
	root.setProperty(scenegraph::PropSunAzimuth, sun.angle[1]);
	const color::RGBA sunColor(sun.rgba.r, sun.rgba.g, sun.rgba.b, sun.rgba.a);
	root.setProperty(scenegraph::PropSunColor, sunColor);
	root.setProperty(scenegraph::PropSunDisk, sun.disk);
}

MVModelToNode::MVModelToNode() : volume(nullptr), nodeId(InvalidNodeId) {
}

MVModelToNode::~MVModelToNode() {
	delete volume;
}

MVModelToNode::MVModelToNode(MVModelToNode &&other) noexcept : volume(other.volume), nodeId(other.nodeId) {
	other.volume = nullptr;
	other.nodeId = InvalidNodeId;
}

MVModelToNode &MVModelToNode::operator=(MVModelToNode &&other) noexcept {
	if (this != &other) {
		delete volume;
		volume = other.volume;
		nodeId = other.nodeId;
		other.volume = nullptr;
		other.nodeId = InvalidNodeId;
	}
	return *this;
}

const char *instanceName(const ogt_vox_scene *scene, const ogt_vox_instance &instance) {
	const char *name = instance.name;
	if (name == nullptr) {
		return "<vox>";
	}
	return name;
}

color::RGBA instanceColor(const ogt_vox_scene *scene, const ogt_vox_instance &instance) {
	if (instance.layer_index >= scene->num_layers) {
		return color::RGBA(255, 255, 255, 255);
	}
	const ogt_vox_layer &layer = scene->layers[instance.layer_index];
	const color::RGBA col(layer.color.r, layer.color.g, layer.color.b, layer.color.a);
	return col;
}

bool instanceHidden(const ogt_vox_scene *scene, const ogt_vox_instance &instance) {
	if (instance.hidden) {
		return true;
	}
	if (instance.layer_index < scene->num_layers) {
		if (scene->layers[instance.layer_index].hidden) {
			return true;
		}
	}
	if (instance.group_index != k_invalid_group_index && instance.group_index < scene->num_groups &&
		scene->groups[instance.group_index].hidden) {
		return true;
	}
	return false;
}

core::DynamicArray<MVModelToNode> loadModels(const ogt_vox_scene *scene, const palette::Palette &palette) {
	core::DynamicArray<MVModelToNode> models;
	models.resize(scene->num_models);
	auto fn = [&scene, &palette, &models](int start, int end) {
		for (int i = start; i < end; ++i) {
			const ogt_vox_model *ogtModel = scene->models[i];
			if (ogtModel == nullptr) {
				models[i] = MVModelToNode(nullptr, InvalidNodeId);
				continue;
			}
			// MagicaVoxel Z-up -> vengi Y-up with X mirrored to match VisitorOrder::YZmX on save.
			voxel::Region region(glm::ivec3(0),
								 glm::ivec3(ogtModel->size_x - 1, ogtModel->size_z - 1, ogtModel->size_y - 1));
			voxel::RawVolume *v = new voxel::RawVolume(region);

			const uint8_t *ogtVoxel = ogtModel->voxel_data;
			voxel::RawVolume::Sampler sampler(v);
			sampler.setPosition(region.getUpperX(), 0, 0);
			for (uint32_t z = 0; z < ogtModel->size_z; ++z) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (uint32_t y = 0; y < ogtModel->size_y; ++y) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (uint32_t x = 0; x < ogtModel->size_x; ++x, ++ogtVoxel) {
						if (ogtVoxel[0] == 0) {
							sampler3.moveNegativeX();
							continue;
						}
						const voxel::Voxel voxel = voxel::createVoxel(palette, ogtVoxel[0] - 1);
						sampler3.setVoxel(voxel);
						sampler3.moveNegativeX();
					}
					sampler2.movePositiveZ();
				}
				sampler.movePositiveY();
			}
			models[i] = MVModelToNode(v, InvalidNodeId);
		}
	};
	app::for_parallel(0, scene->num_models, fn);
	return models;
}

static const float FroxelClipFps = 24.0f;
static const uint32_t FroxelMaxNameLen = 1025;
static const uint32_t FroxelMaxSourceLen = 65537;
static const uint32_t FroxelMaxCount = 256;
static const uint32_t FroxelMaxKeys = 10000;
static const uint32_t FroxelMaxParams = 64;

static bool readFroxelString(io::ReadStream &stream, core::String &out, uint32_t maxLen) {
	return stream.readPascalStringUInt32LE(out, maxLen);
}

static bool writeFroxelString(io::WriteStream &stream, const core::String &value) {
	return stream.writePascalStringUInt32LE(value);
}

static bool readFroxelVec3(io::ReadStream &stream, float v[3]) {
	return stream.readFloat(v[0]) == 0 && stream.readFloat(v[1]) == 0 && stream.readFloat(v[2]) == 0;
}

static bool writeFroxelVec3(io::WriteStream &stream, const float v[3]) {
	return stream.writeFloat(v[0]) && stream.writeFloat(v[1]) && stream.writeFloat(v[2]);
}

static bool readFroxelQuat(io::ReadStream &stream, float r[4]) {
	return stream.readFloat(r[0]) == 0 && stream.readFloat(r[1]) == 0 && stream.readFloat(r[2]) == 0 &&
		   stream.readFloat(r[3]) == 0;
}

static bool writeFroxelQuat(io::WriteStream &stream, const float r[4]) {
	return stream.writeFloat(r[0]) && stream.writeFloat(r[1]) && stream.writeFloat(r[2]) && stream.writeFloat(r[3]);
}

bool parseFroxelMVRG(const uint8_t *content, uint32_t size, FroxelVoxData &out) {
	if (content == nullptr || size == 0) {
		return false;
	}
	io::MemoryReadStream stream(content, size);
	uint32_t version = 0;
	if (stream.readUInt32(version) != 0 || version != 1) {
		Log::warn("Unsupported Froxel MVRG version %u", version);
		return false;
	}
	uint32_t boneCount = 0;
	if (stream.readUInt32(boneCount) != 0 || boneCount > FroxelMaxCount) {
		return false;
	}
	FroxelVoxData parsed;
	parsed.bones.reserve(boneCount);
	for (uint32_t i = 0; i < boneCount; ++i) {
		FroxelBone bone;
		if (!readFroxelString(stream, bone.name, FroxelMaxNameLen) ||
			!readFroxelString(stream, bone.parent, FroxelMaxNameLen) || !readFroxelVec3(stream, bone.pivot) ||
			!readFroxelQuat(stream, bone.rotation)) {
			return false;
		}
		parsed.bones.push_back(core::move(bone));
	}
	uint32_t bindCount = 0;
	if (stream.readUInt32(bindCount) != 0 || bindCount > FroxelMaxCount) {
		return false;
	}
	parsed.binds.reserve(bindCount);
	for (uint32_t i = 0; i < bindCount; ++i) {
		FroxelBind bind;
		if (stream.readInt32(bind.trn) != 0 || stream.readUInt32(bind.index) != 0 ||
			!readFroxelString(stream, bind.name, FroxelMaxNameLen) ||
			!readFroxelString(stream, bind.bone, FroxelMaxNameLen) || bind.bone.empty()) {
			return false;
		}
		parsed.binds.push_back(core::move(bind));
	}
	if (stream.remaining() != 0) {
		Log::warn("Froxel MVRG chunk has trailing bytes");
		return false;
	}
	for (FroxelBone &bone : parsed.bones) {
		out.bones.push_back(core::move(bone));
	}
	for (FroxelBind &bind : parsed.binds) {
		out.binds.push_back(core::move(bind));
	}
	return true;
}

bool parseFroxelMVAN(const uint8_t *content, uint32_t size, FroxelVoxData &out) {
	if (content == nullptr || size == 0) {
		return false;
	}
	io::MemoryReadStream stream(content, size);
	uint32_t version = 0;
	if (stream.readUInt32(version) != 0 || version < 1 || version > 3) {
		Log::warn("Unsupported Froxel MVAN version %u", version);
		return false;
	}
	uint32_t clipCount = 0;
	if (stream.readUInt32(clipCount) != 0 || clipCount > FroxelMaxCount) {
		return false;
	}
	core::DynamicArray<FroxelClip> clips;
	clips.reserve(clipCount);
	for (uint32_t c = 0; c < clipCount; ++c) {
		FroxelClip clip;
		if (!readFroxelString(stream, clip.name, FroxelMaxNameLen)) {
			return false;
		}
		uint32_t trackCount = 0;
		if (stream.readUInt32(trackCount) != 0 || trackCount > FroxelMaxCount) {
			return false;
		}
		clip.tracks.reserve(trackCount);
		for (uint32_t t = 0; t < trackCount; ++t) {
			FroxelClipTrack track;
			if (!readFroxelString(stream, track.bone, FroxelMaxNameLen)) {
				return false;
			}
			uint32_t keyCount = 0;
			if (stream.readUInt32(keyCount) != 0 || keyCount > FroxelMaxKeys) {
				return false;
			}
			track.keys.reserve(keyCount);
			for (uint32_t k = 0; k < keyCount; ++k) {
				FroxelClipKey key;
				if (stream.readFloat(key.time) != 0) {
					return false;
				}
				uint8_t flags = 0x03;
				if (version == 3) {
					if (stream.readUInt8(flags) != 0) {
						return false;
					}
				}
				if (!readFroxelQuat(stream, key.rotation)) {
					return false;
				}
				if (version == 2) {
					if (stream.readUInt8(flags) != 0) {
						return false;
					}
					flags = (uint8_t)(flags | 0x02);
				}
				key.hasRotation = (flags & 0x02) != 0;
				key.hasTranslation = (flags & 0x01) != 0;
				if (!readFroxelVec3(stream, key.translation) || !readFroxelVec3(stream, key.scale)) {
					return false;
				}
				track.keys.push_back(key);
			}
			clip.tracks.push_back(core::move(track));
		}
		clips.push_back(core::move(clip));
	}
	if (stream.remaining() != 0) {
		Log::warn("Froxel MVAN chunk has trailing bytes");
		return false;
	}
	for (FroxelClip &clip : clips) {
		out.clips.push_back(core::move(clip));
	}
	return true;
}

bool parseFroxelMVSC(const uint8_t *content, uint32_t size, FroxelVoxData &out) {
	if (content == nullptr || size == 0) {
		return false;
	}
	io::MemoryReadStream stream(content, size);
	uint32_t version = 0;
	if (stream.readUInt32(version) != 0 || (version != 1 && version != 2)) {
		Log::warn("Unsupported Froxel MVSC version %u", version);
		return false;
	}
	uint32_t scriptCount = 0;
	if (stream.readUInt32(scriptCount) != 0 || scriptCount > FroxelMaxCount) {
		return false;
	}
	core::DynamicArray<FroxelScript> scripts;
	scripts.reserve(scriptCount);
	for (uint32_t i = 0; i < scriptCount; ++i) {
		FroxelScript script;
		if (!readFroxelString(stream, script.name, FroxelMaxNameLen)) {
			return false;
		}
		if (version >= 2) {
			if (stream.readInt32(script.localMin[0]) != 0 || stream.readInt32(script.localMin[1]) != 0 ||
				stream.readInt32(script.localMin[2]) != 0) {
				return false;
			}
			script.hasLocalMin = true;
		}
		if (!readFroxelString(stream, script.source, FroxelMaxSourceLen)) {
			return false;
		}
		uint8_t autoRegen = 0;
		if (stream.readUInt8(autoRegen) != 0 || autoRegen > 1) {
			return false;
		}
		script.autoRegen = autoRegen != 0;
		uint32_t paramCount = 0;
		if (stream.readUInt32(paramCount) != 0 || paramCount > FroxelMaxParams) {
			return false;
		}
		script.params.reserve(paramCount);
		for (uint32_t p = 0; p < paramCount; ++p) {
			FroxelScriptParam param;
			if (!readFroxelString(stream, param.name, FroxelMaxNameLen) || stream.readUInt8(param.kind) != 0) {
				return false;
			}
			if (param.kind == FroxelParamFloat) {
				if (stream.readFloat(param.fvalue) != 0 || stream.readFloat(param.fmin) != 0 ||
					stream.readFloat(param.fmax) != 0) {
					return false;
				}
			} else if (param.kind == FroxelParamInt) {
				if (stream.readInt32(param.ivalue) != 0 || stream.readInt32(param.imin) != 0 ||
					stream.readInt32(param.imax) != 0) {
					return false;
				}
			} else if (param.kind == FroxelParamBool) {
				if (stream.readUInt8(param.bvalue) != 0 || param.bvalue > 1) {
					return false;
				}
			} else if (param.kind == FroxelParamColor) {
				if (stream.readUInt8(param.rgb[0]) != 0 || stream.readUInt8(param.rgb[1]) != 0 ||
					stream.readUInt8(param.rgb[2]) != 0) {
					return false;
				}
			} else if (param.kind == FroxelParamFloats) {
				if (stream.readUInt8(param.width) != 0 || param.width < 1 || param.width > 3 ||
					stream.readFloat(param.fmin) != 0 || stream.readFloat(param.fmax) != 0 ||
					stream.readUInt32(param.maxLen) != 0) {
					return false;
				}
				uint32_t valueCount = 0;
				if (stream.readUInt32(valueCount) != 0 || valueCount > FroxelMaxKeys) {
					return false;
				}
				param.values.reserve(valueCount);
				for (uint32_t v = 0; v < valueCount; ++v) {
					float value = 0.0f;
					if (stream.readFloat(value) != 0) {
						return false;
					}
					param.values.push_back(value);
				}
			} else {
				Log::warn("Unsupported Froxel MVSC parameter kind %u", (unsigned int)param.kind);
				return false;
			}
			script.params.push_back(core::move(param));
		}
		scripts.push_back(core::move(script));
	}
	if (stream.remaining() != 0) {
		Log::warn("Froxel MVSC chunk has trailing bytes");
		return false;
	}
	for (FroxelScript &script : scripts) {
		out.scripts.push_back(core::move(script));
	}
	return true;
}

bool encodeFroxelMVRG(const FroxelVoxData &data, io::WriteStream &out) {
	if (!out.writeUInt32(1) || !out.writeUInt32((uint32_t)data.bones.size())) {
		return false;
	}
	for (const FroxelBone &bone : data.bones) {
		if (!writeFroxelString(out, bone.name) || !writeFroxelString(out, bone.parent) ||
			!writeFroxelVec3(out, bone.pivot) || !writeFroxelQuat(out, bone.rotation)) {
			return false;
		}
	}
	if (!out.writeUInt32((uint32_t)data.binds.size())) {
		return false;
	}
	for (const FroxelBind &bind : data.binds) {
		if (!out.writeInt32(bind.trn) || !out.writeUInt32(bind.index) || !writeFroxelString(out, bind.name) ||
			!writeFroxelString(out, bind.bone)) {
			return false;
		}
	}
	return true;
}

bool encodeFroxelMVAN(const FroxelVoxData &data, io::WriteStream &out) {
	if (!out.writeUInt32(3) || !out.writeUInt32((uint32_t)data.clips.size())) {
		return false;
	}
	for (const FroxelClip &clip : data.clips) {
		if (!writeFroxelString(out, clip.name) || !out.writeUInt32((uint32_t)clip.tracks.size())) {
			return false;
		}
		for (const FroxelClipTrack &track : clip.tracks) {
			if (!writeFroxelString(out, track.bone) || !out.writeUInt32((uint32_t)track.keys.size())) {
				return false;
			}
			for (const FroxelClipKey &key : track.keys) {
				const uint8_t flags = (uint8_t)((key.hasTranslation ? 0x01 : 0) | (key.hasRotation ? 0x02 : 0));
				if (!out.writeFloat(key.time) || !out.writeUInt8(flags) || !writeFroxelQuat(out, key.rotation) ||
					!writeFroxelVec3(out, key.translation) || !writeFroxelVec3(out, key.scale)) {
					return false;
				}
			}
		}
	}
	return true;
}

bool encodeFroxelMVSC(const FroxelVoxData &data, io::WriteStream &out) {
	if (!out.writeUInt32(2) || !out.writeUInt32((uint32_t)data.scripts.size())) {
		return false;
	}
	for (const FroxelScript &script : data.scripts) {
		if (!writeFroxelString(out, script.name) || !out.writeInt32(script.localMin[0]) ||
			!out.writeInt32(script.localMin[1]) || !out.writeInt32(script.localMin[2]) ||
			!writeFroxelString(out, script.source) || !out.writeUInt8(script.autoRegen ? 1 : 0) ||
			!out.writeUInt32((uint32_t)script.params.size())) {
			return false;
		}
		for (const FroxelScriptParam &param : script.params) {
			if (!writeFroxelString(out, param.name) || !out.writeUInt8(param.kind)) {
				return false;
			}
			if (param.kind == FroxelParamFloat) {
				if (!out.writeFloat(param.fvalue) || !out.writeFloat(param.fmin) || !out.writeFloat(param.fmax)) {
					return false;
				}
			} else if (param.kind == FroxelParamInt) {
				if (!out.writeInt32(param.ivalue) || !out.writeInt32(param.imin) || !out.writeInt32(param.imax)) {
					return false;
				}
			} else if (param.kind == FroxelParamBool) {
				if (!out.writeUInt8(param.bvalue)) {
					return false;
				}
			} else if (param.kind == FroxelParamColor) {
				if (!out.writeUInt8(param.rgb[0]) || !out.writeUInt8(param.rgb[1]) || !out.writeUInt8(param.rgb[2])) {
					return false;
				}
			} else if (param.kind == FroxelParamFloats) {
				if (!out.writeUInt8(param.width) || !out.writeFloat(param.fmin) || !out.writeFloat(param.fmax) ||
					!out.writeUInt32(param.maxLen) || !out.writeUInt32((uint32_t)param.values.size())) {
					return false;
				}
				for (float value : param.values) {
					if (!out.writeFloat(value)) {
						return false;
					}
				}
			} else {
				return false;
			}
		}
	}
	return true;
}

void readFroxelCustomChunk(const ogt_vox_custom_chunk *chunk, void *userData) {
	if (chunk == nullptr || userData == nullptr) {
		return;
	}
	FroxelVoxData *data = (FroxelVoxData *)userData;
	if (chunk->id == FroxelChunkMVRG) {
		if (!parseFroxelMVRG(chunk->content, chunk->content_size, *data)) {
			Log::warn("Failed to parse Froxel MVRG chunk");
		}
	} else if (chunk->id == FroxelChunkMVAN) {
		if (!parseFroxelMVAN(chunk->content, chunk->content_size, *data)) {
			Log::warn("Failed to parse Froxel MVAN chunk");
		}
	} else if (chunk->id == FroxelChunkMVSC) {
		if (!parseFroxelMVSC(chunk->content, chunk->content_size, *data)) {
			Log::warn("Failed to parse Froxel MVSC chunk");
		}
	}
}

bool parseTeardownTDCZ(const uint8_t *content, uint32_t size, core::Buffer<uint8_t> &voxels, uint32_t &sizeX,
					   uint32_t &sizeY, uint32_t &sizeZ) {
	if (content == nullptr || size < 12) {
		return false;
	}
	io::MemoryReadStream stream(content, size);
	if (stream.readUInt32(sizeX) != 0 || stream.readUInt32(sizeY) != 0 || stream.readUInt32(sizeZ) != 0) {
		return false;
	}
	if (sizeX == 0 || sizeY == 0 || sizeZ == 0) {
		return false;
	}
	if (sizeY != 0 && sizeX > (UINT32_MAX / sizeY)) {
		return false;
	}
	const uint32_t xy = sizeX * sizeY;
	if (sizeZ != 0 && xy > (UINT32_MAX / sizeZ)) {
		return false;
	}
	const uint32_t voxelCount = xy * sizeZ;
	voxels.resize(voxelCount);
	io::ZipReadStream zip(stream, (int)stream.remaining());
	if (zip.read(voxels.data(), voxelCount) != (int)voxelCount) {
		Log::warn("Failed to decompress Teardown TDCZ chunk");
		voxels.resize(0);
		return false;
	}
	return true;
}

bool encodeTeardownTDCZ(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, const uint8_t *voxels, io::WriteStream &out) {
	if (voxels == nullptr || sizeX == 0 || sizeY == 0 || sizeZ == 0) {
		return false;
	}
	if (sizeY != 0 && sizeX > (UINT32_MAX / sizeY)) {
		return false;
	}
	const uint32_t xy = sizeX * sizeY;
	if (sizeZ != 0 && xy > (UINT32_MAX / sizeZ)) {
		return false;
	}
	const uint32_t voxelCount = xy * sizeZ;
	if (!out.writeUInt32(sizeX) || !out.writeUInt32(sizeY) || !out.writeUInt32(sizeZ)) {
		return false;
	}
	io::ZipWriteStream zip(out);
	if (zip.write(voxels, voxelCount) == -1 || !zip.flush()) {
		return false;
	}
	return true;
}

void readVoxCustomChunk(const ogt_vox_custom_chunk *chunk, void *userData, ogt_vox_custom_model *outModel) {
	if (chunk == nullptr || userData == nullptr) {
		return;
	}
	VoxCustomChunkContext *ctx = (VoxCustomChunkContext *)userData;
	if (chunk->id == TeardownChunkTDCZ) {
		uint32_t sizeX = 0;
		uint32_t sizeY = 0;
		uint32_t sizeZ = 0;
		if (!parseTeardownTDCZ(chunk->content, chunk->content_size, ctx->modelVoxels, sizeX, sizeY, sizeZ)) {
			Log::warn("Failed to parse Teardown TDCZ chunk");
			return;
		}
		if (outModel == nullptr) {
			return;
		}
		outModel->size_x = sizeX;
		outModel->size_y = sizeY;
		outModel->size_z = sizeZ;
		outModel->voxel_data = ctx->modelVoxels.data();
		return;
	}
	readFroxelCustomChunk(chunk, &ctx->froxel);
}

static glm::vec3 froxelVecToVengi(const float v[3]) {
	return glm::vec3(-v[0], v[2], v[1]);
}

static glm::vec3 froxelScaleToVengi(const float s[3]) {
	return glm::vec3(s[0], s[2], s[1]);
}

static glm::quat froxelQuatToVengi(const float r[4]) {
	const glm::quat qMv(r[3], r[0], r[1], r[2]);
	const glm::mat3 A(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::quat qA = glm::normalize(glm::quat_cast(A));
	return glm::normalize(qA * qMv * glm::conjugate(qA));
}

static int matchFroxelBind(const ogt_vox_scene *scene, const FroxelBind &bind,
						   const core::DynamicArray<int> &instanceNodeIds) {
	if (bind.trn >= 0) {
		int hits = 0;
		int found = InvalidNodeId;
		for (uint32_t i = 0; i < scene->num_instances; ++i) {
			if (scene->instances[i].node_id != bind.trn) {
				continue;
			}
			++hits;
			if (i < instanceNodeIds.size()) {
				found = instanceNodeIds[i];
			}
		}
		if (hits == 1) {
			return found;
		}
	}
	int hits = 0;
	int found = InvalidNodeId;
	for (uint32_t i = 0; i < scene->num_instances; ++i) {
		const char *name = scene->instances[i].name != nullptr ? scene->instances[i].name : "";
		if (bind.name != name) {
			continue;
		}
		++hits;
		if (i < instanceNodeIds.size()) {
			found = instanceNodeIds[i];
		}
	}
	if (hits == 1) {
		return found;
	}
	if (bind.index < instanceNodeIds.size() && bind.index < scene->num_instances) {
		const char *name = scene->instances[bind.index].name != nullptr ? scene->instances[bind.index].name : "";
		if (bind.name == name) {
			return instanceNodeIds[bind.index];
		}
	}
	return InvalidNodeId;
}

static core::String froxelParamToString(const FroxelScriptParam &param) {
	if (param.kind == FroxelParamFloat) {
		return core::String::format("float:%f:%f:%f", param.fvalue, param.fmin, param.fmax);
	}
	if (param.kind == FroxelParamInt) {
		return core::String::format("int:%i:%i:%i", param.ivalue, param.imin, param.imax);
	}
	if (param.kind == FroxelParamBool) {
		return core::String::format("bool:%u", (unsigned int)param.bvalue);
	}
	if (param.kind == FroxelParamColor) {
		return core::String::format("color:%u,%u,%u", (unsigned int)param.rgb[0], (unsigned int)param.rgb[1],
									(unsigned int)param.rgb[2]);
	}
	if (param.kind == FroxelParamFloats) {
		core::String out = core::String::format("floats:%u:%f:%f:%u", (unsigned int)param.width, param.fmin, param.fmax,
												param.maxLen);
		for (float value : param.values) {
			out.append(core::String::format(":%f", value));
		}
		return out;
	}
	return core::String::format("unknown:%u", (unsigned int)param.kind);
}

void loadFroxelExtensions(const FroxelVoxData &froxel, const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph,
						  const core::DynamicArray<int> &instanceNodeIds) {
	if (froxel.bones.empty() && froxel.clips.empty() && froxel.scripts.empty() && froxel.binds.empty()) {
		return;
	}

	core::DynamicArray<int> boneNodeIds;
	boneNodeIds.resize(froxel.bones.size());
	boneNodeIds.fill(InvalidNodeId);

	if (!froxel.bones.empty()) {
		scenegraph::SceneGraphNode rig(scenegraph::SceneGraphNodeType::Group);
		rig.setName("FroxelRig");
		const int rigGroupId = sceneGraph.emplace(core::move(rig), sceneGraph.root().id());
		if (rigGroupId == InvalidNodeId) {
			Log::warn("Failed to add Froxel rig group");
			return;
		}
		for (size_t i = 0; i < froxel.bones.size(); ++i) {
			const FroxelBone &bone = froxel.bones[i];
			scenegraph::SceneGraphNode point(scenegraph::SceneGraphNodeType::Point);
			point.setName(bone.name.empty() ? "bone" : bone.name);
			scenegraph::SceneGraphTransform transform;
			transform.setWorldTranslation(froxelVecToVengi(bone.pivot));
			transform.setLocalOrientation(froxelQuatToVengi(bone.rotation));
			point.setTransform(0, transform);
			const int nodeId = sceneGraph.emplace(core::move(point), rigGroupId);
			if (nodeId == InvalidNodeId) {
				Log::warn("Failed to add Froxel bone %s", bone.name.c_str());
				continue;
			}
			boneNodeIds[i] = nodeId;
		}
		for (size_t i = 0; i < froxel.bones.size(); ++i) {
			const FroxelBone &bone = froxel.bones[i];
			if (bone.parent.empty() || boneNodeIds[i] == InvalidNodeId) {
				continue;
			}
			for (size_t p = 0; p < froxel.bones.size(); ++p) {
				if (froxel.bones[p].name == bone.parent && boneNodeIds[p] != InvalidNodeId) {
					sceneGraph.changeParent(boneNodeIds[i], boneNodeIds[p]);
					break;
				}
			}
		}
		sceneGraph.updateTransforms();
	}

	for (const FroxelBind &bind : froxel.binds) {
		const int nodeId = matchFroxelBind(scene, bind, instanceNodeIds);
		if (nodeId == InvalidNodeId || !sceneGraph.hasNode(nodeId)) {
			Log::debug("Froxel bind for object %s did not match a node", bind.name.c_str());
			continue;
		}
		if (!bind.bone.empty()) {
			sceneGraph.node(nodeId).setProperty(scenegraph::PropFroxelBone, bind.bone);
		}
	}

	for (const FroxelScript &script : froxel.scripts) {
		scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName(script.name);
		if (node == nullptr) {
			Log::debug("Froxel script object %s was not found", script.name.c_str());
			continue;
		}
		if (!script.source.empty()) {
			node->setProperty(scenegraph::PropFroxelScript, script.source);
		}
		node->setProperty(scenegraph::PropFroxelScriptAuto, script.autoRegen);
		if (script.hasLocalMin) {
			node->setProperty(scenegraph::PropFroxelScriptLocalMin,
							  core::String::format("%i,%i,%i", script.localMin[0], script.localMin[1],
												   script.localMin[2]));
		}
		for (const FroxelScriptParam &param : script.params) {
			node->setProperty(core::String::format("froxel_param_%s", param.name.c_str()), froxelParamToString(param));
		}
	}

	const core::String previousAnim = sceneGraph.activeAnimation();
	for (size_t c = 0; c < froxel.clips.size(); ++c) {
		const FroxelClip &clip = froxel.clips[c];
		const core::String clipName = clip.name.empty() ? core::String::format("clip%u", (uint32_t)c) : clip.name;
		sceneGraph.addAnimation(clipName);
		for (const FroxelClipTrack &track : clip.tracks) {
			int boneNodeId = InvalidNodeId;
			for (size_t b = 0; b < froxel.bones.size(); ++b) {
				if (froxel.bones[b].name == track.bone) {
					boneNodeId = boneNodeIds[b];
					break;
				}
			}
			if (boneNodeId == InvalidNodeId || !sceneGraph.hasNode(boneNodeId)) {
				continue;
			}
			scenegraph::SceneGraphNode &boneNode = sceneGraph.node(boneNodeId);
			if (!boneNode.setAnimation(clipName)) {
				continue;
			}
			const scenegraph::SceneGraphTransform bindTransform = boneNode.transform(0);
			for (const FroxelClipKey &key : track.keys) {
				const scenegraph::FrameIndex frameIdx =
					(scenegraph::FrameIndex)(key.time * FroxelClipFps + (key.time >= 0.0f ? 0.5f : -0.5f));
				scenegraph::KeyFrameIndex kfIdx = boneNode.addKeyFrame(frameIdx);
				if (kfIdx == InvalidKeyFrame) {
					kfIdx = boneNode.keyFrameForFrame(frameIdx);
					if (kfIdx == InvalidKeyFrame) {
						continue;
					}
				}
				scenegraph::SceneGraphKeyFrame &kf = boneNode.keyFrame(kfIdx);
				kf.interpolation = scenegraph::InterpolationType::Linear;
				scenegraph::SceneGraphTransform &transform = kf.transform();
				if (key.hasTranslation) {
					transform.setLocalTranslation(froxelVecToVengi(key.translation));
				} else {
					transform.setLocalTranslation(bindTransform.localTranslation());
				}
				if (key.hasRotation) {
					transform.setLocalOrientation(froxelQuatToVengi(key.rotation));
				} else {
					transform.setLocalOrientation(bindTransform.localOrientation());
				}
				transform.setLocalScale(froxelScaleToVengi(key.scale));
			}
			sceneGraph.markKeyFramesDirty(boneNodeId);
		}
	}
	if (sceneGraph.hasAnimation(previousAnim)) {
		sceneGraph.setAnimation(previousAnim);
	}
	sceneGraph.updateTransforms();
}

} // namespace voxelformat
