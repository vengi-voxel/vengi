/**
 * @file
 */

#include "FBXFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/TextureLookup.h"

#define ufbx_assert core_assert
#include "voxelformat/external/ufbx.h"

namespace voxelformat {

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Failed to write fbx " CORE_STRINGIFY(read));                                                       \
		return false;                                                                                                  \
	}

bool FBXFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph, const Meshes &meshes,
						   const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale,
						   bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	return saveMeshesAscii(meshes, filename, *stream, scale, quad, withColor, withTexCoords, sceneGraph);
}

bool FBXFormat::saveRecursiveNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								  const core::String &filename, io::SeekableWriteStream &stream, uint32_t sentinelLength) {
	const int64_t endOffsetPos = stream.pos();
	stream.writeUInt32(0); // Placeholder for EndOffset

	// TODO: VOXELFORMAT: write the node name and properties - this is not yet implemented

	// Write children recursively
	const scenegraph::SceneGraphNodeChildren &children = node.children();
	for (int childId : children) {
		const scenegraph::SceneGraphNode &child = sceneGraph.node(childId);
		if (!saveRecursiveNode(sceneGraph, child, filename, stream, sentinelLength)) {
			return false;
		}
	}

	for (uint32_t i = 0; i < sentinelLength; ++i) {
		stream.writeUInt8(0x00);
	}

	const int64_t endOffset = stream.pos();
	stream.seek(endOffsetPos);
	stream.writeUInt32((uint32_t)endOffset);
	stream.seek(endOffset);

	return true;
}

bool FBXFormat::saveMeshesBinary(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
								 const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords,
								 const scenegraph::SceneGraph &sceneGraph) {
	wrapBool(stream.writeString("Kaydara FBX Binary  ", true))
	stream.writeUInt8(0x1A); // unknown
	stream.writeUInt8(0x00); // unknown
	const uint32_t version = 7300;
	stream.writeUInt32(version); // version
	uint32_t sentinelLength = 25;
	if constexpr (version < 7500) {
		sentinelLength = 13;
	}

	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	const scenegraph::SceneGraphNodeChildren &children = root.children();
	for (int child : children) {
		const scenegraph::SceneGraphNode &node = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, node, filename, stream, sentinelLength))
	}

	for (uint32_t i = 0; i < sentinelLength; ++i) {
		stream.writeUInt8(0x0);
	}
	// write footer
	stream.writeUInt8(0xfa);
	stream.writeUInt8(0xbc);
	stream.writeUInt8(0xab);
	stream.writeUInt8(0x09);
	stream.writeUInt8(0xd0);
	stream.writeUInt8(0xc8);
	stream.writeUInt8(0xd4);
	stream.writeUInt8(0x66);
	stream.writeUInt8(0xb1);
	stream.writeUInt8(0x76);
	stream.writeUInt8(0xfb);
	stream.writeUInt8(0x83);
	stream.writeUInt8(0x1c);
	stream.writeUInt8(0xf7);
	stream.writeUInt8(0x26);
	stream.writeUInt8(0x7e);
	stream.writeUInt8(0x00);
	stream.writeUInt8(0x00);
	stream.writeUInt8(0x00);
	stream.writeUInt8(0x00);

	// Padding for 16 byte alignment
	const int offset = (int)stream.pos();
	int pad = ((offset + 15) & ~15) - offset;
	if (pad == 0) {
		pad = 16;
	}
	for (int i = 0; i < pad; ++i) {
		stream.writeUInt8(0x00);
	}

	// Write the FBX version
	stream.writeUInt32(version);

	// Write some footer magic (120 zero bytes)
	for (int i = 0; i < 120; ++i) {
		stream.writeUInt8(0x00);
	}
	stream.writeUInt8(0xf8);
	stream.writeUInt8(0x5a);
	stream.writeUInt8(0x8c);
	stream.writeUInt8(0x6a);
	stream.writeUInt8(0xde);
	stream.writeUInt8(0xf5);
	stream.writeUInt8(0xd9);
	stream.writeUInt8(0x7e);
	stream.writeUInt8(0xec);
	stream.writeUInt8(0xe9);
	stream.writeUInt8(0x0c);
	stream.writeUInt8(0xe3);
	stream.writeUInt8(0x75);
	stream.writeUInt8(0x8f);
	stream.writeUInt8(0x29);
	stream.writeUInt8(0x0b);

	// TODO: VOXELFORMAT: implement me https://code.blender.org/2013/08/fbx-binary-file-format-specification/
	return false;
}

void FBXFormat::writeTransformToProperties(io::SeekableWriteStream &stream,
										   const scenegraph::SceneGraphTransform &transform) {
	const glm::vec3 &lclTranslation = transform.localTranslation();
	stream.writeStringFormat(false, "\t\tProperty: \"Lcl Translation\", \"Lcl Translation\", \"\",%f,%f,%f\n",
							 lclTranslation.x, lclTranslation.y, lclTranslation.z);
	const glm::quat &lclRotationQuat = transform.localOrientation();
	const glm::vec3 lclRotationEulerDegrees(glm::degrees(glm::eulerAngles(lclRotationQuat)));
	stream.writeStringFormat(false, "\t\tProperty: \"Lcl Rotation\", \"Lcl Rotation\", \"\",%f,%f,%f\n",
							 lclRotationEulerDegrees.x, lclRotationEulerDegrees.y, lclRotationEulerDegrees.z);
	const glm::vec3 &lclScaling = transform.localScale();
	stream.writeStringFormat(false, "\t\tProperty: \"Lcl Scaling\", \"Lcl Scaling\", \"\",%f,%f,%f\n", lclScaling.x,
							 lclScaling.y, lclScaling.z);
}

// https://github.com/blender/blender/blob/00e219d8e97afcf3767a6d2b28a6d05bcc984279/release/io/export_fbx.py
bool FBXFormat::saveMeshesAscii(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
								const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords,
								const scenegraph::SceneGraph &sceneGraph) {
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

	stream.writeLine("; FBX 6.1.0 project file");
	stream.writeLine("; ----------------------------------------------------");

	// TODO: VOXELFORMAT: support keyframes (takes)
	stream.writeStringFormat(false, R"(FBXHeaderExtension:  {
	FBXHeaderVersion: 1003
	FBXVersion: 6100
	Creator: "github.com/vengi-voxel/vengi %s"
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
							 PROJECT_VERSION, app::App::getInstance()->fullAppname().c_str(), PROJECT_VERSION,
							 meshCount);

	Log::debug("Exporting %i models", meshCount);

	// https://github.com/libgdx/fbx-conv/blob/master/samples/blender/cube.fbx

	uint32_t objectIndex = 0;
	core::DynamicArray<core::String> connections;

	for (const MeshExt &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
			if (mesh->isEmpty()) {
				continue;
			}
			Log::debug("Exporting model %s", meshExt.name.c_str());
			const int nv = (int)mesh->getNoOfVertices();
			const int ni = (int)mesh->getNoOfIndices();
			if (ni % 3 != 0) {
				Log::error("Unexpected indices amount");
				return false;
			}
			const voxel::NormalArray &normals = mesh->getNormalVector();
			const bool exportNormals = !normals.empty();
			if (exportNormals) {
				Log::debug("Export normals for mesh %i", i);
			}
			const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
			const palette::Palette &palette = graphNode.palette();
			const scenegraph::KeyFrameIndex keyFrameIdx = 0;
			const scenegraph::SceneGraphTransform &transform = graphNode.transform(keyFrameIdx);
			const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
			const voxel::IndexType *indices = mesh->getRawIndexData();
			const char *objectName = meshExt.name.c_str();
			if (objectName[0] == '\0') {
				objectName = graphNode.uuid().c_str();
			}

			const core::String modelName = core::String::format("Model::%s-%u", objectName, objectIndex);
			connections.push_back(modelName);
			stream.writeStringFormat(false, "\tModel: \"%s\", \"Mesh\" {\n", modelName.c_str());
			wrapBool(stream.writeLine("\t\tVersion: 232"))
			wrapBool(stream.writeLine("\t\tCulling: \"CullingOff\""))
			wrapBool(stream.writeLine("\t\tProperties60:  {"))
			stream.writeStringFormat(false, "\t\t\tProperty: \"Show\", \"bool\", \"\",%u\n",
									 graphNode.visible() ? 1 : 0);
			// scenegraph::KeyFrameIndex keyFrameIndex = 0;
			// writeTransformToProperties(stream, graphNode.transform(keyFrameIndex));
			wrapBool(stream.writeLine("\t\t}"))
			// TODO: VOXELFORMAT: add more properties like Color, Lcl Translation, Lcl Rotation, Lcl Scaling ...
			wrapBool(stream.writeString("\t\tVertices: ", false))
			for (int j = 0; j < nv; ++j) {
				const voxel::VoxelVertex &v = vertices[j];

				glm::vec3 pos;
				if (meshExt.applyTransform) {
					pos = transform.apply(v.position, meshExt.pivot * meshExt.size);
				} else {
					pos = v.position;
				}
				pos *= scale;
				if (j > 0) {
					wrapBool(stream.writeString(",", false))
				}
				stream.writeStringFormat(false, "%.04f,%.04f,%.04f", pos.x, pos.y, pos.z);
			}
			wrapBool(stream.writeString("\n", false))

			wrapBool(stream.writeString("\t\tPolygonVertexIndex: ", false))

			for (int j = 0; j < ni; j += 3) {
				const uint32_t one = indices[j + 0];
				const uint32_t two = indices[j + 1];
				const uint32_t three = indices[j + 2];
				if (j > 0) {
					wrapBool(stream.writeString(",", false))
				}
				stream.writeStringFormat(false, "%u,%u,-%u", one, two, (three + 1u));
			}
			wrapBool(stream.writeString("\n", false))
			wrapBool(stream.writeLine("\t\tGeometryVersion: 124"))

			if (exportNormals) {
				stream.writeString("\t\tLayerElementNormal: 0 {\n"
								   "\t\t\tVersion: 101\n"
								   "\t\t\tName: \"\"\n"
								   "\t\t\tMappingInformationType: \"ByVertice\"\n"
								   "\t\t\tReferenceInformationType: \"Direct\"\n",
								   false);

				wrapBool(stream.writeString("\t\t\tNormals: ", false))
				for (size_t j = 0; j < normals.size(); j++) {
					const uint32_t index = indices[j];
					const glm::vec3 &norm = normals[index];
					if (j > 0) {
						wrapBool(stream.writeString(",", false))
					}
					stream.writeStringFormat(false, "%f,%f,%f", norm.x, norm.y, norm.z);
				}
				wrapBool(stream.writeLine("\n\t\t}"))
			}

			if (withTexCoords) {
				wrapBool(stream.writeLine("\t\tLayerElementUV: 0 {"))
				wrapBool(stream.writeLine("\t\t\tVersion: 101"))
				wrapBool(stream.writeLine("\t\t\tName: \"\""))
				wrapBool(stream.writeLine("\t\t\tMappingInformationType: \"ByPolygonVertex\""))
				wrapBool(stream.writeLine("\t\t\tReferenceInformationType: \"Direct\""))
				wrapBool(stream.writeString("\t\t\tUV: ", false))

				for (int j = 0; j < ni; j++) {
					const uint32_t index = indices[j];
					const voxel::VoxelVertex &v = vertices[index];
					const glm::vec2 &uv = paletteUV(v.colorIndex);
					if (j > 0) {
						wrapBool(stream.writeString(",", false))
					}
					stream.writeStringFormat(false, "%f,%f", uv.x, uv.y);
				}
				wrapBool(stream.writeString("\n\t\t}\n", false))
				// TODO: VOXELFORMAT: UVIndex needed or only for IndexToDirect?

				wrapBool(stream.writeString("\t\tLayerElementTexture: 0 {\n"
											"\t\t\tVersion: 101\n"
											"\t\t\tName: \"\"\n"
											"\t\t\tMappingInformationType: \"AllSame\"\n"
											"\t\t\tReferenceInformationType: \"Direct\"\n"
											"\t\t\tBlendMode: \"Translucent\"\n"
											"\t\t\tTextureAlpha: 1\n"
											"\t\t\tTextureId: 0\n"
											"\t\t}\n",
											false))
			}

			if (withColor) {
				wrapBool(stream.writeString("\t\tLayerElementColor: 0 {\n"
											"\t\t\tVersion: 101\n"
											"\t\t\tName: \"\"\n"
											"\t\t\tMappingInformationType: \"ByPolygonVertex\"\n"
											"\t\t\tReferenceInformationType: \"Direct\"\n"
											"\t\t\tColors: ",
											false))
				for (int j = 0; j < ni; j++) {
					const uint32_t index = indices[j];
					const voxel::VoxelVertex &v = vertices[index];
					const glm::vec4 &color = core::Color::fromRGBA(palette.color(v.colorIndex));
					if (j > 0) {
						wrapBool(stream.writeString(",", false))
					}
					stream.writeStringFormat(false, "%f,%f,%f,%f", color.r, color.g, color.b, color.a);
				}
				// close LayerElementColor

				wrapBool(stream.writeLine("\n\t\t}"))
				// TODO: VOXELFORMAT: ColorIndex needed or only for IndexToDirect?
			}

			wrapBool(stream.writeString("\t\tLayer: 0 {\n"
										"\t\t\tVersion: 100\n",
										false))

			if (withColor) {
				wrapBool(stream.writeString("\t\t\tLayerElement: {\n"
											"\t\t\t\tTypedIndex: 0\n"
											"\t\t\t\tType: \"LayerElementColor\"\n"
											"\t\t\t}\n",
											false))
			}
			if (withTexCoords) {
				wrapBool(stream.writeString("\t\t\tLayerElement: {\n"
											"\t\t\t\tTypedIndex: 0\n"
											"\t\t\t\tType: \"LayerElementUV\"\n"
											"\t\t\t}\n",
											false))
				// TODO: VOXELFORMAT: LayerElementTexture
			}
			if (exportNormals) {
				wrapBool(stream.writeString("\t\t\tLayerElement: {\n"
											"\t\t\t\tTypedIndex: 0\n"
											"\t\t\t\tType: \"LayerElementNormal\"\n"
											"\t\t\t}\n",
											false))
			}
			wrapBool(stream.writeLine("\t\t}"))

			// close the model
			wrapBool(stream.writeLine("\t}"))
			++objectIndex;
		}
	}

	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &graphNode = e->second;
		if (!graphNode.isCameraNode()) {
			continue;
		}
		const char *objectName = graphNode.name().c_str();
		if (objectName[0] == '\0') {
			objectName = graphNode.uuid().c_str();
		}
		const core::String modelName = core::String::format("Model::%s-%u", objectName, objectIndex);
		connections.push_back(modelName);
		stream.writeStringFormat(false, "\tModel: \"%s\", \"Camera\" {\n", modelName.c_str());
		wrapBool(stream.writeLine("\t\tVersion: 232"))
		wrapBool(stream.writeLine("\t\tProperties60:  {"))
		scenegraph::KeyFrameIndex keyFrameIndex = 0;
		writeTransformToProperties(stream, graphNode.transform(keyFrameIndex));
		stream.writeStringFormat(false, "\t\t\tProperty: \"Show\", \"bool\", \"\",%u\n", graphNode.visible() ? 1 : 0);
		// TODO: VOXELFORMAT:
		// Property: "NearPlane", "double", "",0.100000
		// Property: "FarPlane", "double", "",99.999994
		// Property: "CameraProjectionType", "enum", "",0
		wrapBool(stream.writeLine("\t\t}"))
		// close the model
		wrapBool(stream.writeLine("\t}"))
		++objectIndex;
	}

	// close objects
	wrapBool(stream.writeLine("}"))

	wrapBool(stream.writeLine("Connections:  {"))
	for (const core::String &connection : connections) {
		stream.writeStringFormat(false, "\tConnect: \"OO\", \"%s\", \"Model::Scene\"\n", connection.c_str());
	}
	wrapBool(stream.writeLine("}"))
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

static inline glm::vec3 _ufbx_to_vec3(const ufbx_vec3 &v) {
	return glm::vec3((float)v.x, (float)v.y, (float)v.z);
}

static inline glm::vec4 _ufbx_to_vec4(const ufbx_vec4 &v) {
	return glm::vec4((float)v.x, (float)v.y, (float)v.z, (float)v.w);
}

static inline core::String _ufbx_to_string(const ufbx_string &s) {
	return core::String(s.data, s.length);
}

static inline glm::quat _ufbx_to_quat(const ufbx_quat &v) {
	return glm::quat((float)v.x, (float)v.y, (float)v.z, (float)v.w);
}

static inline void _ufbx_to_transform(scenegraph::SceneGraphTransform &transform, const ufbx_scene *scene,
									  const ufbx_node *node, const glm::vec3 &scale) {
	const ufbx_transform ufbxTransform = ufbx_evaluate_transform(scene->anim, node, 1.0);
	transform.setLocalTranslation(priv::_ufbx_to_vec3(ufbxTransform.translation) * scale);
	transform.setLocalOrientation(priv::_ufbx_to_quat(ufbxTransform.rotation));
	transform.setLocalScale(priv::_ufbx_to_vec3(ufbxTransform.scale));
}

static core::RGBA _ufbx_to_rgba(const ufbx_material_map &materialMap) {
	glm::vec4 color(1.0f);
	if (materialMap.value_components == 1) {
		color = glm::vec4(materialMap.value_real, materialMap.value_real, materialMap.value_real, 1.0f);
	} else if (materialMap.value_components == 3) {
		color = glm::vec4(_ufbx_to_vec3(materialMap.value_vec3), 1.0f);
	} else if (materialMap.value_components == 4) {
		color = _ufbx_to_vec4(materialMap.value_vec4);
	}
	return core::Color::getRGBA(color);
}

} // namespace priv

int FBXFormat::addMeshNode(const ufbx_scene *scene, const ufbx_node *node, const core::String &filename,
						   const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent) const {
	Log::debug("Add model node");
	const glm::vec3 &scale = getInputScale();
	ufbx_vec2 defaultUV;
	core_memset(&defaultUV, 0, sizeof(defaultUV));
	const ufbx_mesh *mesh = node->mesh;
	core_assert(mesh != nullptr);

	const size_t numTriIndices = mesh->max_face_triangles * 3;
	core::Buffer<uint32_t> triIndices(numTriIndices);

	MeshTriCollection tris;
	MeshMaterialArray meshMaterialArray;
	tris.reserve(numTriIndices);

	Log::debug("There are %i materials in the mesh", (int)mesh->materials.count);
	Log::debug("Vertex colors: %s", mesh->vertex_color.exists ? "true" : "false");
	Log::debug("Scene meter scale: %f", scene->settings.unit_meters);
	Log::debug("Scene original meter scale: %f", scene->settings.original_unit_meters);

	for (const ufbx_mesh_part &meshMaterial : mesh->material_parts) {
		if (meshMaterial.num_triangles == 0) {
			continue;
		}
		Log::debug("Faces: %i - material: %s", (int)meshMaterial.num_faces,
				   mesh->materials[meshMaterial.index] ? "yes" : "no");

		MeshMaterialPtr mat = createMaterial("default");

		if (const ufbx_material *fbxmaterial = mesh->materials[meshMaterial.index]) {
			const core::String &matname = priv::_ufbx_to_string(fbxmaterial->name);
			if (matname.empty()) {
				continue;
			}
			mat = createMaterial(matname);
			const ufbx_texture *texture = fbxmaterial->fbx.diffuse_color.texture;
			if (texture == nullptr) {
				texture = fbxmaterial->pbr.base_color.texture;
			}

			if (texture) {
				const core::String &fbxTextureFilename = priv::_ufbx_to_string(texture->relative_filename);
				const core::String &textureName = lookupTexture(filename, fbxTextureFilename, archive);
				if (!textureName.empty()) {
					const image::ImagePtr &tex = image::loadImage(textureName);
					if (tex->isLoaded()) {
						Log::debug("Use image %s", textureName.c_str());
						mat->texture = tex;
					}
				} else {
					Log::debug("Failed to load image %s for material %s", fbxTextureFilename.c_str(), matname.c_str());
				}
			}
			if (fbxmaterial->features.pbr.enabled) {
				if (fbxmaterial->pbr.base_factor.has_value) {
					mat->baseColorFactor = (float)fbxmaterial->pbr.base_factor.value_real;
				}
				if (fbxmaterial->pbr.base_color.has_value) {
					mat->baseColor = priv::_ufbx_to_rgba(fbxmaterial->pbr.base_color);
				}
				if (fbxmaterial->pbr.metalness.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialMetal,
										   fbxmaterial->pbr.metalness.value_real);
				}
				if (fbxmaterial->pbr.roughness.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialRoughness,
										   fbxmaterial->pbr.roughness.value_real);
				}
				if (fbxmaterial->pbr.specular_ior.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialIndexOfRefraction,
										   fbxmaterial->pbr.specular_ior.value_real);
				}
				if (fbxmaterial->pbr.opacity.has_value) {
					mat->transparency = 1.0f - fbxmaterial->pbr.opacity.value_real;
				}
				if (fbxmaterial->pbr.glossiness.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialPhase,
										   fbxmaterial->pbr.glossiness.value_real);
				}
				if (fbxmaterial->pbr.specular_factor.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialSpecular,
										   fbxmaterial->pbr.specular_factor.value_real);
				}
				if (fbxmaterial->pbr.emission_factor.has_value) {
					mat->material.setValue(palette::MaterialProperty::MaterialEmit,
										   fbxmaterial->pbr.emission_factor.value_real);
				}
				if (fbxmaterial->pbr.emission_color.has_value) {
					mat->emitColor = priv::_ufbx_to_rgba(fbxmaterial->pbr.emission_color);
				}
			}
		} else {
			Log::debug("No material assigned for mesh");
		}
		meshMaterialArray.push_back(mat);

		for (size_t fi = 0; fi < meshMaterial.num_faces; fi++) {
			const ufbx_face face = mesh->faces[meshMaterial.face_indices[fi]];
			const size_t numTris = ufbx_triangulate_face(triIndices.data(), numTriIndices, mesh, face);

			for (size_t vi = 0; vi < numTris; vi++) {
				voxelformat::MeshTri meshTri;
				const uint32_t idx0 = triIndices[vi * 3 + 0];
				const uint32_t idx1 = triIndices[vi * 3 + 1];
				const uint32_t idx2 = triIndices[vi * 3 + 2];
				const ufbx_vec3 &vertex0 = ufbx_get_vertex_vec3(&mesh->vertex_position, idx0);
				const ufbx_vec3 &vertex1 = ufbx_get_vertex_vec3(&mesh->vertex_position, idx1);
				const ufbx_vec3 &vertex2 = ufbx_get_vertex_vec3(&mesh->vertex_position, idx2);
				// TODO: VOXELFORMAT: transform here - see issue
				// https://github.com/vengi-voxel/vengi/issues/227
				meshTri.setVertices(priv::_ufbx_to_vec3(vertex0) * scale, priv::_ufbx_to_vec3(vertex1) * scale,
									priv::_ufbx_to_vec3(vertex2) * scale);
				if (mesh->vertex_color.exists) {
					const ufbx_vec4 &color0 = ufbx_get_vertex_vec4(&mesh->vertex_color, idx0);
					const ufbx_vec4 &color1 = ufbx_get_vertex_vec4(&mesh->vertex_color, idx1);
					const ufbx_vec4 &color2 = ufbx_get_vertex_vec4(&mesh->vertex_color, idx2);
					meshTri.setColor(core::Color::getRGBA(priv::_ufbx_to_vec4(color0)),
									 core::Color::getRGBA(priv::_ufbx_to_vec4(color1)),
									 core::Color::getRGBA(priv::_ufbx_to_vec4(color2)));
				}
				meshTri.materialIdx = meshMaterialArray.size() - 1;
				tris.emplace_back(core::move(meshTri));
			}
		}
	}
	const core::String &name = priv::_ufbx_to_string(node->name);
	const int nodeId = voxelizeNode(name, sceneGraph, tris, meshMaterialArray, parent, false);
	if (nodeId < 0) {
		Log::error("Failed to voxelize node %s", name.c_str());
		return nodeId;
	}

	scenegraph::SceneGraphNode &sceneGraphNode = sceneGraph.node(nodeId);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphTransform &transform = sceneGraphNode.keyFrame(keyFrameIdx).transform();
	priv::_ufbx_to_transform(transform, scene, node, scale);
	for (const auto &prop : node->props.props) {
		if ((prop.flags & UFBX_PROP_FLAG_NO_VALUE) != 0) {
			continue;
		}
		sceneGraphNode.setProperty(priv::_ufbx_to_string(prop.name), priv::_ufbx_to_string(prop.value_str));
	}
	sceneGraphNode.setTransform(keyFrameIdx, transform);
	sceneGraphNode.setVisible(node->visible);
	// TODO: VOXELFORMAT: animations - see ufbx_evaluate_transform
	return nodeId;
}

int FBXFormat::addCameraNode(const ufbx_scene *scene, const ufbx_node *node, scenegraph::SceneGraph &sceneGraph,
							 int parent) const {
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
	priv::_ufbx_to_transform(transform, scene, node, glm::vec3(1.0f));
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	camNode.setTransform(keyFrameIdx, transform);
	return sceneGraph.emplace(core::move(camNode), parent);
}

int FBXFormat::addNode_r(const ufbx_scene *scene, const ufbx_node *node, const core::String &filename,
						 const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent) const {
	int nodeId = parent;
	if (node->mesh != nullptr) {
		nodeId = addMeshNode(scene, node, filename, archive, sceneGraph, parent);
	} else if (node->camera != nullptr) {
		nodeId = addCameraNode(scene, node, sceneGraph, parent);
	} else if (node->light != nullptr) {
		Log::debug("Skip light node");
	} else if (ufbx_as_bone(node->attrib) != nullptr) {
		Log::debug("Skip bone node");
	} else {
		Log::debug("Skip unknown node");
	}
	if (nodeId < 0) {
		Log::error("Failed to add node with parent %i", parent);
		return nodeId;
	}
	for (const ufbx_node *c : node->children) {
		const int newNodeId = addNode_r(scene, c, filename, archive, sceneGraph, nodeId);
		if (newNodeId < 0) {
			const core::String name = priv::_ufbx_to_string(node->name);
			Log::error("Failed to add child node '%s'", name.c_str());
			return newNodeId;
		}
	}
	return nodeId;
}

bool FBXFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	ufbx_stream ufbxstream;
	core_memset(&ufbxstream, 0, sizeof(ufbxstream));
	ufbxstream.user = stream;
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

	// TODO: VOXELFORMAT: see issue https://github.com/vengi-voxel/vengi/issues/227
	ufbxopts.target_axes = ufbx_axes_right_handed_y_up;
	ufbxopts.target_unit_meters = 1.0f;
	ufbxopts.target_light_axes = ufbxopts.target_axes;
	ufbxopts.target_camera_axes = ufbxopts.target_axes;
	ufbxopts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	ufbxopts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK;
	ufbxopts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_IGNORE;
	ufbxopts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT;
	ufbxopts.generate_missing_normals = true;

	ufbx_error ufbxerror;

	ufbx_scene *ufbxscene = ufbx_load_stream(&ufbxstream, &ufbxopts, &ufbxerror);
	if (!ufbxscene) {
		Log::error("Failed to load fbx scene: %s", ufbxerror.description.data);
		return false;
	}
	if (ufbxerror.type != UFBX_ERROR_NONE) {
		char err[4096];
		ufbx_format_error(err, sizeof(err), &ufbxerror);
		Log::error("Error while loading fbx: %s", err);
	}

	Log::debug("right: %i, up: %i, front: %i", ufbxscene->settings.axes.right, ufbxscene->settings.axes.up,
			   ufbxscene->settings.axes.front);

	const ufbx_node *root = ufbxscene->root_node;
	for (const ufbx_node *c : root->children) {
		if (addNode_r(ufbxscene, c, filename, archive, sceneGraph, sceneGraph.root().id()) < 0) {
			const core::String name = priv::_ufbx_to_string(c->name);
			Log::error("Failed to add root child node '%s'", name.c_str());
			ufbx_free_scene(ufbxscene);
			return false;
		}
	}

	ufbx_free_scene(ufbxscene);
	return !sceneGraph.empty();
}

#undef wrapBool

} // namespace voxelformat
