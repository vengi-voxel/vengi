/**
 * @file
 */

#include "GLTFFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/String.h"
#include "engine-config.h"
#include "io/StdStreamBuf.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include <limits.h>

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
		const MeshExt& meshExt = meshes[meshExtIdx];

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

} // namespace voxel
