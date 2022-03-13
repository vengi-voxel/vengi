/**
 * @file
 */

#include "GLTFFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ThreadPool.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include <SDL_timer.h>
#include <strstream>

#define TINYGLTF_IMPLEMENTATION
#include "external/tiny_gltf.h"

namespace voxel {

bool GLTFFormat::saveMeshes(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
							const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	return false;
}

void GLTFFormat::processGltfNode(tinygltf::Model &m, tinygltf::Node &node, tinygltf::Scene &scene,
								 voxel::SceneGraphNode &graphNode, Stack &stack) {
	node.name = graphNode.name().c_str();
	const auto idx = m.nodes.size();

	m.nodes.push_back(node);

	if (stack.back().second != -1) {
		m.nodes[stack.back().second].children.push_back(idx);
	} else {
		scene.nodes.push_back(idx);
	}

	stack.pop();

	auto nodeChidren = graphNode.children();

	for (int i = nodeChidren.size() - 1; i >= 0; i--) {
		stack.push_back(std::pair(nodeChidren[i], idx));
	}
}

bool GLTFFormat::saveMeshes(const SceneGraph &sceneGraph, const Meshes &meshes, const core::String &filename,
							io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad, bool withColor,
							bool withTexCoords) {

	const uint8_t UNSIGNED_SHORT_BYTES = 2;
	const uint8_t FLOAT_BYTES = 4;

	const voxel::Palette &palette = voxel::getPalette();
	// 1 x 256 is the texture format that we are using for our palette
	const float texcoord = 1.0f / (float)palette.colorCount;
	// it is only 1 pixel high - sample the middle
	unsigned char uvYVal[4] = {0, 0, 0, 63};

	core::String palettename = core::string::stripExtension(filename);
	palettename.append(".png");

	const core::String &ext = core::string::extractExtension(filename);
	const bool writeBinary = ext == "glb";
	// const bool embedImages = false; // TODO resreach
	// const bool embedBuffers = true; // TODO resreach
	const bool prettyPrint = true;

	tinygltf::TinyGLTF gltf;
	tinygltf::Model m;

	// Define the asset. The version is required
	tinygltf::Asset asset;
	asset.version = "2.0";
	asset.generator = "vengi via tinygltf";
	m.asset = asset;

	tinygltf::Scene scene;

	tinygltf::Image colorPaletteImg;
	colorPaletteImg.uri = core::string::extractFilenameWithExtension(palettename).c_str();

	m.images.push_back(colorPaletteImg);

	tinygltf::Texture paletteTexture;
	paletteTexture.source = 0;

	m.textures.push_back(paletteTexture);

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

	m.materials.push_back(mat);

	Log::debug("Exporting %i layers", (int)meshes.size());

	unsigned int nthNodeIdx = 0;

	Stack stack;
	stack.push_back(std::pair(0, -1));

	while (!stack.empty()) {

		int nodeId = stack.back().first;
		auto &graphNode = sceneGraph.node(nodeId);

		if (meshIdxNodeMap.find(nodeId) == meshIdxNodeMap.end()) {
			tinygltf::Node node;
			processGltfNode(m, node, scene, graphNode, stack);
			continue;
		}

		int meshExtIdx = 0;
		meshIdxNodeMap.get(nodeId, meshExtIdx);
		const auto meshExt = meshes[meshExtIdx];

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
		tinygltf::Primitive meshPrimitive;
		tinygltf::Node node;
		tinygltf::Buffer buffer;
		tinygltf::BufferView indicesBufferView;
		tinygltf::BufferView verticesUvBufferView;
		tinygltf::Accessor indeicesAccessor;
		tinygltf::Accessor verticesAccessor;
		tinygltf::Accessor colorTexAccessor;

		expMesh.name = std::string(objectName);

		const unsigned int remainder = (UNSIGNED_SHORT_BYTES * ni) % FLOAT_BYTES;
		const unsigned int paddingBytes = remainder != 0 ? FLOAT_BYTES - remainder : 0;

		unsigned int maxIndex = 0;
		unsigned int minIndex = INFINITY;

		for (int i = ni - 1; i >= 0; i--) {
			union {
				int i;
				unsigned char b[UNSIGNED_SHORT_BYTES];
			} intCharUn;

			intCharUn.i = indices[i];

			if (maxIndex < intCharUn.i) {
				maxIndex = intCharUn.i;
			}

			if (intCharUn.i < minIndex) {
				minIndex = intCharUn.i;
			}

			for (int u = 0; u < UNSIGNED_SHORT_BYTES; u++) {
				buffer.data.push_back(intCharUn.b[u]);
			}
		}

		for (unsigned int i = 0; i < paddingBytes; i++) {
			buffer.data.push_back(0x00);
		}

		const unsigned int FLOAT_BUFFER_OFFSET = buffer.data.size();

		glm::vec3 maxVertex(-INFINITY, -INFINITY, -INFINITY);
		glm::vec3 minVertex(INFINITY, INFINITY, INFINITY);
		glm::vec2 minMaxUVX(INFINITY, -INFINITY);

		for (int j = 0; j < nv; j++) {

			const voxel::VoxelVertex &v = vertices[j];

			glm::vec3 pos;
			if (meshExt.applyTransform) {
				pos = meshExt.transform.apply(v.position, meshExt.size);
			} else {
				pos = v.position;
			}

			pos = (offset + pos) * scale;

			for (int coordIndex = 0; coordIndex < 3; coordIndex++) {
				union {
					float f;
					unsigned char b[FLOAT_BYTES];
				} floatCharUn;

				floatCharUn.f = pos[coordIndex];

				if (maxVertex[coordIndex] < floatCharUn.f) {
					maxVertex[coordIndex] = floatCharUn.f;
				}

				if (minVertex[coordIndex] > floatCharUn.f) {
					minVertex[coordIndex] = floatCharUn.f;
				}

				for (int u = 0; u < FLOAT_BYTES; u++) {
					buffer.data.push_back(floatCharUn.b[u]);
				}
			}

			if (withTexCoords) {
				const float uuvYVal = ((float)(v.colorIndex) + 0.5f) * texcoord;

				union {
					float f;
					unsigned char b[FLOAT_BYTES];
				} floatCharUn;

				floatCharUn.f = uuvYVal;

				if (minMaxUVX[0] > floatCharUn.f) {
					minMaxUVX[0] = floatCharUn.f;
				}

				if (minMaxUVX[1] < floatCharUn.f) {
					minMaxUVX[1] = floatCharUn.f;
				}

				for (int u = 0; u < FLOAT_BYTES; u++) {
					buffer.data.push_back(floatCharUn.b[u]);
				}

				for (int u = 0; u < FLOAT_BYTES; u++) {
					buffer.data.push_back(uvYVal[u]);
				}

			} else if (withColor) {
				const glm::vec4 &color = core::Color::fromRGBA(palette.colors[v.colorIndex]);

				for (int colorIdx = 0; colorIdx < 4; colorIdx++) {
					union {
						float f;
						unsigned char b[FLOAT_BYTES];
					} floatCharUn;

					floatCharUn.f = color[colorIdx];

					for (int u = 0; u < FLOAT_BYTES; u++) {
						buffer.data.push_back(floatCharUn.b[u]);
					}
				}
			}
		}

		indicesBufferView.buffer = nthNodeIdx;
		indicesBufferView.byteOffset = 0;
		indicesBufferView.byteLength = FLOAT_BUFFER_OFFSET - paddingBytes;
		indicesBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

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
		indeicesAccessor.bufferView = 2 * nthNodeIdx;
		indeicesAccessor.byteOffset = 0;
		indeicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
		indeicesAccessor.count = ni;
		indeicesAccessor.type = TINYGLTF_TYPE_SCALAR;
		indeicesAccessor.maxValues.push_back(maxIndex);
		indeicesAccessor.minValues.push_back(minIndex);

		// Describe the layout of verticesUvBufferView, the vertices themself
		verticesAccessor.bufferView = 2 * nthNodeIdx + 1;
		verticesAccessor.byteOffset = 0;
		verticesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		verticesAccessor.count = nv;
		verticesAccessor.type = TINYGLTF_TYPE_VEC3;
		verticesAccessor.maxValues = {maxVertex[0], maxVertex[1], maxVertex[2]};
		verticesAccessor.minValues = {minVertex[0], minVertex[1], minVertex[2]};

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

		// Build the mesh meshPrimitive and add it to the mesh
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
		expMesh.primitives.push_back(meshPrimitive);

		node.mesh = nthNodeIdx;

		processGltfNode(m, node, scene, graphNode, stack);

		m.meshes.push_back(expMesh);
		m.buffers.push_back(buffer);
		m.bufferViews.push_back(indicesBufferView);
		m.bufferViews.push_back(verticesUvBufferView);
		m.accessors.push_back(indeicesAccessor);
		m.accessors.push_back(verticesAccessor);

		if (withTexCoords || withColor) {
			m.accessors.push_back(colorTexAccessor);
		}

		nthNodeIdx++;
	}

	m.scenes.push_back(scene);

	std::strstream gltfStream;
	if (gltf.WriteGltfSceneToStream(&m, gltfStream, prettyPrint, writeBinary)) {
		stream.write(gltfStream.str(), gltfStream.pcount());
		gltfStream.clear();
	} else {
		Log::error("Could not save to file");
		return false;
	}

	return voxel::getPalette().save(palettename.c_str());
}

bool GLTFFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
							io::SeekableWriteStream &stream) {
	const bool mergeQuads = core::Var::getSafe(cfg::VoxformatMergequads)->boolVal();
	const bool reuseVertices = core::Var::getSafe(cfg::VoxformatReusevertices)->boolVal();
	const bool ambientOcclusion = core::Var::getSafe(cfg::VoxformatAmbientocclusion)->boolVal();

	const float scale = core::Var::getSafe(cfg::VoxformatScale)->floatVal();

	float scaleX = core::Var::getSafe(cfg::VoxformatScaleX)->floatVal();
	float scaleY = core::Var::getSafe(cfg::VoxformatScaleY)->floatVal();
	float scaleZ = core::Var::getSafe(cfg::VoxformatScaleZ)->floatVal();

	scaleX = scaleX != 1.0f ? scaleX : scale;
	scaleY = scaleY != 1.0f ? scaleY : scale;
	scaleZ = scaleZ != 1.0f ? scaleZ : scale;

	const bool quads = core::Var::getSafe(cfg::VoxformatQuads)->boolVal();
	const bool withColor = core::Var::getSafe(cfg::VoxformatWithcolor)->boolVal();
	const bool withTexCoords = core::Var::getSafe(cfg::VoxformatWithtexcoords)->boolVal();
	const bool applyTransform = core::Var::getSafe(cfg::VoxformatTransform)->boolVal();

	const size_t models = sceneGraph.size();
	core::ThreadPool &threadPool = app::App::getInstance()->threadPool();
	Meshes meshes;
	core_trace_mutex(core::Lock, lock, "MeshExporter");
	for (const SceneGraphNode &node : sceneGraph) {
		auto lambda = [&]() {
			voxel::Mesh *mesh = new voxel::Mesh();
			voxel::Region region = node.region();
			region.shiftUpperCorner(1, 1, 1);
			voxel::extractCubicMesh(node.volume(), region, mesh, voxel::IsQuadNeeded(), glm::ivec3(0), mergeQuads,
									reuseVertices, ambientOcclusion);
			core::ScopedLock scoped(lock);
			meshes.emplace_back(mesh, node, applyTransform);

			meshIdxNodeMap.put(node.id(), meshes.size() - 1);
		};
		threadPool.enqueue(lambda);
	}
	while (meshes.size() < models) {
		SDL_Delay(10);
	}
	Log::debug("Save meshes");
	const bool state = saveMeshes(sceneGraph, meshes, filename, stream, glm::vec3(scaleX, scaleY, scaleZ), quads,
								  withColor, withTexCoords);
	for (MeshExt &meshext : meshes) {
		delete meshext.mesh;
	}
	return state;
}

} // namespace voxel
