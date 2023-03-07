/**
 * @file
 */

#include "FBXFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "engine-config.h"
#include "io/StdStreamBuf.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

#define ufbx_assert core_assert
#include "external/ufbx.h"

namespace voxelformat {

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Failed to write fbx " CORE_STRINGIFY(read));                                                       \
		return false;                                                                                                  \
	}

bool FBXFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph, const Meshes &meshes,
						   const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale,
						   bool quad, bool withColor, bool withTexCoords) {
	return saveMeshesAscii(meshes, filename, stream, scale, quad, withColor, withTexCoords, sceneGraph);
}

class FBXScopedHeader {
private:
	io::SeekableWriteStream& _stream;
	/**
	 * @brief EndOffset is the distance from the beginning of the file to the end of the node record (i.e. the first
	 * byte of whatever comes next). This can be used to easily skip over unknown or not required records.
	 */
	int64_t _endOffsetPos;

public:
	FBXScopedHeader(io::SeekableWriteStream& stream) : _stream(stream) {
		_endOffsetPos = stream.pos();
		stream.writeUInt32(0u);
	}

	~FBXScopedHeader() {
		const int64_t currentPos = _stream.pos();
		_stream.seek(_endOffsetPos);
		_stream.writeUInt32(currentPos);
		_stream.seek(currentPos);
	}
};

bool FBXFormat::saveMeshesBinary(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords, const scenegraph::SceneGraph &sceneGraph) {
	wrapBool(stream.writeString("Kaydara FBX Binary  ", true))
	stream.writeUInt8(0x1A);  // unknown
	stream.writeUInt8(0x00);  // unknown
	stream.writeUInt32(7300); // version
	// TODO: implement me https://code.blender.org/2013/08/fbx-binary-file-format-specification/
	return false;
}

// https://github.com/blender/blender/blob/00e219d8e97afcf3767a6d2b28a6d05bcc984279/release/io/export_fbx.py
bool FBXFormat::saveMeshesAscii(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords, const scenegraph::SceneGraph &sceneGraph) {
	int meshCount = 0;
	for (const MeshExt &meshExt : meshes) {
		for (int i = 0; i < 2; ++i) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
			if (mesh->isEmpty()) {
				continue;
			}
			++meshCount;
		}
	}
	// TODO: support keyframes (takes)
	stream.writeStringFormat(false, R"(FBXHeaderExtension:  {
	FBXHeaderVersion: 1003
	FBXVersion: 6100
	Creator: "github.com/mgerhardy/vengi %s"
	OtherFlags:  {
		FlagPLE: 0
	}
}

Creator: "%s %s"

Definitions: {
	Version: 100
	Count: 1
	ObjectType: "Model" {
		Count: %i
	}
	ObjectType: "Material" {
		Count: 1
	}
}

Objects: {

)",
							PROJECT_VERSION, app::App::getInstance()->appname().c_str(), PROJECT_VERSION, meshCount);

	Log::debug("Exporting %i layers", meshCount);

	// TODO: maybe also export Model: "Model::Camera", "Camera"
	// TODO: are connections and relations needed?
	// https://github.com/libgdx/fbx-conv/blob/master/samples/blender/cube.fbx

	for (const MeshExt &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
			if (mesh->isEmpty()) {
				continue;
			}
			Log::debug("Exporting layer %s", meshExt.name.c_str());
			const int nv = (int)mesh->getNoOfVertices();
			const int ni = (int)mesh->getNoOfIndices();
			if (ni % 3 != 0) {
				Log::error("Unexpected indices amount");
				return false;
			}
			const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
			const voxel::Palette &palette = graphNode.palette();
			const scenegraph::KeyFrameIndex keyFrameIdx = 0;
			const scenegraph::SceneGraphTransform &transform = graphNode.transform(keyFrameIdx);
			const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
			const voxel::IndexType *indices = mesh->getRawIndexData();
			const char *objectName = meshExt.name.c_str();
			if (objectName[0] == '\0') {
				objectName = "Noname";
			}

			stream.writeStringFormat(false, "\tModel: \"%s\", \"Mesh\" {\n", objectName);
			wrapBool(stream.writeString("\t\tVersion: 232\n", false))
			wrapBool(stream.writeString("\t\tVertices: ", false))
			for (int i = 0; i < nv; ++i) {
				const voxel::VoxelVertex &v = vertices[i];

				glm::vec3 pos;
				if (meshExt.applyTransform) {
					pos = transform.apply(v.position, meshExt.size);
				} else {
					pos = v.position;
				}
				pos *= scale;
				if (i > 0) {
					wrapBool(stream.writeString(",", false))
				}
				stream.writeStringFormat(false, "%.04f,%.04f,%.04f", pos.x, pos.y, pos.z);
			}
			wrapBool(stream.writeString("\n", false))

			wrapBool(stream.writeString("\t\tPolygonVertexIndex: ", false))

			for (int i = 0; i < ni; i += 3) {
				const uint32_t one = indices[i + 0] + 1;
				const uint32_t two = indices[i + 1] + 1;
				const uint32_t three = indices[i + 2] + 1;
				if (i > 0) {
					wrapBool(stream.writeString(",", false))
				}
				stream.writeStringFormat(false, "%i,%i,%i", (int)one, (int)two, (int)three);
			}
			wrapBool(stream.writeString("\n", false))
			wrapBool(stream.writeString("\t\tGeometryVersion: 124\n", false))

			if (withTexCoords) {
				wrapBool(stream.writeString("\t\tLayerElementUV: 0 {\n", false))
				wrapBool(stream.writeString("\t\t\tVersion: 101\n", false))
				stream.writeStringFormat(false, "\t\t\tName: \"%sUV\"\n", objectName);
				wrapBool(stream.writeString("\t\t\tMappingInformationType: \"ByPolygonVertex\"\n", false))
				wrapBool(stream.writeString("\t\t\tReferenceInformationType: \"Direct\"\n", false))
				wrapBool(stream.writeString("\t\t\tUV: ", false))

				for (int i = 0; i < ni; i++) {
					const uint32_t index = indices[i];
					const voxel::VoxelVertex &v = vertices[index];
					const glm::vec2 &uv = paletteUV(v.colorIndex);
					if (i > 0) {
						wrapBool(stream.writeString(",", false))
					}
					stream.writeStringFormat(false, "%f,%f", uv.x, uv.y);
				}
				wrapBool(stream.writeString("\n\n", false))
				// TODO: UVIndex needed or only for IndexToDirect?

				wrapBool(stream.writeString(
					"\t\tLayerElementTexture: 0 {\n"
					"\t\t\tVersion: 101\n"
					"\t\t\tName: \"\"\n" // TODO
					"\t\t\tMappingInformationType: \"AllSame\"\n"
					"\t\t\tReferenceInformationType: \"Direct\"\n"
					"\t\t\tBlendMode: \"Translucent\"\n"
					"\t\t\tTextureAlpha: 1\n"
					"\t\t\tTextureId: 0\n"
					"\t\t}\n"))
			}

			if (withColor) {
				stream.writeStringFormat(false,
										"\t\tLayerElementColor: 0 {\n"
										"\t\t\tVersion: 101\n"
										"\t\t\tName: \"%sColors\"\n"
										"\t\t\tMappingInformationType: \"ByPolygonVertex\"\n"
										"\t\t\tReferenceInformationType: \"Direct\"\n"
										"\t\t\tColors: ",
										objectName);
				for (int i = 0; i < ni; i++) {
					const uint32_t index = indices[i];
					const voxel::VoxelVertex &v = vertices[index];
					const glm::vec4 &color = core::Color::fromRGBA(palette.color(v.colorIndex));
					if (i > 0) {
						wrapBool(stream.writeString(",", false))
					}
					stream.writeStringFormat(false, "%f,%f,%f,%f", color.r, color.g, color.b, color.a);
				}
				wrapBool(stream.writeString("\n\n", false))
				// TODO: ColorIndex needed or only for IndexToDirect?

				// close LayerElementColor
				wrapBool(stream.writeString("\t\t}\n", false))

				wrapBool(stream.writeString("\t\tLayer: 0 {\n"
								"\t\t\tVersion: 100\n"
								"\t\t\tLayerElement: {\n"
								"\t\t\t\tTypedIndex: 0\n"
								"\t\t\t\tType: \"LayerElementColor\"\n"
								"\t\t\t}\n"
								"\t\t}\n",
								false))
			}

			// close the model
			wrapBool(stream.writeString("\t}\n}\n\n", false))
		}
	}
	return true;
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

static inline glm::vec2 _ufbx_to_vec2(const ufbx_vec2 &v) {
	return glm::vec2((float)v.x, (float)v.y);
}

const ufbx_prop *_ufbx_color_prop(const ufbx_props &props) {
	const ufbx_prop *p;
	p = ufbx_find_prop(&props, "DiffuseColor");
	if (p != nullptr) {
		return p;
	}
	p = ufbx_find_prop(&props, "Diffuse");
	if (p != nullptr) {
		return p;
	}
	p = ufbx_find_prop(&props, "Color");
	return p;
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

static inline glm::mat4 _ufbx_to_um_mat(const ufbx_matrix &m) {
	return glm::mat4{
		(float)m.m00, (float)m.m01, (float)m.m02, (float)m.m03,
		(float)m.m10, (float)m.m11, (float)m.m12, (float)m.m13,
		(float)m.m20, (float)m.m21, (float)m.m22, (float)m.m23,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}

static inline void _ufbx_to_transform(scenegraph::SceneGraphTransform &transform, const ufbx_node *node) {
	const glm::mat4 &mat = _ufbx_to_um_mat(node->node_to_parent);
	const glm::vec3 lt = transform.localTranslation();
	transform.setLocalMatrix(mat);
	transform.setLocalTranslation(transform.localTranslation() + lt);
}

} // namespace priv

int FBXFormat::addMeshNode(const ufbx_scene *scene, const ufbx_node *node, const core::String &filename, scenegraph::SceneGraph &sceneGraph, const core::StringMap<image::ImagePtr> &textures, int parent) const {
	Log::debug("Add model node");
	const glm::vec3 &scale = getScale();
	ufbx_vec2 defaultUV;
	core_memset(&defaultUV, 0, sizeof(defaultUV));
	const ufbx_mesh *mesh = node->mesh;
	core_assert(mesh != nullptr);

	const size_t numTriIndices = mesh->max_face_triangles * 3;
	core::Buffer<uint32_t> triIndices(numTriIndices);

	TriCollection tris;
	tris.reserve(numTriIndices);

	Log::debug("There are %i materials in the mesh", (int)mesh->materials.count);
	Log::debug("Vertex colors: %s", mesh->vertex_color.exists ? "true" : "false");

	for (const ufbx_mesh_material &meshMaterial : mesh->materials) {
		if (meshMaterial.num_triangles == 0) {
			continue;
		}
		Log::debug("Faces: %i - material: %s", (int)meshMaterial.num_faces, meshMaterial.material ? "yes" : "no");

		const image::Image* texture = nullptr;
		core::RGBA diffuseColor(0, 0, 0, 255);
		if (const ufbx_material *material = meshMaterial.material) {
			const core::String &materialName = priv::_ufbx_to_string(material->name);
			auto textureIter = textures.find(materialName);
			if (textureIter != textures.end()) {
				texture = textureIter->second.get();
			} else if (const ufbx_prop *prop = priv::_ufbx_color_prop(material->props)) {
				if (prop->type == UFBX_PROP_COLOR) {
					const glm::vec3 rgb = priv::_ufbx_to_vec3(prop->value_vec3);
					diffuseColor = core::Color::getRGBA(glm::vec4(rgb, 1.0f));
					Log::debug("Found rgb diffuse color for '%s'", materialName.c_str());
				} else if (prop->type == UFBX_PROP_COLOR_WITH_ALPHA) {
					const glm::vec4 rgba = priv::_ufbx_to_vec4(prop->value_vec4);
					diffuseColor = core::Color::getRGBA(rgba);
					Log::debug("Found rgba diffuse color for '%s'", materialName.c_str());
				} else {
					Log::debug("Unknown material color type: %i for '%s'", (int)prop->type, materialName.c_str());
				}
			} else {
				Log::debug("Failed to find texture and diffuse color for '%s'", materialName.c_str());
			}
		} else {
			Log::debug("No material assigned for mesh");
		}

		for (size_t fi = 0; fi < meshMaterial.num_faces; fi++) {
			const ufbx_face face = mesh->faces[meshMaterial.face_indices[fi]];
			const size_t numTris = ufbx_triangulate_face(triIndices.data(), numTriIndices, mesh, face);

			for (size_t vi = 0; vi < numTris; vi++) {
				Tri tri;
				for (int ti = 0; ti < 3; ++ti) {
					const uint32_t ix = triIndices[vi * 3 + ti];
					const ufbx_vec3 &pos = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
					if (mesh->vertex_color.exists) {
						const ufbx_vec4 &color = ufbx_get_vertex_vec4(&mesh->vertex_color, ix);
						tri.color[ti] = core::Color::getRGBA(priv::_ufbx_to_vec4(color));
					} else {
						tri.color[ti] = diffuseColor;
					}
					const ufbx_vec2 &uv = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, ix) : defaultUV;
					tri.vertices[ti] = priv::_ufbx_to_vec3(pos) * scale;
					tri.uv[ti] = priv::_ufbx_to_vec2(uv);
				}
				tri.texture = texture;
				tris.push_back(tri);
			}
		}
	}
	const core::String &name = priv::_ufbx_to_string(node->name);
	const int nodeId = voxelizeNode(name, sceneGraph, tris, parent);
	if (nodeId < 0) {
		Log::error("Failed to voxelize node %s", name.c_str());
		return nodeId;
	}

	scenegraph::SceneGraphNode &sceneGraphNode = sceneGraph.node(nodeId);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphTransform &transform = sceneGraphNode.keyFrame(keyFrameIdx).transform();
	priv::_ufbx_to_transform(transform, node);
	for (const auto &prop : node->props.props) {
		sceneGraphNode.setProperty(priv::_ufbx_to_string(prop.name), priv::_ufbx_to_string(prop.value_str));
	}
	sceneGraphNode.setTransform(keyFrameIdx, transform);
	// TODO: animations - see ufbx_evaluate_transform
	return nodeId;
}

int FBXFormat::addCameraNode(const ufbx_scene *scene, const ufbx_node *node, scenegraph::SceneGraph &sceneGraph, int parent) const {
	Log::debug("Add camera node");
	const ufbx_camera *camera = node->camera;
	core_assert(camera != nullptr);

	scenegraph::SceneGraphNodeCamera camNode;
	camNode.setName(priv::_ufbx_to_string(node->name));
	camNode.setAspectRatio((float)camera->aspect_ratio);
	camNode.setNearPlane((float)camera->near_plane);
	camNode.setFarPlane((float)camera->far_plane);
	if (camera->projection_mode == UFBX_PROJECTION_MODE_PERSPECTIVE) {
		camNode.setPerspective();
		camNode.setFieldOfView((int)camera->field_of_view_deg.x);
	} else if (camera->projection_mode == UFBX_PROJECTION_MODE_ORTHOGRAPHIC) {
		camNode.setOrthographic();
		camNode.setWidth((int)camera->orthographic_size.x);
		camNode.setHeight((int)camera->orthographic_size.y);
	}
	scenegraph::SceneGraphTransform transform;
	priv::_ufbx_to_transform(transform, node);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	camNode.setTransform(keyFrameIdx, transform);
	return sceneGraph.emplace(core::move(camNode), parent);
}

int FBXFormat::addNode_r(const ufbx_scene *scene, const ufbx_node *node, const core::String &filename, scenegraph::SceneGraph &sceneGraph, const core::StringMap<image::ImagePtr> &textures, int parent) const {
	int nodeId = parent;
	if (node->mesh != nullptr) {
		nodeId = addMeshNode(scene, node, filename, sceneGraph, textures, parent);
	} else if (node->camera != nullptr) {
		nodeId = addCameraNode(scene, node, sceneGraph, parent);
	} else if (node->light != nullptr) {
		Log::debug("Skip light node");
	} else if (node->bone != nullptr) {
		Log::debug("Skip bone node");
	} else {
		Log::debug("Skip unknown node");
	}
	if (nodeId < 0) {
		Log::error("Failed to add node with parent %i", parent);
		return nodeId;
	}
	for (const ufbx_node *c : node->children) {
		const int newNodeId = addNode_r(scene, c, filename, sceneGraph, textures, nodeId);
		if (newNodeId < 0) {
			const core::String name = priv::_ufbx_to_string(node->name);
			Log::error("Failed to add child node '%s'", name.c_str());
			return newNodeId;
		}
	}
	return nodeId;
}

bool FBXFormat::voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	ufbx_stream ufbxstream;
	core_memset(&ufbxstream, 0, sizeof(ufbxstream));
	ufbxstream.user = &stream;
	ufbxstream.read_fn = priv::_ufbx_read_fn;
	ufbxstream.skip_fn = priv::_ufbx_skip_fn;

	ufbx_load_opts ufbxopts;
	core_memset(&ufbxopts, 0, sizeof(ufbxopts));

	ufbxopts.temp_allocator.allocator.alloc_fn = priv::_ufbx_alloc;
	ufbxopts.temp_allocator.allocator.free_fn = priv::_ufbx_free;
	ufbxopts.temp_allocator.allocator.realloc_fn = priv::_ufbx_realloc_fn;

	ufbxopts.result_allocator.allocator.alloc_fn = priv::_ufbx_alloc;
	ufbxopts.result_allocator.allocator.free_fn = priv::_ufbx_free;
	ufbxopts.result_allocator.allocator.realloc_fn = priv::_ufbx_realloc_fn;

	ufbxopts.path_separator = '/';

	ufbxopts.raw_filename.data = filename.c_str();
	ufbxopts.raw_filename.size = filename.size();

	ufbxopts.allow_null_material = true;
	ufbxopts.target_axes = ufbx_axes_right_handed_y_up; // TODO: see issue https://github.com/mgerhardy/vengi/issues/227
	ufbxopts.target_unit_meters = 1.0f;

	ufbx_error ufbxerror;

	ufbx_scene *ufbxscene = ufbx_load_stream(&ufbxstream, &ufbxopts, &ufbxerror);
	if (!ufbxscene) {
		Log::error("Failed to load: %s", ufbxerror.description.data);
		return false;
	}
	if (ufbxerror.type != UFBX_ERROR_NONE) {
		char err[4096];
		ufbx_format_error(err, sizeof(err), &ufbxerror);
		Log::error("Error while loading fbx: %s", err);
	}

	core::StringMap<image::ImagePtr> textures;
	for (size_t i = 0; i < ufbxscene->meshes.count; ++i) {
		const ufbx_mesh *mesh = ufbxscene->meshes[i];
		for (size_t pi = 0; pi < mesh->materials.count; pi++) {
			const ufbx_mesh_material *mesh_mat = &mesh->materials[pi];
			if (mesh_mat->num_triangles == 0) {
				continue;
			}
			const ufbx_material *material = mesh_mat->material;
			if (material == nullptr || material->textures.count == 0) {
				continue;
			}
			const core::String &texname = priv::_ufbx_to_string(material->name);
			if (texname.empty()) {
				continue;
			}

			const ufbx_texture *texture = material->fbx.diffuse_color.texture;
			if (texture == nullptr) {
				texture = material->pbr.base_color.texture;
			}

			// TODO: rendering features for e.g. magicavoxel
			/*if (material->features.ior.enabled) {
			}*/

			if (textures.hasKey(texname)) {
				Log::debug("Texture for material '%s' is already loaded", texname.c_str());
				continue;
			}

			const core::String &relativeFilename = priv::_ufbx_to_string(texture ? texture->relative_filename : material->name);
			const core::String &name = lookupTexture(filename, relativeFilename);
			image::ImagePtr tex = image::loadImage(name);
			if (tex->isLoaded()) {
				Log::debug("Use image %s", name.c_str());
				textures.put(texname, tex);
			} else {
				Log::debug("Failed to load image %s", relativeFilename.c_str());
				textures.put(texname, image::ImagePtr());
			}
		}
	}

	const ufbx_node *root = ufbxscene->root_node;
	for (const ufbx_node *c : root->children) {
		if (addNode_r(ufbxscene, c, filename, sceneGraph, textures, sceneGraph.root().id()) < 0) {
			const core::String name = priv::_ufbx_to_string(c->name);
			Log::error("Failed to add root child node '%s'", name.c_str());
			return false;
		}
	}

	ufbx_free_scene(ufbxscene);
	return !sceneGraph.empty();
}

#undef wrapBool

} // namespace voxelformat
