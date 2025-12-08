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
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtc/type_ptr.hpp>

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
	stream.writeStringFormat(false, "\t\t\tProperty: \"Lcl Translation\", \"Lcl Translation\", \"\",%f,%f,%f\n",
							 lclTranslation.x, lclTranslation.y, lclTranslation.z);
	const glm::quat &lclRotationQuat = transform.localOrientation();
	const glm::vec3 lclRotationEulerDegrees(glm::degrees(glm::eulerAngles(lclRotationQuat)));
	stream.writeStringFormat(false, "\t\t\tProperty: \"Lcl Rotation\", \"Lcl Rotation\", \"\",%f,%f,%f\n",
							 lclRotationEulerDegrees.x, lclRotationEulerDegrees.y, lclRotationEulerDegrees.z);
	const glm::vec3 &lclScaling = transform.localScale();
	stream.writeStringFormat(false, "\t\t\tProperty: \"Lcl Scaling\", \"Lcl Scaling\", \"\",%f,%f,%f\n", lclScaling.x,
							 lclScaling.y, lclScaling.z);
	stream.writeStringFormat(false, "\t\t\tProperty: \"InheritType\", \"enum\", \"\",1\n");
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
	ObjectType: "Geometry" {
		Count: %i
	}
	ObjectType: "Material" {
		Count: %i
		PropertyTemplate: "FbxSurfacePhong" {
			Properties60:  {
				Property: "ShadingModel", "KString", "", "Phong"
				Property: "MultiLayer", "bool", "", 0
				Property: "EmissiveColor", "ColorRGB", "", 0, 0, 0
				Property: "EmissiveFactor", "double", "", 1
				Property: "AmbientColor", "ColorRGB", "", 0.2, 0.2, 0.2
				Property: "AmbientFactor", "double", "", 1
				Property: "DiffuseColor", "ColorRGB", "", 0.8, 0.8, 0.8
				Property: "DiffuseFactor", "double", "", 1
				Property: "Bump", "Vector3D", "", 0, 0, 0
				Property: "NormalMap", "Vector3D", "", 0, 0, 0
				Property: "BumpFactor", "double", "", 1
				Property: "TransparentColor", "ColorRGB", "", 0, 0, 0
				Property: "TransparencyFactor", "double", "", 0
				Property: "DisplacementColor", "ColorRGB", "", 0, 0, 0
				Property: "DisplacementFactor", "double", "", 1
				Property: "VectorDisplacementColor", "ColorRGB", "", 0, 0, 0
				Property: "VectorDisplacementFactor", "double", "", 1
				Property: "SpecularColor", "ColorRGB", "", 0.2, 0.2, 0.2
				Property: "SpecularFactor", "double", "", 1
				Property: "ShininessExponent", "double", "", 20
				Property: "ReflectionColor", "ColorRGB", "", 0, 0, 0
				Property: "ReflectionFactor", "double", "", 1
			}
		}
	}
	ObjectType: "GlobalSettings" {
		Count: 1
	}
}

Objects: {
	GlobalSettings:  {
		Version: 1000
		Properties60:  {
			Property: "UpAxis", "int", "",1
			Property: "UpAxisSign", "int", "",1
			Property: "FrontAxis", "int", "",2
			Property: "FrontAxisSign", "int", "",1
			Property: "CoordAxis", "int", "",0
			Property: "CoordAxisSign", "int", "",1
			Property: "OriginalUpAxis", "int", "",1
			Property: "OriginalUpAxisSign", "int", "",1
			Property: "UnitScaleFactor", "double", "",1.0
			Property: "OriginalUnitScaleFactor", "double", "",1.0
			Property: "AmbientColor", "ColorRGB", "",0,0,0
			Property: "DefaultCamera", "KString", "", "Producer Perspective"
			Property: "TimeMode", "enum", "",6
			Property: "TimeSpan", "time", "",0,4611686018427387904
		}
	}
)",
							 PROJECT_VERSION, app::App::getInstance()->fullAppname().c_str(), PROJECT_VERSION,
							 meshCount, meshCount, meshCount);

	Log::debug("Exporting %i models", meshCount);

	// https://github.com/libgdx/fbx-conv/blob/master/samples/blender/cube.fbx

	uint32_t objectIndex = 0;
	core::DynamicArray<core::String> connections;
	core::Map<int, core::DynamicArray<core::String>> nodeModelNames;

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
			auto iter = nodeModelNames.find(meshExt.nodeId);
			if (iter == nodeModelNames.end()) {
				core::DynamicArray<core::String> names;
				names.push_back(modelName);
				nodeModelNames.put(meshExt.nodeId, names);
			} else {
				iter->value.push_back(modelName);
			}
			const core::String geometryName = core::String::format("Geometry::%s-%u", objectName, objectIndex);
			connections.push_back(
				core::String::format("\tConnect: \"OO\", \"%s\", \"%s\"\n", geometryName.c_str(), modelName.c_str()));
			const core::String materialName = core::String::format("Material::Material-%u", objectIndex);
			connections.push_back(
				core::String::format("\tConnect: \"OO\", \"%s\", \"%s\"\n", materialName.c_str(), modelName.c_str()));

			// TODO: MATERIAL: implement palette material export
			stream.writeStringFormat(false, "\tMaterial: \"%s\", \"\" {\n", materialName.c_str());
			wrapBool(stream.writeLine("\t\tVersion: 102"))
			wrapBool(stream.writeLine("\t\tShadingModel: \"Phong\""))
			wrapBool(stream.writeLine("\t\tMultiLayer: 0"))
			wrapBool(stream.writeLine("\t\tProperties60:  {"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"ShadingModel\", \"KString\", \"\", \"Phong\""))
			wrapBool(stream.writeLine("\t\t\tProperty: \"MultiLayer\", \"bool\", \"\",0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"EmissiveColor\", \"ColorRGB\", \"\",0,0,0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"EmissiveFactor\", \"double\", \"\",1"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"AmbientColor\", \"ColorRGB\", \"\",0.2,0.2,0.2"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"AmbientFactor\", \"double\", \"\",1"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"DiffuseColor\", \"ColorRGB\", \"\",0.8,0.8,0.8"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"DiffuseFactor\", \"double\", \"\",1"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"Bump\", \"Vector3D\", \"\",0,0,0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"NormalMap\", \"Vector3D\", \"\",0,0,0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"BumpFactor\", \"double\", \"\",1"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"TransparentColor\", \"ColorRGB\", \"\",0,0,0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"TransparencyFactor\", \"double\", \"\",0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"DisplacementColor\", \"ColorRGB\", \"\",0,0,0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"DisplacementFactor\", \"double\", \"\",1"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"VectorDisplacementColor\", \"ColorRGB\", \"\",0,0,0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"VectorDisplacementFactor\", \"double\", \"\",1"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"SpecularColor\", \"ColorRGB\", \"\",0.2,0.2,0.2"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"SpecularFactor\", \"double\", \"\",1"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"ShininessExponent\", \"double\", \"\",20"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"ReflectionColor\", \"ColorRGB\", \"\",0,0,0"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"ReflectionFactor\", \"double\", \"\",1"))
			wrapBool(stream.writeLine("\t\t}"))
			wrapBool(stream.writeLine("\t}"))

			stream.writeStringFormat(false, "\tModel: \"%s\", \"Mesh\" {\n", modelName.c_str());
			wrapBool(stream.writeLine("\t\tVersion: 232"))
			wrapBool(stream.writeLine("\t\tProperties60:  {"))
			stream.writeStringFormat(false, "\t\t\tProperty: \"Show\", \"bool\", \"\",%u\n",
									 graphNode.visible() ? 1 : 0);
			stream.writeStringFormat(false, "\t\t\tProperty: \"DefaultAttributeIndex\", \"int\", \"\",0\n");
			if (meshExt.applyTransform) {
				writeTransformToProperties(stream, scenegraph::SceneGraphTransform());
			} else {
				writeTransformToProperties(stream, graphNode.transform(keyFrameIdx));
			}
			wrapBool(stream.writeLine("\t\t}"))
			wrapBool(stream.writeLine("\t\tShading: Y"))
			wrapBool(stream.writeLine("\t\tCulling: \"CullingOff\""))
			wrapBool(stream.writeLine("\t}"))

			stream.writeStringFormat(false, "\tGeometry: \"%s\", \"Mesh\" {\n", geometryName.c_str());
			wrapBool(stream.writeLine("\t\tProperties60:  {"))
			wrapBool(stream.writeLine("\t\t\tProperty: \"Color\", \"ColorRGB\", \"\",0.8,0.8,0.8"))
			wrapBool(stream.writeLine("\t\t}"))

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

			wrapBool(stream.writeString("\t\tLayerElementMaterial: 0 {\n"
										"\t\t\tVersion: 101\n"
										"\t\t\tName: \"\"\n"
										"\t\t\tMappingInformationType: \"ByPolygon\"\n"
										"\t\t\tReferenceInformationType: \"IndexToDirect\"\n"
										"\t\t\tMaterials: ",
										false))
			const int polyCount = ni / 3;
			stream.writeStringFormat(false, "*%i {\n\t\t\t\ta: ", polyCount);
			for (int k = 0; k < polyCount; ++k) {
				if (k > 0) {
					stream.writeString(",", false);
				}
				stream.writeString("0", false);
			}
			wrapBool(stream.writeString("\n\t\t\t}\n\t\t}\n", false))

			if (exportNormals) {
				stream.writeString("\t\tLayerElementNormal: 0 {\n"
								   "\t\t\tVersion: 101\n"
								   "\t\t\tName: \"\"\n"
								   "\t\t\tMappingInformationType: \"ByVertice\"\n"
								   "\t\t\tReferenceInformationType: \"Direct\"\n",
								   false);

				wrapBool(stream.writeString("\t\t\tNormals: ", false))
				for (size_t j = 0; j < normals.size(); j++) {
					const glm::vec3 &norm = normals[j];
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
			}
			if (withColor) {
				wrapBool(stream.writeString("\t\tLayerElementColor: 0 {\n"
											"\t\t\tVersion: 101\n"
											"\t\t\tName: \"\"\n"
											"\t\t\tMappingInformationType: \"ByVertice\"\n"
											"\t\t\tReferenceInformationType: \"Direct\"\n"
											"\t\t\tColors: ",
											false))
				for (int j = 0; j < nv; j++) {
					const voxel::VoxelVertex &v = vertices[j];
					const glm::vec4 &color = color::fromRGBA(palette.color(v.colorIndex));
					if (j > 0) {
						wrapBool(stream.writeString(",", false))
					}
					stream.writeStringFormat(false, "%f,%f,%f,%f", color.r, color.g, color.b, color.a);
				}
				// close LayerElementColor

				wrapBool(stream.writeLine("\n\t\t}"))
			}

			wrapBool(stream.writeString("\t\tLayer: 0 {\n"
										"\t\t\tVersion: 100\n",
										false))

			wrapBool(stream.writeString("\t\t\tLayerElement: {\n"
										"\t\t\t\tTypedIndex: 0\n"
										"\t\t\t\tType: \"LayerElementMaterial\"\n"
										"\t\t\t}\n",
										false))

			if (exportNormals) {
				wrapBool(stream.writeString("\t\t\tLayerElement: {\n"
											"\t\t\t\tTypedIndex: 0\n"
											"\t\t\t\tType: \"LayerElementNormal\"\n"
											"\t\t\t}\n",
											false))
			}
			if (withTexCoords) {
				wrapBool(stream.writeString("\t\t\tLayerElement: {\n"
											"\t\t\t\tTypedIndex: 0\n"
											"\t\t\t\tType: \"LayerElementUV\"\n"
											"\t\t\t}\n",
											false))
			}
			if (withColor) {
				wrapBool(stream.writeString("\t\t\tLayerElement: {\n"
											"\t\t\t\tTypedIndex: 0\n"
											"\t\t\t\tType: \"LayerElementColor\"\n"
											"\t\t\t}\n",
											false))
			}
			wrapBool(stream.writeLine("\t\t}"))

			// close the geometry
			wrapBool(stream.writeLine("\t}"))
			++objectIndex;
		}
	}

	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &graphNode = e->second;
		if (nodeModelNames.find(graphNode.id()) != nodeModelNames.end()) {
			continue;
		}
		const char *objectName = graphNode.name().c_str();
		const core::String &uuidStr = graphNode.uuid().str();
		if (objectName[0] == '\0') {
			objectName = uuidStr.c_str();
		}
		const core::String modelName = core::String::format("Model::%s-%u", objectName, objectIndex);
		core::DynamicArray<core::String> names;
		names.push_back(modelName);
		nodeModelNames.put(graphNode.id(), names);

		const char *type = "Null";
		if (graphNode.isCameraNode()) {
			type = "Camera";
		}
		stream.writeStringFormat(false, "\tModel: \"%s\", \"%s\" {\n", modelName.c_str(), type);
		wrapBool(stream.writeLine("\t\tVersion: 232"))
		wrapBool(stream.writeLine("\t\tProperties60:  {"))
		scenegraph::KeyFrameIndex keyFrameIndex = 0;
		writeTransformToProperties(stream, graphNode.transform(keyFrameIndex));
		stream.writeStringFormat(false, "\t\t\tProperty: \"Show\", \"bool\", \"\",%u\n", graphNode.visible() ? 1 : 0);

		if (graphNode.isCameraNode()) {
			const scenegraph::SceneGraphNodeCamera &camera = scenegraph::toCameraNode(graphNode);
			stream.writeStringFormat(false, "\t\t\tProperty: \"NearPlane\", \"double\", \"\",%f\n", camera.nearPlane());
			stream.writeStringFormat(false, "\t\t\tProperty: \"FarPlane\", \"double\", \"\",%f\n", camera.farPlane());
			stream.writeStringFormat(false, "\t\t\tProperty: \"CameraProjectionType\", \"enum\", \"\",%d\n",
									 camera.isPerspective() ? 0 : 1);
		}

		wrapBool(stream.writeLine("\t\t}"))
		wrapBool(stream.writeLine("\t}"))
		++objectIndex;
	}

	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &graphNode = e->second;
		auto iter = nodeModelNames.find(graphNode.id());
		if (iter == nodeModelNames.end()) {
			continue;
		}
		const core::DynamicArray<core::String> &myModels = iter->value;
		int parentId = graphNode.parent();
		core::String parentModelName = "Model::Scene";
		if (parentId != -1) {
			auto parentIter = nodeModelNames.find(parentId);
			if (parentIter != nodeModelNames.end() && !parentIter->value.empty()) {
				parentModelName = parentIter->value[0];
			}
		}

		for (const core::String &modelName : myModels) {
			connections.push_back(core::String::format("\tConnect: \"OO\", \"%s\", \"%s\"\n", modelName.c_str(),
													   parentModelName.c_str()));
		}
	}

	// close objects
	wrapBool(stream.writeLine("}"))

	wrapBool(stream.writeLine("Connections:  {"))
	for (const core::String &connection : connections) {
		stream.writeString(connection, false);
	}
	wrapBool(stream.writeLine("}"))

	wrapBool(stream.writeLine("Takes:  {"))
	wrapBool(stream.writeLine("\tCurrent: \"Default\""))
	for (const core::String &anim : sceneGraph.animations()) {
		stream.writeStringFormat(false, "\tTake: \"%s\" {\n", anim.c_str());
		if (!stream.writeStringFormat(false, "\t\tFileName: \"%s.tak\"\n", anim.c_str())) {
			Log::error("Failed to write take filename");
			return false;
		}
		scenegraph::FrameIndex maxFrame = 0;
		for (const auto &e : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &graphNode = e->second;
			if (!graphNode.allKeyFrames().hasKey(anim)) {
				continue;
			}
			const scenegraph::SceneGraphKeyFrames &keyFrames = graphNode.keyFrames(anim);
			for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
				maxFrame = core_max(maxFrame, kf.frameIdx);
			}
		}
		const int64_t endTime = (int64_t)maxFrame * 1539538600L;
		stream.writeStringFormat(false, "\t\tLocalTime: 0, %" SDL_PRIs64 "\n", endTime);
		stream.writeStringFormat(false, "\t\tReferenceTime: 0, %" SDL_PRIs64 "\n", endTime);

		for (const auto &e : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &graphNode = e->second;
			if (!graphNode.allKeyFrames().hasKey(anim)) {
				continue;
			}
			const scenegraph::SceneGraphKeyFrames &keyFrames = graphNode.keyFrames(anim);
			if (keyFrames.empty()) {
				continue;
			}
			core::DynamicArray<core::String> modelNames;
			if (!nodeModelNames.get(graphNode.id(), modelNames)) {
				continue;
			}
			for (const core::String &modelName : modelNames) {
				stream.writeStringFormat(false, "\t\tModel: \"%s\" {\n", modelName.c_str());
				wrapBool(stream.writeLine("\t\t\tVersion: 100"))
				wrapBool(stream.writeLine("\t\t\tChannel: \"Transform\" {"))

				// Translation
				wrapBool(stream.writeLine("\t\t\t\tChannel: \"T\" {"))
				for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
					const int64_t time = (int64_t)kf.frameIdx * 1539538600L;
					const glm::vec3 &pos = kf.transform().localTranslation();
					stream.writeStringFormat(false, "\t\t\t\t\tKey: %" SDL_PRIs64 ",%f,%f,%f,L\n", time, pos.x, pos.y,
											 pos.z);
				}
				wrapBool(stream.writeLine("\t\t\t\t}"))

				// Rotation
				wrapBool(stream.writeLine("\t\t\t\tChannel: \"R\" {"))
				for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
					const int64_t time = (int64_t)kf.frameIdx * 1539538600L;
					const glm::quat &rot = kf.transform().localOrientation();
					const glm::vec3 euler = glm::degrees(glm::eulerAngles(rot));
					stream.writeStringFormat(false, "\t\t\t\t\tKey: %" SDL_PRIs64 ",%f,%f,%f,L\n", time, euler.x,
											 euler.y, euler.z);
				}
				wrapBool(stream.writeLine("\t\t\t\t}"))

				// Scaling
				wrapBool(stream.writeLine("\t\t\t\tChannel: \"S\" {"))
				for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
					const int64_t time = (int64_t)kf.frameIdx * 1539538600L;
					const glm::vec3 &localScale = kf.transform().localScale();
					stream.writeStringFormat(false, "\t\t\t\t\tKey: %" SDL_PRIs64 ",%f,%f,%f,L\n", time, localScale.x,
											 localScale.y, localScale.z);
				}
				wrapBool(stream.writeLine("\t\t\t\t}"))

				wrapBool(stream.writeLine("\t\t\t}")) // Channel: Transform
				wrapBool(stream.writeLine("\t\t}"))	  // Model
			}
		}
		wrapBool(stream.writeLine("\t}")) // Take
	}
	wrapBool(stream.writeLine("}")) // Takes

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

static inline void _ufbx_to_transform(scenegraph::SceneGraphTransform &transform, const ufbx_transform &ufbxTransform,
									  const glm::vec3 &scale) {
	transform.setLocalTranslation(priv::_ufbx_to_vec3(ufbxTransform.translation) * scale);
	transform.setLocalOrientation(priv::_ufbx_to_quat(ufbxTransform.rotation));
}

static inline void _ufbx_to_transform(scenegraph::SceneGraphTransform &transform, const ufbx_scene *ufbxScene,
									  const ufbx_node *ufbxNode, const glm::vec3 &scale) {
	const ufbx_transform ufbxTransform = ufbx_evaluate_transform(ufbxScene->anim, ufbxNode, 1.0);
	_ufbx_to_transform(transform, ufbxTransform, scale);
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
					// TODO: VOXELFORMAT: this is sRGB - need to convert to linear ??
#if 1
					meshTri.setColor(color::getRGBA(priv::_ufbx_to_vec4(color0)),
									 color::getRGBA(priv::_ufbx_to_vec4(color1)),
									 color::getRGBA(priv::_ufbx_to_vec4(color2)));
#else
					meshTri.setColor(color::getRGBA(glm::convertSRGBToLinear(priv::_ufbx_to_vec4(color0))),
									 color::getRGBA(glm::convertSRGBToLinear(priv::_ufbx_to_vec4(color1))),
									 color::getRGBA(glm::convertSRGBToLinear(priv::_ufbx_to_vec4(color2))));
#endif
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

	for (const ufbx_prop &ufbxProp : ufbxNode->props.props) {
		if ((ufbxProp.flags & UFBX_PROP_FLAG_NO_VALUE) != 0) {
			continue;
		}
		sceneGraphNode.setProperty(priv::_ufbx_to_string(ufbxProp.name), priv::_ufbx_to_string(ufbxProp.value_str));
	}
	return nodeId;
}

void FBXFormat::importAnimation(const ufbx_scene *ufbxScene, const ufbx_node *ufbxNode,
								scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &sceneGraphNode) const {
	for (const ufbx_anim_stack *stack : ufbxScene->anim_stacks) {
		const core::String &animId = priv::_ufbx_to_string(stack->name);
		const double duration = stack->time_end - stack->time_begin;
		if (duration <= 0.0) {
			Log::warn("Could not import animation '%s' with non-positive duration %f", animId.c_str(), duration);
			continue;
		}
		if (!sceneGraphNode.setAnimation(animId)) {
			Log::warn("Failed to set animation '%s' for node '%s'", animId.c_str(), sceneGraphNode.name().c_str());
			continue;
		}

		const int fps = ufbxScene->settings.frames_per_second > 0 ? (int)ufbxScene->settings.frames_per_second : 30;
		const int frames = (int)(duration * fps);
		Log::debug("Import %i frames for animation '%s' on node '%s' (duration: %f, fps: %i)", frames, animId.c_str(),
				   sceneGraphNode.name().c_str(), duration, fps);
		for (int i = 0; i < frames; ++i) {
			const double time = stack->time_begin + (double)i / (double)fps;
			const ufbx_transform ufbxTransform = ufbx_evaluate_transform(stack->anim, ufbxNode, time);
			scenegraph::KeyFrameIndex keyFrameIdx = sceneGraphNode.addKeyFrame(i);
			if (keyFrameIdx == InvalidKeyFrame) {
				keyFrameIdx = sceneGraphNode.keyFrameForFrame(i);
				if (keyFrameIdx == InvalidKeyFrame) {
					Log::warn("Failed to add or get keyframe %i/%i for animation '%s' on node '%s'", i, frames,
							  animId.c_str(), sceneGraphNode.name().c_str());
					continue;
				}
			}
			Log::debug("Import frame %i/%i for animation '%s' on node '%s'", i, frames, animId.c_str(),
					   sceneGraphNode.name().c_str());
			scenegraph::SceneGraphKeyFrame &keyFrame = sceneGraphNode.keyFrame(keyFrameIdx);
			keyFrame.interpolation = scenegraph::InterpolationType::Linear;
			priv::_ufbx_to_transform(keyFrame.transform(), ufbxTransform, getInputScale());
		}
	}
}

int FBXFormat::addGroupNode(const ufbx_scene *ufbxScene, const ufbx_node *ufbxNode, scenegraph::SceneGraph &sceneGraph,
							int parent) const {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
	node.setName(priv::_ufbx_to_string(ufbxNode->name));

	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphTransform &transform = node.keyFrame(keyFrameIdx).transform();
	priv::_ufbx_to_transform(transform, ufbxScene, ufbxNode, getInputScale());
	node.setTransform(keyFrameIdx, transform);

	return sceneGraph.emplace(core::move(node), parent);
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
	priv::_ufbx_to_transform(transform, ufbxScene, ufbxNode, getInputScale());
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
		nodeId = addGroupNode(ufbxScene, ufbxNode, sceneGraph, parent);
	}
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to add node with parent %i", parent);
		return nodeId;
	}

	importAnimation(ufbxScene, ufbxNode, sceneGraph, sceneGraph.node(nodeId));

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

	if (addNode_r(ufbxScene, ufbxScene->root_node, filename, archive, sceneGraph, sceneGraph.root().id()) < 0) {
		Log::error("Failed to add root child node");
		ufbx_free_scene(ufbxScene);
		return false;
	}
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
