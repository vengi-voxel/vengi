/**
 * @file
 */

#include "GLTFFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/ConfigVar.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/StdStreamBuf.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/TextureLookup.h"
#include "voxelutil/VoxelUtil.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits.h>

#define TINYGLTF_IMPLEMENTATION
// #define TINYGLTF_NO_FS // TODO: VOXELFORMAT: use our own file abstraction
#define JSON_HAS_CPP_11
// #define TINYGLTF_NOEXCEPTION
// #define TINYGLTF_NO_INCLUDE_STB_IMAGE
// #define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "voxelformat/external/tiny_gltf.h"

namespace voxelformat {

namespace _priv {

const float FPS = 24.0f;

static int addBuffer(tinygltf::Model &gltfModel, io::BufferedReadWriteStream &stream, const char *name) {
	tinygltf::Buffer gltfBuffer;
	gltfBuffer.name = name;
	gltfBuffer.data.insert(gltfBuffer.data.end(), stream.getBuffer(), stream.getBuffer() + stream.size());
	gltfModel.buffers.emplace_back(core::move(gltfBuffer));
	return (int)(gltfModel.buffers.size() - 1);
}

static image::TextureWrap convertTextureWrap(int wrap) {
	if (wrap == TINYGLTF_TEXTURE_WRAP_REPEAT) {
		return image::TextureWrap::Repeat;
	} else if (wrap == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) {
		return image::TextureWrap::ClampToEdge;
	} else if (wrap == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) {
		return image::TextureWrap::MirroredRepeat;
	}
	Log::warn("Unknown wrap mode found in sampler: %i", wrap);
	return image::TextureWrap::Repeat;
}

static core::RGBA toColor(const tinygltf::Accessor *gltfAttributeAccessor, const uint8_t *buf) {
	const bool hasAlpha = gltfAttributeAccessor->type == TINYGLTF_TYPE_VEC4;
	if (gltfAttributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
		io::MemoryReadStream colorStream(buf, hasAlpha ? 4 * sizeof(float) : 3 * sizeof(float));
		glm::vec4 color;
		colorStream.readFloat(color.r);
		colorStream.readFloat(color.g);
		colorStream.readFloat(color.b);
		if (hasAlpha) {
			colorStream.readFloat(color.a);
		} else {
			color.a = 1.0f;
		}
		return core::Color::getRGBA(color);
	} else if (gltfAttributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
		io::MemoryReadStream colorStream(buf, hasAlpha ? 4 * sizeof(float) : 3 * sizeof(float));
		core::RGBA color;
		colorStream.readUInt8(color.r);
		colorStream.readUInt8(color.g);
		colorStream.readUInt8(color.b);
		if (hasAlpha) {
			colorStream.readUInt8(color.a);
		} else {
			color.a = 255u;
		}
		return color;
	} else if (gltfAttributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
		io::MemoryReadStream colorStream(buf, hasAlpha ? 4 * sizeof(float) : 3 * sizeof(float));
		glm::u16vec4 color;
		colorStream.readUInt16(color.r);
		colorStream.readUInt16(color.g);
		colorStream.readUInt16(color.b);
		if (hasAlpha) {
			colorStream.readUInt16(color.a);
			color.a /= 256u;
		} else {
			color.a = 255u;
		}
		return core::RGBA(color.r / 256u, color.g / 256u, color.b / 256u, color.a);
	} else {
		Log::warn("Skip unknown type for vertex colors (%i)", gltfAttributeAccessor->componentType);
	}
	return core::RGBA(0, 0, 0, 255);
}

static tinygltf::Camera processCamera(const scenegraph::SceneGraphNodeCamera &camera) {
	tinygltf::Camera gltfCamera;
	gltfCamera.name = camera.name().c_str();
	if (camera.isPerspective()) {
		gltfCamera.type = "perspective";
		gltfCamera.perspective.aspectRatio = camera.aspectRatio();
		gltfCamera.perspective.yfov = camera.fieldOfView();
		gltfCamera.perspective.zfar = camera.farPlane();
		gltfCamera.perspective.znear = camera.nearPlane();
	} else if (camera.isOrthographic()) {
		gltfCamera.type = "orthographic";
		gltfCamera.orthographic.xmag = camera.width() / 2.0;
		gltfCamera.orthographic.ymag = camera.height() / 2.0;
		gltfCamera.orthographic.zfar = camera.farPlane();
		gltfCamera.orthographic.znear = camera.nearPlane();
	}
	return gltfCamera;
}

static bool validateCamera(const tinygltf::Camera &camera) {
	if (camera.type == "perspective") {
		if (camera.perspective.aspectRatio <= 0.0) {
			return false;
		}
		if (camera.perspective.yfov <= 0.0) {
			return false;
		}
		if (camera.perspective.znear <= 0.0) {
			return false;
		}
		if (camera.perspective.zfar <= 0.0) {
			return false;
		}
		// if (camera.perspective.zfar <= camera.perspective.znear) {
		// 	return false;
		// }
		return true;
	} else if (camera.type == "orthographic") {
		if (camera.orthographic.xmag == 0.0) {
			return false;
		}
		if (camera.orthographic.ymag == 0.0) {
			return false;
		}
		if (camera.orthographic.zfar <= camera.orthographic.znear) {
			return false;
		}
		return true;
	}
	return false;
}

} // namespace _priv

void GLTFFormat::createPointMesh(tinygltf::Model &gltfModel, const scenegraph::SceneGraphNode &node) const {
	tinygltf::Mesh gltfMesh;
	gltfMesh.name = node.name().c_str();
	const glm::vec3 position = node.transform().localTranslation();
	// create a mesh with a single point at the node origin
	tinygltf::Primitive gltfPrimitive;
	gltfPrimitive.mode = TINYGLTF_MODE_POINTS;
	gltfPrimitive.attributes["POSITION"] = (int)gltfModel.accessors.size();
	gltfMesh.primitives.emplace_back(core::move(gltfPrimitive));

	tinygltf::Accessor gltfAccessor;
	gltfAccessor.count = 1;
	gltfAccessor.type = TINYGLTF_TYPE_VEC3;
	gltfAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	gltfAccessor.minValues = {position.x, position.y, position.z};
	gltfAccessor.maxValues = {position.x, position.y, position.z};
	gltfAccessor.bufferView = (int)gltfModel.bufferViews.size();
	gltfModel.accessors.emplace_back(gltfAccessor);

	io::BufferedReadWriteStream os;
	os.writeFloat(position.x);
	os.writeFloat(position.y);
	os.writeFloat(position.z);

	tinygltf::BufferView gltfVerticesBufferView;
	gltfVerticesBufferView.buffer = (int)gltfModel.buffers.size();
	gltfVerticesBufferView.byteOffset = 0;
	gltfVerticesBufferView.byteLength = os.size();
	gltfVerticesBufferView.byteStride = 0;
	gltfVerticesBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

	gltfModel.bufferViews.emplace_back(core::move(gltfVerticesBufferView));

	tinygltf::Buffer gltfBuffer;
	gltfBuffer.data.insert(gltfBuffer.data.end(), os.getBuffer(), os.getBuffer() + os.size());
	gltfModel.buffers.emplace_back(core::move(gltfBuffer));
	gltfModel.meshes.emplace_back(core::move(gltfMesh));
}

void GLTFFormat::saveGltfNode(core::Map<int, int> &nodeMapping, tinygltf::Model &gltfModel, tinygltf::Scene &gltfScene,
							  const scenegraph::SceneGraphNode &node, Stack &stack,
							  const scenegraph::SceneGraph &sceneGraph, const glm::vec3 &scale, bool exportAnimations) {
	tinygltf::Node gltfNode;
	if (node.isAnyModelNode()) {
		gltfNode.mesh = (int)gltfModel.meshes.size();
	}
	if (node.type() == scenegraph::SceneGraphNodeType::Point) {
		createPointMesh(gltfModel, node);
		gltfNode.mesh = (int)gltfModel.meshes.size();
	}
	gltfNode.name = node.name().c_str();
	Log::debug("process node %s", gltfNode.name.c_str());
	const int idx = (int)gltfModel.nodes.size();

	if (!exportAnimations) {
		glm::mat4x4 nodeLocalMatrix = node.transform().localMatrix();
		if (node.id() == 0) {
			nodeLocalMatrix = glm::scale(nodeLocalMatrix, scale);
		}

		if (nodeLocalMatrix != glm::mat4(1.0f)) {
			std::vector<double> nodeMatrixArray;
			nodeMatrixArray.reserve(16);
			const float *pSource = (const float *)glm::value_ptr(nodeLocalMatrix);

			for (int i = 0; i < 16; ++i) {
				nodeMatrixArray.push_back(pSource[i]);
			}

			gltfNode.matrix = core::move(nodeMatrixArray);
		}
	}

	gltfModel.nodes.emplace_back(core::move(gltfNode));
	nodeMapping.put(node.id(), idx);

	if (!stack.empty() && stack.back().second != -1) {
		gltfModel.nodes[stack.back().second].children.push_back(idx);
	} else {
		gltfScene.nodes.push_back(idx);
	}

	if (!stack.empty()) {
		stack.pop();
	}

	const scenegraph::SceneGraphNodeChildren &nodeChildren = node.children();

	for (int i = (int)nodeChildren.size() - 1; i >= 0; i--) {
		stack.emplace_back(nodeChildren[i], idx);
	}
}

uint32_t GLTFFormat::writeBuffer(const voxel::Mesh *mesh, uint8_t idx, io::SeekableWriteStream &os, bool withColor,
								 bool withTexCoords, bool colorAsFloat, bool exportNormals, bool applyTransform,
								 const glm::vec3 &pivotOffset, const palette::Palette &palette, Bounds &bounds) {
	const int nv = (int)mesh->getNoOfVertices();
	const int ni = (int)mesh->getNoOfIndices();

	const voxel::VertexArray &vertices = mesh->getVertexVector();
	const voxel::NormalArray &normals = mesh->getNormalVector();
	const voxel::IndexArray &indices = mesh->getIndexVector();

	for (int i = 0; i < ni; i++) {
		if (vertices[indices[i]].colorIndex != idx) {
			continue;
		}
		if (bounds.maxIndex < indices[i]) {
			bounds.maxIndex = indices[i];
		}
		if (indices[i] < bounds.minIndex) {
			bounds.minIndex = indices[i];
		}
		os.writeUInt32(indices[i]);
		++bounds.ni;
	}
	static_assert(sizeof(voxel::IndexType) == 4, "if not 4 bytes - we might need padding here");
	const uint32_t indexOffset = (uint32_t)os.size();

	for (int i = 0; i < nv; i++) {
		glm::vec3 pos = vertices[i].position;
		if (applyTransform) {
			pos += pivotOffset;
		}

		for (int coordIndex = 0; coordIndex < glm::vec3::length(); coordIndex++) {
			os.writeFloat(pos[coordIndex]);
			if (bounds.maxVertex[coordIndex] < pos[coordIndex]) {
				bounds.maxVertex[coordIndex] = pos[coordIndex];
			}
			if (bounds.minVertex[coordIndex] > pos[coordIndex]) {
				bounds.minVertex[coordIndex] = pos[coordIndex];
			}
		}
		++bounds.nv;

		if (exportNormals) {
			for (int coordIndex = 0; coordIndex < glm::vec3::length(); coordIndex++) {
				os.writeFloat(normals[i][coordIndex]);
			}
		}

		if (withTexCoords) {
			const glm::vec2 &uv = paletteUV(vertices[i].colorIndex);
			os.writeFloat(uv.x);
			os.writeFloat(uv.y);
		} else if (withColor) {
			const core::RGBA paletteColor = palette.color(vertices[i].colorIndex);
			if (colorAsFloat) {
				const glm::vec4 &color = core::Color::fromRGBA(paletteColor);
				for (int colorIdx = 0; colorIdx < glm::vec4::length(); colorIdx++) {
					os.writeFloat(color[colorIdx]);
				}
			} else {
				os.writeUInt8(paletteColor.r);
				os.writeUInt8(paletteColor.g);
				os.writeUInt8(paletteColor.b);
				os.writeUInt8(paletteColor.a);
			}
		}
	}
	return indexOffset;
}

bool GLTFFormat::savePrimitivesPerMaterial(uint8_t idx, const glm::vec3 &pivotOffset, tinygltf::Model &gltfModel,
										   tinygltf::Mesh &gltfMesh, const voxel::Mesh *mesh,
										   const palette::Palette &palette, bool withColor, bool withTexCoords,
										   bool colorAsFloat, bool exportNormals, bool applyTransform,
										   int texcoordIndex, const MaterialMap &paletteMaterialIndices) {

	const size_t expectedSize =
		mesh->getNoOfIndices() * sizeof(voxel::IndexType) + mesh->getNoOfVertices() * 10 * sizeof(float);
	io::BufferedReadWriteStream os((int64_t)expectedSize);

	Bounds bounds;
	bounds.minIndex = UINT_MAX;
	bounds.maxVertex = glm::vec3{-FLT_MAX};
	bounds.minVertex = glm::vec3{FLT_MAX};

	const uint32_t indicesBufferByteLen = writeBuffer(mesh, idx, os, withColor, withTexCoords, colorAsFloat,
													  exportNormals, applyTransform, pivotOffset, palette, bounds);
	if (indicesBufferByteLen == 0u) {
		return false;
	}
	tinygltf::BufferView gltfIndicesBufferView;
	gltfIndicesBufferView.buffer = (int)gltfModel.buffers.size();
	gltfIndicesBufferView.byteOffset = 0;
	gltfIndicesBufferView.byteLength = indicesBufferByteLen;
	gltfIndicesBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

	tinygltf::BufferView gltfVerticesBufferView;
	gltfVerticesBufferView.buffer = (int)gltfModel.buffers.size();
	gltfVerticesBufferView.byteOffset = indicesBufferByteLen;
	gltfVerticesBufferView.byteLength = os.size() - indicesBufferByteLen;
	gltfVerticesBufferView.byteStride = 3 * sizeof(float);
	if (exportNormals) {
		gltfVerticesBufferView.byteStride += 3 * sizeof(float);
	}
	if (withTexCoords) {
		gltfVerticesBufferView.byteStride += 2 * sizeof(float);
	} else if (withColor) {
		if (colorAsFloat) {
			gltfVerticesBufferView.byteStride += 4 * sizeof(float);
		} else {
			gltfVerticesBufferView.byteStride += 4 * sizeof(uint8_t);
		}
	}
	gltfVerticesBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

	// Describe the layout of indicesBufferView, the indices of the vertices
	tinygltf::Accessor gltfIndicesAccessor;
	gltfIndicesAccessor.bufferView = (int)gltfModel.bufferViews.size();
	gltfIndicesAccessor.byteOffset = 0;
	gltfIndicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
	gltfIndicesAccessor.count = bounds.ni;
	gltfIndicesAccessor.type = TINYGLTF_TYPE_SCALAR;
	gltfIndicesAccessor.maxValues.push_back(bounds.maxIndex);
	gltfIndicesAccessor.minValues.push_back(bounds.minIndex);

	// Describe the layout of verticesUvBufferView, the vertices themself
	tinygltf::Accessor gltfVerticesAccessor;
	gltfVerticesAccessor.bufferView = (int)gltfModel.bufferViews.size() + 1;
	gltfVerticesAccessor.byteOffset = 0;
	gltfVerticesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	gltfVerticesAccessor.count = bounds.nv;
	gltfVerticesAccessor.type = TINYGLTF_TYPE_VEC3;
	gltfVerticesAccessor.maxValues = {bounds.maxVertex[0], bounds.maxVertex[1], bounds.maxVertex[2]};
	gltfVerticesAccessor.minValues = {bounds.minVertex[0], bounds.minVertex[1], bounds.minVertex[2]};

	// Describe the layout of normals - they are followed
	tinygltf::Accessor gltfNormalAccessor;
	gltfNormalAccessor.bufferView = (int)gltfModel.bufferViews.size() + 1;
	gltfNormalAccessor.byteOffset = 3 * sizeof(float);
	gltfNormalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	gltfNormalAccessor.count = bounds.nv;
	gltfNormalAccessor.type = TINYGLTF_TYPE_VEC3;

	tinygltf::Accessor gltfColorAccessor;
	if (withTexCoords) {
		gltfColorAccessor.bufferView = (int)gltfModel.bufferViews.size() + 1;
		gltfColorAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		gltfColorAccessor.count = bounds.nv;
		gltfColorAccessor.byteOffset = (exportNormals ? 2 : 1) * 3 * sizeof(float);
		gltfColorAccessor.type = TINYGLTF_TYPE_VEC2;
	} else if (withColor) {
		gltfColorAccessor.bufferView = (int)gltfModel.bufferViews.size() + 1;
		gltfColorAccessor.count = bounds.nv;
		gltfColorAccessor.type = TINYGLTF_TYPE_VEC4;
		gltfColorAccessor.byteOffset = (exportNormals ? 2 : 1) * 3 * sizeof(float);
		if (colorAsFloat) {
			gltfColorAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		} else {
			gltfColorAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		}
	}

	{
		// Build the mesh meshPrimitive and add it to the mesh
		tinygltf::Primitive gltfMeshPrimitive;
		// The index of the accessor for the vertex indices
		gltfMeshPrimitive.indices = (int)gltfModel.accessors.size();
		// The index of the accessor for positions
		gltfMeshPrimitive.attributes["POSITION"] = (int)gltfModel.accessors.size() + 1;
		if (exportNormals) {
			gltfMeshPrimitive.attributes["NORMAL"] = (int)gltfModel.accessors.size() + 2;
		}
		if (withTexCoords) {
			const core::String &texcoordsKey = core::String::format("TEXCOORD_%i", texcoordIndex);
			gltfMeshPrimitive.attributes[texcoordsKey.c_str()] =
				(int)gltfModel.accessors.size() + (exportNormals ? 3 : 2);
		} else if (withColor) {
			gltfMeshPrimitive.attributes["COLOR_0"] = (int)gltfModel.accessors.size() + (exportNormals ? 3 : 2);
		}
		auto paletteMaterialIter = paletteMaterialIndices.find(palette.hash());
		core_assert(paletteMaterialIter != paletteMaterialIndices.end());
		const int material = paletteMaterialIter->value[idx];
		core_assert(material >= 0);
		gltfMeshPrimitive.material = material;
		gltfMeshPrimitive.mode = TINYGLTF_MODE_TRIANGLES;
		gltfMesh.primitives.emplace_back(core::move(gltfMeshPrimitive));
	}

	{
		// indices and vertices
		tinygltf::Buffer gltfBuffer;
		gltfBuffer.data.insert(gltfBuffer.data.end(), os.getBuffer(), os.getBuffer() + os.size());
		gltfModel.buffers.emplace_back(core::move(gltfBuffer));
	}

	Log::debug("Index buffer view at %i", (int)gltfModel.bufferViews.size());
	gltfModel.bufferViews.emplace_back(core::move(gltfIndicesBufferView));
	Log::debug("vertex buffer view at %i", (int)gltfModel.bufferViews.size());
	gltfModel.bufferViews.emplace_back(core::move(gltfVerticesBufferView));
	gltfModel.accessors.emplace_back(core::move(gltfIndicesAccessor));
	gltfModel.accessors.emplace_back(core::move(gltfVerticesAccessor));
	if (exportNormals) {
		gltfModel.accessors.emplace_back(core::move(gltfNormalAccessor));
	}
	if (withTexCoords || withColor) {
		gltfModel.accessors.emplace_back(core::move(gltfColorAccessor));
	}

	return true;
}

static void addExtension(tinygltf::Model &gltfModel, const core::String &extension) {
	std::string ext = extension.c_str();
	if (core::find(gltfModel.extensionsUsed.begin(), gltfModel.extensionsUsed.end(), ext) ==
		gltfModel.extensionsUsed.end()) {
		gltfModel.extensionsUsed.push_back(ext);
	}
}

void GLTFFormat::save_KHR_materials_emissive_strength(const palette::Material &material,
													  tinygltf::Material &gltfMaterial,
													  tinygltf::Model &gltfModel) const {
	if (!material.has(palette::MaterialProperty::MaterialEmit)) {
		return;
	}
	// TODO: VOXELFORMAT: needed?
#if 0
	const float emissiveStrength = material.value(palette::MaterialProperty::MaterialEmit);
	tinygltf::Value::Object sg;
	sg["emissiveStrength"] = tinygltf::Value(emissiveStrength);
	gltfMaterial.extensions["KHR_materials_emissive_strength"] = tinygltf::Value(sg);
	addExtension(gltfModel, "KHR_materials_emissive_strength");
#endif
}

void GLTFFormat::save_KHR_materials_volume(const palette::Material &material, const core::RGBA &color,
										   tinygltf::Material &gltfMaterial, tinygltf::Model &gltfModel) const {
	if (!material.has(palette::MaterialProperty::MaterialAttenuation)) {
		return;
	}
	const float attenuation = material.value(palette::MaterialProperty::MaterialAttenuation);
	tinygltf::Value::Object sg;
	const glm::vec4 &fcolor = core::Color::fromRGBA(color);
	std::vector<tinygltf::Value> attenuationColor(3);
	attenuationColor[0] = tinygltf::Value(fcolor[0] * attenuation);
	attenuationColor[1] = tinygltf::Value(fcolor[1] * attenuation);
	attenuationColor[2] = tinygltf::Value(fcolor[2] * attenuation);
	sg["attenuationColor"] = tinygltf::Value(attenuationColor);

	gltfMaterial.extensions["KHR_materials_volume"] = tinygltf::Value(sg);
	addExtension(gltfModel, "KHR_materials_volume");
}

void GLTFFormat::save_KHR_materials_ior(const palette::Material &material, tinygltf::Material &gltfMaterial,
										tinygltf::Model &gltfModel) const {
	if (!material.has(palette::MaterialProperty::MaterialIndexOfRefraction)) {
		return;
	}
	const float v = material.value(palette::MaterialProperty::MaterialIndexOfRefraction);
	tinygltf::Value::Object sg;
	sg["ior"] = tinygltf::Value(v);
	gltfMaterial.extensions["KHR_materials_ior"] = tinygltf::Value(sg);
	addExtension(gltfModel, "KHR_materials_ior");
}

void GLTFFormat::save_KHR_materials_specular(const palette::Material &material, const core::RGBA &color,
											 tinygltf::Material &gltfMaterial, tinygltf::Model &gltfModel) const {
	if (!material.has(palette::MaterialProperty::MaterialSpecular)) {
		return;
	}
	const float specular = material.value(palette::MaterialProperty::MaterialSpecular);
	tinygltf::Value::Object sg;
	const glm::vec4 &fcolor = core::Color::fromRGBA(color);
	std::vector<tinygltf::Value> specularFactor(3);
	specularFactor[0] = tinygltf::Value(fcolor[0] * specular);
	specularFactor[1] = tinygltf::Value(fcolor[1] * specular);
	specularFactor[2] = tinygltf::Value(fcolor[2] * specular);
	sg["specularFactor"] = tinygltf::Value(specularFactor);
	gltfMaterial.extensions["KHR_materials_specular"] = tinygltf::Value(sg);
	addExtension(gltfModel, "KHR_materials_specular");
}

bool GLTFFormat::save_KHR_materials_pbrSpecularGlossiness(const palette::Material &material, const core::RGBA &color,
														  tinygltf::Material &gltfMaterial,
														  tinygltf::Model &gltfModel) const {
	if (!material.has(palette::MaterialProperty::MaterialDensity) &&
		!material.has(palette::MaterialProperty::MaterialSpecular)) {
		return false;
	}
	tinygltf::Value::Object sg;
	const glm::vec4 &fcolor = core::Color::fromRGBA(color);

	// The reflected diffuse factor of the material
	if (material.has(palette::MaterialProperty::MaterialDensity)) {
		std::vector<tinygltf::Value> diffuseFactor(4);
		const float diffusion = material.value(palette::MaterialProperty::MaterialDensity);
		diffuseFactor[0] = tinygltf::Value(fcolor[0] * diffusion);
		diffuseFactor[1] = tinygltf::Value(fcolor[1] * diffusion);
		diffuseFactor[2] = tinygltf::Value(fcolor[2] * diffusion);
		// TODO: MATERIAL: maybe the transparent factor would fit here?
		diffuseFactor[3] = tinygltf::Value(fcolor[3]);
		sg["diffuseFactor"] = tinygltf::Value(diffuseFactor);
	}
	// The specular RGB color of the material.
	if (material.has(palette::MaterialProperty::MaterialSpecular)) {
		std::vector<tinygltf::Value> specularFactor(3);
		const float specular = material.value(palette::MaterialProperty::MaterialSpecular);
		specularFactor[0] = tinygltf::Value(fcolor[0] * specular);
		specularFactor[1] = tinygltf::Value(fcolor[1] * specular);
		specularFactor[2] = tinygltf::Value(fcolor[2] * specular);
		sg["specularFactor"] = tinygltf::Value(specularFactor);
	}
	// The glossiness or smoothness of the material. A value of 1.0 means the material has full glossiness or is
	// perfectly smooth. A value of 0.0 means the material has no glossiness or is perfectly rough. This value is
	// linear.
	if (material.has(palette::MaterialProperty::MaterialPhase)) {
		const float glossiness = material.value(palette::MaterialProperty::MaterialPhase);
		sg["glossinessFactor"] = tinygltf::Value(glossiness);
	} else if (material.has(palette::MaterialProperty::MaterialRoughness)) {
		sg["glossinessFactor"] =
			tinygltf::Value(1.0 - material.value(palette::MaterialProperty::MaterialRoughness));
	}
	gltfMaterial.extensions["KHR_materials_pbrSpecularGlossiness"] = tinygltf::Value(sg);
	addExtension(gltfModel, "KHR_materials_pbrSpecularGlossiness");
	return true;
}

int GLTFFormat::saveEmissiveTexture(tinygltf::Model &gltfModel, const palette::Palette &palette) const {
	bool hasEmit = false;
	core::RGBA colors[palette::PaletteMaxColors];
	for (int i = 0; i < palette::PaletteMaxColors; i++) {
		if (palette.hasEmit(i)) {
			hasEmit = true;
		}
		colors[i] = palette.emitColor(i);
	}
	if (hasEmit) {
		int emissiveTextureIndex = (int)gltfModel.textures.size();
		const int emissiveImageIndex = (int)gltfModel.images.size();

		tinygltf::Image gltfEmitImage;
		image::Image image("pal");
		image.loadRGBA((const unsigned char *)colors, palette::PaletteMaxColors, 1);
		const core::String &pal64 = image.pngBase64();
		gltfEmitImage.uri = "data:image/png;base64,";
		gltfEmitImage.width = palette::PaletteMaxColors;
		gltfEmitImage.height = 1;
		gltfEmitImage.component = 4;
		gltfEmitImage.bits = 32;
		gltfEmitImage.uri += pal64.c_str();
		gltfModel.images.emplace_back(core::move(gltfEmitImage));

		tinygltf::Texture gltfEmitTexture;
		gltfEmitTexture.name = palette.name().c_str();
		gltfEmitTexture.source = emissiveImageIndex;
		gltfModel.textures.emplace_back(core::move(gltfEmitTexture));
		return emissiveTextureIndex;
	}
	return -1;
}

int GLTFFormat::saveTexture(tinygltf::Model &gltfModel, const palette::Palette &palette) const {
	const int textureIndex = (int)gltfModel.textures.size();
	const int imageIndex = (int)gltfModel.images.size();

	tinygltf::Image gltfPaletteImage;
	image::Image image("pal");
	core::RGBA colors[palette::PaletteMaxColors];
	for (int i = 0; i < palette::PaletteMaxColors; i++) {
		colors[i] = palette.color(i);
	}
	image.loadRGBA((const unsigned char *)colors, palette::PaletteMaxColors, 1);
	const core::String &pal64 = image.pngBase64();
	gltfPaletteImage.uri = "data:image/png;base64,";
	gltfPaletteImage.width = palette::PaletteMaxColors;
	gltfPaletteImage.height = 1;
	gltfPaletteImage.component = 4;
	gltfPaletteImage.bits = 32;
	gltfPaletteImage.uri += pal64.c_str();
	gltfModel.images.emplace_back(core::move(gltfPaletteImage));

	tinygltf::Texture gltfPaletteTexture;
	gltfPaletteTexture.name = palette.name().c_str();
	gltfPaletteTexture.source = imageIndex;
	gltfModel.textures.emplace_back(core::move(gltfPaletteTexture));
	return textureIndex;
}

void GLTFFormat::generateMaterials(bool withTexCoords, tinygltf::Model &gltfModel, MaterialMap &paletteMaterialIndices,
								   const scenegraph::SceneGraphNode &node, const palette::Palette &palette,
								   int &texcoordIndex) const {
	const auto paletteMaterialIter = paletteMaterialIndices.find(palette.hash());
	if (paletteMaterialIter == paletteMaterialIndices.end()) {
		const core::String hashId = core::String::format("%" PRIu64, palette.hash());

		const int textureIndex = withTexCoords ? saveTexture(gltfModel, palette) : -1;
		const int emissiveTextureIndex = withTexCoords ? saveEmissiveTexture(gltfModel, palette) : -1;
		const bool KHR_materials_pbrSpecularGlossiness =
			core::Var::getSafe(cfg::VoxformatGLTF_KHR_materials_pbrSpecularGlossiness)->boolVal();
		const bool withMaterials = core::Var::getSafe(cfg::VoxformatWithMaterials)->boolVal();

		core::Array<int, palette::PaletteMaxColors> materialIds;
		materialIds.fill(-1);
		for (int i = 0; i < palette.colorCount(); i++) {
			if (palette.color(i).a == 0) {
				continue;
			}
			const palette::Material &material = palette.material(i);
			const core::RGBA color = palette.color(i);
			tinygltf::Material gltfMaterial;
			if (withTexCoords) {
				gltfMaterial.pbrMetallicRoughness.baseColorTexture.index = textureIndex;
				gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord = texcoordIndex;
				if (emissiveTextureIndex != -1) {
					gltfMaterial.emissiveTexture.index = emissiveTextureIndex;
					gltfMaterial.emissiveTexture.texCoord = texcoordIndex;
				}
			}

			gltfMaterial.name = hashId.c_str();
			gltfMaterial.alphaMode = color.a < 255 ? "BLEND" : "OPAQUE";
			gltfMaterial.doubleSided = false;

			if (withMaterials) {
				if (material.has(palette::MaterialProperty::MaterialEmit)) {
					gltfMaterial.emissiveFactor[0] = material.value(palette::MaterialProperty::MaterialEmit);
					gltfMaterial.emissiveFactor[1] = gltfMaterial.emissiveFactor[2] = gltfMaterial.emissiveFactor[0];
				}
				if (material.has(palette::MaterialProperty::MaterialRoughness)) {
					gltfMaterial.pbrMetallicRoughness.roughnessFactor =
						material.value(palette::MaterialProperty::MaterialRoughness);
				}
				if (material.has(palette::MaterialProperty::MaterialMetal)) {
					gltfMaterial.pbrMetallicRoughness.metallicFactor =
						material.value(palette::MaterialProperty::MaterialMetal);
				}

				bool pbrSpecularGlossiness = false;
				if (KHR_materials_pbrSpecularGlossiness) {
					pbrSpecularGlossiness = save_KHR_materials_pbrSpecularGlossiness(material, color, gltfMaterial, gltfModel);
				}
				if (!pbrSpecularGlossiness) {
					if (core::Var::getSafe(cfg::VoxformatGLTF_KHR_materials_specular)->boolVal()) {
						save_KHR_materials_specular(material, color, gltfMaterial, gltfModel);
					}
					save_KHR_materials_ior(material, gltfMaterial, gltfModel);
					save_KHR_materials_volume(material, color, gltfMaterial, gltfModel);
				}
				save_KHR_materials_emissive_strength(material, gltfMaterial, gltfModel);
			}
			int materialId = (int)gltfModel.materials.size();
			gltfModel.materials.emplace_back(core::move(gltfMaterial));
			materialIds[i] = materialId;
		}
		paletteMaterialIndices.put(palette.hash(), materialIds);
		Log::debug("New material ids for hash %" PRIu64, palette.hash());
	}
}

bool GLTFFormat::saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
							const Meshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
							const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const core::String &ext = core::string::extractExtension(filename);
	const bool writeBinary = ext == "glb";

	tinygltf::TinyGLTF gltf;
	tinygltf::Model gltfModel;
	tinygltf::Scene gltfScene;

	const bool colorAsFloat = core::Var::get(cfg::VoxformatColorAsFloat)->boolVal();
	if (colorAsFloat) {
		Log::debug("Export colors as float");
	} else {
		Log::debug("Export colors as byte");
	}

	const size_t modelNodes = meshes.size();
	const core::String &appname = app::App::getInstance()->fullAppname();
	const core::String &generator = core::String::format("%s " PROJECT_VERSION, appname.c_str());
	// Define the asset. The version is required
	gltfModel.asset.generator = generator.c_str();
	gltfModel.asset.version = "2.0";
	gltfModel.asset.copyright = sceneGraph.root().property("Copyright").c_str();
	gltfModel.accessors.reserve(modelNodes * 4 + sceneGraph.animations().size() * 4);

	Stack stack;
	stack.emplace_back(0, -1);

	const bool exportAnimations = sceneGraph.hasAnimations();

	MaterialMap paletteMaterialIndices((int)sceneGraph.size());
	core::Map<int, int> nodeMapping((int)sceneGraph.nodeSize());
	while (!stack.empty()) {
		const int nodeId = stack.back().first;
		const scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
		palette::Palette palette = node.palette();

		if (meshIdxNodeMap.find(nodeId) == meshIdxNodeMap.end()) {
			saveGltfNode(nodeMapping, gltfModel, gltfScene, node, stack, sceneGraph, scale, false);
			continue;
		}

		int meshExtIdx = 0;
		core_assert_always(meshIdxNodeMap.get(nodeId, meshExtIdx));
		const MeshExt &meshExt = meshes[meshExtIdx];

		int texcoordIndex = 0;
		if (node.isAnyModelNode()) {
			for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
				const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
				if (mesh->isEmpty()) {
					continue;
				}
				generateMaterials(withTexCoords, gltfModel, paletteMaterialIndices, node, palette, texcoordIndex);
			}
		}

		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
			if (mesh->isEmpty()) {
				continue;
			}

			Log::debug("Exporting model %s", meshExt.name.c_str());

			const int ni = (int)mesh->getNoOfIndices();
			if (ni % 3 != 0) {
				Log::error("Unexpected indices amount");
				return false;
			}

			const voxel::NormalArray &normals = mesh->getNormalVector();
			const char *objectName = meshExt.name.c_str();
			const bool exportNormals = !normals.empty();
			if (exportNormals) {
				Log::debug("Export normals for mesh %i", i);
			}

			if (objectName[0] == '\0') {
				objectName = "Noname";
			}
			const glm::vec3 &offset = mesh->getOffset();
			const glm::vec3 pivotOffset = offset - meshExt.pivot * meshExt.size;

			tinygltf::Mesh gltfMesh;
			gltfMesh.name = objectName;
			for (int j = 0; j < palette.colorCount(); ++j) {
				if (palette.color(j).a == 0) {
					continue;
				}
				savePrimitivesPerMaterial(j, pivotOffset, gltfModel, gltfMesh, mesh, palette, withColor, withTexCoords,
										  colorAsFloat, exportNormals, meshExt.applyTransform, texcoordIndex,
										  paletteMaterialIndices);
			}
			saveGltfNode(nodeMapping, gltfModel, gltfScene, node, stack, sceneGraph, scale, exportAnimations);
			gltfModel.meshes.emplace_back(core::move(gltfMesh));
		}
	}

	if (exportAnimations) {
		Log::debug("Export %i animations for %i nodes", (int)sceneGraph.animations().size(), (int)nodeMapping.size());
		gltfModel.animations.reserve(sceneGraph.animations().size());
		for (const core::String &animationId : sceneGraph.animations()) {
			tinygltf::Animation gltfAnimation;
			gltfAnimation.name = animationId.c_str();
			Log::debug("save animation: %s", animationId.c_str());
			for (const auto &e : nodeMapping) {
				const scenegraph::SceneGraphNode &node = sceneGraph.node(e->key);
				saveAnimation(e->value, gltfModel, node, gltfAnimation);
			}
			gltfModel.animations.emplace_back(gltfAnimation);
		}
	} else {
		Log::debug("No animations found");
	}

	gltfModel.scenes.emplace_back(core::move(gltfScene));
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Camera); iter != sceneGraph.end(); ++iter) {
		tinygltf::Camera gltfCamera = _priv::processCamera(scenegraph::toCameraNode(*iter));
		if (!_priv::validateCamera(gltfCamera)) {
			continue;
		}
		gltfModel.cameras.push_back(gltfCamera);
	}

	io::StdOStreamBuf buf(*stream);
	std::ostream gltfStream(&buf);
	if (!gltf.WriteGltfSceneToStream(&gltfModel, gltfStream, false, writeBinary)) {
		Log::error("Could not save to file");
		return false;
	}

	return true;
}

void GLTFFormat::saveAnimation(int targetNode, tinygltf::Model &gltfModel, const scenegraph::SceneGraphNode &node,
							   tinygltf::Animation &gltfAnimation) {
	const core::String animationId = gltfAnimation.name.c_str();
	const scenegraph::SceneGraphKeyFrames &keyFrames = node.keyFrames(animationId);
	const int maxFrames = (int)keyFrames.size();
	Log::debug("Save animation %s for node %s with %i frames", animationId.c_str(), node.name().c_str(), maxFrames);
	io::BufferedReadWriteStream osTime((int64_t)(maxFrames * sizeof(float)));
	io::BufferedReadWriteStream osTranslation((int64_t)((size_t)maxFrames * 3 * sizeof(float)));
	io::BufferedReadWriteStream osRotation((int64_t)((size_t)maxFrames * 4 * sizeof(float)));
	io::BufferedReadWriteStream osScale((int64_t)((size_t)maxFrames * 3 * sizeof(float)));

	for (const scenegraph::SceneGraphKeyFrame &keyFrame : keyFrames) {
		osTime.writeFloat((float)keyFrame.frameIdx / _priv::FPS);

		const scenegraph::SceneGraphTransform &transform = keyFrame.transform();
		const glm::vec3 &translation = transform.localTranslation();
		osTranslation.writeFloat(translation.x);
		osTranslation.writeFloat(translation.y);
		osTranslation.writeFloat(translation.z);

		const glm::quat &rotation = transform.localOrientation();
		osRotation.writeFloat(rotation.x);
		osRotation.writeFloat(rotation.y);
		osRotation.writeFloat(rotation.z);
		osRotation.writeFloat(rotation.w);

		const glm::vec3 &scale = transform.localScale();
		osScale.writeFloat(scale.x);
		osScale.writeFloat(scale.y);
		osScale.writeFloat(scale.z);
	}

	const int bufferTimeId = _priv::addBuffer(gltfModel, osTime, "time");
	const int bufferTranslationId = _priv::addBuffer(gltfModel, osTranslation, "translation");
	const int bufferRotationId = _priv::addBuffer(gltfModel, osRotation, "rotation");
	const int bufferScaleId = _priv::addBuffer(gltfModel, osScale, "scale");

	const int timeAccessorIdx = (int)gltfModel.accessors.size();
	{
		tinygltf::Accessor gltfAccessor;
		gltfAccessor.type = TINYGLTF_TYPE_SCALAR;
		gltfAccessor.bufferView = (int)gltfModel.bufferViews.size();
		gltfAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		gltfAccessor.count = maxFrames;
		gltfAccessor.minValues.push_back(0.0);
		gltfAccessor.maxValues.push_back((double)(maxFrames - 1) / _priv::FPS);
		gltfModel.accessors.emplace_back(gltfAccessor);

		tinygltf::BufferView gltfBufferView;
		gltfBufferView.buffer = bufferTimeId;
		gltfBufferView.byteLength = osTime.size();
		Log::debug("animation %s time buffer view at %i", animationId.c_str(), (int)gltfModel.bufferViews.size());
		gltfModel.bufferViews.emplace_back(gltfBufferView);
	}

	const int translationAccessorIndex = (int)gltfModel.accessors.size();
	{
		tinygltf::Accessor gltfAccessor;
		gltfAccessor.type = TINYGLTF_TYPE_VEC3;
		gltfAccessor.bufferView = (int)gltfModel.bufferViews.size();
		gltfAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		gltfAccessor.count = maxFrames;
		gltfModel.accessors.emplace_back(gltfAccessor);

		tinygltf::BufferView gltfBufferView;
		gltfBufferView.buffer = bufferTranslationId;
		gltfBufferView.byteLength = osTranslation.size();
		Log::debug("animation %s time buffer view at %i", animationId.c_str(), (int)gltfModel.bufferViews.size());
		gltfModel.bufferViews.emplace_back(gltfBufferView);
	}
	const int rotationAccessorIndex = (int)gltfModel.accessors.size();
	{
		tinygltf::Accessor gltfAccessor;
		gltfAccessor.type = TINYGLTF_TYPE_VEC4;
		gltfAccessor.bufferView = (int)gltfModel.bufferViews.size();
		gltfAccessor.byteOffset = 0;
		gltfAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		gltfAccessor.count = maxFrames;
		gltfModel.accessors.emplace_back(gltfAccessor);

		tinygltf::BufferView gltfBufferView;
		gltfBufferView.buffer = bufferRotationId;
		gltfBufferView.byteLength = osRotation.size();
		Log::debug("anim rotation buffer: %i", gltfAccessor.bufferView);
		gltfModel.bufferViews.emplace_back(gltfBufferView);
	}
	const int scaleAccessorIndex = (int)gltfModel.accessors.size();
	{
		tinygltf::Accessor gltfAccessor;
		gltfAccessor.type = TINYGLTF_TYPE_VEC3;
		gltfAccessor.bufferView = (int)gltfModel.bufferViews.size();
		gltfAccessor.byteOffset = 0;
		gltfAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		gltfAccessor.count = maxFrames;
		gltfModel.accessors.emplace_back(gltfAccessor);

		tinygltf::BufferView gltfBufferView;
		gltfBufferView.buffer = bufferScaleId;
		gltfBufferView.byteLength = osScale.size();
		Log::debug("anim scale buffer: %i", gltfAccessor.bufferView);
		gltfModel.bufferViews.emplace_back(gltfBufferView);
	}

	{
		tinygltf::AnimationSampler gltfAnimSampler;
		gltfAnimSampler.input = timeAccessorIdx;
		gltfAnimSampler.output = translationAccessorIndex;
		gltfAnimSampler.interpolation = "LINEAR";
		gltfAnimation.samplers.emplace_back(gltfAnimSampler);

		tinygltf::AnimationChannel gltfAnimChannel;
		gltfAnimChannel.sampler = (int)gltfAnimation.samplers.size() - 1;
		gltfAnimChannel.target_node = targetNode;
		gltfAnimChannel.target_path = "translation";
		gltfAnimation.channels.emplace_back(gltfAnimChannel);
	}

	{
		tinygltf::AnimationSampler gltfAnimSampler;
		gltfAnimSampler.input = timeAccessorIdx;
		gltfAnimSampler.output = rotationAccessorIndex;
		gltfAnimSampler.interpolation = "LINEAR";
		gltfAnimation.samplers.emplace_back(gltfAnimSampler);

		tinygltf::AnimationChannel gltfAnimChannel;
		gltfAnimChannel.sampler = (int)gltfAnimation.samplers.size() - 1;
		gltfAnimChannel.target_node = targetNode;
		gltfAnimChannel.target_path = "rotation";
		gltfAnimation.channels.emplace_back(gltfAnimChannel);
	}

	{
		tinygltf::AnimationSampler gltfAnimSampler;
		gltfAnimSampler.input = timeAccessorIdx;
		gltfAnimSampler.output = scaleAccessorIndex;
		gltfAnimSampler.interpolation = "LINEAR";
		gltfAnimation.samplers.emplace_back(gltfAnimSampler);

		tinygltf::AnimationChannel gltfAnimChannel;
		gltfAnimChannel.sampler = (int)gltfAnimation.samplers.size() - 1;
		gltfAnimChannel.target_node = targetNode;
		gltfAnimChannel.target_path = "scale";
		gltfAnimation.channels.emplace_back(gltfAnimChannel);
	}
}

size_t GLTFFormat::accessorSize(const tinygltf::Accessor &gltfAccessor) const {
	return (size_t)tinygltf::GetComponentSizeInBytes(gltfAccessor.componentType) *
		   tinygltf::GetNumComponentsInType(gltfAccessor.type);
}

const tinygltf::Accessor *GLTFFormat::getAccessor(const tinygltf::Model &gltfModel, int id) const {
	if ((size_t)id >= gltfModel.accessors.size()) {
		Log::debug("Invalid accessor id: %i", id);
		return nullptr;
	}

	const tinygltf::Accessor &gltfAccessor = gltfModel.accessors[id];
	if (gltfAccessor.sparse.isSparse) {
		Log::debug("Sparse accessor");
		return nullptr;
	}
	if (gltfAccessor.bufferView < 0 || gltfAccessor.bufferView >= (int)gltfModel.bufferViews.size()) {
		Log::debug("Invalid bufferview id: %i (%i vs max %i)", id, gltfAccessor.bufferView,
				   (int)gltfModel.bufferViews.size());
		return nullptr;
	}

	const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[gltfAccessor.bufferView];
	if (gltfBufferView.buffer < 0 || gltfBufferView.buffer >= (int)gltfModel.buffers.size()) {
		return nullptr;
	}

	const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];
	const size_t viewSize = gltfBufferView.byteOffset + gltfBufferView.byteLength;
	if (gltfBuffer.data.size() < viewSize) {
		return nullptr;
	}

	return &gltfAccessor;
}

scenegraph::SceneGraphTransform GLTFFormat::loadTransform(const tinygltf::Node &gltfNode) const {
	scenegraph::SceneGraphTransform transform;
	if (gltfNode.matrix.size() == 16) {
		transform.setLocalMatrix(glm::mat4(
			(float)gltfNode.matrix[0], (float)gltfNode.matrix[1], (float)gltfNode.matrix[2], (float)gltfNode.matrix[3],
			(float)gltfNode.matrix[4], (float)gltfNode.matrix[5], (float)gltfNode.matrix[6], (float)gltfNode.matrix[7],
			(float)gltfNode.matrix[8], (float)gltfNode.matrix[9], (float)gltfNode.matrix[10],
			(float)gltfNode.matrix[11], (float)gltfNode.matrix[12], (float)gltfNode.matrix[13],
			(float)gltfNode.matrix[14], (float)gltfNode.matrix[15]));
	} else {
		if (gltfNode.scale.size() == 3) {
			transform.setLocalScale(glm::vec3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]));
		}
		if (gltfNode.rotation.size() == 4) {
			const glm::quat quat((float)gltfNode.rotation[3], (float)gltfNode.rotation[0], (float)gltfNode.rotation[1],
								 (float)gltfNode.rotation[2]);
			transform.setLocalOrientation(quat);
		}
		if (gltfNode.translation.size() == 3) {
			transform.setLocalTranslation(glm::vec3((float)gltfNode.translation[0], (float)gltfNode.translation[1],
													(float)gltfNode.translation[2]));
		}
	}
	return transform;
}

#define wrap(x) \
	if ((x) == -1) { \
		Log::error("Failed to read from index buffer"); \
		return false; \
	}

bool GLTFFormat::loadIndices(const tinygltf::Model &gltfModel, const tinygltf::Primitive &gltfPrimitive,
							 core::Buffer<uint32_t> &indices, size_t indicesOffset) const {
	const tinygltf::Accessor *accessor = getAccessor(gltfModel, gltfPrimitive.indices);
	if (accessor == nullptr) {
		Log::warn("Could not get accessor for indices");
		return false;
	}
	const size_t size = accessorSize(*accessor);
	const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[accessor->bufferView];
	const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];
	const size_t stride = gltfBufferView.byteStride ? gltfBufferView.byteStride : size;
	core_assert(stride > 0);

	const size_t offset = accessor->byteOffset + gltfBufferView.byteOffset;
	const uint8_t *indexBuf = gltfBuffer.data.data() + offset;

	Log::debug("indicesOffset: %i", (int)indicesOffset);

	// Temporary raw indices buffer
	core::Buffer<uint32_t> rawIndices;
	rawIndices.reserve(accessor->count);
	io::MemoryReadStream stream(indexBuf, accessor->count * stride);

	switch (accessor->componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		for (size_t i = 0; i < accessor->count; i++) {
			wrap(stream.seek(i * stride))
			uint8_t idx;
			wrap(stream.readUInt8(idx))
			rawIndices.push_back((uint32_t)idx + indicesOffset);
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		for (size_t i = 0; i < accessor->count; i++) {
			wrap(stream.seek(i * stride))
			int8_t idx;
			wrap(stream.readInt8(idx))
			rawIndices.push_back((uint32_t)idx + indicesOffset);
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		for (size_t i = 0; i < accessor->count; i++) {
			wrap(stream.seek(i * stride))
			uint16_t idx;
			wrap(stream.readUInt16(idx))
			rawIndices.push_back((uint32_t)idx + indicesOffset);
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		for (size_t i = 0; i < accessor->count; i++) {
			wrap(stream.seek(i * stride))
			int16_t idx;
			wrap(stream.readInt16(idx))
			rawIndices.push_back((uint32_t)idx + indicesOffset);
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		for (size_t i = 0; i < accessor->count; i++) {
			wrap(stream.seek(i * stride))
			uint32_t idx;
			wrap(stream.readUInt32(idx))
			rawIndices.push_back(idx + indicesOffset);
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		for (size_t i = 0; i < accessor->count; i++) {
			wrap(stream.seek(i * stride))
			int32_t idx;
			wrap(stream.readInt32(idx))
			rawIndices.push_back((uint32_t)idx + indicesOffset);
		}
		break;
	default:
		Log::error("Unknown component type for indices: %i", accessor->componentType);
		return false;
	}

	// Convert to triangles depending on primitive mode
	switch (gltfPrimitive.mode) {
	case TINYGLTF_MODE_TRIANGLES:
		indices.insert(indices.end(), rawIndices.begin(), rawIndices.end());
		break;
	case TINYGLTF_MODE_TRIANGLE_FAN:
		if (rawIndices.size() < 3) {
			Log::warn("Not enough indices for triangle fan");
			return false;
		}
		for (size_t i = 1; i < rawIndices.size() - 1; ++i) {
			indices.push_back(rawIndices[0]);
			indices.push_back(rawIndices[i]);
			indices.push_back(rawIndices[i + 1]);
		}
		break;
	case TINYGLTF_MODE_TRIANGLE_STRIP:
		if (rawIndices.size() < 3) {
			Log::warn("Not enough indices for triangle strip");
			return false;
		}
		for (size_t i = 0; i < rawIndices.size() - 2; ++i) {
			if (i % 2 == 0) {
				indices.push_back(rawIndices[i]);
				indices.push_back(rawIndices[i + 1]);
				indices.push_back(rawIndices[i + 2]);
			} else {
				indices.push_back(rawIndices[i + 2]);
				indices.push_back(rawIndices[i + 1]);
				indices.push_back(rawIndices[i]);
			}
		}
		break;
	default:
		Log::warn("Unsupported primitive mode: %i", gltfPrimitive.mode);
		return false;
	}

	return true;
}

#undef wrap

void GLTFFormat::loadTexture(const core::String &filename, const io::ArchivePtr &archive,
							 const tinygltf::Model &gltfModel, MeshMaterialPtr &meshMaterial,
							 const tinygltf::TextureInfo &gltfTextureInfo, const int textureIndex) const {
	const tinygltf::Texture &gltfTexture = gltfModel.textures[textureIndex];
	if (gltfTexture.source >= 0 && gltfTexture.source < (int)gltfModel.images.size()) {
		if (gltfTexture.sampler >= 0 && gltfTexture.sampler < (int)gltfModel.samplers.size()) {
			const tinygltf::Sampler &gltfTextureSampler = gltfModel.samplers[gltfTexture.sampler];
			Log::debug("Sampler: '%s', wrapS: %i, wrapT: %i", gltfTextureSampler.name.c_str(), gltfTextureSampler.wrapS,
					   gltfTextureSampler.wrapT);
			meshMaterial->wrapS = _priv::convertTextureWrap(gltfTextureSampler.wrapS);
			meshMaterial->wrapT = _priv::convertTextureWrap(gltfTextureSampler.wrapT);
		}
		const tinygltf::Image &gltfImage = gltfModel.images[gltfTexture.source];
		Log::debug("Image '%s': components: %i, width: %i, height: %i, bits: %i", gltfImage.uri.c_str(),
				   gltfImage.component, gltfImage.width, gltfImage.height, gltfImage.bits);
		if (gltfImage.uri.empty()) {
			if (gltfImage.bufferView >= 0 && gltfImage.bufferView < (int)gltfModel.bufferViews.size()) {
				const tinygltf::BufferView &gltfImgBufferView = gltfModel.bufferViews[gltfImage.bufferView];
				if (gltfImgBufferView.buffer >= 0 && gltfImgBufferView.buffer < (int)gltfModel.buffers.size()) {
					const tinygltf::Buffer &gltfImgBuffer = gltfModel.buffers[gltfImgBufferView.buffer];
					const size_t offset = gltfImgBufferView.byteOffset;
					const uint8_t *buf = gltfImgBuffer.data.data() + offset;
					core::String name = gltfImage.name.c_str();
					if (name.empty()) {
						name = core::String::format("image%i", gltfTexture.source);
					}
					image::ImagePtr tex = image::createEmptyImage(name.c_str());
					io::MemoryReadStream pngStream(buf, (int)gltfImgBufferView.byteLength);
					if (!tex->load(image::ImageType::PNG, pngStream, pngStream.size())) {
						Log::warn("Failed to load embedded image %s", name.c_str());
					} else {
						Log::debug("Loaded embedded image %s", name.c_str());
					}
					meshMaterial->texture = tex;
				} else {
					Log::warn("Invalid buffer index for image: %i", gltfImgBufferView.buffer);
				}
			} else if (!gltfImage.image.empty()) {
				if (gltfImage.component == 4) {
					core::String name = gltfImage.name.c_str();
					if (name.empty()) {
						name = core::String::format("image%i", gltfTexture.source);
					}
					meshMaterial->texture = image::createEmptyImage(name);
					core_assert(gltfImage.image.size() ==
								(size_t)(gltfImage.width * gltfImage.height * gltfImage.component));
					meshMaterial->texture->loadRGBA(gltfImage.image.data(), gltfImage.width, gltfImage.height);
					Log::debug("Use image %s", name.c_str());
					meshMaterial->uvIndex = gltfTextureInfo.texCoord;
				} else {
					Log::warn("Failed to load image with %i components", gltfImage.component);
				}
			} else {
				Log::warn("Invalid buffer view index for image: %i", gltfImage.bufferView);
			}
		} else {
			core::String name = gltfImage.uri.c_str();
			meshMaterial->texture = image::loadImage(name);
			if (!meshMaterial->texture->isLoaded()) {
				name = lookupTexture(filename, name, archive);
				meshMaterial->texture = image::loadImage(name);
				if (meshMaterial->texture->isLoaded()) {
					Log::debug("Use image %s", name.c_str());
					meshMaterial->uvIndex = gltfTextureInfo.texCoord;
				} else {
					Log::warn("Failed to load %s", name.c_str());
				}
			}
		}
	} else {
		Log::debug("Invalid image index given %i", gltfTexture.source);
	}
}

void GLTFFormat::load_KHR_materials_ior(palette::Material &material, const tinygltf::Material &gltfMaterial) const {
	auto extIter = gltfMaterial.extensions.find("KHR_materials_ior");
	if (extIter == gltfMaterial.extensions.end()) {
		return;
	}
	const tinygltf::Value::Object &sg = extIter->second.Get<tinygltf::Value::Object>();
	auto iorIter = sg.find("ior");
	if (iorIter != sg.end()) {
		const float v = (float)iorIter->second.Get<double>();
		material.setValue(palette::MaterialProperty::MaterialIndexOfRefraction, v);
	}
}

void GLTFFormat::load_KHR_materials_specular(palette::Material &material,
											 const tinygltf::Material &gltfMaterial) const {
	auto extIter = gltfMaterial.extensions.find("KHR_materials_specular");
	if (extIter == gltfMaterial.extensions.end()) {
		return;
	}
	const tinygltf::Value::Object &sg = extIter->second.Get<tinygltf::Value::Object>();
	auto specularColorFactorIter = sg.find("specularColorFactor");
	if (specularColorFactorIter != sg.end()) {
		const tinygltf::Value &color = specularColorFactorIter->second;
		const float r = (float)color.Get(0).Get<double>();
		const float g = (float)color.Get(1).Get<double>();
		const float b = (float)color.Get(2).Get<double>();
		// TODO: MATERIAL
		(void)r;
		(void)g;
		(void)b;
	}
	auto specularFactorIter = sg.find("specularFactor");
	if (specularFactorIter != sg.end()) {
		const float v = (float)specularFactorIter->second.Get<double>();
		material.setValue(palette::MaterialProperty::MaterialSpecular, v);
	}
}

void GLTFFormat::load_KHR_materials_pbrSpecularGlossiness(palette::Material &material,
														  const tinygltf::Material &gltfMaterial) const {
	auto extIter = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
	if (extIter == gltfMaterial.extensions.end()) {
		return;
	}
	const tinygltf::Value::Object &sg = extIter->second.Get<tinygltf::Value::Object>();

	auto diffuseFactorIter = sg.find("diffuseFactor");
	if (diffuseFactorIter != sg.end()) {
		const tinygltf::Value &color = diffuseFactorIter->second;
		const float r = (float)color.Get(0).Get<double>();
		const float g = (float)color.Get(1).Get<double>();
		const float b = (float)color.Get(2).Get<double>();
		const float a = (float)color.Get(3).Get<double>();
		// TODO: MATERIAL
		(void)r;
		(void)g;
		(void)b;
		(void)a;
	}

	auto diffuseTextureIter = sg.find("diffuseTexture");
	if (diffuseTextureIter != sg.end()) {
		const tinygltf::Value::Object &tex = diffuseTextureIter->second.Get<tinygltf::Value::Object>();
		const auto iter = tex.find("index");
		if (iter != tex.end()) {
			const int idx = iter->second.Get<int>();
			(void)idx; // TODO: MATERIAL
		}
	}

	auto glossinessFactorIter = sg.find("glossinessFactor");
	if (glossinessFactorIter != sg.end()) {
		const float v = (float)glossinessFactorIter->second.Get<double>();
		material.setValue(palette::MaterialProperty::MaterialPhase, v);
	}

	auto specularFactorIter = sg.find("specularFactor");
	if (specularFactorIter != sg.end()) {
		const tinygltf::Value &color = specularFactorIter->second;
		const float r = (float)color.Get(0).Get<double>();
		const float g = (float)color.Get(1).Get<double>();
		const float b = (float)color.Get(2).Get<double>();
		// TODO: MATERIAL
		material.setValue(palette::MaterialProperty::MaterialSpecular, r * g * b);
	}

	auto specularGlossinessTextureIter = sg.find("specularGlossinessTexture");
	if (specularGlossinessTextureIter != sg.end()) {
		const tinygltf::Value::Object &tex = specularGlossinessTextureIter->second.Get<tinygltf::Value::Object>();
		const auto iter = tex.find("index");
		if (iter != tex.end()) {
			const int idx = iter->second.Get<int>();
			(void)idx; // TODO: MATERIAL
		}
	}
}

void GLTFFormat::load_KHR_materials_emissive_strength(palette::Material &material,
													  const tinygltf::Material &gltfMaterial) const {
	auto extIter = gltfMaterial.extensions.find("KHR_materials_emissive_strength");
	if (extIter == gltfMaterial.extensions.end()) {
		return;
	}
	const tinygltf::Value::Object &emissiveStrength = extIter->second.Get<tinygltf::Value::Object>();
	const auto strengthIter = emissiveStrength.find("emissiveStrength");
	if (strengthIter == emissiveStrength.end()) {
		return;
	}
	const float strength = (float)strengthIter->second.Get<double>();
	material.setValue(palette::MaterialProperty::MaterialEmit,
					  material.value(palette::MaterialProperty::MaterialEmit) * strength);
}

bool GLTFFormat::loadMaterial(const core::String &filename, const io::ArchivePtr &archive,
							  const tinygltf::Model &gltfModel, const tinygltf::Material &gltfMaterial,
							  MeshMaterialPtr &meshMaterial) const {
	meshMaterial = core::make_shared<MeshMaterial>(gltfMaterial.name.c_str());
	const tinygltf::TextureInfo &gltfTextureInfo = gltfMaterial.pbrMetallicRoughness.baseColorTexture;
	if (gltfTextureInfo.index != -1 && gltfTextureInfo.index < (int)gltfModel.textures.size()) {
		loadTexture(filename, archive, gltfModel, meshMaterial, gltfTextureInfo, gltfTextureInfo.index);
	} else {
		Log::debug("Invalid texture index given %i", gltfTextureInfo.index);
	}
	palette::Material &material = meshMaterial->material;
	material.setValue(palette::MaterialProperty::MaterialRoughness,
							 gltfMaterial.pbrMetallicRoughness.roughnessFactor);
	material.setValue(palette::MaterialProperty::MaterialMetal,
							 gltfMaterial.pbrMetallicRoughness.metallicFactor);
	// TODO: MATERIAL: load baseColor
	// const glm::vec4 color = glm::make_vec4(&gltfMaterial.pbrMetallicRoughness.baseColorFactor[0]);
	// meshMaterial->baseColor = core::Color::getRGBA(color);
	// meshMaterial->baseColorFactor = gltfMaterial.pbrMetallicRoughness.baseColorFactor[0];
	// TODO: MATERIAL: load emissiveTexture
	// TODO: MATERIAL: maybe load it as average - there is no 1:1 mapping here
	material.setValue(palette::MaterialProperty::MaterialEmit, gltfMaterial.emissiveFactor[0]);

	// load extensions - some of these rely on values loaded before - that's why they must be loaded last
	load_KHR_materials_emissive_strength(material, gltfMaterial);
	load_KHR_materials_pbrSpecularGlossiness(material, gltfMaterial);
	load_KHR_materials_specular(material, gltfMaterial);
	load_KHR_materials_ior(material, gltfMaterial);

	return true;
}

bool GLTFFormat::loadAttributes(const core::String &filename, const tinygltf::Model &gltfModel,
								const MeshMaterialArray &meshMaterialArray,
								const tinygltf::Primitive &gltfPrimitive,
								core::DynamicArray<GltfVertex> &vertices) const {
	MeshMaterialPtr gltfMaterial;
	if (gltfPrimitive.material >= 0 && gltfPrimitive.material < (int)meshMaterialArray.size()) {
		gltfMaterial = meshMaterialArray[gltfPrimitive.material];
	}
	int foundPositions = 0;
	bool foundColor = false;
	size_t verticesOffset = vertices.size();
	const core::String texCoordAttribute = gltfMaterial ? core::String::format("TEXCOORD_%i", gltfMaterial->uvIndex) : "_NOT_FOUND";
	for (auto &attrIter : gltfPrimitive.attributes) {
		const std::string &attrType = attrIter.first;
		const tinygltf::Accessor *gltfAttributeAccessor = getAccessor(gltfModel, attrIter.second);
		if (gltfAttributeAccessor == nullptr) {
			Log::warn("Could not get accessor for %s", attrType.c_str());
			continue;
		}
		if (verticesOffset + gltfAttributeAccessor->count > vertices.size()) {
			vertices.resize(verticesOffset + gltfAttributeAccessor->count);
		}
		const size_t size = accessorSize(*gltfAttributeAccessor);
		const tinygltf::BufferView &gltfAttributeBufferView = gltfModel.bufferViews[gltfAttributeAccessor->bufferView];
		const size_t stride = gltfAttributeBufferView.byteStride ? gltfAttributeBufferView.byteStride : size;
		const tinygltf::Buffer &gltfAttributeBuffer = gltfModel.buffers[gltfAttributeBufferView.buffer];
		const size_t offset = gltfAttributeAccessor->byteOffset + gltfAttributeBufferView.byteOffset;
		Log::debug("%s: %i (offset: %i, stride: %i)", attrType.c_str(), (int)gltfAttributeAccessor->count, (int)offset,
				   (int)stride);
		const uint8_t *buf = gltfAttributeBuffer.data.data() + offset;
		if (attrType == "POSITION") {
			if (gltfAttributeAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				Log::debug("Skip non float type for %s", attrType.c_str());
				continue;
			}
			foundPositions = gltfAttributeAccessor->count;
			core_assert(gltfAttributeAccessor->type == TINYGLTF_TYPE_VEC3);
			for (size_t i = 0; i < gltfAttributeAccessor->count; i++) {
				io::MemoryReadStream posStream(buf, stride);
				glm::vec3 pos;
				posStream.readFloat(pos.x);
				posStream.readFloat(pos.y);
				posStream.readFloat(pos.z);
				vertices[verticesOffset + i].pos = pos;
				vertices[verticesOffset + i].materialIdx = gltfPrimitive.material;
				buf += stride;
			}
		} else if (attrType == texCoordAttribute.c_str()) {
			if (gltfAttributeAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				Log::debug("Skip non float type (%i) for %s", gltfAttributeAccessor->componentType, attrType.c_str());
				continue;
			}
			core_assert(gltfAttributeAccessor->type == TINYGLTF_TYPE_VEC2);
			for (size_t i = 0; i < gltfAttributeAccessor->count; i++) {
				io::MemoryReadStream uvStream(buf, stride);
				glm::vec2 uv;
				uvStream.readFloat(uv.x);
				uvStream.readFloat(uv.y);
				if (!gltfAttributeAccessor->normalized) {
					uv.y = 1.0f - uv.y;
				}
				vertices[verticesOffset + i].uv = uv;
				buf += stride;
			}
		} else if (core::string::startsWith(attrType.c_str(), "COLOR")) {
			for (size_t i = 0; i < gltfAttributeAccessor->count; i++) {
				vertices[verticesOffset + i].color = _priv::toColor(gltfAttributeAccessor, buf);
				buf += stride;
			}
			foundColor |= gltfAttributeAccessor->count > 0;
		} else {
			Log::debug("Skip unhandled attribute %s", attrType.c_str());
		}
	}
	if (!foundColor) {
		for (int i = 0; i < foundPositions; i++) {
			vertices[verticesOffset + i].color = core::RGBA(127, 127, 127, 255);
		}
	}
	return foundPositions > 0;
}

bool GLTFFormat::loadAnimationChannel(const tinygltf::Model &gltfModel, const tinygltf::Animation &gltfAnimation,
									  const tinygltf::AnimationChannel &gltfAnimChannel,
									  scenegraph::SceneGraphNode &node) const {
	const tinygltf::AnimationSampler &gltfAnimSampler = gltfAnimation.samplers[gltfAnimChannel.sampler];
	scenegraph::InterpolationType interpolation = scenegraph::InterpolationType::Linear;
	if (gltfAnimSampler.interpolation == "LINEAR") {
		interpolation = scenegraph::InterpolationType::Linear;
	} else if (gltfAnimSampler.interpolation == "STEP") {
		interpolation = scenegraph::InterpolationType::Instant;
	} else if (gltfAnimSampler.interpolation == "CUBICSPLINE") {
		interpolation = scenegraph::InterpolationType::CubicBezier;
	} else {
		Log::debug("Unsupported interpolation type: %s", gltfAnimSampler.interpolation.c_str());
	}

	// get the key frame seconds (float)
	{
		const tinygltf::Accessor *gltfFrameTimeAccessor = getAccessor(gltfModel, gltfAnimSampler.input);
		if (gltfFrameTimeAccessor == nullptr || gltfFrameTimeAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
			gltfFrameTimeAccessor->type != TINYGLTF_TYPE_SCALAR) {
			Log::warn("Could not get accessor for samplers");
			return false;
		}
		const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[gltfFrameTimeAccessor->bufferView];
		const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];
		const size_t stride = gltfBufferView.byteStride ? gltfBufferView.byteStride : 4;

		const size_t offset = gltfFrameTimeAccessor->byteOffset + gltfBufferView.byteOffset;
		io::MemoryReadStream stream(gltfBuffer.data.data() + offset, gltfFrameTimeAccessor->count * stride);
		for (size_t i = 0; i < gltfFrameTimeAccessor->count; ++i) {
			stream.seek(i * stride);
			float seconds = 0.0f;
			stream.readFloat(seconds);
			if (node.addKeyFrame((scenegraph::FrameIndex)(seconds * _priv::FPS)) == InvalidKeyFrame) {
				Log::debug("Failed to add keyframe for %f seconds (%i) for node %s", seconds,
						   (int)gltfFrameTimeAccessor->count, node.name().c_str());
			} else {
				Log::debug("Added keyframe for %f seconds (%i) for node %s", seconds, (int)gltfFrameTimeAccessor->count,
						   node.name().c_str());
			}
		}
	}

	// get the key frame values (xyz for translation and scale and xyzw for the rotation)
	{
		const tinygltf::Accessor *gltfTransformAccessor = getAccessor(gltfModel, gltfAnimSampler.output);
		if (gltfTransformAccessor == nullptr) {
			Log::warn("Could not get accessor for samplers");
			return false;
		}

		const size_t size = accessorSize(*gltfTransformAccessor);
		const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[gltfTransformAccessor->bufferView];
		const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];
		const size_t stride = gltfBufferView.byteStride ? gltfBufferView.byteStride : size;

		const size_t offset = gltfTransformAccessor->byteOffset + gltfBufferView.byteOffset;
		const uint8_t *transformBuf = gltfBuffer.data.data() + offset;

		if (gltfTransformAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
			Log::warn("Skip non float type for sampler output");
			return false;
		}
		for (scenegraph::KeyFrameIndex keyFrameIdx = 0;
			 keyFrameIdx < (scenegraph::KeyFrameIndex)gltfTransformAccessor->count; ++keyFrameIdx) {
			io::MemoryReadStream stream(transformBuf, gltfBufferView.byteLength);
			transformBuf += stride;
			scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
			keyFrame.interpolation = interpolation;
			scenegraph::SceneGraphTransform &transform = keyFrame.transform();
			if (gltfAnimChannel.target_path == "translation") {
				core_assert(gltfTransformAccessor->type == TINYGLTF_TYPE_VEC3);
				glm::vec3 v{0.0f};
				stream.readFloat(v.x);
				stream.readFloat(v.y);
				stream.readFloat(v.z);
				transform.setLocalTranslation(v);
			} else if (gltfAnimChannel.target_path == "rotation") {
				core_assert(gltfTransformAccessor->type == TINYGLTF_TYPE_VEC4);
				float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
				stream.readFloat(w);
				stream.readFloat(x);
				stream.readFloat(y);
				stream.readFloat(z);
				glm::quat orientation(w, x, y, z);
				transform.setLocalOrientation(orientation);
			} else if (gltfAnimChannel.target_path == "scale") {
				core_assert(gltfTransformAccessor->type == TINYGLTF_TYPE_VEC3);
				glm::vec3 v{0.0f};
				stream.readFloat(v.x);
				stream.readFloat(v.y);
				stream.readFloat(v.z);
				transform.setLocalScale(v);
			} else {
				Log::debug("Unsupported target path %s", gltfAnimChannel.target_path.c_str());
				break;
			}
		}
	}
	return true;
}

// keyframes https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_007_Animations.md
bool GLTFFormat::loadAnimations(scenegraph::SceneGraph &sceneGraph, const tinygltf::Model &gltfModel, int gltfNodeIdx,
								scenegraph::SceneGraphNode &node) const {
	const size_t animCnt = gltfModel.animations.size();
	int frames = 0;
	for (size_t animIdx = 0; animIdx < animCnt; ++animIdx) {
		const tinygltf::Animation &gltfAnimation = gltfModel.animations[animIdx];
		core::String animationName = gltfAnimation.name.c_str();
		if (animationName.empty()) {
			animationName = core::String::format("animation %i", (int)animIdx);
		}
		sceneGraph.addAnimation(animationName);
		if (!node.setAnimation(animationName)) {
			Log::error("Failed to switch animation to %s", animationName.c_str());
			return false;
		}

		const std::vector<tinygltf::AnimationChannel> &gltfAnimChannels = gltfAnimation.channels;
		for (const tinygltf::AnimationChannel &gltfAnimChannel : gltfAnimChannels) {
			if (gltfAnimChannel.target_node != gltfNodeIdx) {
				continue;
			}
			++frames;
			loadAnimationChannel(gltfModel, gltfAnimation, gltfAnimChannel, node);
		}
	}
	return frames > 0;
}

bool GLTFFormat::loadNode_r(const core::String &filename, scenegraph::SceneGraph &sceneGraph,
							const tinygltf::Model &gltfModel, const MeshMaterialArray &meshMaterialArray,
							int gltfNodeIdx, int parentNodeId) const {
	const tinygltf::Node &gltfNode = gltfModel.nodes[gltfNodeIdx];
	Log::debug("Found node with name '%s'", gltfNode.name.c_str());
	Log::debug(" - camera: %i", gltfNode.camera);
	Log::debug(" - mesh: %i", gltfNode.mesh);
	Log::debug(" - skin: %i", gltfNode.skin);
	Log::debug(" - children: %i", (int)gltfNode.children.size());

	if (gltfNode.camera != -1) {
		const scenegraph::SceneGraphTransform &transform = loadTransform(gltfNode);
		if (gltfNode.camera < 0 || gltfNode.camera >= (int)gltfModel.cameras.size()) {
			Log::debug("Skip invalid camera node %i", gltfNode.camera);
			for (int childId : gltfNode.children) {
				loadNode_r(filename, sceneGraph, gltfModel, meshMaterialArray, childId, parentNodeId);
			}
			return true;
		}
		Log::debug("Camera node %i", gltfNodeIdx);
		const tinygltf::Camera &gltfCamera = gltfModel.cameras[gltfNode.camera];
		scenegraph::SceneGraphNodeCamera node;
		if (!gltfCamera.name.empty()) {
			node.setName(gltfCamera.name.c_str());
		} else {
			node.setName(gltfNode.name.c_str());
		}
		const scenegraph::KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);
		if (gltfCamera.type == "orthographic") {
			node.setOrthographic();
			node.setWidth((int)(gltfCamera.orthographic.xmag * 2.0));
			node.setHeight((int)(gltfCamera.orthographic.ymag * 2.0));
			node.setFarPlane((float)gltfCamera.orthographic.zfar);
			node.setNearPlane((float)gltfCamera.orthographic.znear);
		} else if (gltfCamera.type == "perspective") {
			node.setPerspective();
			node.setAspectRatio((float)gltfCamera.perspective.aspectRatio);
			node.setFieldOfView(
				(int)glm::degrees(gltfCamera.perspective.yfov)); // Field Of View in Y-direction in radians
			node.setFarPlane((float)gltfCamera.perspective.zfar);
			node.setNearPlane((float)gltfCamera.perspective.znear);
		}
		const int cameraId = sceneGraph.emplace(core::move(node), parentNodeId);
		for (int childId : gltfNode.children) {
			loadNode_r(filename, sceneGraph, gltfModel, meshMaterialArray, childId, cameraId);
		}
		return true;
	}

	if (gltfNode.mesh < 0 || gltfNode.mesh >= (int)gltfModel.meshes.size()) {
		int groupId = -1;
		if (!sceneGraph.root().children().empty()) {
			const scenegraph::SceneGraphTransform &transform = loadTransform(gltfNode);
			Log::debug("No mesh node (%i) - add a group %i", gltfNode.mesh, gltfNodeIdx);
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
			node.setName(gltfNode.name.c_str());
			scenegraph::KeyFrameIndex keyFrameIdx = 0;
			node.setTransform(keyFrameIdx, transform);
			groupId = sceneGraph.emplace(core::move(node), parentNodeId);
		}
		if (groupId == -1) {
			groupId = parentNodeId;
		}
		for (int childId : gltfNode.children) {
			loadNode_r(filename, sceneGraph, gltfModel, meshMaterialArray, childId, groupId);
		}
		return true;
	}

	Log::debug("Mesh node %i", gltfNodeIdx);

	const glm::vec3 &scale = getInputScale();
	MeshTriCollection tris;

	const tinygltf::Mesh &gltfMesh = gltfModel.meshes[gltfNode.mesh];
	Log::debug("Primitives: %i in mesh %i", (int)gltfMesh.primitives.size(), gltfNode.mesh);

	for (const tinygltf::Primitive &primitive : gltfMesh.primitives) {
		core::Buffer<uint32_t> indices;
		core::DynamicArray<GltfVertex> vertices;
		if (!loadAttributes(filename, gltfModel, meshMaterialArray, primitive, vertices)) {
			Log::warn("Failed to load vertices");
			continue;
		}
		if (primitive.mode == TINYGLTF_MODE_POINTS) {
			int nodeId = InvalidNodeId;
			if (vertices.size() == 1) {
				scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Point);
				node.setName(gltfNode.name.c_str());
				scenegraph::SceneGraphTransform transform;
				transform.setLocalTranslation(vertices[0].pos);
				node.setTransform(0, transform);
				nodeId = sceneGraph.emplace(core::move(node), parentNodeId);
			} else {
				core::Buffer<PointCloudVertex> pointCloud;
				pointCloud.resize(vertices.size());
				for (int i = 0; i < (int)vertices.size(); ++i) {
					pointCloud[i].position = vertices[i].pos;
					pointCloud[i].color = vertices[i].color;
				}
				voxelizePointCloud(filename, sceneGraph, pointCloud);
			}
			scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
			if (!loadAnimations(sceneGraph, gltfModel, gltfNodeIdx, node)) {
				Log::debug("No animation found or loaded for node %s", node.name().c_str());
				scenegraph::SceneGraphTransform transform = loadTransform(gltfNode);
				scenegraph::KeyFrameIndex keyFrameIdx = 0;
				node.setTransform(keyFrameIdx, transform);
			}

			for (int childId : gltfNode.children) {
				loadNode_r(filename, sceneGraph, gltfModel, meshMaterialArray, childId, nodeId);
			}
		} else if (primitive.indices == -1) {
			if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
				const size_t indicedEnd = vertices.size();
				for (size_t i = 0; i < indicedEnd; ++i) {
					indices.push_back((uint32_t)i);
				}
			} else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) {
				if (vertices.size() < 3) {
					Log::warn("Not enough vertices for triangle fan");
					return false;
				}
				for (size_t i = 1; i < vertices.size() - 1; ++i) {
					indices.push_back((uint32_t)0);
					indices.push_back((uint32_t)i);
					indices.push_back((uint32_t)(i + 1));
				}
			} else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
				if (vertices.size() < 3) {
					Log::warn("Not enough vertices for triangle strip");
					return false;
				}
				for (size_t i = 0; i < vertices.size() - 2; ++i) {
					if (i % 2 == 0) {
						indices.push_back((uint32_t)i);
						indices.push_back((uint32_t)(i + 1));
						indices.push_back((uint32_t)(i + 2));
					} else {
						indices.push_back((uint32_t)(i + 2));
						indices.push_back((uint32_t)(i + 1));
						indices.push_back((uint32_t)i);
					}
				}
			} else {
				Log::warn("Unexpected primitive mode for assembling the indices: %i", primitive.mode);
				return false;
			}
		} else if (!loadIndices(gltfModel, primitive, indices, 0)) {
			Log::warn("Failed to load indices");
			return false;
		}
		// skip empty meshes
		if (indices.empty() || vertices.empty()) {
			Log::debug("No indices (%i) or vertices (%i) found for mesh %i", (int)indices.size(), (int)vertices.size(),
					   gltfNode.mesh);
			for (int childId : gltfNode.children) {
				loadNode_r(filename, sceneGraph, gltfModel, meshMaterialArray, childId, parentNodeId);
			}
			return true;
		}
		Log::debug("Indices (%i) or vertices (%i) found for mesh %i", (int)indices.size(), (int)vertices.size(),
				   gltfNode.mesh);

		if (indices.size() % 3 != 0) {
			Log::error("Unexpected amount of indices %i", (int)indices.size());
			return false;
		}

		const size_t maxIndices = indices.size();
		tris.reserve(tris.size() + maxIndices / 3);
		for (size_t indexOffset = 0; indexOffset < maxIndices; indexOffset += 3) {
			voxelformat::MeshTri meshTri;
			const size_t idx0 = indices[indexOffset];
			const size_t idx1 = indices[indexOffset + 1];
			const size_t idx2 = indices[indexOffset + 2];
			meshTri.setUVs(vertices[idx0].uv, vertices[idx1].uv, vertices[idx2].uv);
			meshTri.setColor(vertices[idx0].color, vertices[idx1].color, vertices[idx2].color);
			meshTri.setVertices(vertices[idx0].pos * scale,
							 vertices[idx1].pos * scale, vertices[idx2].pos * scale);
			const size_t textureIdx = indices[indexOffset];
			const GltfVertex &v = vertices[textureIdx];
			meshTri.materialIdx = v.materialIdx;
			tris.emplace_back(meshTri);
		}
	}

	const int nodeId = voxelizeNode(gltfNode.name.c_str(), sceneGraph, tris, meshMaterialArray, parentNodeId, false);
	if (nodeId == InvalidNodeId) {
		// ignore this node
		return true;
	}
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	if (!loadAnimations(sceneGraph, gltfModel, gltfNodeIdx, node)) {
		Log::debug("No animation found or loaded for node %s", node.name().c_str());
		scenegraph::SceneGraphTransform transform = loadTransform(gltfNode);
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);
	}

	for (int childId : gltfNode.children) {
		loadNode_r(filename, sceneGraph, gltfModel, meshMaterialArray, childId, nodeId);
	}
	return true;
}

bool GLTFFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	stream->peekUInt32(magic);
	const int64_t size = stream->size();
	uint8_t *data = (uint8_t *)core_malloc(size);
	if (stream->read(data, size) == -1) {
		Log::error("Failed to read gltf stream for %s of size %i", filename.c_str(), (int)size);
		core_free(data);
		return false;
	}

	std::string err;
	bool state;

	const core::String filePath = core::string::extractDir(filename);
	tinygltf::TinyGLTF gltfLoader;
	tinygltf::Model gltfModel;
	if (magic == FourCC('g', 'l', 'T', 'F')) {
		Log::debug("Detected binary gltf stream");
		state = gltfLoader.LoadBinaryFromMemory(&gltfModel, &err, nullptr, data, (unsigned int)size, filePath.c_str(),
												tinygltf::SectionCheck::NO_REQUIRE);
		if (!state) {
			Log::error("Failed to load binary gltf file: %s", err.c_str());
		}
	} else {
		Log::debug("Detected ascii gltf stream");
		state = gltfLoader.LoadASCIIFromString(&gltfModel, &err, nullptr, (const char *)data, (unsigned int)size,
											   filePath.c_str(), tinygltf::SectionCheck::NO_REQUIRE);
		if (!state) {
			Log::error("Failed to load ascii gltf file: %s", err.c_str());
		}
	}
	core_free(data);
	if (!state) {
		return false;
	}

	Log::debug("Materials: %i", (int)gltfModel.materials.size());
	Log::debug("Animations: %i", (int)gltfModel.animations.size());
	Log::debug("Meshes: %i", (int)gltfModel.meshes.size());
	Log::debug("Nodes: %i", (int)gltfModel.nodes.size());
	Log::debug("Textures: %i", (int)gltfModel.textures.size());
	Log::debug("Images: %i", (int)gltfModel.images.size());
	Log::debug("Skins: %i", (int)gltfModel.skins.size());
	Log::debug("Samplers: %i", (int)gltfModel.samplers.size());
	Log::debug("Cameras: %i", (int)gltfModel.cameras.size());
	Log::debug("Scenes: %i", (int)gltfModel.scenes.size());
	Log::debug("Lights: %i", (int)gltfModel.lights.size());
	const int parentNodeId = sceneGraph.root().id();

	MeshMaterialArray meshMaterialArray;
	meshMaterialArray.resize(gltfModel.materials.size());
	for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
		const tinygltf::Material &gltfMaterial = gltfModel.materials[i];
		loadMaterial(filename, archive, gltfModel, gltfMaterial, meshMaterialArray[i]);
	}

	scenegraph::SceneGraphNode &root = sceneGraph.node(parentNodeId);
	if (!gltfModel.asset.generator.empty()) {
		root.setProperty(scenegraph::PropGenerator, gltfModel.asset.generator.c_str());
	}
	if (!gltfModel.asset.copyright.empty()) {
		root.setProperty(scenegraph::PropCopyright, gltfModel.asset.copyright.c_str());
	}
	if (!gltfModel.asset.version.empty()) {
		root.setProperty(scenegraph::PropVersion, gltfModel.asset.version.c_str());
	}

	for (const tinygltf::Scene &gltfScene : gltfModel.scenes) {
		Log::debug("Found %i nodes in scene %s", (int)gltfScene.nodes.size(), gltfScene.name.c_str());
		for (int gltfNodeIdx : gltfScene.nodes) {
			loadNode_r(filename, sceneGraph, gltfModel, meshMaterialArray, gltfNodeIdx, parentNodeId);
		}
	}
	return true;
}

} // namespace voxelformat
