/**
 * @file
 */

#include "FBXFormat.h"
#include "app/App.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/MemoryReadStream.h"
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

bool FBXFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph,
						   const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
						   const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	return saveMeshesAscii(meshes, filename, *stream, scale, quad, withColor, withTexCoords, sceneGraph);
}

bool FBXFormat::saveRecursiveNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								  const core::String &filename, io::SeekableWriteStream &stream,
								  uint32_t sentinelLength) {
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

bool FBXFormat::saveMeshesBinary(const ChunkMeshes &meshes, const core::String &filename,
								 io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad, bool withColor,
								 bool withTexCoords, const scenegraph::SceneGraph &sceneGraph) {
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
bool FBXFormat::saveMeshesAscii(const ChunkMeshes &meshes, const core::String &filename,
								io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad, bool withColor,
								bool withTexCoords, const scenegraph::SceneGraph &sceneGraph) {
	int meshCount = 0;
	for (const ChunkMeshExt &meshExt : meshes) {
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

	for (const ChunkMeshExt &meshExt : meshes) {
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
			const core::String &uuidStr = graphNode.uuid().str();
			if (objectName[0] == '\0') {
				objectName = uuidStr.c_str();
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
					const glm::vec4 &color = color::Color::fromRGBA(palette.color(v.colorIndex));
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
		const core::String &uuidStr = graphNode.uuid().str();
		if (objectName[0] == '\0') {
			objectName = uuidStr.c_str();
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

#if 0
static inline glm::mat4 _ufbx_to_mat(const ufbx_matrix& v) {
	glm::mat4 mat(1.0f);
	for (int column = 0; column < 4; ++column) {
		mat[column].x = v.cols[column].x;
		mat[column].y = v.cols[column].y;
		mat[column].z = v.cols[column].z;
	}
	return mat;
}
#endif

static inline void _ufbx_to_transform(scenegraph::SceneGraphTransform &transform, const ufbx_scene *ufbxScene,
									  const ufbx_node *ufbxNode, const glm::vec3 &scale) {
#if 0
	transform.setWorldMatrix(priv::_ufbx_to_mat(ufbxNode->unscaled_node_to_world));
#else
	const ufbx_transform ufbxTransform = ufbx_evaluate_transform(ufbxScene->anim, ufbxNode, 1.0);
	transform.setLocalTranslation(priv::_ufbx_to_vec3(ufbxTransform.translation) * scale);
	transform.setLocalOrientation(priv::_ufbx_to_quat(ufbxTransform.rotation));
#endif
	// transform.setLocalScale(priv::_ufbx_to_vec3(ufbxTransform.scale)); // UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY is
	// used - localScale not needed
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
	return color::Color::getRGBA(color);
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
					// TODO: VOXELFORMAT: this is sRGB - need to convert to linear
					meshTri.setColor(color::Color::getRGBA(priv::_ufbx_to_vec4(color0)),
									 color::Color::getRGBA(priv::_ufbx_to_vec4(color1)),
									 color::Color::getRGBA(priv::_ufbx_to_vec4(color2)));
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
	const int nodeId = voxelizeMesh(name, sceneGraph, core::move(mesh), parent, false);
	if (nodeId < 0) {
		Log::error("Failed to voxelize node %s", name.c_str());
		return nodeId;
	}

	scenegraph::SceneGraphNode &sceneGraphNode = sceneGraph.node(nodeId);
	sceneGraphNode.setVisible(ufbxNode->visible);

#if 0
	// TODO: VOXELFORMAT: animations - see ufbx_evaluate_transform
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphTransform &transform = sceneGraphNode.keyFrame(keyFrameIdx).transform();
	priv::_ufbx_to_transform(transform, ufbxScene, ufbxNode, getInputScale());
	for (const ufbx_prop &ufbxProp : ufbxNode->props.props) {
		if ((ufbxProp.flags & UFBX_PROP_FLAG_NO_VALUE) != 0) {
			continue;
		}
		sceneGraphNode.setProperty(priv::_ufbx_to_string(ufbxProp.name), priv::_ufbx_to_string(ufbxProp.value_str));
	}
	sceneGraphNode.setTransform(keyFrameIdx, transform);
#endif
	return nodeId;
}

int FBXFormat::addCameraNode(const ufbx_scene *ufbxScene, const ufbx_node *ufbxNode, scenegraph::SceneGraph &sceneGraph,
							 int parent) const {
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
	priv::_ufbx_to_transform(transform, ufbxScene, ufbxNode, glm::vec3(1.0f));
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	camNode.setTransform(keyFrameIdx, transform);
	return sceneGraph.emplace(core::move(camNode), parent);
}

int FBXFormat::addNode_r(const ufbx_scene *ufbxScene, const ufbx_node *ufbxNode, const core::String &filename,
						 const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph, int parent) const {
	int nodeId = parent;
	if (ufbxNode->attrib_type == UFBX_ELEMENT_MESH) {
		nodeId = addMeshNode(ufbxScene, ufbxNode, filename, archive, sceneGraph, parent);
	} else if (ufbxNode->attrib_type == UFBX_ELEMENT_CAMERA) {
		nodeId = addCameraNode(ufbxScene, ufbxNode, sceneGraph, parent);
	} else {
		Log::debug("Unhandled node type: %i", ufbxNode->attrib_type);
	}
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to add node with parent %i", parent);
		return nodeId;
	}
	for (const ufbx_node *ufbxChildNode : ufbxNode->children) {
		const int newNodeId = addNode_r(ufbxScene, ufbxChildNode, filename, archive, sceneGraph, nodeId);
		if (newNodeId == InvalidNodeId) {
			const core::String name = priv::_ufbx_to_string(ufbxNode->name);
			Log::error("Failed to add child node '%s'", name.c_str());
			return newNodeId;
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
	ufbxOpts.target_unit_meters = 1.0f;
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

	if (addNode_r(ufbxScene, ufbxScene->root_node, filename, archive, sceneGraph, sceneGraph.root().id()) < 0) {
		Log::error("Failed to add root child node");
		ufbx_free_scene(ufbxScene);
		return false;
	}

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
			uint8_t r, g, b, a = 255;
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

#undef wrapBool

} // namespace voxelformat
