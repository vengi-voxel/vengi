/**
 * @file
 */

#include "color/ColorUtil.h"
#include "FBXFormat.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"
#include "core/collection/Map.h"
#include "engine-config.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/TextureLookup.h"
#include <glm/gtc/type_ptr.hpp>
#include <limits>

#define ufbx_assert core_assert
#include "voxelformat/external/ufbx.h"

#define ufbxw_assert core_assert
#include "voxelformat/external/ufbx_write.h"

namespace voxelformat {

bool FBXFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph,
						   const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
						   const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	return saveMeshesBinary(meshes, filename, *stream, scale, quad, withColor, withTexCoords, sceneGraph);
}

bool FBXFormat::saveMeshesBinary(const ChunkMeshes &meshes, const core::String &filename,
								 io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad, bool withColor,
								 bool withTexCoords, const scenegraph::SceneGraph &sceneGraph) {
	ufbxw_scene_opts sceneOpts;
	core_memset(&sceneOpts, 0, sizeof(sceneOpts));
	if (quad) {
		Log::warn("FBX format does not support quads - exporting as triangles");
	}
	if (withColor) {
		Log::warn("FBX vertex colors are currently disabled due to a bug in ufbx_write");
	}
	(void)quad;
	ufbxw_scene *ws = ufbxw_create_scene(&sceneOpts);
	if (!ws) {
		Log::error("Failed to create ufbx_write scene");
		return false;
	}

	ufbxw_scene_set_coordinate_axes(ws, {UFBXW_COORDINATE_AXIS_POSITIVE_X, UFBXW_COORDINATE_AXIS_POSITIVE_Y,
										 UFBXW_COORDINATE_AXIS_POSITIVE_Z});
	ufbxw_scene_set_unit_scale_factor(ws, 1.0);

	// Map vengi node id -> ufbxw_node
	core::Map<int, ufbxw_node> nodeMap;

	// Create animation stacks and layers
	core::DynamicStringMap<ufbxw_anim_layer> animLayerMap;
	for (const core::String &animName : sceneGraph.animations()) {
		ufbxw_anim_stack wStack = ufbxw_create_anim_stack(ws);
		ufbxw_set_name(ws, wStack.id, animName.c_str());
		ufbxw_anim_layer layer = ufbxw_create_anim_layer(ws, wStack);
		ufbxw_set_name(ws, layer.id, animName.c_str());
		animLayerMap.put(animName, layer);
	}

	// Build mesh index: nodeId -> ChunkMeshExt index
	core::Map<int, int> meshNodeMap;
	for (size_t mi = 0; mi < meshes.size(); ++mi) {
		meshNodeMap.put(meshes[mi].nodeId, (int)mi);
	}

	const int fps = 30;

	// Recursive function to create nodes
	const auto createNodes = [&](const auto &self, const scenegraph::SceneGraphNode &sgNode,
								 ufbxw_node parentWNode) -> void {
		ufbxw_node wNode = ufbxw_create_node(ws);
		ufbxw_set_name(ws, wNode.id, sgNode.name().c_str());
		if (parentWNode.id != 0) {
			ufbxw_node_set_parent(ws, wNode, parentWNode);
		}
		nodeMap.put(sgNode.id(), wNode);

		// Set rest pose transform - only for non-mesh nodes or when applyTransform is false
		// (when applyTransform is true, vertices are already transformed)
		bool meshAppliesTransform = false;
		if (meshNodeMap.hasKey(sgNode.id())) {
			int mi = -1;
			meshNodeMap.get(sgNode.id(), mi);
			if (mi >= 0) {
				meshAppliesTransform = meshes[mi].applyTransform;
			}
		}

		const scenegraph::KeyFrameIndex kfIdx = 0;
		const scenegraph::SceneGraphTransform &transform = sgNode.transform(kfIdx);
		if (!meshAppliesTransform) {
			const glm::vec3 &t = transform.localTranslation();
			const glm::quat &r = transform.localOrientation();
			const glm::vec3 &s = transform.localScale();
			ufbxw_node_set_translation(ws, wNode, {t.x, t.y, t.z});
			ufbxw_node_set_rotation_quat(ws, wNode, {r.x, r.y, r.z, r.w}, UFBXW_ROTATION_ORDER_XYZ);
			ufbxw_node_set_scaling(ws, wNode, {s.x, s.y, s.z});
		}
		ufbxw_node_set_visibility(ws, wNode, sgNode.visible());

		// Attach mesh
		int meshIdx = -1;
		if (meshNodeMap.get(sgNode.id(), meshIdx)) {
			const ChunkMeshExt &meshExt = meshes[meshIdx];
			for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
				const voxel::Mesh *vmesh = &meshExt.mesh->mesh[i];
				if (vmesh->isEmpty()) {
					continue;
				}
				const int nv = (int)vmesh->getNoOfVertices();
				const int ni = (int)vmesh->getNoOfIndices();
				if (nv == 0 || ni == 0) {
					continue;
				}
				const voxel::VoxelVertex *vertices = vmesh->getRawVertexData();
				const voxel::IndexType *indices = vmesh->getRawIndexData();

				ufbxw_mesh wMesh = ufbxw_create_mesh(ws);
				ufbxw_node_set_attribute(ws, wNode, wMesh.id);

				core::DynamicArray<ufbxw_vec3> verts(nv);
				for (int j = 0; j < nv; ++j) {
					glm::vec3 pos;
					if (meshExt.applyTransform) {
						pos = transform.apply(vertices[j].position, meshExt.pivot * meshExt.size);
					} else {
						pos = vertices[j].position;
					}
					pos *= scale;
					verts[j] = {pos.x, pos.y, pos.z};
				}
				ufbxw_mesh_set_vertices(ws, wMesh, ufbxw_copy_vec3_array(ws, verts.data(), nv));

				core::DynamicArray<int32_t> triIndices(ni);
				for (int j = 0; j < ni; ++j) {
					triIndices[j] = (int32_t)indices[j];
				}
				ufbxw_mesh_set_triangles(ws, wMesh, ufbxw_copy_int_array(ws, triIndices.data(), ni));

				// TODO: vertex colors cause a crash in ufbx_write's ufbxwi_generate_indices
				// Re-enable once ufbx_write is fixed
				(void)withColor;
#if 0
				if (withColor) {
					const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
					const palette::Palette &palette = graphNode.palette();
					core::DynamicArray<ufbxw_vec4> colors(nv);
					for (int j = 0; j < nv; ++j) {
						const color::RGBA rgba = palette.color(vertices[j].colorIndex);
						colors[j] = {(double)rgba.r / 255.0, (double)rgba.g / 255.0,
									 (double)rgba.b / 255.0, (double)rgba.a / 255.0};
					}
					ufbxw_mesh_attribute_desc colorDesc;
					core_memset(&colorDesc, 0, sizeof(colorDesc));
					colorDesc.mapping = UFBXW_ATTRIBUTE_MAPPING_VERTEX;
					colorDesc.values = ufbxw_copy_vec4_array(ws, colors.data(), nv).id;
					colorDesc.generate_indices = true;
					ufbxw_mesh_set_attribute(ws, wMesh, UFBXW_MESH_ATTRIBUTE_COLOR, 0, &colorDesc);
				}
#endif

				if (withTexCoords) {
					const voxel::UVArray &uvs = vmesh->getUVVector();
					if (!uvs.empty() && meshExt.texture && meshExt.texture->isLoaded()) {
						core::DynamicArray<ufbxw_vec2> uvData(nv);
						for (int j = 0; j < nv; ++j) {
							uvData[j] = {(double)uvs[j].x, (double)(1.0f - uvs[j].y)};
						}
						ufbxw_mesh_attribute_desc uvDesc;
						core_memset(&uvDesc, 0, sizeof(uvDesc));
						uvDesc.mapping = UFBXW_ATTRIBUTE_MAPPING_VERTEX;
						uvDesc.values = ufbxw_copy_vec2_array(ws, uvData.data(), nv).id;
						uvDesc.generate_indices = true;
						ufbxw_mesh_set_attribute(ws, wMesh, UFBXW_MESH_ATTRIBUTE_UV, 0, &uvDesc);
					}
				}
			}
		}

		// Export animations
		for (const core::String &animName : sceneGraph.animations()) {
			if (!sgNode.allKeyFrames().hasKey(animName)) {
				continue;
			}
			const scenegraph::SceneGraphKeyFrames &kfs = sgNode.keyFrames(animName);
			if (kfs.size() <= 1) {
				continue;
			}
			ufbxw_anim_layer layer;
			if (!animLayerMap.get(animName, layer)) {
				continue;
			}
			ufbxw_anim_prop animT = ufbxw_node_animate_translation(ws, wNode, layer);
			ufbxw_anim_prop animR = ufbxw_node_animate_rotation(ws, wNode, layer);
			ufbxw_anim_prop animS = ufbxw_node_animate_scaling(ws, wNode, layer);
			for (const scenegraph::SceneGraphKeyFrame &kf : kfs) {
				const ufbxw_ktime time = (ufbxw_ktime)kf.frameIdx * UFBXW_KTIME_SECOND / fps;
				const glm::vec3 &kt = kf.transform().localTranslation();
				ufbxw_anim_add_keyframe_vec3(ws, animT, time, {kt.x, kt.y, kt.z}, UFBXW_KEYFRAME_LINEAR);
				const glm::quat &kr = kf.transform().localOrientation();
				const glm::vec3 euler = glm::degrees(glm::eulerAngles(kr));
				ufbxw_anim_add_keyframe_vec3(ws, animR, time, {euler.x, euler.y, euler.z}, UFBXW_KEYFRAME_LINEAR);
				const glm::vec3 &ks = kf.transform().localScale();
				ufbxw_anim_add_keyframe_vec3(ws, animS, time, {ks.x, ks.y, ks.z}, UFBXW_KEYFRAME_LINEAR);
			}
		}

		for (int childId : sgNode.children()) {
			self(self, sceneGraph.node(childId), wNode);
		}
	};

	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	for (int childId : root.children()) {
		createNodes(createNodes, sceneGraph.node(childId), ufbxw_null_node);
	}

	ufbxw_prepare_opts prepOpts = ufbxw_default_prepare_opts;
	ufbxw_prepare_scene(ws, &prepOpts);

	struct WriteCtx {
		io::SeekableWriteStream *stream;
	};
	WriteCtx wCtx;
	wCtx.stream = &stream;

	ufbxw_write_stream wStream;
	core_memset(&wStream, 0, sizeof(wStream));
	wStream.user = &wCtx;
	wStream.write_fn = [](void *user, uint64_t offset, const void *data, size_t size) -> bool {
		WriteCtx *c = (WriteCtx *)user;
		if (c->stream->seek((int64_t)offset) == -1) {
			return false;
		}
		return c->stream->write(data, size) == (int64_t)size;
	};

	ufbxw_save_opts saveOpts;
	core_memset(&saveOpts, 0, sizeof(saveOpts));
	saveOpts.format = UFBXW_SAVE_FORMAT_BINARY;

	ufbxw_error wError;
	const bool ok = ufbxw_save_stream(ws, &wStream, &saveOpts, &wError);
	if (!ok) {
		Log::error("Failed to save FBX binary: %s", wError.description);
	}

	ufbxw_free_scene(ws);
	return ok;
}

namespace priv {

static void *_ufbx_alloc(void *, size_t size) {
	return core_malloc(size);
}

static void _ufbx_free(void *, void *mem, size_t) {
	core_free(mem);
}

static void *_ufbx_realloc_fn(void *user, void *old_ptr, size_t old_size, size_t new_size) {
	return core_realloc(old_ptr, new_size);
}

static size_t _ufbx_read_fn(void *user, void *data, size_t size) {
	io::SeekableReadStream *stream = (io::SeekableReadStream *)user;
	const int ret = stream->read(data, size);
	if (ret < 0) {
		return 0;
	}
	return (size_t)ret;
}

static bool _ufbx_skip_fn(void *user, size_t size) {
	io::SeekableReadStream *stream = (io::SeekableReadStream *)user;
	return stream->skip((int64_t)size) != -1;
}

static uint64_t _ufbx_size_fn(void *user) {
	io::SeekableReadStream *stream = (io::SeekableReadStream *)user;
	return (uint64_t)stream->size();
}

static inline glm::vec2 _ufbx_to_vec2(const ufbx_vec2 &v) {
	return glm::vec2((float)v.x, (float)v.y);
}

static inline glm::vec3 _ufbx_to_vec3(const ufbx_vec3 &v) {
	return glm::vec3((float)v.x, (float)v.y, (float)v.z);
}

static inline glm::vec4 _ufbx_to_vec4(const ufbx_vec4 &v) {
	return glm::vec4((float)v.x, (float)v.y, (float)v.z, (float)v.w);
}

static inline core::String _ufbx_to_string(const ufbx_string &s) {
	return core::String(s.data, s.length);
}

// Convert a ufbx_vec3 from the scene's coordinate axes into engine axes
// (assumed right=X, up=Y, front=Z). The scene provides a `ufbx_coordinate_axes`
// struct where each component is one of the `ufbx_coordinate_axis` enum values
// (eg. UFBX_COORDINATE_AXIS_POSITIVE_Z). We map components and signs
// accordingly so we don't rely on ad-hoc swaps.
static inline ufbx_vec3 _ufbx_axes_to_engine(const ufbx_vec3 &v, const ufbx_coordinate_axes &axes) {
	auto get_comp = [&](ufbx_coordinate_axis a) -> float {
		switch (a) {
		case UFBX_COORDINATE_AXIS_POSITIVE_X:
			return v.x;
		case UFBX_COORDINATE_AXIS_NEGATIVE_X:
			return -v.x;
		case UFBX_COORDINATE_AXIS_POSITIVE_Y:
			return v.y;
		case UFBX_COORDINATE_AXIS_NEGATIVE_Y:
			return -v.y;
		case UFBX_COORDINATE_AXIS_POSITIVE_Z:
			return v.z;
		case UFBX_COORDINATE_AXIS_NEGATIVE_Z:
			return -v.z;
		default:
			return 0.0f;
		}
	};
	ufbx_vec3 out;
	out.x = get_comp(axes.right);
	out.y = get_comp(axes.up);
	out.z = get_comp(axes.front);
	return out;
}

static inline glm::quat _ufbx_to_quat(const ufbx_quat &v) {
	return glm::quat::wxyz((float)v.w, (float)v.x, (float)v.y, (float)v.z);
}

static inline void _ufbx_to_transform(scenegraph::SceneGraphTransform &transform, const ufbx_transform &ufbxTransform,
									  const glm::vec3 &scale) {
	transform.setLocalTranslation(priv::_ufbx_to_vec3(ufbxTransform.translation) * scale);
	transform.setLocalOrientation(priv::_ufbx_to_quat(ufbxTransform.rotation));
	transform.setLocalScale(priv::_ufbx_to_vec3(ufbxTransform.scale));
}

static inline void _ufbx_to_transform(scenegraph::SceneGraphTransform &transform, const ufbx_scene *ufbxScene,
									  const ufbx_node *ufbxNode, const glm::vec3 &scale) {
	_ufbx_to_transform(transform, ufbxNode->local_transform, scale);
}

static color::RGBA _ufbx_to_rgba(const ufbx_material_map &materialMap) {
	glm::vec4 color(1.0f);
	if (materialMap.value_components == 1) {
		color = glm::vec4(materialMap.value_real, materialMap.value_real, materialMap.value_real, 1.0f);
	} else if (materialMap.value_components == 3) {
		color = glm::vec4(_ufbx_to_vec3(materialMap.value_vec3), 1.0f);
	} else if (materialMap.value_components == 4) {
		color = _ufbx_to_vec4(materialMap.value_vec4);
	}
	return color::getRGBA(color);
}

} // namespace priv

int FBXFormat::addMeshNode(const ufbx_scene *ufbxScene, const ufbx_node *ufbxNode, const core::String &filename,
						   const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent) const {
	Log::debug("Add model node");
	ufbx_vec2 ufbxDefaultUV;
	core_memset(&ufbxDefaultUV, 0, sizeof(ufbxDefaultUV));
	const ufbx_mesh *ufbxMesh = ufbxNode->mesh;
	core_assert(ufbxMesh != nullptr);

	const size_t numTriIndices = ufbxMesh->max_face_triangles * 3;
	voxel::IndexArray triIndices(numTriIndices);

	Mesh mesh;
	mesh.vertices.reserve(numTriIndices);
	mesh.indices.reserve(numTriIndices);

	Log::debug("There are %i materials in the mesh", (int)ufbxMesh->materials.count);
	Log::debug("Vertex colors: %s", ufbxMesh->vertex_color.exists ? "true" : "false");
	Log::debug("UV coordinates: %s", ufbxMesh->vertex_uv.exists ? "true" : "false");
	Log::debug("Scene meter scale: %f", ufbxScene->settings.unit_meters);
	Log::debug("Scene original meter scale: %f", ufbxScene->settings.original_unit_meters);
	Log::debug("Scene original up axis: %i", ufbxScene->settings.original_axis_up);

	for (const ufbx_mesh_part &ufbxMeshPart : ufbxMesh->material_parts) {
		if (ufbxMeshPart.num_triangles == 0) {
			continue;
		}

		mesh.reserveAdditionalTris(ufbxMeshPart.num_triangles);

		const ufbx_material *ufbxMaterial = nullptr;
		if (ufbxMeshPart.index < ufbxMesh->materials.count) {
			ufbxMaterial = ufbxMesh->materials[ufbxMeshPart.index];
		}
		Log::debug("Faces: %i - material: %s (mesh part index: %u)", (int)ufbxMeshPart.num_faces,
				   ufbxMaterial ? "yes" : "no", ufbxMeshPart.index);

		MeshMaterialPtr mat = createMaterial("default");

		bool useUVs = ufbxMesh->vertex_uv.exists;
		if (ufbxMaterial) {
			const core::String &matname = priv::_ufbx_to_string(ufbxMaterial->name);
			if (matname.empty()) {
				Log::warn("No material name, using default");
			} else {
				mat = createMaterial(matname);
			}

			const ufbx_material_texture *ufbxMaterialTexture = nullptr;
			for (size_t i = 0; i < ufbxMaterial->textures.count; ++i) {
				ufbxMaterialTexture = &ufbxMaterial->textures[i];
				if (ufbxMaterialTexture) {
					break;
				}
			}

			const ufbx_texture *ufbxTexture = ufbxMaterialTexture ? ufbxMaterialTexture->texture : nullptr;
			if (ufbxTexture) {
				const core::String &fbxTextureFilename = priv::_ufbx_to_string(ufbxTexture->relative_filename);
				const core::String &textureName = lookupTexture(filename, fbxTextureFilename, archive);
				if (!textureName.empty()) {
					const image::ImagePtr &tex = image::loadImage(textureName);
					if (tex->isLoaded()) {
						Log::debug("Use image %s", textureName.c_str());
						mat->texture = tex;
					} else {
						useUVs = false;
					}
				} else {
					Log::debug("Failed to load image %s for material %s", fbxTextureFilename.c_str(), matname.c_str());
					useUVs = false;
				}
			} else if (useUVs) {
				Log::warn("Mesh has UV coordinates but no texture assigned in material %s", matname.c_str());
				useUVs = false;
			}
			if (ufbxMaterial->features.pbr.enabled) {
				if (ufbxMaterial->pbr.base_factor.has_value) {
					mat->baseColorFactor = (float)ufbxMaterial->pbr.base_factor.value_real;
				}
				if (ufbxMaterial->pbr.base_color.has_value) {
					mat->baseColor = priv::_ufbx_to_rgba(ufbxMaterial->pbr.base_color);
				}
				if (ufbxMaterial->pbr.metalness.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialMetal,
										   ufbxMaterial->pbr.metalness.value_real);
				}
				if (ufbxMaterial->pbr.roughness.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialRoughness,
										   ufbxMaterial->pbr.roughness.value_real);
				}
				if (ufbxMaterial->pbr.specular_ior.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialIndexOfRefraction,
										   ufbxMaterial->pbr.specular_ior.value_real);
				}
				if (ufbxMaterial->pbr.opacity.has_value) {
					mat->transparency = 1.0f - ufbxMaterial->pbr.opacity.value_real;
				}
				if (ufbxMaterial->pbr.glossiness.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialPhase,
										   ufbxMaterial->pbr.glossiness.value_real);
				}
				if (ufbxMaterial->pbr.specular_factor.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialSpecular,
										   ufbxMaterial->pbr.specular_factor.value_real);
				}
				if (ufbxMaterial->pbr.emission_factor.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialEmit,
										   ufbxMaterial->pbr.emission_factor.value_real);
				}
				if (ufbxMaterial->pbr.emission_color.has_value) {
					mat->emitColor = priv::_ufbx_to_rgba(ufbxMaterial->pbr.emission_color);
				}
			} else {
				// if (ufbxMaterial->fbx.diffuse_factor.has_value) {
				// 	mat->baseColorFactor = (float)ufbxMaterial->fbx.diffuse_factor.value_real;
				// }
				// if (ufbxMaterial->fbx.diffuse_color.has_value) {
				// 	mat->baseColor = priv::_ufbx_to_rgba(ufbxMaterial->fbx.diffuse_color);
				// }
				if (ufbxMaterial->fbx.specular_factor.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialSpecular,
										   ufbxMaterial->fbx.specular_factor.value_real);
				}
				if (ufbxMaterial->fbx.emission_factor.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialEmit,
										   ufbxMaterial->fbx.emission_factor.value_real);
				}
				if (ufbxMaterial->fbx.emission_color.has_value) {
					mat->emitColor = priv::_ufbx_to_rgba(ufbxMaterial->fbx.emission_color);
				}
				// if (ufbxMaterial->fbx.transparency_factor.has_value) {
				// 	mat->transparency = 1.0f - ufbxMaterial->fbx.transparency_factor.value_real;
				// }
			}
		} else {
			Log::debug("No material assigned for mesh");
			useUVs = false;
		}
		mesh.materials.emplace_back(core::move(mat));
		const MeshMaterialIndex materialIndex = mesh.materials.size() - 1;

		for (size_t fi = 0; fi < ufbxMeshPart.num_faces; fi++) {
			const ufbx_face ufbxFace = ufbxMesh->faces[ufbxMeshPart.face_indices[fi]];
			const size_t numTris = ufbx_triangulate_face(triIndices.data(), numTriIndices, ufbxMesh, ufbxFace);

			for (size_t vi = 0; vi < numTris; vi++) {
				voxelformat::MeshTri meshTri;
				meshTri.materialIdx = materialIndex;
				const uint32_t idx0 = triIndices[vi * 3 + 0];
				const uint32_t idx1 = triIndices[vi * 3 + 1];
				const uint32_t idx2 = triIndices[vi * 3 + 2];
				ufbx_vec3 vertex0 = ufbx_get_vertex_vec3(&ufbxMesh->vertex_position, idx0);
				ufbx_vec3 vertex1 = ufbx_get_vertex_vec3(&ufbxMesh->vertex_position, idx1);
				ufbx_vec3 vertex2 = ufbx_get_vertex_vec3(&ufbxMesh->vertex_position, idx2);

				vertex0 = priv::_ufbx_axes_to_engine(vertex0, ufbxScene->settings.axes);
				vertex1 = priv::_ufbx_axes_to_engine(vertex1, ufbxScene->settings.axes);
				vertex2 = priv::_ufbx_axes_to_engine(vertex2, ufbxScene->settings.axes);

				// TODO: VOXELFORMAT: transform here - see issue
				// https://github.com/vengi-voxel/vengi/issues/447
				meshTri.setVertices(priv::_ufbx_to_vec3(vertex0), priv::_ufbx_to_vec3(vertex1),
									priv::_ufbx_to_vec3(vertex2));
				if (ufbxMesh->vertex_color.exists) {
					const ufbx_vec4 &color0 = ufbx_get_vertex_vec4(&ufbxMesh->vertex_color, idx0);
					const ufbx_vec4 &color1 = ufbx_get_vertex_vec4(&ufbxMesh->vertex_color, idx1);
					const ufbx_vec4 &color2 = ufbx_get_vertex_vec4(&ufbxMesh->vertex_color, idx2);
					meshTri.setColor(color::getRGBA(priv::_ufbx_to_vec4(color0)),
									 color::getRGBA(priv::_ufbx_to_vec4(color1)),
									 color::getRGBA(priv::_ufbx_to_vec4(color2)));
				}
				if (useUVs) {
					const ufbx_vec2 &uv0 = ufbx_get_vertex_vec2(&ufbxMesh->vertex_uv, idx0);
					const ufbx_vec2 &uv1 = ufbx_get_vertex_vec2(&ufbxMesh->vertex_uv, idx1);
					const ufbx_vec2 &uv2 = ufbx_get_vertex_vec2(&ufbxMesh->vertex_uv, idx2);
					meshTri.setUVs(priv::_ufbx_to_vec2(uv0), priv::_ufbx_to_vec2(uv1), priv::_ufbx_to_vec2(uv2));
				}
				mesh.addTriangle(meshTri);
			}
		}
	}
	const core::String &name = priv::_ufbx_to_string(ufbxNode->name);
	const int nodeId = voxelizeMesh(name, sceneGraph, core::move(mesh), parent, true);
	if (nodeId < 0) {
		Log::error("Failed to voxelize node %s", name.c_str());
		return nodeId;
	}

	scenegraph::SceneGraphNode &sceneGraphNode = sceneGraph.node(nodeId);
	sceneGraphNode.setVisible(ufbxNode->visible);

	for (const ufbx_prop &ufbxProp : ufbxNode->props.props) {
		if ((ufbxProp.flags & UFBX_PROP_FLAG_NO_VALUE) != 0) {
			continue;
		}
		sceneGraphNode.setProperty(priv::_ufbx_to_string(ufbxProp.name), priv::_ufbx_to_string(ufbxProp.value_str));
	}
	return nodeId;
}

static scenegraph::FrameIndex _timeToFrame(double time, double timeBegin, int fps) {
	return core_max(0, (scenegraph::FrameIndex)((time - timeBegin) * fps + 0.5));
}

static void _insertFrameSorted(core::DynamicArray<scenegraph::FrameIndex> &frames, scenegraph::FrameIndex f) {
	for (size_t j = 0; j < frames.size(); ++j) {
		if (frames[j] == f) {
			return;
		}
		if (frames[j] > f) {
			frames.insert(frames.begin() + j, f);
			return;
		}
	}
	frames.push_back(f);
}

void FBXFormat::importAnimations(const ufbx_scene *ufbxScene, scenegraph::SceneGraph &sceneGraph,
								 const core::Map<const ufbx_node *, int> &ufbxNodeMap, const glm::vec3 &scale) const {
	const int fps = ufbxScene->settings.frames_per_second > 0 ? (int)ufbxScene->settings.frames_per_second : 30;

	for (const ufbx_anim_stack *stack : ufbxScene->anim_stacks) {
		const core::String &animId = priv::_ufbx_to_string(stack->name);

		ufbx_bake_opts bakeOpts;
		core_memset(&bakeOpts, 0, sizeof(bakeOpts));
		bakeOpts.temp_allocator.allocator.alloc_fn = priv::_ufbx_alloc;
		bakeOpts.temp_allocator.allocator.free_fn = priv::_ufbx_free;
		bakeOpts.temp_allocator.allocator.realloc_fn = priv::_ufbx_realloc_fn;
		bakeOpts.result_allocator.allocator.alloc_fn = priv::_ufbx_alloc;
		bakeOpts.result_allocator.allocator.free_fn = priv::_ufbx_free;
		bakeOpts.result_allocator.allocator.realloc_fn = priv::_ufbx_realloc_fn;
		bakeOpts.resample_rate = (double)fps;
		bakeOpts.key_reduction_enabled = true;
		bakeOpts.key_reduction_rotation = true;

		ufbx_error bakeError;
		ufbx_baked_anim *bake = ufbx_bake_anim(ufbxScene, stack->anim, &bakeOpts, &bakeError);
		if (!bake) {
			char err[512];
			ufbx_format_error(err, sizeof(err), &bakeError);
			Log::warn("Failed to bake animation '%s': %s", animId.c_str(), err);
			continue;
		}

		const double timeBegin = bake->key_time_min;
		const double keyDuration = bake->key_time_max - bake->key_time_min;
		const scenegraph::FrameIndex maxFrame = (scenegraph::FrameIndex)(keyDuration * fps);

		for (size_t ni = 0; ni < bake->nodes.count; ++ni) {
			const ufbx_baked_node &bakedNode = bake->nodes.data[ni];
			const ufbx_node *ufbxNode = ufbxScene->nodes.data[bakedNode.typed_id];

			int sgNodeId = InvalidNodeId;
			if (!ufbxNodeMap.get(ufbxNode, sgNodeId)) {
				continue;
			}
			if (!sceneGraph.hasNode(sgNodeId)) {
				continue;
			}
			scenegraph::SceneGraphNode &sceneGraphNode = sceneGraph.node(sgNodeId);

			if (!sceneGraphNode.setAnimation(animId)) {
				continue;
			}

			core::DynamicArray<scenegraph::FrameIndex> frames;
			for (size_t i = 0; i < bakedNode.translation_keys.count; ++i) {
				const scenegraph::FrameIndex f = _timeToFrame(bakedNode.translation_keys.data[i].time, timeBegin, fps);
				if (f <= maxFrame) {
					_insertFrameSorted(frames, f);
				}
			}
			for (size_t i = 0; i < bakedNode.rotation_keys.count; ++i) {
				const scenegraph::FrameIndex f = _timeToFrame(bakedNode.rotation_keys.data[i].time, timeBegin, fps);
				if (f <= maxFrame) {
					_insertFrameSorted(frames, f);
				}
			}
			for (size_t i = 0; i < bakedNode.scale_keys.count; ++i) {
				const scenegraph::FrameIndex f = _timeToFrame(bakedNode.scale_keys.data[i].time, timeBegin, fps);
				if (f <= maxFrame) {
					_insertFrameSorted(frames, f);
				}
			}

			if (frames.empty()) {
				continue;
			}

			Log::debug("Import %i keyframes for animation '%s' on node '%s'", (int)frames.size(), animId.c_str(),
					   sceneGraphNode.name().c_str());

			for (size_t i = 0; i < frames.size(); ++i) {
				const scenegraph::FrameIndex frameIdx = frames[i];
				const double time = timeBegin + (double)frameIdx / (double)fps;
				scenegraph::KeyFrameIndex kfIdx = sceneGraphNode.addKeyFrame(frameIdx);
				if (kfIdx == InvalidKeyFrame) {
					kfIdx = sceneGraphNode.keyFrameForFrame(frameIdx);
					if (kfIdx == InvalidKeyFrame) {
						continue;
					}
				}
				scenegraph::SceneGraphKeyFrame &kf = sceneGraphNode.keyFrame(kfIdx);
				kf.interpolation = scenegraph::InterpolationType::Linear;
				scenegraph::SceneGraphTransform &transform = kf.transform();
				const ufbx_vec3 t = ufbx_evaluate_baked_vec3(bakedNode.translation_keys, time);
				transform.setLocalTranslation(priv::_ufbx_to_vec3(t) * scale);
				const ufbx_quat r = ufbx_evaluate_baked_quat(bakedNode.rotation_keys, time);
				transform.setLocalOrientation(priv::_ufbx_to_quat(r));
				const ufbx_vec3 s = ufbx_evaluate_baked_vec3(bakedNode.scale_keys, time);
				transform.setLocalScale(priv::_ufbx_to_vec3(s));
			}
		}

		ufbx_free_baked_anim(bake);
	}
}

int FBXFormat::addGroupNode(const ufbx_scene *ufbxScene, const ufbx_node *ufbxNode, scenegraph::SceneGraph &sceneGraph,
							int parent, const glm::vec3 &scale) const {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
	node.setName(priv::_ufbx_to_string(ufbxNode->name));

	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphTransform &transform = node.keyFrame(keyFrameIdx).transform();
	priv::_ufbx_to_transform(transform, ufbxScene, ufbxNode, scale);
	node.setTransform(keyFrameIdx, transform);

	return sceneGraph.emplace(core::move(node), parent);
}

int FBXFormat::addCameraNode(const ufbx_scene *ufbxScene, const ufbx_node *ufbxNode, scenegraph::SceneGraph &sceneGraph,
							 int parent, const glm::vec3 &scale) const {
	Log::debug("Add camera node");
	const ufbx_camera *ufbxCamera = ufbxNode->camera;
	core_assert(ufbxCamera != nullptr);

	scenegraph::SceneGraphNodeCamera camNode;
	camNode.setName(priv::_ufbx_to_string(ufbxNode->name));
	camNode.setAspectRatio((float)ufbxCamera->aspect_ratio);
	camNode.setNearPlane((float)ufbxCamera->near_plane);
	camNode.setFarPlane((float)ufbxCamera->far_plane);
	if (ufbxCamera->projection_mode == UFBX_PROJECTION_MODE_PERSPECTIVE) {
		camNode.setPerspective();
		camNode.setFieldOfView((int)ufbxCamera->field_of_view_deg.x);
	} else if (ufbxCamera->projection_mode == UFBX_PROJECTION_MODE_ORTHOGRAPHIC) {
		camNode.setOrthographic();
		camNode.setWidth((int)ufbxCamera->orthographic_size.x);
		camNode.setHeight((int)ufbxCamera->orthographic_size.y);
	}
	scenegraph::SceneGraphTransform transform;
	priv::_ufbx_to_transform(transform, ufbxScene, ufbxNode, scale);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	camNode.setTransform(keyFrameIdx, transform);

	return sceneGraph.emplace(core::move(camNode), parent);
}

int FBXFormat::addNode_r(const ufbx_scene *ufbxScene, const ufbx_node *ufbxNode, const core::String &filename,
						 const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent,
						 const glm::vec3 &scale, core::Map<const ufbx_node *, int> &ufbxNodeMap) const {
	int nodeId = parent;
	if (ufbxNode->attrib_type == UFBX_ELEMENT_MESH) {
		nodeId = addMeshNode(ufbxScene, ufbxNode, filename, archive, sceneGraph, parent);
	} else if (ufbxNode->attrib_type == UFBX_ELEMENT_CAMERA) {
		nodeId = addCameraNode(ufbxScene, ufbxNode, sceneGraph, parent, scale);
	} else {
		nodeId = addGroupNode(ufbxScene, ufbxNode, sceneGraph, parent, scale);
	}
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to add node with parent %i", parent);
		return nodeId;
	}

	ufbxNodeMap.put(ufbxNode, nodeId);

	for (const ufbx_node *ufbxChildNode : ufbxNode->children) {
		const int newNodeId = addNode_r(ufbxScene, ufbxChildNode, filename, archive, sceneGraph, nodeId, scale, ufbxNodeMap);
		if (newNodeId == InvalidNodeId) {
			const core::String name = priv::_ufbx_to_string(ufbxChildNode->name);
			Log::warn("Failed to add child node '%s', skipping", name.c_str());
		}
	}
	return nodeId;
}

static void configureUFBXOpts(ufbx_load_opts &ufbxOpts, const core::String &filename) {
	core_memset(&ufbxOpts, 0, sizeof(ufbxOpts));

	ufbxOpts.temp_allocator.allocator.alloc_fn = priv::_ufbx_alloc;
	ufbxOpts.temp_allocator.allocator.free_fn = priv::_ufbx_free;
	ufbxOpts.temp_allocator.allocator.realloc_fn = priv::_ufbx_realloc_fn;

	ufbxOpts.result_allocator.allocator.alloc_fn = priv::_ufbx_alloc;
	ufbxOpts.result_allocator.allocator.free_fn = priv::_ufbx_free;
	ufbxOpts.result_allocator.allocator.realloc_fn = priv::_ufbx_realloc_fn;

	ufbxOpts.path_separator = '/';

	// TODO: VOXELFORMAT: see issue https://github.com/vengi-voxel/vengi/issues/227
	ufbxOpts.target_axes = ufbx_axes_right_handed_y_up;
	ufbxOpts.target_light_axes = ufbxOpts.target_axes;
	ufbxOpts.target_camera_axes = ufbxOpts.target_axes;
	ufbxOpts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	ufbxOpts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK;
	ufbxOpts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_IGNORE;
	ufbxOpts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT;
	ufbxOpts.generate_missing_normals = false; // we don't load them yet but generate them with the Tri class

	ufbxOpts.raw_filename.data = filename.c_str();
	ufbxOpts.raw_filename.size = filename.size();
}

static void configureUFBXStream(core::ScopedPtr<io::SeekableReadStream> &stream, ufbx_stream &ufbxStream) {
	core_memset(&ufbxStream, 0, sizeof(ufbxStream));
	ufbxStream.user = stream;
	ufbxStream.read_fn = priv::_ufbx_read_fn;
	ufbxStream.skip_fn = priv::_ufbx_skip_fn;
	ufbxStream.size_fn = priv::_ufbx_size_fn;
	ufbxStream.close_fn = nullptr;
}

bool FBXFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	ufbx_stream ufbxStream;
	configureUFBXStream(stream, ufbxStream);

	ufbx_load_opts ufbxOpts;
	configureUFBXOpts(ufbxOpts, filename);

	ufbx_error ufbxError;

	ufbx_scene *ufbxScene = ufbx_load_stream(&ufbxStream, &ufbxOpts, &ufbxError);
	if (ufbxError.type != UFBX_ERROR_NONE) {
		char err[4096];
		ufbx_format_error(err, sizeof(err), &ufbxError);
		Log::error("Error while loading fbx: %s", err);
	}
	if (!ufbxScene) {
		Log::error("Failed to load fbx scene: %s", ufbxError.description.data);
		return false;
	}

	Log::debug("right: %i, up: %i, front: %i", ufbxScene->settings.axes.right, ufbxScene->settings.axes.up,
			   ufbxScene->settings.axes.front);

	for (const ufbx_anim_stack *stack : ufbxScene->anim_stacks) {
		sceneGraph.addAnimation(priv::_ufbx_to_string(stack->name));
	}

	glm::vec3 sceneMins{std::numeric_limits<float>::max()};
	glm::vec3 sceneMaxs{std::numeric_limits<float>::lowest()};
	for (size_t mi = 0; mi < ufbxScene->meshes.count; ++mi) {
		const ufbx_mesh *ufbxMesh = ufbxScene->meshes[mi];
		for (size_t vi = 0; vi < ufbxMesh->vertex_position.values.count; ++vi) {
			const ufbx_vec3 v = priv::_ufbx_axes_to_engine(ufbxMesh->vertex_position.values[vi], ufbxScene->settings.axes);
			const glm::vec3 gv = priv::_ufbx_to_vec3(v);
			sceneMins = glm::min(sceneMins, gv);
			sceneMaxs = glm::max(sceneMaxs, gv);
		}
	}
	const glm::vec3 scale = getInputScale(sceneMins, sceneMaxs);

	core::Map<const ufbx_node *, int> ufbxNodeMap(64);
	if (addNode_r(ufbxScene, ufbxScene->root_node, filename, archive, sceneGraph, sceneGraph.root().id(), scale, ufbxNodeMap) < 0) {
		Log::error("Failed to add root child node");
		ufbx_free_scene(ufbxScene);
		return false;
	}

	importAnimations(ufbxScene, sceneGraph, ufbxNodeMap, scale);

	// Use skin deformers to parent mesh nodes under their primary bone.
	// This makes the bone animation propagate to the mesh nodes.
	sceneGraph.updateTransforms();
	for (size_t mi = 0; mi < ufbxScene->meshes.count; ++mi) {
		const ufbx_mesh *ufbxMesh = ufbxScene->meshes[mi];
		if (ufbxMesh->skin_deformers.count == 0) {
			continue;
		}
		// Find the mesh's ufbx_node
		const ufbx_node *meshNode = nullptr;
		for (size_t ni = 0; ni < ufbxMesh->instances.count; ++ni) {
			meshNode = ufbxMesh->instances[ni];
			break;
		}
		if (!meshNode) {
			continue;
		}
		// Find the vengi node for this mesh using the ufbx_node-to-nodeId map
		int meshNodeId = InvalidNodeId;
		if (!ufbxNodeMap.get(meshNode, meshNodeId)) {
			continue;
		}

		// Find the primary bone (highest total weight) from the first skin deformer
		const ufbx_skin_deformer *skin = ufbxMesh->skin_deformers[0];
		const ufbx_node *bestBone = nullptr;
		size_t bestCount = 0;
		for (const ufbx_skin_cluster *cluster : skin->clusters) {
			if (cluster->bone_node && cluster->num_weights > bestCount) {
				bestCount = cluster->num_weights;
				bestBone = cluster->bone_node;
			}
		}
		if (!bestBone) {
			continue;
		}

		// Find the vengi node for this bone using the ufbx_node-to-nodeId map
		int boneNodeId = InvalidNodeId;
		if (!ufbxNodeMap.get(bestBone, boneNodeId)) {
			continue;
		}

		const core::String meshName = priv::_ufbx_to_string(meshNode->name);
		const core::String boneName = priv::_ufbx_to_string(bestBone->name);
		Log::debug("Re-parenting mesh '%s' under bone '%s'", meshName.c_str(), boneName.c_str());
		sceneGraph.changeParent(meshNodeId, boneNodeId, scenegraph::NodeMoveFlag::KeepWorldTransform);

		// Copy Default keyframes to all other animations so the mesh stays at the
		// correct relative position regardless of which animation is active
		scenegraph::SceneGraphNode &meshSgNode = sceneGraph.node(meshNodeId);
		if (meshSgNode.allKeyFrames().hasKey(DEFAULT_ANIMATION)) {
			const scenegraph::SceneGraphKeyFrames defaultKfs = meshSgNode.keyFrames(DEFAULT_ANIMATION);
			for (const core::String &animName : sceneGraph.animations()) {
				if (animName == DEFAULT_ANIMATION) {
					continue;
				}
				meshSgNode.setAnimation(animName);
				meshSgNode.setKeyFrames(defaultKfs);
			}
			meshSgNode.setAnimation(DEFAULT_ANIMATION);
		}
	}

	sceneGraph.updateTransforms();
	sceneGraph.setAnimation(DEFAULT_ANIMATION);

	ufbx_free_scene(ufbxScene);
	return !sceneGraph.empty();
}

image::ImagePtr FBXFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return image::ImagePtr();
	}
	ufbx_stream ufbxStream;
	configureUFBXStream(stream, ufbxStream);

	ufbx_load_opts ufbxOpts;
	configureUFBXOpts(ufbxOpts, filename);

	ufbxOpts.raw_filename.data = filename.c_str();
	ufbxOpts.raw_filename.size = filename.size();

	ufbx_error ufbxError;

	ufbx_scene *ufbxScene = ufbx_load_stream(&ufbxStream, &ufbxOpts, &ufbxError);
	if (ufbxError.type != UFBX_ERROR_NONE) {
		char err[4096];
		ufbx_format_error(err, sizeof(err), &ufbxError);
		ufbx_free_scene(ufbxScene);
		Log::error("Error while loading fbx file %s: %s", filename.c_str(), err);
		return image::ImagePtr();
	}
	if (!ufbxScene) {
		Log::error("Failed to load fbx scene: %s", filename.c_str());
		return image::ImagePtr();
	}
	if (ufbxScene->metadata.thumbnail.width <= 0 || ufbxScene->metadata.thumbnail.height <= 0 ||
		ufbxScene->metadata.thumbnail.data.size <= 0) {
		Log::debug("Invalid thumbnail data in fbx file %s", filename.c_str());
		ufbx_free_scene(ufbxScene);
		return image::ImagePtr();
	}

	const int w = (int)ufbxScene->metadata.thumbnail.width;
	const int h = (int)ufbxScene->metadata.thumbnail.height;
	Log::debug("Found thumbnail in fbx file %s with size %ix%i", filename.c_str(), w, h);
	const int bpp = (int)ufbxScene->metadata.thumbnail.format == UFBX_THUMBNAIL_FORMAT_RGBA_32 ? 4 : 3;
	const int rowStride = w * bpp;
	image::ImagePtr img(image::createEmptyImage("screenshot"));
	img->resize(w, h);
	const uint8_t *src = (const uint8_t *)ufbxScene->metadata.thumbnail.data.data;
	for (int y = 0; y < h; ++y) {
		const uint8_t *s = src + (size_t)(h - 1 - y) * rowStride;
		io::MemoryReadStream rowStream(s, rowStride);
		for (int x = 0; x < w; ++x) {
			uint8_t r = 0, g = 0, b = 0, a = 255;
			if (bpp == 4) {
				rowStream.readUInt8(r);
				rowStream.readUInt8(g);
				rowStream.readUInt8(b);
				rowStream.readUInt8(a);
			} else {
				rowStream.readUInt8(r);
				rowStream.readUInt8(g);
				rowStream.readUInt8(b);
			}
			img->setColor(x, y, color::RGBA(r, g, b, a));
		}
	}
	img->markLoaded();
	ufbx_free_scene(ufbxScene);
	return img;
}

} // namespace voxelformat
