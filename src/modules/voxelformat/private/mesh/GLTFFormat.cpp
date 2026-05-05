/**
 * @file
 */

#include "GLTFFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "engine-config.h"
#include "image/Image.h"
#include "image/ImageType.h"
#include "io/Archive.h"
#include "io/Base64ReadStream.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/TextureLookup.h"
#include <glm/gtc/type_ptr.hpp>

#define CGLTF_MALLOC(size) core_malloc(size)
#define CGLTF_FREE(ptr) core_free(ptr)
#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "../../external/cgltf_write.h"

namespace voxelformat {

static const float GLTF_FPS = 24.0f;

MeshMaterialPtr GLTFFormat::loadMaterial(const cgltf_data *data, const cgltf_material *mat,
										 const core::String &filename, const io::ArchivePtr &archive) const {
	core::String name = mat->name ? mat->name : "default";
	MeshMaterialPtr meshMat = createMaterial(name);
	palette::Material &palMat = meshMat->material;

	if (mat->has_pbr_metallic_roughness) {
		const cgltf_pbr_metallic_roughness &pbr = mat->pbr_metallic_roughness;
		palMat.setValue(palette::MaterialProperty::MaterialMetal, pbr.metallic_factor);
		palMat.setValue(palette::MaterialProperty::MaterialRoughness, pbr.roughness_factor);
		meshMat->baseColor = color::RGBA(
			(uint8_t)(pbr.base_color_factor[0] * 255.0f),
			(uint8_t)(pbr.base_color_factor[1] * 255.0f),
			(uint8_t)(pbr.base_color_factor[2] * 255.0f),
			(uint8_t)(pbr.base_color_factor[3] * 255.0f));

		if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image) {
			const cgltf_image *img = pbr.base_color_texture.texture->image;
			if (img->buffer_view) {
				const uint8_t *bufData = (const uint8_t *)cgltf_buffer_view_data(img->buffer_view);
				if (bufData) {
					image::ImagePtr tex = image::createEmptyImage(name);
					io::MemoryReadStream stream(bufData, img->buffer_view->size);
					const char *mimeType = img->mime_type ? img->mime_type : "";
					image::ImageType imgType = image::ImageType::PNG;
					if (core::string::contains(mimeType, "jpeg") || core::string::contains(mimeType, "jpg")) {
						imgType = image::ImageType::JPEG;
					}
					if (tex->load(imgType, stream, (int)img->buffer_view->size)) {
						meshMat->texture = tex;
					}
				}
			} else if (img->uri) {
				if (core::string::startsWith(img->uri, "data:")) {
					// data URI - decode base64 image
					const core::String source = img->uri;
					const size_t commaPos = source.find(",");
					if (commaPos != core::String::npos) {
						const core::String &dataStr = source.substr(commaPos + 1);
						if (dataStr.size() >= 16) {
							io::MemoryReadStream dataStream(dataStr.c_str(), dataStr.size());
							io::Base64ReadStream base64Stream(dataStream);
							io::BufferedReadWriteStream bufferedStream(base64Stream, dataStr.size());
							image::ImagePtr tex = image::loadImage(name, bufferedStream);
							if (tex->isLoaded()) {
								meshMat->texture = tex;
							}
						}
					}
				} else {
					const core::String &texName = lookupTexture(filename, img->uri, archive);
					if (!texName.empty()) {
						image::ImagePtr tex = image::loadImage(texName);
						if (tex->isLoaded()) {
							meshMat->texture = tex;
						}
					}
				}
			}
		}
	}

	if (mat->has_pbr_specular_glossiness) {
		const cgltf_pbr_specular_glossiness &sg = mat->pbr_specular_glossiness;
		palMat.setValue(palette::MaterialProperty::MaterialDensity, sg.diffuse_factor[0]);
		palMat.setValue(palette::MaterialProperty::MaterialPhase, sg.glossiness_factor);
	}

	if (mat->has_ior) {
		palMat.setValue(palette::MaterialProperty::MaterialIndexOfRefraction, mat->ior.ior);
	}

	if (mat->has_specular) {
		palMat.setValue(palette::MaterialProperty::MaterialSpecular, mat->specular.specular_factor);
	}

	if (mat->has_volume) {
		palMat.setValue(palette::MaterialProperty::MaterialAttenuation, mat->volume.attenuation_distance > 0.0f ? 1.0f / mat->volume.attenuation_distance : 0.0f);
	}

	if (mat->has_emissive_strength) {
		palMat.setValue(palette::MaterialProperty::MaterialEmit, mat->emissive_strength.emissive_strength);
	} else if (mat->emissive_factor[0] > 0.0f || mat->emissive_factor[1] > 0.0f || mat->emissive_factor[2] > 0.0f) {
		float emit = (mat->emissive_factor[0] + mat->emissive_factor[1] + mat->emissive_factor[2]) / 3.0f;
		palMat.setValue(palette::MaterialProperty::MaterialEmit, emit);
	}

	meshMat->emitColor = color::RGBA(
		(uint8_t)(mat->emissive_factor[0] * 255.0f),
		(uint8_t)(mat->emissive_factor[1] * 255.0f),
		(uint8_t)(mat->emissive_factor[2] * 255.0f), 255);

	if (mat->alpha_mode == cgltf_alpha_mode_blend) {
		meshMat->transparency = 1.0f - mat->pbr_metallic_roughness.base_color_factor[3];
	}

	return meshMat;
}

int GLTFFormat::addNode_r(const cgltf_data *data, const cgltf_node *node, const core::String &filename,
						  const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent,
						  core::Map<const cgltf_node *, int> &nodeMap) const {
	int nodeId = parent;

	if (node->mesh) {
		const cgltf_mesh *gltfMesh = node->mesh;

		// Check for point cloud primitives first
		for (cgltf_size pi = 0; pi < gltfMesh->primitives_count; ++pi) {
			const cgltf_primitive &prim = gltfMesh->primitives[pi];
			if (prim.type == cgltf_primitive_type_points) {
				const cgltf_accessor *posAccessor = cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0);
				const cgltf_accessor *colAccessor = cgltf_find_accessor(&prim, cgltf_attribute_type_color, 0);
				if (!posAccessor) {
					continue;
				}
				PointCloud pointCloud;
				pointCloud.reserve((int)posAccessor->count);
				for (cgltf_size vi = 0; vi < posAccessor->count; ++vi) {
					float pos[3] = {0};
					cgltf_accessor_read_float(posAccessor, vi, pos, 3);
					PointCloudVertex v;
					v.position = glm::vec3(pos[0], pos[1], pos[2]);
					if (colAccessor) {
						float col[4] = {1, 1, 1, 1};
						cgltf_accessor_read_float(colAccessor, vi, col, 4);
						v.color = color::RGBA((uint8_t)(col[0] * 255.0f), (uint8_t)(col[1] * 255.0f),
											  (uint8_t)(col[2] * 255.0f), (uint8_t)(col[3] * 255.0f));
					} else {
						v.color = color::RGBA(255, 255, 255, 255);
					}
					pointCloud.push_back(v);
				}
				nodeId = voxelizePointCloud(filename, sceneGraph, core::move(pointCloud));
			}
		}

		// Combine all triangle primitives into a single mesh
		Mesh mesh;

		for (cgltf_size pi = 0; pi < gltfMesh->primitives_count; ++pi) {
			const cgltf_primitive &prim = gltfMesh->primitives[pi];
			if (prim.type != cgltf_primitive_type_triangles) {
				continue;
			}

			const cgltf_accessor *posAccessor = cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0);
			if (!posAccessor) {
				continue;
			}
			const cgltf_accessor *normalAccessor = cgltf_find_accessor(&prim, cgltf_attribute_type_normal, 0);
			const cgltf_accessor *uvAccessor = cgltf_find_accessor(&prim, cgltf_attribute_type_texcoord, 0);
			const cgltf_accessor *colAccessor = cgltf_find_accessor(&prim, cgltf_attribute_type_color, 0);

			MeshMaterialPtr mat = createMaterial("default");
			if (prim.material) {
				mat = loadMaterial(data, prim.material, filename, archive);
			}
			mesh.materials.push_back(mat);
			const MeshMaterialIndex materialIndex = (MeshMaterialIndex)(mesh.materials.size() - 1);

			const voxel::IndexType baseVertex = (voxel::IndexType)mesh.vertices.size();
			const cgltf_size vertexCount = posAccessor->count;

			for (cgltf_size vi = 0; vi < vertexCount; ++vi) {
				MeshVertex v;
				float pos[3] = {0};
				cgltf_accessor_read_float(posAccessor, vi, pos, 3);
				v.pos = glm::vec3(pos[0], pos[1], pos[2]);

				if (normalAccessor) {
					float n[3] = {0};
					cgltf_accessor_read_float(normalAccessor, vi, n, 3);
					v.normal = glm::vec3(n[0], n[1], n[2]);
				}
				if (uvAccessor) {
					float uv[2] = {0};
					cgltf_accessor_read_float(uvAccessor, vi, uv, 2);
					v.uv = glm::vec2(uv[0], uv[1]);
				}
				if (colAccessor) {
					float col[4] = {1, 1, 1, 1};
					cgltf_accessor_read_float(colAccessor, vi, col, 4);
					v.color = color::RGBA((uint8_t)(col[0] * 255.0f), (uint8_t)(col[1] * 255.0f),
										  (uint8_t)(col[2] * 255.0f), (uint8_t)(col[3] * 255.0f));
				} else {
					v.color = mat->baseColor;
				}
				v.materialIdx = materialIndex;
				mesh.vertices.push_back(v);
			}

			if (prim.indices) {
				for (cgltf_size ii = 0; ii < prim.indices->count; ++ii) {
					mesh.indices.push_back(baseVertex + (voxel::IndexType)cgltf_accessor_read_index(prim.indices, ii));
				}
			} else {
				for (cgltf_size ii = 0; ii < vertexCount; ++ii) {
					mesh.indices.push_back(baseVertex + (voxel::IndexType)ii);
				}
			}
		}

		if (!mesh.vertices.empty()) {
			core::String meshName = node->name ? node->name : "";
			if (meshName.empty() && gltfMesh->name) {
				meshName = gltfMesh->name;
			}
			if (meshName.empty()) {
				meshName = "mesh";
			}
			nodeId = voxelizeMesh(meshName, sceneGraph, core::move(mesh), parent, false);
			if (nodeId != InvalidNodeId && sceneGraph.hasNode(nodeId)) {
				scenegraph::SceneGraphNode &sgNode = sceneGraph.node(nodeId);
				scenegraph::SceneGraphTransform transform;
				if (node->has_matrix) {
					transform.setLocalMatrix(glm::make_mat4(node->matrix));
				} else {
					if (node->has_translation) {
						transform.setLocalTranslation(glm::vec3(node->translation[0], node->translation[1], node->translation[2]));
					}
					if (node->has_rotation) {
						transform.setLocalOrientation(glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]));
					}
					if (node->has_scale) {
						transform.setLocalScale(glm::vec3(node->scale[0], node->scale[1], node->scale[2]));
					}
				}
				sgNode.setTransform(0, transform);
			}
		}
	} else {
		// No mesh - just recurse into children with the same parent
		nodeId = parent;
		for (cgltf_size ci = 0; ci < node->children_count; ++ci) {
			addNode_r(data, node->children[ci], filename, archive, sceneGraph, nodeId, nodeMap);
		}
		nodeMap.put(node, nodeId);
		return nodeId;
	}

	for (cgltf_size ci = 0; ci < node->children_count; ++ci) {
		addNode_r(data, node->children[ci], filename, archive, sceneGraph, nodeId, nodeMap);
	}

	nodeMap.put(node, nodeId);
	return nodeId;
}

void GLTFFormat::importAnimations(const cgltf_data *data, scenegraph::SceneGraph &sceneGraph,
								  const core::Map<const cgltf_node *, int> &nodeMap) const {
	for (cgltf_size ai = 0; ai < data->animations_count; ++ai) {
		const cgltf_animation &anim = data->animations[ai];
		core::String animName = anim.name ? anim.name : core::String::format("animation %i", (int)ai);
		sceneGraph.addAnimation(animName);

		for (cgltf_size ci = 0; ci < anim.channels_count; ++ci) {
			const cgltf_animation_channel &channel = anim.channels[ci];
			if (!channel.target_node || !channel.sampler) {
				continue;
			}

			int sgNodeId = InvalidNodeId;
			if (!nodeMap.get(channel.target_node, sgNodeId)) {
				continue;
			}
			if (!sceneGraph.hasNode(sgNodeId)) {
				continue;
			}
			scenegraph::SceneGraphNode &sgNode = sceneGraph.node(sgNodeId);
			if (!sgNode.setAnimation(animName)) {
				continue;
			}

			const cgltf_animation_sampler *sampler = channel.sampler;
			const cgltf_accessor *input = sampler->input;
			const cgltf_accessor *output = sampler->output;
			if (!input || !output) {
				continue;
			}

			for (cgltf_size ki = 0; ki < input->count; ++ki) {
				float time = 0.0f;
				cgltf_accessor_read_float(input, ki, &time, 1);
				scenegraph::FrameIndex frameIdx = (scenegraph::FrameIndex)(time * GLTF_FPS + 0.5f);
				scenegraph::KeyFrameIndex kfIdx = sgNode.addKeyFrame(frameIdx);
				if (kfIdx == InvalidKeyFrame) {
					kfIdx = sgNode.keyFrameForFrame(frameIdx);
					if (kfIdx == InvalidKeyFrame) {
						continue;
					}
				}
				scenegraph::SceneGraphKeyFrame &kf = sgNode.keyFrame(kfIdx);
				kf.interpolation = scenegraph::InterpolationType::Linear;
				scenegraph::SceneGraphTransform &transform = kf.transform();

				switch (channel.target_path) {
				case cgltf_animation_path_type_translation: {
					float v[3] = {0};
					cgltf_accessor_read_float(output, ki, v, 3);
					transform.setLocalTranslation(glm::vec3(v[0], v[1], v[2]));
					break;
				}
				case cgltf_animation_path_type_rotation: {
					float v[4] = {0, 0, 0, 1};
					cgltf_accessor_read_float(output, ki, v, 4);
					transform.setLocalOrientation(glm::quat(v[3], v[0], v[1], v[2]));
					break;
				}
				case cgltf_animation_path_type_scale: {
					float v[3] = {1, 1, 1};
					cgltf_accessor_read_float(output, ki, v, 3);
					transform.setLocalScale(glm::vec3(v[0], v[1], v[2]));
					break;
				}
				default:
					break;
				}
			}
		}
	}
}

bool GLTFFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	const int64_t size = stream->size();
	uint8_t *buf = (uint8_t *)core_malloc(size);
	if (stream->read(buf, size) != size) {
		core_free(buf);
		Log::error("Failed to read %s", filename.c_str());
		return false;
	}

	cgltf_options options;
	core_memset(&options, 0, sizeof(options));
	cgltf_data *data = nullptr;
	cgltf_result result = cgltf_parse(&options, buf, size, &data);
	if (result != cgltf_result_success) {
		core_free(buf);
		Log::error("Failed to parse gltf file %s: %i", filename.c_str(), (int)result);
		return false;
	}

	// Load buffer data - for GLB the data is already in the parsed file,
	// for .gltf we need to load external .bin files through the archive
	if (data->file_type == cgltf_file_type_glb) {
		// For GLB, buffer data is embedded - cgltf_parse already handles this
		// but we still need to call cgltf_load_buffers for data URI handling
		result = cgltf_load_buffers(&options, data, ".");
		if (result != cgltf_result_success) {
			// Try without path for embedded buffers
			for (cgltf_size bi = 0; bi < data->buffers_count; ++bi) {
				if (data->buffers[bi].data == nullptr && data->buffers[bi].uri == nullptr) {
					// embedded GLB buffer - data should come from the bin chunk
					data->buffers[bi].data = (void *)((uint8_t *)data->file_data + (size_t)data->bin);
				}
			}
		}
	} else {
		// For .gltf, load external buffer files through the archive
		const core::String dir = core::string::extractDir(filename);
		for (cgltf_size bi = 0; bi < data->buffers_count; ++bi) {
			cgltf_buffer *buffer = &data->buffers[bi];
			if (buffer->data != nullptr) {
				continue;
			}
			if (buffer->uri == nullptr) {
				continue;
			}
			// Skip data URIs - let cgltf handle those
			if (core::string::startsWith(buffer->uri, "data:")) {
				continue;
			}
			core::String bufferPath = dir + buffer->uri;
			core::ScopedPtr<io::SeekableReadStream> bufStream(archive->readStream(bufferPath));
			if (!bufStream) {
				cgltf_free(data);
				core_free(buf);
				Log::error("Failed to load buffer %s", bufferPath.c_str());
				return false;
			}
			const int64_t bufSize = bufStream->size();
			void *bufData = core_malloc(bufSize);
			if (bufStream->read(bufData, bufSize) != bufSize) {
				core_free(bufData);
				cgltf_free(data);
				core_free(buf);
				Log::error("Failed to read buffer %s", bufferPath.c_str());
				return false;
			}
			buffer->data = bufData;
			buffer->data_free_method = cgltf_data_free_method_memory_free;
		}
		// Handle data URIs via cgltf's built-in base64 decoder
		for (cgltf_size bi = 0; bi < data->buffers_count; ++bi) {
			cgltf_buffer *buffer = &data->buffers[bi];
			if (buffer->data != nullptr || buffer->uri == nullptr) {
				continue;
			}
			if (core::string::startsWith(buffer->uri, "data:")) {
				const char *uri = buffer->uri;
				while (*uri && *uri != ',') {
					++uri;
				}
				if (*uri == ',') {
					cgltf_load_buffer_base64(&options, buffer->size, uri + 1, &buffer->data);
					if (buffer->data) {
						buffer->data_free_method = cgltf_data_free_method_memory_free;
					}
				}
			}
		}
	}

	// Build a map from cgltf_node* to scenegraph node id for animation import
	core::Map<const cgltf_node *, int> nodeMap(64);

	const cgltf_scene *scene = data->scene ? data->scene : &data->scenes[0];
	for (cgltf_size ni = 0; ni < scene->nodes_count; ++ni) {
		const cgltf_node *node = scene->nodes[ni];
		addNode_r(data, node, filename, archive, sceneGraph, 0, nodeMap);
	}

	if (data->animations_count > 0) {
		importAnimations(data, sceneGraph, nodeMap);
	}

	cgltf_free(data);
	core_free(buf);
	return !sceneGraph.empty();
}

bool GLTFFormat::saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
						   const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
						   const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	// Count total meshes
	int totalMeshes = 0;
	int totalAccessors = 0;
	int totalBufferViews = 0;
	for (const ChunkMeshExt &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			if (!meshExt.mesh->mesh[i].isEmpty()) {
				++totalMeshes;
			}
		}
	}
	if (totalMeshes == 0) {
		Log::error("No meshes to export");
		return false;
	}

	// Calculate buffer size needed
	size_t bufferSize = 0;
	struct MeshInfo {
		int meshExtIdx;
		int subMeshIdx;
		size_t vertexOffset;
		size_t vertexSize;
		size_t indexOffset;
		size_t indexSize;
		int vertexCount;
		int indexCount;
		bool hasTexture;
		int floatsPerVertex; // 7 without UV, 9 with UV
	};
	core::DynamicArray<MeshInfo> meshInfos;
	meshInfos.reserve(totalMeshes);

	// Check if output is GLB (textures not supported in this implementation for GLB)
	const bool outputIsGlb = core::string::extractExtension(filename) == "glb";

	for (int mi = 0; mi < (int)meshes.size(); ++mi) {
		const ChunkMeshExt &meshExt = meshes[mi];
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
			if (mesh->isEmpty()) {
				continue;
			}
			MeshInfo info;
			info.meshExtIdx = mi;
			info.subMeshIdx = i;
			info.vertexCount = (int)mesh->getNoOfVertices();
			info.indexCount = (int)mesh->getNoOfIndices();
			info.hasTexture = !outputIsGlb && meshExt.texture && meshExt.texture->isLoaded() && !mesh->getUVVector().empty();
			info.floatsPerVertex = info.hasTexture ? 9 : 7; // pos(3) + color(4) + uv(2)
			info.vertexOffset = bufferSize;
			info.vertexSize = info.vertexCount * info.floatsPerVertex * sizeof(float);
			bufferSize += info.vertexSize;
			// index data
			info.indexOffset = bufferSize;
			info.indexSize = info.indexCount * sizeof(uint32_t);
			bufferSize += info.indexSize;
			meshInfos.push_back(info);
		}
	}

	// Allocate and fill buffer
	uint8_t *buffer = (uint8_t *)core_malloc(bufferSize);
	for (const MeshInfo &info : meshInfos) {
		const ChunkMeshExt &meshExt = meshes[info.meshExtIdx];
		const voxel::Mesh *mesh = &meshExt.mesh->mesh[info.subMeshIdx];
		const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
		const voxel::IndexType *indices = mesh->getRawIndexData();
		const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
		const palette::Palette &palette = graphNode.palette();

		float *vBuf = (float *)(buffer + info.vertexOffset);
		const int stride = info.floatsPerVertex;
		const voxel::UVArray &uvs = mesh->getUVVector();
		const glm::vec3 pivotOffset = glm::vec3(mesh->getOffset()) - meshExt.pivot * meshExt.size;
		for (int j = 0; j < info.vertexCount; ++j) {
			glm::vec3 pos = vertices[j].position;
			if (meshExt.applyTransform) {
				pos += pivotOffset;
			}
			pos *= scale;
			vBuf[j * stride + 0] = pos.x;
			vBuf[j * stride + 1] = pos.y;
			vBuf[j * stride + 2] = pos.z;
			const color::RGBA rgba = palette.color(vertices[j].colorIndex);
			vBuf[j * stride + 3] = (float)rgba.r / 255.0f;
			vBuf[j * stride + 4] = (float)rgba.g / 255.0f;
			vBuf[j * stride + 5] = (float)rgba.b / 255.0f;
			vBuf[j * stride + 6] = (float)rgba.a / 255.0f;
			if (info.hasTexture) {
				vBuf[j * stride + 7] = uvs[j].x;
				vBuf[j * stride + 8] = 1.0f - uvs[j].y; // GLTF uses top-left origin
			}
		}

		uint32_t *iBuf = (uint32_t *)(buffer + info.indexOffset);
		for (int j = 0; j < info.indexCount; ++j) {
			iBuf[j] = (uint32_t)indices[j];
		}
	}

	// Build cgltf_data
	int texturedMeshCount = 0;
	for (const MeshInfo &info : meshInfos) {
		if (info.hasTexture) ++texturedMeshCount;
	}
	totalAccessors = totalMeshes * 3 + texturedMeshCount; // position, color, indices + texcoord per textured mesh
	totalBufferViews = totalMeshes * 3 + texturedMeshCount;

	cgltf_buffer gltfBuffer;
	core_memset(&gltfBuffer, 0, sizeof(gltfBuffer));
	gltfBuffer.size = bufferSize;
	gltfBuffer.data = buffer;

	// Allocate arrays
	cgltf_buffer_view *bufferViews = (cgltf_buffer_view *)core_malloc(totalBufferViews * sizeof(cgltf_buffer_view));
	core_memset(bufferViews, 0, totalBufferViews * sizeof(cgltf_buffer_view));
	cgltf_accessor *accessors = (cgltf_accessor *)core_malloc(totalAccessors * sizeof(cgltf_accessor));
	core_memset(accessors, 0, totalAccessors * sizeof(cgltf_accessor));
	cgltf_mesh *gltfMeshes = (cgltf_mesh *)core_malloc(totalMeshes * sizeof(cgltf_mesh));
	core_memset(gltfMeshes, 0, totalMeshes * sizeof(cgltf_mesh));
	cgltf_primitive *primitives = (cgltf_primitive *)core_malloc(totalMeshes * sizeof(cgltf_primitive));
	core_memset(primitives, 0, totalMeshes * sizeof(cgltf_primitive));
	int maxAttrsPerMesh = 3; // POSITION, COLOR_0, TEXCOORD_0
	cgltf_attribute *attributes = (cgltf_attribute *)core_malloc(totalMeshes * maxAttrsPerMesh * sizeof(cgltf_attribute));
	core_memset(attributes, 0, totalMeshes * maxAttrsPerMesh * sizeof(cgltf_attribute));
	cgltf_node *nodes = (cgltf_node *)core_malloc(totalMeshes * sizeof(cgltf_node));
	core_memset(nodes, 0, totalMeshes * sizeof(cgltf_node));
	cgltf_node **sceneNodes = (cgltf_node **)core_malloc(totalMeshes * sizeof(cgltf_node *));
	core_memset(sceneNodes, 0, totalMeshes * sizeof(cgltf_node *));

	// Texture/material/image arrays for textured meshes
	cgltf_image *gltfImages = nullptr;
	cgltf_texture *gltfTextures = nullptr;
	cgltf_material *gltfMaterials = nullptr;
	if (texturedMeshCount > 0) {
		gltfImages = (cgltf_image *)core_malloc(texturedMeshCount * sizeof(cgltf_image));
		core_memset(gltfImages, 0, texturedMeshCount * sizeof(cgltf_image));
		gltfTextures = (cgltf_texture *)core_malloc(texturedMeshCount * sizeof(cgltf_texture));
		core_memset(gltfTextures, 0, texturedMeshCount * sizeof(cgltf_texture));
		gltfMaterials = (cgltf_material *)core_malloc(texturedMeshCount * sizeof(cgltf_material));
		core_memset(gltfMaterials, 0, texturedMeshCount * sizeof(cgltf_material));
	}

	int bvIdx = 0, accIdx = 0, texIdx = 0;
	for (int mi = 0; mi < (int)meshInfos.size(); ++mi) {
		const MeshInfo &info = meshInfos[mi];
		const int stride = info.floatsPerVertex;
		const int strideBytes = stride * (int)sizeof(float);

		// Position buffer view (interleaved, stride = floatsPerVertex * 4)
		bufferViews[bvIdx].buffer = &gltfBuffer;
		bufferViews[bvIdx].offset = info.vertexOffset;
		bufferViews[bvIdx].size = info.vertexSize;
		bufferViews[bvIdx].stride = strideBytes;
		bufferViews[bvIdx].type = cgltf_buffer_view_type_vertices;

		// Position accessor
		accessors[accIdx].buffer_view = &bufferViews[bvIdx];
		accessors[accIdx].component_type = cgltf_component_type_r_32f;
		accessors[accIdx].type = cgltf_type_vec3;
		accessors[accIdx].count = info.vertexCount;
		/* stride is defined on buffer view */
		accessors[accIdx].has_min = true;
		accessors[accIdx].has_max = true;
		float *vBuf = (float *)(buffer + info.vertexOffset);
		accessors[accIdx].min[0] = accessors[accIdx].min[1] = accessors[accIdx].min[2] = 1e30f;
		accessors[accIdx].max[0] = accessors[accIdx].max[1] = accessors[accIdx].max[2] = -1e30f;
		for (int j = 0; j < info.vertexCount; ++j) {
			for (int k = 0; k < 3; ++k) {
				float v = vBuf[j * stride + k];
				if (v < accessors[accIdx].min[k]) accessors[accIdx].min[k] = v;
				if (v > accessors[accIdx].max[k]) accessors[accIdx].max[k] = v;
			}
		}
		int posAccIdx = accIdx;
		++bvIdx; ++accIdx;

		// Color buffer view (same interleaved view, different offset in accessor)
		bufferViews[bvIdx].buffer = &gltfBuffer;
		bufferViews[bvIdx].offset = info.vertexOffset;
		bufferViews[bvIdx].size = info.vertexSize;
		bufferViews[bvIdx].stride = strideBytes;
		bufferViews[bvIdx].type = cgltf_buffer_view_type_vertices;

		// Color accessor
		accessors[accIdx].buffer_view = &bufferViews[bvIdx];
		accessors[accIdx].component_type = cgltf_component_type_r_32f;
		accessors[accIdx].type = cgltf_type_vec4;
		accessors[accIdx].count = info.vertexCount;
		accessors[accIdx].offset = 3 * sizeof(float);
		/* stride is defined on buffer view */
		int colAccIdx = accIdx;
		++bvIdx; ++accIdx;

		// Texcoord buffer view + accessor (if textured)
		int uvAccIdx = -1;
		if (info.hasTexture) {
			bufferViews[bvIdx].buffer = &gltfBuffer;
			bufferViews[bvIdx].offset = info.vertexOffset;
			bufferViews[bvIdx].size = info.vertexSize;
			bufferViews[bvIdx].stride = strideBytes;
			bufferViews[bvIdx].type = cgltf_buffer_view_type_vertices;

			accessors[accIdx].buffer_view = &bufferViews[bvIdx];
			accessors[accIdx].component_type = cgltf_component_type_r_32f;
			accessors[accIdx].type = cgltf_type_vec2;
			accessors[accIdx].count = info.vertexCount;
			accessors[accIdx].offset = 7 * sizeof(float);
			/* stride is defined on buffer view */
			uvAccIdx = accIdx;
			++bvIdx; ++accIdx;
		}

		// Index buffer view
		bufferViews[bvIdx].buffer = &gltfBuffer;
		bufferViews[bvIdx].offset = info.indexOffset;
		bufferViews[bvIdx].size = info.indexSize;
		bufferViews[bvIdx].type = cgltf_buffer_view_type_indices;

		// Index accessor
		accessors[accIdx].buffer_view = &bufferViews[bvIdx];
		accessors[accIdx].component_type = cgltf_component_type_r_32u;
		accessors[accIdx].type = cgltf_type_scalar;
		accessors[accIdx].count = info.indexCount;
		int idxAccIdx = accIdx;
		++bvIdx; ++accIdx;

		// Primitive
		int attrBase = mi * maxAttrsPerMesh;
		attributes[attrBase + 0].name = (char *)"POSITION";
		attributes[attrBase + 0].type = cgltf_attribute_type_position;
		attributes[attrBase + 0].data = &accessors[posAccIdx];
		attributes[attrBase + 1].name = (char *)"COLOR_0";
		attributes[attrBase + 1].type = cgltf_attribute_type_color;
		attributes[attrBase + 1].data = &accessors[colAccIdx];
		int attrCount = 2;
		if (info.hasTexture && uvAccIdx >= 0) {
			attributes[attrBase + 2].name = (char *)"TEXCOORD_0";
			attributes[attrBase + 2].type = cgltf_attribute_type_texcoord;
			attributes[attrBase + 2].data = &accessors[uvAccIdx];
			attrCount = 3;

			// Set up image/texture/material for this mesh
			const ChunkMeshExt &meshExt = meshes[info.meshExtIdx];
			// Write texture as PNG to archive
			const core::String texName = core::String::format("texture_%i.png", texIdx);
			core::ScopedPtr<io::SeekableWriteStream> texStream(archive->writeStream(
				core::string::extractDir(filename) + texName));
			if (texStream && meshExt.texture) {
				image::writePNG(meshExt.texture, *texStream);
			}
			gltfImages[texIdx].uri = (char *)core_malloc(texName.size() + 1);
			core_memcpy(gltfImages[texIdx].uri, texName.c_str(), texName.size() + 1);
			gltfTextures[texIdx].image = &gltfImages[texIdx];
			gltfMaterials[texIdx].has_pbr_metallic_roughness = true;
			gltfMaterials[texIdx].pbr_metallic_roughness.base_color_texture.texture = &gltfTextures[texIdx];
			gltfMaterials[texIdx].pbr_metallic_roughness.base_color_factor[0] = 1.0f;
			gltfMaterials[texIdx].pbr_metallic_roughness.base_color_factor[1] = 1.0f;
			gltfMaterials[texIdx].pbr_metallic_roughness.base_color_factor[2] = 1.0f;
			gltfMaterials[texIdx].pbr_metallic_roughness.base_color_factor[3] = 1.0f;
			primitives[mi].material = &gltfMaterials[texIdx];
			++texIdx;
		}

		primitives[mi].type = cgltf_primitive_type_triangles;
		primitives[mi].indices = &accessors[idxAccIdx];
		primitives[mi].attributes = &attributes[attrBase];
		primitives[mi].attributes_count = attrCount;

		// Mesh
		gltfMeshes[mi].primitives = &primitives[mi];
		gltfMeshes[mi].primitives_count = 1;

		// Node
		nodes[mi].mesh = &gltfMeshes[mi];
		nodes[mi].name = (char *)meshes[info.meshExtIdx].name.c_str();
		{
			const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(meshes[info.meshExtIdx].nodeId);
			const scenegraph::SceneGraphTransform &transform = graphNode.transform(0);
			glm::mat4x4 nodeLocalMatrix = transform.localMatrix();
			if (nodeLocalMatrix != glm::mat4(1.0f)) {
				nodes[mi].has_matrix = true;
				const float *pSource = (const float *)glm::value_ptr(nodeLocalMatrix);
				for (int k = 0; k < 16; ++k) {
					nodes[mi].matrix[k] = pSource[k];
				}
			}
		}
		sceneNodes[mi] = &nodes[mi];
	}

	// Scene
	cgltf_scene gltfScene;
	core_memset(&gltfScene, 0, sizeof(gltfScene));
	gltfScene.nodes = sceneNodes;
	gltfScene.nodes_count = totalMeshes;

	// Data
	cgltf_data gltfData;
	core_memset(&gltfData, 0, sizeof(gltfData));
	gltfData.asset.version = (char *)"2.0";
	gltfData.asset.generator = (char *)"vengi " PROJECT_VERSION;
	gltfData.meshes = gltfMeshes;
	gltfData.meshes_count = totalMeshes;
	gltfData.accessors = accessors;
	gltfData.accessors_count = totalAccessors;
	gltfData.buffer_views = bufferViews;
	gltfData.buffer_views_count = totalBufferViews;
	gltfData.buffers = &gltfBuffer;
	gltfData.buffers_count = 1;
	gltfData.nodes = nodes;
	gltfData.nodes_count = totalMeshes;
	gltfData.scenes = &gltfScene;
	gltfData.scenes_count = 1;
	gltfData.scene = &gltfScene;
	if (texturedMeshCount > 0) {
		gltfData.images = gltfImages;
		gltfData.images_count = texturedMeshCount;
		gltfData.textures = gltfTextures;
		gltfData.textures_count = texturedMeshCount;
		gltfData.materials = gltfMaterials;
		gltfData.materials_count = texturedMeshCount;
	}

	// Export animations
	const scenegraph::SceneGraphAnimationIds &animIds = sceneGraph.animations();
	// Count total keyframes across all animations and nodes
	size_t totalAnimKeyframes = 0;
	for (const core::String &animName : animIds) {
		for (int mi = 0; mi < (int)meshInfos.size(); ++mi) {
			const int sgNodeId = meshes[meshInfos[mi].meshExtIdx].nodeId;
			if (!sceneGraph.hasNode(sgNodeId)) continue;
			const scenegraph::SceneGraphNode &sgNode = sceneGraph.node(sgNodeId);
			if (!sgNode.allKeyFrames().hasKey(animName)) continue;
			const scenegraph::SceneGraphKeyFrames &kfs = sgNode.keyFrames(animName);
			if (kfs.size() <= 1) continue;
			totalAnimKeyframes += kfs.size();
		}
	}

	// Animation data structures
	cgltf_animation *gltfAnimations = nullptr;
	cgltf_animation_sampler *gltfSamplers = nullptr;
	cgltf_animation_channel *gltfChannels = nullptr;
	cgltf_accessor *animAccessors = nullptr;
	cgltf_buffer_view *animBufferViews = nullptr;
	uint8_t *animBuffer = nullptr;
	size_t animBufferSize = 0;
	int animCount = 0;
	core::DynamicArray<core::String> uniqueAnims;

	if (totalAnimKeyframes > 0) {
		// Calculate animation buffer size: for each keyframe we need time(1f) + translation(3f) + rotation(4f) + scale(3f)
		// Per node per animation: N times + N translations + N rotations + N scales
		// Buffer views: 4 per animated node (time, T, R, S)
		// Accessors: 4 per animated node
		// Channels: 3 per animated node (T, R, S)
		// Samplers: 3 per animated node

		struct AnimNodeInfo {
			int meshIdx;
			int sgNodeId;
			core::String animName;
			int keyframeCount;
		};
		core::DynamicArray<AnimNodeInfo> animNodes;

		for (const core::String &animName : animIds) {
			for (int mi = 0; mi < (int)meshInfos.size(); ++mi) {
				const int sgNodeId = meshes[meshInfos[mi].meshExtIdx].nodeId;
				if (!sceneGraph.hasNode(sgNodeId)) continue;
				const scenegraph::SceneGraphNode &sgNode = sceneGraph.node(sgNodeId);
				if (!sgNode.allKeyFrames().hasKey(animName)) continue;
				const scenegraph::SceneGraphKeyFrames &kfs = sgNode.keyFrames(animName);
				if (kfs.size() <= 1) continue;
				AnimNodeInfo info;
				info.meshIdx = mi;
				info.sgNodeId = sgNodeId;
				info.animName = animName;
				info.keyframeCount = (int)kfs.size();
				animNodes.push_back(info);
			}
		}

		if (!animNodes.empty()) {
			// Calculate buffer size
			for (const AnimNodeInfo &ani : animNodes) {
				int n = ani.keyframeCount;
				animBufferSize += n * sizeof(float);       // times
				animBufferSize += n * 3 * sizeof(float);   // translations
				animBufferSize += n * 4 * sizeof(float);   // rotations
				animBufferSize += n * 3 * sizeof(float);   // scales
			}

			animBuffer = (uint8_t *)core_malloc(animBufferSize);
			int totalAnimBVs = (int)animNodes.size() * 4;
			int totalAnimAccs = (int)animNodes.size() * 4;
			int totalSamplers = (int)animNodes.size() * 3;
			int totalChannels = (int)animNodes.size() * 3;

			animBufferViews = (cgltf_buffer_view *)core_malloc(totalAnimBVs * sizeof(cgltf_buffer_view));
			core_memset(animBufferViews, 0, totalAnimBVs * sizeof(cgltf_buffer_view));
			animAccessors = (cgltf_accessor *)core_malloc(totalAnimAccs * sizeof(cgltf_accessor));
			core_memset(animAccessors, 0, totalAnimAccs * sizeof(cgltf_accessor));
			gltfSamplers = (cgltf_animation_sampler *)core_malloc(totalSamplers * sizeof(cgltf_animation_sampler));
			core_memset(gltfSamplers, 0, totalSamplers * sizeof(cgltf_animation_sampler));
			gltfChannels = (cgltf_animation_channel *)core_malloc(totalChannels * sizeof(cgltf_animation_channel));
			core_memset(gltfChannels, 0, totalChannels * sizeof(cgltf_animation_channel));

			// Count unique animations
			for (const AnimNodeInfo &ani : animNodes) {
				bool found = false;
				for (const core::String &s : uniqueAnims) {
					if (s == ani.animName) { found = true; break; }
				}
				if (!found) uniqueAnims.push_back(ani.animName);
			}
			animCount = (int)uniqueAnims.size();
			gltfAnimations = (cgltf_animation *)core_malloc(animCount * sizeof(cgltf_animation));
			core_memset(gltfAnimations, 0, animCount * sizeof(cgltf_animation));

			// We need a separate buffer for animation data
			cgltf_buffer animGltfBuffer;
			core_memset(&animGltfBuffer, 0, sizeof(animGltfBuffer));
			animGltfBuffer.size = animBufferSize;
			animGltfBuffer.data = animBuffer;

			size_t bufOffset = 0;
			int bvI = 0, accI = 0, sampI = 0, chanI = 0;

			for (int ai = 0; ai < animCount; ++ai) {
				const core::String &animName = uniqueAnims[ai];
				int firstSampler = sampI;
				int firstChannel = chanI;

				for (const AnimNodeInfo &ani : animNodes) {
					if (ani.animName != animName) continue;
					const scenegraph::SceneGraphNode &sgNode = sceneGraph.node(ani.sgNodeId);
					const scenegraph::SceneGraphKeyFrames &kfs = sgNode.keyFrames(animName);
					int n = (int)kfs.size();

					// Write time values
					size_t timeOffset = bufOffset;
					float *timePtr = (float *)(animBuffer + bufOffset);
					for (int k = 0; k < n; ++k) {
						timePtr[k] = (float)kfs[k].frameIdx / GLTF_FPS;
					}
					bufOffset += n * sizeof(float);

					// Write translations
					size_t transOffset = bufOffset;
					float *transPtr = (float *)(animBuffer + bufOffset);
					for (int k = 0; k < n; ++k) {
						const glm::vec3 &t = kfs[k].transform().localTranslation();
						transPtr[k * 3 + 0] = t.x;
						transPtr[k * 3 + 1] = t.y;
						transPtr[k * 3 + 2] = t.z;
					}
					bufOffset += n * 3 * sizeof(float);

					// Write rotations (xyzw)
					size_t rotOffset = bufOffset;
					float *rotPtr = (float *)(animBuffer + bufOffset);
					for (int k = 0; k < n; ++k) {
						const glm::quat &r = kfs[k].transform().localOrientation();
						rotPtr[k * 4 + 0] = r.x;
						rotPtr[k * 4 + 1] = r.y;
						rotPtr[k * 4 + 2] = r.z;
						rotPtr[k * 4 + 3] = r.w;
					}
					bufOffset += n * 4 * sizeof(float);

					// Write scales
					size_t scaleOffset = bufOffset;
					float *scalePtr = (float *)(animBuffer + bufOffset);
					for (int k = 0; k < n; ++k) {
						const glm::vec3 &s = kfs[k].transform().localScale();
						scalePtr[k * 3 + 0] = s.x;
						scalePtr[k * 3 + 1] = s.y;
						scalePtr[k * 3 + 2] = s.z;
					}
					bufOffset += n * 3 * sizeof(float);

					// Buffer views: time, trans, rot, scale
					int timeBV = bvI;
					animBufferViews[bvI].buffer = &gltfBuffer; // will be fixed below
					animBufferViews[bvI].offset = timeOffset;
					animBufferViews[bvI].size = n * sizeof(float);
					++bvI;
					int transBV = bvI;
					animBufferViews[bvI].buffer = &gltfBuffer;
					animBufferViews[bvI].offset = transOffset;
					animBufferViews[bvI].size = n * 3 * sizeof(float);
					++bvI;
					int rotBV = bvI;
					animBufferViews[bvI].buffer = &gltfBuffer;
					animBufferViews[bvI].offset = rotOffset;
					animBufferViews[bvI].size = n * 4 * sizeof(float);
					++bvI;
					int scaleBV = bvI;
					animBufferViews[bvI].buffer = &gltfBuffer;
					animBufferViews[bvI].offset = scaleOffset;
					animBufferViews[bvI].size = n * 3 * sizeof(float);
					++bvI;

					// Accessors
					int timeAcc = accI;
					animAccessors[accI].buffer_view = &animBufferViews[timeBV];
					animAccessors[accI].component_type = cgltf_component_type_r_32f;
					animAccessors[accI].type = cgltf_type_scalar;
					animAccessors[accI].count = n;
					animAccessors[accI].has_min = true;
					animAccessors[accI].has_max = true;
					animAccessors[accI].min[0] = timePtr[0];
					animAccessors[accI].max[0] = timePtr[n - 1];
					++accI;
					int transAcc = accI;
					animAccessors[accI].buffer_view = &animBufferViews[transBV];
					animAccessors[accI].component_type = cgltf_component_type_r_32f;
					animAccessors[accI].type = cgltf_type_vec3;
					animAccessors[accI].count = n;
					++accI;
					int rotAcc = accI;
					animAccessors[accI].buffer_view = &animBufferViews[rotBV];
					animAccessors[accI].component_type = cgltf_component_type_r_32f;
					animAccessors[accI].type = cgltf_type_vec4;
					animAccessors[accI].count = n;
					++accI;
					int scaleAcc = accI;
					animAccessors[accI].buffer_view = &animBufferViews[scaleBV];
					animAccessors[accI].component_type = cgltf_component_type_r_32f;
					animAccessors[accI].type = cgltf_type_vec3;
					animAccessors[accI].count = n;
					++accI;

					// Samplers (T, R, S)
					gltfSamplers[sampI].input = &animAccessors[timeAcc];
					gltfSamplers[sampI].output = &animAccessors[transAcc];
					gltfSamplers[sampI].interpolation = cgltf_interpolation_type_linear;
					int transSamp = sampI; ++sampI;
					gltfSamplers[sampI].input = &animAccessors[timeAcc];
					gltfSamplers[sampI].output = &animAccessors[rotAcc];
					gltfSamplers[sampI].interpolation = cgltf_interpolation_type_linear;
					int rotSamp = sampI; ++sampI;
					gltfSamplers[sampI].input = &animAccessors[timeAcc];
					gltfSamplers[sampI].output = &animAccessors[scaleAcc];
					gltfSamplers[sampI].interpolation = cgltf_interpolation_type_linear;
					int scaleSamp = sampI; ++sampI;

					// Channels
					gltfChannels[chanI].sampler = &gltfSamplers[transSamp];
					gltfChannels[chanI].target_node = &nodes[ani.meshIdx];
					gltfChannels[chanI].target_path = cgltf_animation_path_type_translation;
					++chanI;
					gltfChannels[chanI].sampler = &gltfSamplers[rotSamp];
					gltfChannels[chanI].target_node = &nodes[ani.meshIdx];
					gltfChannels[chanI].target_path = cgltf_animation_path_type_rotation;
					++chanI;
					gltfChannels[chanI].sampler = &gltfSamplers[scaleSamp];
					gltfChannels[chanI].target_node = &nodes[ani.meshIdx];
					gltfChannels[chanI].target_path = cgltf_animation_path_type_scale;
					++chanI;
				}

				gltfAnimations[ai].name = (char *)animName.c_str();
				gltfAnimations[ai].samplers = &gltfSamplers[firstSampler];
				gltfAnimations[ai].samplers_count = sampI - firstSampler;
				gltfAnimations[ai].channels = &gltfChannels[firstChannel];
				gltfAnimations[ai].channels_count = chanI - firstChannel;
			}

			// Merge animation buffer into main buffer
			size_t newBufferSize = bufferSize + animBufferSize;
			uint8_t *newBuffer = (uint8_t *)core_malloc(newBufferSize);
			core_memcpy(newBuffer, buffer, bufferSize);
			core_memcpy(newBuffer + bufferSize, animBuffer, animBufferSize);
			core_free(buffer);
			core_free(animBuffer);
			buffer = newBuffer;
			animBuffer = nullptr;

			// Fix animation buffer view offsets (they're relative to animBuffer, need to add bufferSize)
			for (int i = 0; i < totalAnimBVs; ++i) {
				animBufferViews[i].offset += bufferSize;
				animBufferViews[i].buffer = &gltfBuffer;
			}
			bufferSize = newBufferSize;
			gltfBuffer.size = bufferSize;
			gltfBuffer.data = buffer;

			// Merge accessors and buffer views into gltfData
			int newTotalAccessors = totalAccessors + totalAnimAccs;
			int newTotalBVs = totalBufferViews + totalAnimBVs;
			cgltf_accessor *mergedAccessors = (cgltf_accessor *)core_malloc(newTotalAccessors * sizeof(cgltf_accessor));
			core_memcpy(mergedAccessors, accessors, totalAccessors * sizeof(cgltf_accessor));
			core_memcpy(mergedAccessors + totalAccessors, animAccessors, totalAnimAccs * sizeof(cgltf_accessor));
			cgltf_buffer_view *mergedBVs = (cgltf_buffer_view *)core_malloc(newTotalBVs * sizeof(cgltf_buffer_view));
			core_memcpy(mergedBVs, bufferViews, totalBufferViews * sizeof(cgltf_buffer_view));
			core_memcpy(mergedBVs + totalBufferViews, animBufferViews, totalAnimBVs * sizeof(cgltf_buffer_view));

			// Fix pointers: accessors point to buffer_views, need to update
			for (int i = 0; i < totalAccessors; ++i) {
				ptrdiff_t idx = mergedAccessors[i].buffer_view - bufferViews;
				mergedAccessors[i].buffer_view = &mergedBVs[idx];
			}
			for (int i = 0; i < totalAnimAccs; ++i) {
				ptrdiff_t idx = mergedAccessors[totalAccessors + i].buffer_view - animBufferViews;
				mergedAccessors[totalAccessors + i].buffer_view = &mergedBVs[totalBufferViews + idx];
			}
			// Fix primitive index accessor pointers
			for (int i = 0; i < totalMeshes; ++i) {
				ptrdiff_t idx = primitives[i].indices - accessors;
				primitives[i].indices = &mergedAccessors[idx];
			}
			// Fix attribute accessor pointers
			for (int i = 0; i < totalMeshes * maxAttrsPerMesh; ++i) {
				if (attributes[i].data == nullptr) continue;
				ptrdiff_t idx = attributes[i].data - accessors;
				attributes[i].data = &mergedAccessors[idx];
			}
			// Fix sampler input/output pointers
			for (int i = 0; i < totalSamplers; ++i) {
				ptrdiff_t inIdx = gltfSamplers[i].input - animAccessors;
				gltfSamplers[i].input = &mergedAccessors[totalAccessors + inIdx];
				ptrdiff_t outIdx = gltfSamplers[i].output - animAccessors;
				gltfSamplers[i].output = &mergedAccessors[totalAccessors + outIdx];
			}

			core_free(accessors);
			core_free(bufferViews);
			core_free(animAccessors);
			core_free(animBufferViews);
			accessors = mergedAccessors;
			bufferViews = mergedBVs;
			totalAccessors = newTotalAccessors;
			totalBufferViews = newTotalBVs;

			gltfData.accessors = accessors;
			gltfData.accessors_count = totalAccessors;
			gltfData.buffer_views = bufferViews;
			gltfData.buffer_views_count = totalBufferViews;
			gltfData.animations = gltfAnimations;
			gltfData.animations_count = animCount;
		}
	}

	// Determine file type
	const bool isGlb = core::string::extractExtension(filename) == "glb";
	cgltf_options writeOptions;
	core_memset(&writeOptions, 0, sizeof(writeOptions));

	const core::String binFilename = core::string::extractFilenameWithExtension(
		core::string::replaceExtension(filename, "bin"));

	if (isGlb) {
		writeOptions.type = cgltf_file_type_glb;
	} else {
		writeOptions.type = cgltf_file_type_gltf;
		gltfBuffer.uri = (char *)binFilename.c_str();
	}

	// Get JSON size
	cgltf_size jsonSize = cgltf_write(&writeOptions, nullptr, 0, &gltfData);
	if (jsonSize == 0) {
		core_free(buffer);
		core_free(bufferViews);
		core_free(accessors);
		core_free(gltfMeshes);
		core_free(primitives);
		core_free(attributes);
		core_free(nodes);
		core_free(sceneNodes);
		core_free(gltfAnimations);
		core_free(gltfSamplers);
		core_free(gltfChannels);
		Log::error("Failed to calculate gltf json size");
		return false;
	}

	char *jsonBuf = (char *)core_malloc(jsonSize);
	cgltf_write(&writeOptions, jsonBuf, jsonSize, &gltfData);

	bool success = false;
	if (isGlb) {
		core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
		if (stream) {
			// GLB header
			const uint32_t glbMagic = 0x46546C67;
			const uint32_t glbVersion = 2;
			// JSON chunk (padded to 4 bytes with spaces)
			uint32_t jsonLen = (uint32_t)(jsonSize - 1); // exclude null terminator
			uint32_t jsonPadded = (jsonLen + 3) & ~3u;
			// BIN chunk (padded to 4 bytes with zeros)
			uint32_t binLen = (uint32_t)bufferSize;
			uint32_t binPadded = (binLen + 3) & ~3u;
			uint32_t totalLen = 12 + 8 + jsonPadded + 8 + binPadded;

			stream->writeUInt32(glbMagic);
			stream->writeUInt32(glbVersion);
			stream->writeUInt32(totalLen);
			// JSON chunk
			stream->writeUInt32(jsonPadded);
			stream->writeUInt32(0x4E4F534A); // "JSON"
			stream->write(jsonBuf, jsonLen);
			for (uint32_t p = jsonLen; p < jsonPadded; ++p) {
				stream->writeUInt8(' ');
			}
			// BIN chunk
			stream->writeUInt32(binPadded);
			stream->writeUInt32(0x004E4942); // "BIN\0"
			stream->write(buffer, binLen);
			for (uint32_t p = binLen; p < binPadded; ++p) {
				stream->writeUInt8(0);
			}
			success = true;
		}
	} else {
		core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
		if (stream) {
			stream->write(jsonBuf, jsonSize - 1);
			success = true;
		}
		// Write binary buffer
		const core::String binPath = core::string::replaceExtension(filename, "bin");
		core::ScopedPtr<io::SeekableWriteStream> binStream(archive->writeStream(binPath));
		if (binStream) {
			binStream->write(buffer, bufferSize);
		}
	}

	core_free(jsonBuf);
	core_free(buffer);
	core_free(bufferViews);
	core_free(accessors);
	core_free(gltfMeshes);
	core_free(primitives);
	core_free(attributes);
	core_free(nodes);
	core_free(sceneNodes);
	core_free(gltfAnimations);
	core_free(gltfSamplers);
	core_free(gltfChannels);
	if (gltfImages) {
		for (int i = 0; i < texturedMeshCount; ++i) {
			core_free(gltfImages[i].uri);
		}
		core_free(gltfImages);
	}
	core_free(gltfTextures);
	core_free(gltfMaterials);

	if (!success) {
		Log::error("Failed to write gltf file %s", filename.c_str());
	}
	return success;
}

bool GLTFFormat::savePointCloud(const scenegraph::SceneGraph &sceneGraph, const PointCloud &pointCloud,
								const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale,
								bool withColor) const {
	if (pointCloud.empty()) {
		Log::error("Empty point cloud");
		return false;
	}

	const int vertexCount = (int)pointCloud.size();
	// position(3f) + color(4f) = 7 floats per vertex
	const size_t bufferSize = vertexCount * 7 * sizeof(float);
	uint8_t *buffer = (uint8_t *)core_malloc(bufferSize);
	float *vBuf = (float *)buffer;

	for (int i = 0; i < vertexCount; ++i) {
		const glm::vec3 pos = pointCloud[i].position * scale;
		vBuf[i * 7 + 0] = pos.x;
		vBuf[i * 7 + 1] = pos.y;
		vBuf[i * 7 + 2] = pos.z;
		if (withColor) {
			vBuf[i * 7 + 3] = (float)pointCloud[i].color.r / 255.0f;
			vBuf[i * 7 + 4] = (float)pointCloud[i].color.g / 255.0f;
			vBuf[i * 7 + 5] = (float)pointCloud[i].color.b / 255.0f;
			vBuf[i * 7 + 6] = (float)pointCloud[i].color.a / 255.0f;
		} else {
			vBuf[i * 7 + 3] = 1.0f;
			vBuf[i * 7 + 4] = 1.0f;
			vBuf[i * 7 + 5] = 1.0f;
			vBuf[i * 7 + 6] = 1.0f;
		}
	}

	cgltf_buffer gltfBuffer;
	core_memset(&gltfBuffer, 0, sizeof(gltfBuffer));
	gltfBuffer.size = bufferSize;
	gltfBuffer.data = buffer;

	cgltf_buffer_view bufferViews[2];
	core_memset(bufferViews, 0, sizeof(bufferViews));

	// Position buffer view
	bufferViews[0].buffer = &gltfBuffer;
	bufferViews[0].offset = 0;
	bufferViews[0].size = vertexCount * 3 * sizeof(float);
	bufferViews[0].stride = 7 * sizeof(float);
	bufferViews[0].type = cgltf_buffer_view_type_vertices;

	// Color buffer view
	bufferViews[1].buffer = &gltfBuffer;
	bufferViews[1].offset = 3 * sizeof(float);
	bufferViews[1].size = vertexCount * 4 * sizeof(float);
	bufferViews[1].stride = 7 * sizeof(float);
	bufferViews[1].type = cgltf_buffer_view_type_vertices;

	cgltf_accessor accessors[2];
	core_memset(accessors, 0, sizeof(accessors));

	// Position accessor
	accessors[0].buffer_view = &bufferViews[0];
	accessors[0].component_type = cgltf_component_type_r_32f;
	accessors[0].type = cgltf_type_vec3;
	accessors[0].count = vertexCount;
	accessors[0].stride = 7 * sizeof(float);
	accessors[0].has_min = true;
	accessors[0].has_max = true;
	accessors[0].min[0] = accessors[0].min[1] = accessors[0].min[2] = 1e30f;
	accessors[0].max[0] = accessors[0].max[1] = accessors[0].max[2] = -1e30f;
	for (int i = 0; i < vertexCount; ++i) {
		for (int k = 0; k < 3; ++k) {
			float v = vBuf[i * 7 + k];
			if (v < accessors[0].min[k]) accessors[0].min[k] = v;
			if (v > accessors[0].max[k]) accessors[0].max[k] = v;
		}
	}

	// Color accessor
	accessors[1].buffer_view = &bufferViews[1];
	accessors[1].component_type = cgltf_component_type_r_32f;
	accessors[1].type = cgltf_type_vec4;
	accessors[1].count = vertexCount;
	accessors[1].stride = 7 * sizeof(float);

	cgltf_attribute attrs[2];
	core_memset(attrs, 0, sizeof(attrs));
	attrs[0].name = (char *)"POSITION";
	attrs[0].type = cgltf_attribute_type_position;
	attrs[0].data = &accessors[0];
	attrs[1].name = (char *)"COLOR_0";
	attrs[1].type = cgltf_attribute_type_color;
	attrs[1].data = &accessors[1];

	cgltf_primitive primitive;
	core_memset(&primitive, 0, sizeof(primitive));
	primitive.type = cgltf_primitive_type_points;
	primitive.attributes = attrs;
	primitive.attributes_count = 2;

	cgltf_mesh gltfMesh;
	core_memset(&gltfMesh, 0, sizeof(gltfMesh));
	gltfMesh.primitives = &primitive;
	gltfMesh.primitives_count = 1;

	cgltf_node node;
	core_memset(&node, 0, sizeof(node));
	node.mesh = &gltfMesh;
	node.name = (char *)"PointCloud";

	cgltf_node *sceneNodes[1] = {&node};
	cgltf_scene gltfScene;
	core_memset(&gltfScene, 0, sizeof(gltfScene));
	gltfScene.nodes = sceneNodes;
	gltfScene.nodes_count = 1;

	cgltf_data gltfData;
	core_memset(&gltfData, 0, sizeof(gltfData));
	gltfData.asset.version = (char *)"2.0";
	gltfData.asset.generator = (char *)"vengi " PROJECT_VERSION;
	gltfData.meshes = &gltfMesh;
	gltfData.meshes_count = 1;
	gltfData.accessors = accessors;
	gltfData.accessors_count = 2;
	gltfData.buffer_views = bufferViews;
	gltfData.buffer_views_count = 2;
	gltfData.buffers = &gltfBuffer;
	gltfData.buffers_count = 1;
	gltfData.nodes = &node;
	gltfData.nodes_count = 1;
	gltfData.scenes = &gltfScene;
	gltfData.scenes_count = 1;
	gltfData.scene = &gltfScene;

	const bool isGlb = core::string::extractExtension(filename) == "glb";
	cgltf_options writeOptions;
	core_memset(&writeOptions, 0, sizeof(writeOptions));

	const core::String binFilename = core::string::extractFilenameWithExtension(
		core::string::replaceExtension(filename, "bin"));

	if (isGlb) {
		writeOptions.type = cgltf_file_type_glb;
	} else {
		writeOptions.type = cgltf_file_type_gltf;
		gltfBuffer.uri = (char *)binFilename.c_str();
	}

	cgltf_size jsonSize = cgltf_write(&writeOptions, nullptr, 0, &gltfData);
	if (jsonSize == 0) {
		core_free(buffer);
		Log::error("Failed to calculate gltf json size for point cloud");
		return false;
	}

	char *jsonBuf = (char *)core_malloc(jsonSize);
	cgltf_write(&writeOptions, jsonBuf, jsonSize, &gltfData);

	bool success = false;
	if (isGlb) {
		core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
		if (stream) {
			const uint32_t glbMagic = 0x46546C67;
			const uint32_t glbVersion = 2;
			uint32_t jsonLen = (uint32_t)(jsonSize - 1);
			uint32_t jsonPadded = (jsonLen + 3) & ~3u;
			uint32_t binLen = (uint32_t)bufferSize;
			uint32_t binPadded = (binLen + 3) & ~3u;
			uint32_t totalLen = 12 + 8 + jsonPadded + 8 + binPadded;

			stream->writeUInt32(glbMagic);
			stream->writeUInt32(glbVersion);
			stream->writeUInt32(totalLen);
			stream->writeUInt32(jsonPadded);
			stream->writeUInt32(0x4E4F534A);
			stream->write(jsonBuf, jsonLen);
			for (uint32_t p = jsonLen; p < jsonPadded; ++p) {
				stream->writeUInt8(' ');
			}
			stream->writeUInt32(binPadded);
			stream->writeUInt32(0x004E4942);
			stream->write(buffer, binLen);
			for (uint32_t p = binLen; p < binPadded; ++p) {
				stream->writeUInt8(0);
			}
			success = true;
		}
	} else {
		core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
		if (stream) {
			stream->write(jsonBuf, jsonSize - 1);
			success = true;
		}
		const core::String binPath = core::string::replaceExtension(filename, "bin");
		core::ScopedPtr<io::SeekableWriteStream> binStream(archive->writeStream(binPath));
		if (binStream) {
			binStream->write(buffer, bufferSize);
		}
	}

	core_free(jsonBuf);
	core_free(buffer);

	if (!success) {
		Log::error("Failed to write gltf point cloud file %s", filename.c_str());
	}
	return success;
}

} // namespace voxelformat
