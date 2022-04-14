/**
 * @file
 */

#include "GLTFFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/concurrent/Lock.h"
#include "engine-config.h"
#include "io/StdStreamBuf.h"
#include "io/Filesystem.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "core/collection/DynamicArray.h"
#include "voxelformat/SceneGraph.h"

#include <limits.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define JSON_HAS_CPP_11
#include "external/tiny_gltf.h"

namespace voxelformat {

void GLTFFormat::processGltfNode(tinygltf::Model &m, tinygltf::Node &node, tinygltf::Scene &scene,
								 const SceneGraphNode &graphNode, Stack &stack) {
	node.name = graphNode.name().c_str();
	const int idx = (int)m.nodes.size();

	m.nodes.push_back(node);

	if (stack.back().second != -1) {
		m.nodes[stack.back().second].children.push_back(idx);
	} else {
		scene.nodes.push_back(idx);
	}

	stack.pop();

	auto nodeChidren = graphNode.children();

	for (int i = (int)nodeChidren.size() - 1; i >= 0; i--) {
		stack.emplace_back(nodeChidren[i], idx);
	}
}

bool GLTFFormat::saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const SceneGraph &sceneGraph,
							const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
							const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	const voxel::Palette &palette = voxel::getPalette();
	// 1 x 256 is the texture format that we are using for our palette
	const float texcoord = 1.0f / (float)palette.colorCount;
	unsigned char uvYVal[4] = {0, 0, 0, 63};

	core::String palettename = core::string::stripExtension(filename);
	palettename.append(".png");

	const core::String &ext = core::string::extractExtension(filename);
	const bool writeBinary = ext == "glb";
	// const bool embedImages = false; // TODO research
	// const bool embedBuffers = true; // TODO research
	const bool prettyPrint = true;

	tinygltf::TinyGLTF gltf;
	tinygltf::Model m;

	// Define the asset. The version is required
	tinygltf::Asset asset;
	asset.version = "2.0";
	const core::String &generator =
		core::string::format("%s " PROJECT_VERSION, app::App::getInstance()->appname().c_str());
	asset.generator = generator.c_str();
	m.asset = asset;

	tinygltf::Scene scene;

	{
		tinygltf::Image colorPaletteImg;
		colorPaletteImg.uri = core::string::extractFilenameWithExtension(palettename).c_str();
		m.images.emplace_back(core::move(colorPaletteImg));
	}
	{
		tinygltf::Texture paletteTexture;
		paletteTexture.source = 0;
		m.textures.emplace_back(core::move(paletteTexture));
	}

	{
		tinygltf::Material mat;

		if (withTexCoords) {
			mat.pbrMetallicRoughness.baseColorTexture.index = 0;
		} else if (withColor) {
			mat.pbrMetallicRoughness.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
		}

		mat.name = "Default";
		mat.pbrMetallicRoughness.roughnessFactor = 1;
		mat.pbrMetallicRoughness.metallicFactor = 0;
		mat.doubleSided = false;

		m.materials.emplace_back(core::move(mat));
	}

	Log::debug("Exporting %i layers", (int)meshes.size());

	int nthNodeIdx = 0;

	Stack stack;
	stack.emplace_back(0, -1);

	union FloatUnion {
		float f;
		uint8_t b[4];
	};
	union IndexUnion {
		voxel::IndexType i;
		uint8_t b[4];
	};
	static_assert(sizeof(IndexUnion) == sizeof(voxel::IndexType), "");

	while (!stack.empty()) {
		int nodeId = stack.back().first;
		const SceneGraphNode &graphNode = sceneGraph.node(nodeId);

		if (meshIdxNodeMap.find(nodeId) == meshIdxNodeMap.end()) {
			tinygltf::Node node;
			processGltfNode(m, node, scene, graphNode, stack);
			continue;
		}

		int meshExtIdx = 0;
		meshIdxNodeMap.get(nodeId, meshExtIdx);
		const MeshExt &meshExt = meshes[meshExtIdx];

		const voxel::Mesh *mesh = meshExt.mesh;
		Log::debug("Exporting layer %s", meshExt.name.c_str());

		const int nv = (int)mesh->getNoOfVertices();
		const int ni = (int)mesh->getNoOfIndices();

		if (ni % 3 != 0) {
			Log::error("Unexpected indices amount");
			return false;
		}

		const glm::vec3 offset(mesh->getOffset());
		const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
		const voxel::IndexType *indices = mesh->getRawIndexData();
		const char *objectName = meshExt.name.c_str();

		if (objectName[0] == '\0') {
			objectName = "Noname";
		}

		tinygltf::Mesh expMesh;
		tinygltf::Buffer buffer;

		expMesh.name = std::string(objectName);

		const unsigned int remainder = (sizeof(IndexUnion) * ni) % sizeof(FloatUnion);
		const unsigned int paddingBytes = remainder != 0 ? sizeof(FloatUnion) - remainder : 0;

		unsigned int maxIndex = 0;
		unsigned int minIndex = UINT_MAX;

		const bool flip = flipWinding(scale);

		for (int i = 0; i < ni; i++) {
			int idx = i;

			if (flip) {
				idx = ni - i - 1;
			}
			IndexUnion intCharUn;
			intCharUn.i = indices[idx];

			if (maxIndex < intCharUn.i) {
				maxIndex = intCharUn.i;
			}

			if (intCharUn.i < minIndex) {
				minIndex = intCharUn.i;
			}

			for (size_t u = 0; u < sizeof(intCharUn); u++) {
				buffer.data.push_back(intCharUn.b[u]);
			}
		}

		for (unsigned int i = 0; i < paddingBytes; i++) {
			buffer.data.push_back(0x00);
		}

		const unsigned int FLOAT_BUFFER_OFFSET = buffer.data.size();

		glm::vec3 maxVertex(FLT_MIN);
		glm::vec3 minVertex(FLT_MAX);
		glm::vec2 minMaxUVX(FLT_MAX, FLT_MIN);

		for (int j = 0; j < nv; j++) {
			const voxel::VoxelVertex &v = vertices[j];

			glm::vec3 pos;
			if (meshExt.applyTransform) {
				pos = meshExt.transform.apply(v.position, meshExt.size);
			} else {
				pos = v.position;
			}

			pos = (offset + pos) * scale;

			for (int coordIndex = 0; coordIndex < glm::vec3::length(); coordIndex++) {
				FloatUnion floatCharUn;
				floatCharUn.f = pos[coordIndex];

				if (maxVertex[coordIndex] < floatCharUn.f) {
					maxVertex[coordIndex] = floatCharUn.f;
				}

				if (minVertex[coordIndex] > floatCharUn.f) {
					minVertex[coordIndex] = floatCharUn.f;
				}

				for (size_t u = 0; u < sizeof(floatCharUn); u++) {
					buffer.data.push_back(floatCharUn.b[u]);
				}
			}

			if (withTexCoords) {
				const float uuvYVal = ((float)(v.colorIndex) + 0.5f) * texcoord;

				FloatUnion floatCharUn;
				floatCharUn.f = uuvYVal;

				if (minMaxUVX[0] > floatCharUn.f) {
					minMaxUVX[0] = floatCharUn.f;
				}

				if (minMaxUVX[1] < floatCharUn.f) {
					minMaxUVX[1] = floatCharUn.f;
				}

				for (size_t u = 0; u < sizeof(floatCharUn); u++) {
					buffer.data.push_back(floatCharUn.b[u]);
				}

				for (size_t u = 0; u < sizeof(floatCharUn); u++) {
					buffer.data.push_back(uvYVal[u]);
				}

			} else if (withColor) {
				const glm::vec4 &color = core::Color::fromRGBA(palette.colors[v.colorIndex]);

				for (int colorIdx = 0; colorIdx < glm::vec4::length(); colorIdx++) {
					FloatUnion floatCharUn;
					floatCharUn.f = color[colorIdx];

					for (size_t u = 0; u < sizeof(floatCharUn); u++) {
						buffer.data.push_back(floatCharUn.b[u]);
					}
				}
			}
		}

		tinygltf::BufferView indicesBufferView;
		indicesBufferView.buffer = nthNodeIdx;
		indicesBufferView.byteOffset = 0;
		indicesBufferView.byteLength = FLOAT_BUFFER_OFFSET - paddingBytes;
		indicesBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

		tinygltf::BufferView verticesUvBufferView;
		verticesUvBufferView.buffer = nthNodeIdx;
		verticesUvBufferView.byteOffset = FLOAT_BUFFER_OFFSET;
		verticesUvBufferView.byteLength = buffer.data.size() - FLOAT_BUFFER_OFFSET;
		if (withTexCoords) {
			verticesUvBufferView.byteStride = 20;
		} else if (withColor) {
			verticesUvBufferView.byteStride = 28;
		}
		verticesUvBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

		// Describe the layout of indicesBufferView, the indices of the vertices
		tinygltf::Accessor indeicesAccessor;
		indeicesAccessor.bufferView = 2 * nthNodeIdx;
		indeicesAccessor.byteOffset = 0;
		indeicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
		static_assert(sizeof(IndexUnion) == 4, "");
		indeicesAccessor.count = ni;
		indeicesAccessor.type = TINYGLTF_TYPE_SCALAR;
		indeicesAccessor.maxValues.push_back(maxIndex);
		indeicesAccessor.minValues.push_back(minIndex);

		// Describe the layout of verticesUvBufferView, the vertices themself
		tinygltf::Accessor verticesAccessor;
		verticesAccessor.bufferView = 2 * nthNodeIdx + 1;
		verticesAccessor.byteOffset = 0;
		verticesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		verticesAccessor.count = nv;
		verticesAccessor.type = TINYGLTF_TYPE_VEC3;
		verticesAccessor.maxValues = {maxVertex[0], maxVertex[1], maxVertex[2]};
		verticesAccessor.minValues = {minVertex[0], minVertex[1], minVertex[2]};

		tinygltf::Accessor colorTexAccessor;
		if (withTexCoords) {
			// Uvs
			colorTexAccessor.bufferView = 2 * nthNodeIdx + 1;
			colorTexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			colorTexAccessor.count = nv;
			colorTexAccessor.byteOffset = 12;
			colorTexAccessor.type = TINYGLTF_TYPE_VEC2;
			colorTexAccessor.maxValues = {minMaxUVX[1], 0.5};
			colorTexAccessor.minValues = {minMaxUVX[0], 0.5};
		} else if (withColor) {
			// Uvs
			colorTexAccessor.bufferView = 2 * nthNodeIdx + 1;
			colorTexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			colorTexAccessor.count = nv;
			colorTexAccessor.byteOffset = 12;
			colorTexAccessor.type = TINYGLTF_TYPE_VEC4;
		}

		uint8_t primIdxFactor = (withTexCoords || withColor) ? 3 : 2;

		{
			// Build the mesh meshPrimitive and add it to the mesh
			tinygltf::Primitive meshPrimitive;
			meshPrimitive.indices = primIdxFactor * nthNodeIdx; // The index of the accessor for the vertex indices
			meshPrimitive.attributes["POSITION"] =
				primIdxFactor * nthNodeIdx + 1; // The index of the accessor for positions
			if (withTexCoords) {
				meshPrimitive.attributes["TEXCOORD_0"] = primIdxFactor * nthNodeIdx + 2;
			} else if (withColor) {
				meshPrimitive.attributes["COLOR_0"] = primIdxFactor * nthNodeIdx + 2;
			}
			meshPrimitive.material = 0;
			meshPrimitive.mode = TINYGLTF_MODE_TRIANGLES;
			expMesh.primitives.emplace_back(core::move(meshPrimitive));
		}

		tinygltf::Node node;
		node.mesh = nthNodeIdx;
		processGltfNode(m, node, scene, graphNode, stack);

		m.meshes.emplace_back(core::move(expMesh));
		m.buffers.emplace_back(core::move(buffer));
		m.bufferViews.emplace_back(core::move(indicesBufferView));
		m.bufferViews.emplace_back(core::move(verticesUvBufferView));
		m.accessors.emplace_back(core::move(indeicesAccessor));
		m.accessors.emplace_back(core::move(verticesAccessor));

		if (withTexCoords || withColor) {
			m.accessors.emplace_back(core::move(colorTexAccessor));
		}

		nthNodeIdx++;
	}

	m.scenes.emplace_back(core::move(scene));

	io::StdStreamBuf buf(stream);
	std::ostream gltfStream(&buf);
	if (!gltf.WriteGltfSceneToStream(&m, gltfStream, prettyPrint, writeBinary)) {
		Log::error("Could not save to file");
		return false;
	}

	return voxel::getPalette().save(palettename.c_str());
}

size_t GLTFFormat::getGltfAccessorSize(const tinygltf::Accessor &accessor) const {
	return tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
}

const tinygltf::Accessor *GLTFFormat::getGltfAccessor(const tinygltf::Model &model, int id) const {
	if ((size_t)id >= model.accessors.size()) {
		return nullptr;
	}

	const tinygltf::Accessor &accessor = model.accessors[id];
	if (accessor.sparse.isSparse) {
		return nullptr;
	}
	if (accessor.bufferView < 0 || accessor.bufferView >= (int)model.bufferViews.size()) {
		return nullptr;
	}

	const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
	if (bufferView.buffer < 0 || bufferView.buffer >= (int)model.buffers.size()) {
		return nullptr;
	}

	const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
	const size_t viewSize = bufferView.byteOffset + bufferView.byteLength;
	if (buffer.data.size() < viewSize) {
		return nullptr;
	}

	return &accessor;
}

namespace gltf_priv {

template<typename T>
void copyGltfIndices(const uint8_t *data, size_t count, size_t stride, core::DynamicArray<uint32_t> &indices) {
	for (size_t i = 0; i < count; i++) {
		indices.push_back(*(const T*)data);
		data += stride;
	}
}

} // namespace gltf_priv

voxelformat::SceneGraphTransform GLTFFormat::loadGltfTransform(const tinygltf::Node &gltfNode) const {
	SceneGraphTransform transform;
	if (gltfNode.matrix.size() == 16) {
		transform.setMatrix(glm::mat4((float)gltfNode.matrix[0], (float)gltfNode.matrix[1], (float)gltfNode.matrix[2],
									  (float)gltfNode.matrix[3], (float)gltfNode.matrix[4], (float)gltfNode.matrix[5],
									  (float)gltfNode.matrix[6], (float)gltfNode.matrix[7], (float)gltfNode.matrix[8],
									  (float)gltfNode.matrix[9], (float)gltfNode.matrix[10], (float)gltfNode.matrix[11],
									  (float)gltfNode.matrix[12], (float)gltfNode.matrix[13],
									  (float)gltfNode.matrix[14], (float)gltfNode.matrix[15]));
	} else {
		glm::mat4 scaleMat(1.0f);
		glm::mat4 rotMat(1.0f);
		glm::mat4 transMat(1.0f);
		if (gltfNode.scale.size() == 3) {
			scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]));
		}
		if (gltfNode.rotation.size() == 4) {
			const glm::quat quat((float)gltfNode.rotation[0], (float)gltfNode.rotation[1], (float)gltfNode.rotation[2],
								 (float)gltfNode.rotation[3]);
			rotMat = glm::mat4_cast(quat);
		}
		if (gltfNode.translation.size() == 3) {
			transMat = glm::translate(glm::mat4(1.0f),
									  glm::vec3((float)gltfNode.translation[0], (float)gltfNode.translation[1],
												(float)gltfNode.translation[2]));
		}
		const glm::mat4 modelMat = scaleMat * rotMat * transMat;
		transform.setMatrix(modelMat);
	}
	return transform;
}

bool GLTFFormat::loadGltfIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive, core::DynamicArray<uint32_t> &indices) const {
	const tinygltf::Accessor *accessor = getGltfAccessor(model, primitive.indices);
	if (accessor == nullptr) {
		Log::warn("Could not get accessor for indices");
		return false;
	}
	const size_t size = getGltfAccessorSize(*accessor);
	const tinygltf::BufferView& bufferView = model.bufferViews[accessor->bufferView];
	const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
	const size_t stride = bufferView.byteStride ? bufferView.byteStride : size;

	const size_t offset = accessor->byteOffset + bufferView.byteOffset;
	const uint8_t *indexBuf = buffer.data.data() + offset;

	switch (accessor->componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		gltf_priv::copyGltfIndices<uint8_t>(indexBuf, accessor->count, stride, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		gltf_priv::copyGltfIndices<int8_t>(indexBuf, accessor->count, stride, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		gltf_priv::copyGltfIndices<uint16_t>(indexBuf, accessor->count, stride, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		gltf_priv::copyGltfIndices<int16_t>(indexBuf, accessor->count, stride, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		gltf_priv::copyGltfIndices<uint32_t>(indexBuf, accessor->count, stride, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		gltf_priv::copyGltfIndices<int32_t>(indexBuf, accessor->count, stride, indices);
		break;
	default:
		Log::error("Unknown component type for indices: %i", accessor->componentType);
		break;
	}
	return true;
}

bool GLTFFormat::loadGlftAttributes(const core::String &filename, core::StringMap<image::ImagePtr> &textures,
									const tinygltf::Model &model, const tinygltf::Primitive &primitive,
									core::DynamicArray<GltfVertex> &vertices, core::DynamicArray<glm::vec2> &texcoords,
									core::DynamicArray<glm::vec4> &color) const {
	core::String diffuseTexture;
	Log::debug("Primitive material: %i", primitive.material);
	Log::debug("Primitive mode: %i", primitive.mode);
	if (primitive.material >= 0 && primitive.material < (int)model.materials.size()) {
		const tinygltf::Material *gltfMaterial = &model.materials[primitive.material];
		// TODO: load emissiveTexture
		const int textureIndex = gltfMaterial->pbrMetallicRoughness.baseColorTexture.index;
		if (textureIndex != -1 && textureIndex < (int)model.textures.size()) {
			const tinygltf::Texture &colorTexture = model.textures[textureIndex];
			if (colorTexture.source >= 0 && colorTexture.source < (int)model.images.size()) {
				const tinygltf::Image &image = model.images[colorTexture.source];
				core::String name = image.uri.c_str();
				if (!textures.hasKey(name)) {
					if (!core::string::isAbsolutePath(name)) {
						const core::String& path = core::string::extractPath(filename);
						Log::debug("Search image %s in path %s", name.c_str(), path.c_str());
						name = path + name;
					}
					image::ImagePtr tex = image::loadImage(name, false);
					if (tex->isLoaded()) {
						Log::debug("Use image %s", name.c_str());
						diffuseTexture = image.uri.c_str();
						textures.put(diffuseTexture, tex);
					} else {
						Log::warn("Failed to load %s", name.c_str());
					}
				}
			} else {
				Log::debug("Invalid image index given %i", colorTexture.source);
			}
		} else {
			Log::debug("Invalid texture index given %i", textureIndex);
		}
	}

	bool foundPosition = false;
	size_t verticesOffset = 0;
	size_t uvOffset = 0;
	size_t vertexColorOffset = 0;
	for (auto &attrIter : primitive.attributes) {
		const std::string &attrType = attrIter.first;
		const tinygltf::Accessor *attributeAccessor = getGltfAccessor(model, attrIter.second);
		if (attributeAccessor == nullptr) {
			Log::warn("Could not get accessor for %s", attrType.c_str());
			continue;
		}

		const tinygltf::BufferView &attributeBufferView = model.bufferViews[attributeAccessor->bufferView];
		const tinygltf::Buffer &attributeBuffer = model.buffers[attributeBufferView.buffer];
		const size_t offset = attributeAccessor->byteOffset + attributeBufferView.byteOffset;
		const uint8_t *buf = attributeBuffer.data.data() + offset;
		if (attrType == "POSITION") {
			if (attributeAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				Log::debug("Skip non float type for %s", attrType.c_str());
				continue;
			}
			foundPosition = true;
			verticesOffset = vertices.size();
			if (verticesOffset + attributeAccessor->count > vertices.size()) {
				vertices.resize(verticesOffset + attributeAccessor->count);
			}

			const float *posData = (const float *)(buf);
			for (size_t i = 0; i < attributeAccessor->count; i++) {
				vertices[verticesOffset + i].pos = glm::vec3(posData[i * 3 + 0], posData[i * 3 + 1], posData[i * 3 + 2]);
				vertices[verticesOffset + i].texture = diffuseTexture;
			}
		} else if (core::string::startsWith(attrType.c_str(), "TEXCOORD")) {
			if (attributeAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				Log::debug("Skip non float type (%i) for %s", attributeAccessor->componentType, attrType.c_str());
				continue;
			}
			uvOffset = texcoords.size();
			if (uvOffset + attributeAccessor->count > texcoords.size()) {
				texcoords.resize(uvOffset + attributeAccessor->count);
			}
			const float *uvData = (const float *)(buf);
			for (size_t i = 0; i < attributeAccessor->count; i++) {
				texcoords[uvOffset + i] = glm::vec2(uvData[i * 2 + 0], uvData[i * 2 + 1]);
			}
		} else if (core::string::startsWith(attrType.c_str(), "COLOR")) {
			vertexColorOffset = color.size();
			if (vertexColorOffset + attributeAccessor->count > color.size()) {
				color.resize(vertexColorOffset + attributeAccessor->count);
			}
			if (attributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
				const float *colorData = (const float *)(buf);
				for (size_t i = 0; i < attributeAccessor->count; i++) {
					color[vertexColorOffset + i] = glm::vec4(colorData[i * 2 + 0], colorData[i * 2 + 1], colorData[i * 2 + 2], colorData[i * 2 + 3]);
				}
			} else if (attributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
				const uint8_t *colorData = (const uint8_t *)(buf);
				for (size_t i = 0; i < attributeAccessor->count; i++) {
					color[vertexColorOffset + i] = core::Color::fromRGBA(colorData[i * 2 + 0], colorData[i * 2 + 1], colorData[i * 2 + 2], colorData[i * 2 + 3]);
				}
			} else {
				Log::debug("Skip unknown type (%i) for %s", attributeAccessor->componentType, attrType.c_str());
				continue;
			}
		} else {
			Log::debug("Skip unhandled attribute %s", attrType.c_str());
		}
	}
	return foundPosition;
}

bool GLTFFormat::subdivideShape(const tinygltf::Model &model, const core::DynamicArray<uint32_t> &indices,
								const core::DynamicArray<GltfVertex> &vertices,
								const core::DynamicArray<glm::vec2> &texcoords,
								const core::DynamicArray<glm::vec4> &colors,
								const core::StringMap<image::ImagePtr> &textures,
								core::DynamicArray<Tri> &subdivided) const {
	const glm::vec3 &scale = getScale();
	if (indices.size() % 3 != 0) {
		Log::error("Unexpected amount of indices %i", (int)indices.size());
		return false;
	}
	for (size_t indexOffset = 0; indexOffset < indices.size(); indexOffset += 3) {
		Tri tri;
		for (size_t i = 0; i < 3; ++i) {
			const size_t idx = indices[i + indexOffset];
			core_assert_msg(idx < vertices.size(), "Index out of bounds %i/%i", (int)idx, (int)vertices.size());
			tri.vertices[i] = vertices[idx].pos * scale;
			if (idx < texcoords.size()) {
				tri.uv[i] = texcoords[idx];
			} else {
				tri.uv[i] = glm::vec2(0.0f);
			}
		}
		const size_t textureIdx = indices[indexOffset];
		core_assert_msg(textureIdx < vertices.size(), "Index out of bounds %i/%i for textures", (int)textureIdx, (int)vertices.size());
		const core::String &texture = vertices[textureIdx].texture;
		if (!texture.empty()) {
			auto textureIter = textures.find(texture);
			if (textureIter != textures.end()) {
				tri.texture = textureIter->second;
			}
		}
		if (indexOffset < colors.size()) {
			tri.color = core::Color::getRGBA(colors[indexOffset]).rgba;
		}
		subdivideTri(tri, subdivided);
	}
	return !subdivided.empty();
}

void GLTFFormat::calculateAABB(const core::DynamicArray<GltfVertex> &vertices, glm::vec3 &mins, glm::vec3 &maxs) const {
	maxs = glm::vec3(-100000.0f);
	mins = glm::vec3(+100000.0f);

	const glm::vec3 &scale = getScale();

	for (const GltfVertex &v : vertices) {
		const glm::vec3 sv = v.pos * scale;
		maxs.x = core_max(maxs.x, sv.x);
		maxs.y = core_max(maxs.y, sv.y);
		maxs.z = core_max(maxs.z, sv.z);
		mins.x = core_min(mins.x, sv.x);
		mins.y = core_min(mins.y, sv.y);
		mins.z = core_min(mins.z, sv.z);
	}
}

bool GLTFFormat::loadGltfNode_r(const core::String &filename, SceneGraph &sceneGraph, core::StringMap<image::ImagePtr> &textures, tinygltf::Model &model, int gltfNodeIdx, int parentNodeId) const {
	tinygltf::Node &gltfNode = model.nodes[gltfNodeIdx];
	Log::debug("Found node with name '%s'", gltfNode.name.c_str());
	Log::debug(" - camera: %i", gltfNode.camera);
	Log::debug(" - mesh: %i", gltfNode.mesh);
	Log::debug(" - skin: %i", gltfNode.skin);
	Log::debug(" - children: %i", (int)gltfNode.children.size());

	if (gltfNode.camera != -1) {
		const SceneGraphTransform &transform = loadGltfTransform(gltfNode);
		if (gltfNode.camera < 0 || gltfNode.camera >= (int)model.cameras.size()) {
			Log::debug("Skip invalid camera node %i", gltfNode.camera);
			return true;
		}
		Log::debug("Camera node %i", gltfNodeIdx);
		const tinygltf::Camera &cam = model.cameras[gltfNode.camera];
		SceneGraphNode node(SceneGraphNodeType::Camera);
		node.setName(gltfNode.name.c_str());
		node.setTransform(0, transform, true);
		node.setProperty("type", cam.type.c_str());
		int cameraId = sceneGraph.emplace(core::move(node), parentNodeId);
		for (int childId : gltfNode.children) {
			loadGltfNode_r(filename, sceneGraph, textures, model, childId, cameraId);
		}
		return true;
	}

	if (gltfNode.mesh < 0 || gltfNode.mesh >= (int)model.meshes.size()) {
		const SceneGraphTransform &transform = loadGltfTransform(gltfNode);
		Log::debug("No mesh node (%i) - add a group %i", gltfNode.mesh, gltfNodeIdx);
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName(gltfNode.name.c_str());
		node.setTransform(0, transform, true);
		int groupId = sceneGraph.emplace(core::move(node), parentNodeId);
		for (int childId : gltfNode.children) {
			loadGltfNode_r(filename, sceneGraph, textures, model, childId, groupId);
		}
		return true;
	}

	Log::debug("Mesh node %i", gltfNodeIdx);
	tinygltf::Mesh &mesh = model.meshes[gltfNode.mesh];
	core::DynamicArray<uint32_t> indices;
	core::DynamicArray<GltfVertex> vertices;
	core::DynamicArray<glm::vec2> texcoords;
	core::DynamicArray<glm::vec4> colors;
	Log::debug("Primitives: %i in mesh %i", (int)mesh.primitives.size(), gltfNode.mesh);
	for (tinygltf::Primitive &primitive : mesh.primitives) {
		if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
			Log::warn("Unexpected primitive mode: %i", primitive.mode);
			continue;
		}
		if (!loadGltfIndices(model, primitive, indices)) {
			Log::warn("Failed to load indices");
			continue;
		}
		if (!loadGlftAttributes(filename, textures, model, primitive, vertices, texcoords, colors)) {
			Log::warn("Failed to load vertices");
			continue;
		}
	};
	if (indices.empty() || vertices.empty()) {
		Log::error("No indices (%i) or vertices (%i) found for mesh %i", (int)indices.size(), (int)vertices.size(), gltfNode.mesh);
		return false;
	}
	Log::debug("Indices (%i) or vertices (%i) found for mesh %i", (int)indices.size(), (int)vertices.size(), gltfNode.mesh);

	glm::vec3 mins;
	glm::vec3 maxs;
	calculateAABB(vertices, mins, maxs);
	const glm::ivec3 imins = glm::floor(mins);
	const glm::ivec3 imaxs = glm::ceil(maxs);
	voxel::Region region(imins, imaxs);
	if (!region.isValid()) {
		Log::error("Invalid region found %s", region.toString().c_str());
		return false;
	}
	if (glm::any(glm::greaterThan(region.getDimensionsInVoxels(), glm::ivec3(512)))) {
		Log::warn(
			"Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh!");
	}

	SceneGraphNode node;
	const SceneGraphTransform &transform = loadGltfTransform(gltfNode);
	node.setName(gltfNode.name.c_str());
	// TODO: keyframes https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_007_Animations.md
	node.setTransform(0, transform, true);

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	node.setVolume(volume, true);
	core::DynamicArray<Tri> subdivided;
	int newParent = parentNodeId;
	if (!subdivideShape(model, indices, vertices, texcoords, colors, textures, subdivided)) {
		Log::error("Failed to subdivide node %i", gltfNodeIdx);
	} else {
		voxelizeTris(volume, subdivided);
		newParent = sceneGraph.emplace(core::move(node));
	}
	for (int childId : gltfNode.children) {
		loadGltfNode_r(filename, sceneGraph, textures, model, childId, newParent);
	}
	return true;
}

bool GLTFFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	uint32_t magic;
	stream.peekUInt32(magic);
	const int64_t size = stream.size();
	uint8_t* data = (uint8_t*)core_malloc(size);
	if (stream.read(data, size) == -1) {
		Log::error("Failed to read gltf stream");
		core_free(data);
		return false;
	}

	std::string err;
	bool state;

	const core::String filePath = core::string::extractPath(filename);
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	if (magic == FourCC('g','l','T','F')) {
		Log::debug("Detected binary gltf stream");
		state = loader.LoadBinaryFromMemory(&model, &err, nullptr, data, size, filePath.c_str(), tinygltf::SectionCheck::NO_REQUIRE);
		if (!state) {
			Log::error("Failed to load binary gltf file: %s", err.c_str());
		}
	} else {
		Log::debug("Detected ascii gltf stream");
		state = loader.LoadASCIIFromString(&model, &err, nullptr, (const char *)data, size, filePath.c_str(), tinygltf::SectionCheck::NO_REQUIRE);
		if (!state) {
			Log::error("Failed to load ascii gltf file: %s", err.c_str());
		}
	}
	core_free(data);
	if (!state) {
		return false;
	}

	core::StringMap<image::ImagePtr> textures;

	Log::debug("Materials: %i", (int)model.materials.size());
	Log::debug("Animations: %i", (int)model.animations.size());
	Log::debug("Meshes: %i", (int)model.meshes.size());
	Log::debug("Nodes: %i", (int)model.nodes.size());
	Log::debug("Textures: %i", (int)model.textures.size());
	Log::debug("Images: %i", (int)model.images.size());
	Log::debug("Skins: %i", (int)model.skins.size());
	Log::debug("Samplers: %i", (int)model.samplers.size());
	Log::debug("Cameras: %i", (int)model.cameras.size());
	Log::debug("Scenes: %i", (int)model.scenes.size());
	Log::debug("Lights: %i", (int)model.lights.size());
	int parentNodeId = sceneGraph.root().id();

	for (tinygltf::Scene &gltfScene : model.scenes) {
		Log::debug("Found %i nodes in scene %s", (int)gltfScene.nodes.size(), gltfScene.name.c_str());
		for (int gltfNodeIdx : gltfScene.nodes) {
			loadGltfNode_r(filename, sceneGraph, textures, model, gltfNodeIdx, parentNodeId);
		}
	}

	return true;
}

} // namespace voxelformat
