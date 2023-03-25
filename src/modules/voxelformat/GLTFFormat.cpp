/**
 * @file
 */

#include "GLTFFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ThreadPool.h"
#include "engine-config.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/StdStreamBuf.h"
#include "io/Filesystem.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/Palette.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/VoxelVertex.h"
#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/PaletteLookup.h"
#include "voxelutil/VoxelUtil.h"

#include <future>
#include <limits.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_IMPLEMENTATION
// #define TINYGLTF_NO_FS // TODO: use our own file abstraction
#define JSON_HAS_CPP_11
#include "external/tiny_gltf.h"

namespace voxelformat {

namespace _priv {
const float FPS = 24.0f;
}

void GLTFFormat::processGltfNode(tinygltf::Model &m, tinygltf::Node &node, tinygltf::Scene &scene,
								 const scenegraph::SceneGraphNode &graphNode, Stack &stack, const scenegraph::SceneGraph &sceneGraph,
								 const glm::vec3 &scale, bool exportAnimations) {
	node.name = graphNode.name().c_str();
	Log::debug("process node %s", node.name.c_str());
	const int idx = (int)m.nodes.size();

	if (!exportAnimations) {
		glm::mat4x4 nodeLocalMatrix = graphNode.transform().localMatrix();
		if (graphNode.id() == 0) {
			nodeLocalMatrix = glm::scale(nodeLocalMatrix, scale);
		}

		if (nodeLocalMatrix != glm::mat4(1.0f)) {
			std::vector<double> nodeMatrixArray;
			nodeMatrixArray.reserve(16);
			const float *pSource = (const float *)glm::value_ptr(nodeLocalMatrix);

			for (int i = 0; i < 16; ++i) {
				nodeMatrixArray.push_back(pSource[i]);
			}

			node.matrix = nodeMatrixArray;
		}
	}

	m.nodes.push_back(node);

	if (stack.back().second != -1) {
		m.nodes[stack.back().second].children.push_back(idx);
	} else {
		scene.nodes.push_back(idx);
	}

	stack.pop();

	const scenegraph::SceneGraphNodeChildren &nodeChildren = graphNode.children();

	for (int i = (int)nodeChildren.size() - 1; i >= 0; i--) {
		stack.emplace_back(nodeChildren[i], idx);
	}
}

static tinygltf::Camera processCamera(const scenegraph::SceneGraphNodeCamera &cam) {
	tinygltf::Camera c;
	c.name = cam.name().c_str();
	if (cam.isPerspective()) {
		c.type = "perspective";
		c.perspective.aspectRatio = cam.aspectRatio();
		c.perspective.yfov = cam.fieldOfView();
		c.perspective.zfar = cam.farPlane();
		c.perspective.znear = cam.nearPlane();
	} else if (cam.isOrthographic()) {
		c.type = "orthographic";
		c.orthographic.xmag = cam.width() / 2.0;
		c.orthographic.ymag = cam.height() / 2.0;
		c.orthographic.zfar = cam.farPlane();
		c.orthographic.znear = cam.nearPlane();
	}
	return c;
}

bool GLTFFormat::saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
							const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
							const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	const core::String &ext = core::string::extractExtension(filename);
	const bool writeBinary = ext == "glb";
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

	Log::debug("Exporting %i layers", (int)meshes.size());

	Stack stack;
	stack.emplace_back(0, -1);

	core::Map<uint64_t, int> paletteMaterialIndices((int)sceneGraph.size());
	while (!stack.empty()) {
		const int nodeId = stack.back().first;
		const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(nodeId);
		const voxel::Palette &palette = graphNode.palette();

		int materialId = -1;
		int texcoordIndex = 0;
		if (graphNode.type() == scenegraph::SceneGraphNodeType::Model) {
			const auto palTexIter = paletteMaterialIndices.find(palette.hash());
			if (palTexIter != paletteMaterialIndices.end()) {
				materialId = palTexIter->second;
				Log::debug("Re-use material id %i for hash %" PRIu64, materialId, palette.hash());
			} else {
				const core::String hashId = core::String::format("%" PRIu64, palette.hash());

				const int imageIndex = (int)m.images.size();
				{
					tinygltf::Image colorPaletteImg;
					image::Image image("pal");
					image.loadRGBA((const unsigned char *)palette.colors(), voxel::PaletteMaxColors, 1);
					const core::String &pal64 = image.pngBase64();
					colorPaletteImg.uri = "data:image/png;base64,";
					colorPaletteImg.width = voxel::PaletteMaxColors;
					colorPaletteImg.height = 1;
					colorPaletteImg.component = 4;
					colorPaletteImg.bits = 32;
					colorPaletteImg.uri += pal64.c_str();
					m.images.emplace_back(core::move(colorPaletteImg));
				}

				const int textureIndex = (int)m.textures.size();
				{
					tinygltf::Texture paletteTexture;
					paletteTexture.source = imageIndex;
					m.textures.emplace_back(core::move(paletteTexture));
				}
				// TODO: save emissiveTexture

				{
					tinygltf::Material mat;
					if (withTexCoords) {
						mat.pbrMetallicRoughness.baseColorTexture.index = textureIndex;
						mat.pbrMetallicRoughness.baseColorTexture.texCoord = texcoordIndex;
					} else if (withColor) {
						mat.pbrMetallicRoughness.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
					}

					mat.name = hashId.c_str();
					mat.pbrMetallicRoughness.roughnessFactor = 1;
					mat.pbrMetallicRoughness.metallicFactor = 0;
					mat.doubleSided = false;

					materialId = (int)m.materials.size();
					m.materials.emplace_back(core::move(mat));
				}
				paletteMaterialIndices.put(palette.hash(), materialId);
				Log::debug("New material id %i for hash %" PRIu64, materialId, palette.hash());
			}
		}
		bool exportAnimations = true;

		if (meshIdxNodeMap.find(nodeId) == meshIdxNodeMap.end()) {
			tinygltf::Node node;
			processGltfNode(m, node, scene, graphNode, stack, sceneGraph, scale, false);
			continue;
		}

		int meshExtIdx = 0;
		meshIdxNodeMap.get(nodeId, meshExtIdx);
		const MeshExt &meshExt = meshes[meshExtIdx];

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

			const voxel::VertexArray &vertices = mesh->getVertexVector();
			const voxel::IndexArray &indices = mesh->getIndexVector();
			const char *objectName = meshExt.name.c_str();

			if (objectName[0] == '\0') {
				objectName = "Noname";
			}

			tinygltf::Mesh expMesh;
			const size_t expectedSize = (size_t)ni * sizeof(voxel::IndexType) + (size_t)nv * 7 * sizeof(float);
			io::BufferedReadWriteStream os((int64_t)expectedSize);

			expMesh.name = std::string(objectName);

			unsigned int maxIndex = 0;
			unsigned int minIndex = UINT_MAX;

			for (int i = 0; i < ni; i++) {
				const int idx = i;
				os.writeUInt32(indices[idx]);

				if (maxIndex < indices[idx]) {
					maxIndex = indices[idx];
				}

				if (indices[idx] < minIndex) {
					minIndex = indices[idx];
				}
			}

			static_assert(sizeof(voxel::IndexType) == 4, "if not 4 bytes - we might need padding here");
			const unsigned int FLOAT_BUFFER_OFFSET = os.size();

			glm::vec3 maxVertex(-FLT_MAX);
			glm::vec3 minVertex(FLT_MAX);

			const glm::vec3 &offset = mesh->getOffset();

			const glm::vec3 pivotOffset = offset - meshExt.pivot * meshExt.size;
			for (int j = 0; j < nv; j++) {
				glm::vec3 pos = vertices[j].position;

				if (meshExt.applyTransform) {
					pos = pos + pivotOffset;
				}

				for (int coordIndex = 0; coordIndex < glm::vec3::length(); coordIndex++) {
					os.writeFloat(pos[coordIndex]);

					if (maxVertex[coordIndex] < pos[coordIndex]) {
						maxVertex[coordIndex] = pos[coordIndex];
					}

					if (minVertex[coordIndex] > pos[coordIndex]) {
						minVertex[coordIndex] = pos[coordIndex];
					}
				}

				if (withTexCoords) {
					const glm::vec2 &uv = paletteUV(vertices[j].colorIndex);
					os.writeFloat(uv.x);
					os.writeFloat(uv.y);
				} else if (withColor) {
					const glm::vec4 &color = core::Color::fromRGBA(palette.color(vertices[j].colorIndex));
					for (int colorIdx = 0; colorIdx < glm::vec4::length(); colorIdx++) {
						os.writeFloat(color[colorIdx]);
					}
				}
			}

			tinygltf::BufferView indicesBufferView;
			indicesBufferView.buffer = (int)m.buffers.size();
			indicesBufferView.byteOffset = 0;
			indicesBufferView.byteLength = FLOAT_BUFFER_OFFSET;
			indicesBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

			tinygltf::BufferView verticesUvBufferView;
			verticesUvBufferView.buffer = (int)m.buffers.size();
			verticesUvBufferView.byteOffset = FLOAT_BUFFER_OFFSET;
			verticesUvBufferView.byteLength = os.size() - FLOAT_BUFFER_OFFSET;
			verticesUvBufferView.byteStride = sizeof(glm::vec3);
			if (withTexCoords) {
				verticesUvBufferView.byteStride += sizeof(glm::vec2);
			} else if (withColor) {
				verticesUvBufferView.byteStride += sizeof(glm::vec4);
			}
			verticesUvBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

			// Describe the layout of indicesBufferView, the indices of the vertices
			tinygltf::Accessor indicesAccessor;
			indicesAccessor.bufferView = (int)m.bufferViews.size();
			indicesAccessor.byteOffset = 0;
			indicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
			indicesAccessor.count = ni;
			indicesAccessor.type = TINYGLTF_TYPE_SCALAR;
			indicesAccessor.maxValues.push_back(maxIndex);
			indicesAccessor.minValues.push_back(minIndex);

			// Describe the layout of verticesUvBufferView, the vertices themself
			tinygltf::Accessor verticesAccessor;
			verticesAccessor.bufferView = (int)m.bufferViews.size() + 1;
			verticesAccessor.byteOffset = 0;
			verticesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			verticesAccessor.count = nv;
			verticesAccessor.type = TINYGLTF_TYPE_VEC3;
			verticesAccessor.maxValues = {maxVertex[0], maxVertex[1], maxVertex[2]};
			verticesAccessor.minValues = {minVertex[0], minVertex[1], minVertex[2]};

			tinygltf::Accessor colorTexAccessor;
			if (withTexCoords) {
				colorTexAccessor.bufferView = (int)m.bufferViews.size() + 1;
				colorTexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				colorTexAccessor.count = nv;
				colorTexAccessor.byteOffset = 1 * sizeof(glm::vec3);
				colorTexAccessor.type = TINYGLTF_TYPE_VEC2;
			} else if (withColor) {
				colorTexAccessor.bufferView = (int)m.bufferViews.size() + 1;
				colorTexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				colorTexAccessor.count = nv;
				colorTexAccessor.byteOffset = 1 * sizeof(glm::vec3);
				colorTexAccessor.type = TINYGLTF_TYPE_VEC4;
			}

			// TODO: normals
			{
				// Build the mesh meshPrimitive and add it to the mesh
				tinygltf::Primitive meshPrimitive;
				// The index of the accessor for the vertex indices
				meshPrimitive.indices = (int)m.accessors.size();
				// The index of the accessor for positions
				meshPrimitive.attributes["POSITION"] = (int)m.accessors.size() + 1;
				if (withTexCoords) {
					const core::String &texcoordsKey = core::String::format("TEXCOORD_%i", texcoordIndex);
					meshPrimitive.attributes[texcoordsKey.c_str()] = (int)m.accessors.size() + 2;
				} else if (withColor) {
					meshPrimitive.attributes["COLOR_0"] = (int)m.accessors.size() + 2;
				}
				meshPrimitive.material = materialId;
				meshPrimitive.mode = TINYGLTF_MODE_TRIANGLES;
				expMesh.primitives.emplace_back(core::move(meshPrimitive));
			}

			{
				tinygltf::Node node;
				node.mesh = (int)m.meshes.size();
				processGltfNode(m, node, scene, graphNode, stack, sceneGraph, scale, exportAnimations);
			}


			{
				tinygltf::Buffer buffer;
				buffer.data.insert(buffer.data.end(), os.getBuffer(), os.getBuffer() + os.size());
				m.buffers.emplace_back(core::move(buffer));
			}

			m.meshes.emplace_back(core::move(expMesh));
			Log::debug("Index buffer view at %i", (int)m.bufferViews.size());
			m.bufferViews.emplace_back(core::move(indicesBufferView));
			Log::debug("vertex buffer view at %i", (int)m.bufferViews.size());
			m.bufferViews.emplace_back(core::move(verticesUvBufferView));
			m.accessors.emplace_back(core::move(indicesAccessor));
			m.accessors.emplace_back(core::move(verticesAccessor));

			if (withTexCoords || withColor) {
				m.accessors.emplace_back(core::move(colorTexAccessor));
			}

			if (exportAnimations) {
				// export the current active animation as first animation
				processAnimation(m, scene, graphNode, sceneGraph, sceneGraph.activeAnimation());
				for (const core::String &animation : sceneGraph.animations()) {
					if (animation ==  sceneGraph.activeAnimation()) {
						continue;
					}
					processAnimation(m, scene, graphNode, sceneGraph, animation);
				}
			}
		}
	}

	m.scenes.emplace_back(core::move(scene));
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Camera); iter != sceneGraph.end(); ++iter) {
		tinygltf::Camera gltfCamera = processCamera(toCameraNode(*iter));
		if (gltfCamera.type.empty()) {
			continue;
		}
		m.cameras.push_back(gltfCamera);
	}

	io::StdOStreamBuf buf(stream);
	std::ostream gltfStream(&buf);
	if (!gltf.WriteGltfSceneToStream(&m, gltfStream, prettyPrint, writeBinary)) {
		Log::error("Could not save to file");
		return false;
	}

	return true;
}

static int addBuffer(tinygltf::Model &m, io::BufferedReadWriteStream &stream, const char *name) {
	tinygltf::Buffer buffer;
	buffer.name = name;
	buffer.data.insert(buffer.data.end(), stream.getBuffer(), stream.getBuffer() + stream.size());
	m.buffers.emplace_back(core::move(buffer));
	return (int)(m.buffers.size() - 1);
}

void GLTFFormat::processAnimation(tinygltf::Model &m, tinygltf::Scene &scene,
								  const scenegraph::SceneGraphNode &graphNode, const scenegraph::SceneGraph &sceneGraph,
								  const core::String &animationId) {
	const scenegraph::FrameIndex maxFrames = sceneGraph.maxFrames(animationId);
	if (maxFrames <= 0) {
		return;
	}
	const int targetNode = (int)m.meshes.size() - 1;
	io::BufferedReadWriteStream osTime((int64_t)(maxFrames * sizeof(float)));
	io::BufferedReadWriteStream osTranslation((int64_t)(maxFrames * 3 * sizeof(float)));
	io::BufferedReadWriteStream osRotation((int64_t)(maxFrames * 4 * sizeof(float)));
	io::BufferedReadWriteStream osScale((int64_t)(maxFrames * 3 * sizeof(float)));

	for (scenegraph::FrameIndex i = 0; i < maxFrames; ++i) {
		osTime.writeFloat((float)i / _priv::FPS);

		const scenegraph::SceneGraphTransform &transform = graphNode.transformForFrame(animationId, i);
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

	int bufferTimeId = addBuffer(m, osTime, "time");
	int bufferTranslationId = addBuffer(m, osTranslation, "translation");
	int bufferRotationId = addBuffer(m, osRotation, "rotation");
	int bufferScaleId = addBuffer(m, osScale, "scale");

	const int timeAccessorIdx = (int)m.accessors.size();
	{
		tinygltf::Accessor accessor;
		accessor.type = TINYGLTF_TYPE_SCALAR;
		accessor.bufferView = (int)m.bufferViews.size();
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.count = maxFrames;
		accessor.minValues.push_back(0.0);
		accessor.maxValues.push_back((double)(maxFrames - 1) / _priv::FPS);
		m.accessors.emplace_back(accessor);

		tinygltf::BufferView bufferView;
		bufferView.buffer = bufferTimeId;
		bufferView.byteLength = osTime.size();
		Log::debug("animation %s time buffer view at %i", animationId.c_str(), (int)m.bufferViews.size());
		m.bufferViews.emplace_back(bufferView);
	}

	const int translationAccessorIndex = (int)m.accessors.size();
	{
		tinygltf::Accessor accessor;
		accessor.type = TINYGLTF_TYPE_VEC3;
		accessor.bufferView = (int)m.bufferViews.size();
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.count = maxFrames;
		m.accessors.emplace_back(accessor);

		tinygltf::BufferView bufferView;
		bufferView.buffer = bufferTranslationId;
		bufferView.byteLength = osTranslation.size();
		Log::debug("animation %s time buffer view at %i", animationId.c_str(), (int)m.bufferViews.size());
		m.bufferViews.emplace_back(bufferView);
	}
	const int rotationAccessorIndex = (int)m.accessors.size();
	{
		tinygltf::Accessor accessor;
		accessor.type = TINYGLTF_TYPE_VEC4;
		accessor.bufferView = (int)m.bufferViews.size();
		accessor.byteOffset = 0;
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.count = maxFrames;
		m.accessors.emplace_back(accessor);

		tinygltf::BufferView bufferView;
		bufferView.buffer = bufferRotationId;
		bufferView.byteLength = osRotation.size();
		Log::debug("anim rotation buffer: %i", accessor.bufferView);
		m.bufferViews.emplace_back(bufferView);
	}
	const int scaleAccessorIndex = (int)m.accessors.size();
	{
		tinygltf::Accessor accessor;
		accessor.type = TINYGLTF_TYPE_VEC3;
		accessor.bufferView = (int)m.bufferViews.size();
		accessor.byteOffset = 0;
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.count = maxFrames;
		m.accessors.emplace_back(accessor);

		tinygltf::BufferView bufferView;
		bufferView.buffer = bufferScaleId;
		bufferView.byteLength = osScale.size();
		Log::debug("anim scale buffer: %i", accessor.bufferView);
		m.bufferViews.emplace_back(bufferView);
	}

	tinygltf::Animation animation;
	animation.name = animationId.c_str();
	{
		tinygltf::AnimationSampler sampler;
		sampler.input = timeAccessorIdx;
		sampler.output = translationAccessorIndex;
		sampler.interpolation = "LINEAR";
		animation.samplers.emplace_back(sampler);

		tinygltf::AnimationChannel channel;
		channel.sampler = (int)animation.samplers.size() - 1;
		channel.target_node = targetNode;
		channel.target_path = "translation";
		animation.channels.emplace_back(channel);
	}

	{
		tinygltf::AnimationSampler sampler;
		sampler.input = timeAccessorIdx;
		sampler.output = rotationAccessorIndex;
		sampler.interpolation = "LINEAR";
		animation.samplers.emplace_back(sampler);

		tinygltf::AnimationChannel channel;
		channel.sampler = (int)animation.samplers.size() - 1;
		channel.target_node = targetNode;
		channel.target_path = "rotation";
		animation.channels.emplace_back(channel);
	}

	{
		tinygltf::AnimationSampler sampler;
		sampler.input = timeAccessorIdx;
		sampler.output = scaleAccessorIndex;
		sampler.interpolation = "LINEAR";
		animation.samplers.emplace_back(sampler);

		tinygltf::AnimationChannel channel;
		channel.sampler = (int)animation.samplers.size() - 1;
		channel.target_node = targetNode;
		channel.target_path = "scale";
		animation.channels.emplace_back(channel);
	}

	m.animations.emplace_back(animation);
}

size_t GLTFFormat::getGltfAccessorSize(const tinygltf::Accessor &accessor) const {
	return tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
}

const tinygltf::Accessor *GLTFFormat::getGltfAccessor(const tinygltf::Model &model, int id) const {
	if ((size_t)id >= model.accessors.size()) {
		Log::debug("Invalid accessor id: %i", id);
		return nullptr;
	}

	const tinygltf::Accessor &accessor = model.accessors[id];
	if (accessor.sparse.isSparse) {
		Log::debug("Sparse accessor");
		return nullptr;
	}
	if (accessor.bufferView < 0 || accessor.bufferView >= (int)model.bufferViews.size()) {
		Log::debug("Invalid bufferview id: %i (%i vs max %i)", id, accessor.bufferView, (int)model.bufferViews.size());
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
void copyGltfIndices(const uint8_t *data, size_t count, size_t stride, core::DynamicArray<uint32_t> &indices, uint32_t offset) {
	for (size_t i = 0; i < count; i++) {
		indices.push_back((uint32_t)(*(const T*)data) + offset);
		data += stride;
	}
}

} // namespace gltf_priv

scenegraph::SceneGraphTransform GLTFFormat::loadGltfTransform(const tinygltf::Node &gltfNode) const {
	scenegraph::SceneGraphTransform transform;
	if (gltfNode.matrix.size() == 16) {
		transform.setLocalMatrix(glm::mat4((float)gltfNode.matrix[0], (float)gltfNode.matrix[1], (float)gltfNode.matrix[2],
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
		transform.setLocalMatrix(modelMat);
	}
	transform.setLocalScale(transform.localScale() / getScale());
	return transform;
}

bool GLTFFormat::loadGltfIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive, core::DynamicArray<uint32_t> &indices, size_t indicesOffset) const {
	if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
		Log::warn("Unexpected primitive mode: %i", primitive.mode);
		return false;
	}
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

	Log::debug("indicesOffset: %i", (int)indicesOffset);

	switch (accessor->componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		gltf_priv::copyGltfIndices<uint8_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		gltf_priv::copyGltfIndices<int8_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		gltf_priv::copyGltfIndices<uint16_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		gltf_priv::copyGltfIndices<int16_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		gltf_priv::copyGltfIndices<uint32_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		gltf_priv::copyGltfIndices<int32_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	default:
		Log::error("Unknown component type for indices: %i", accessor->componentType);
		break;
	}
	return true;
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

bool GLTFFormat::loadGlftAttributes(const core::String &filename, core::StringMap<image::ImagePtr> &textures,
									const tinygltf::Model &model, const tinygltf::Primitive &primitive,
									core::DynamicArray<GltfVertex> &vertices) const {
	core::String diffuseTexture;
	Log::debug("Primitive material: %i", primitive.material);
	Log::debug("Primitive mode: %i", primitive.mode);
	int texCoordIndex = 0;
	image::TextureWrap wrapS = image::TextureWrap::Repeat;
	image::TextureWrap wrapT = image::TextureWrap::Repeat;
	if (primitive.material >= 0 && primitive.material < (int)model.materials.size()) {
		const tinygltf::Material *gltfMaterial = &model.materials[primitive.material];
		// TODO: load emissiveTexture
		const tinygltf::TextureInfo &textureInfo = gltfMaterial->pbrMetallicRoughness.baseColorTexture;
		const int textureIndex = textureInfo.index;
		if (textureIndex != -1 && textureIndex < (int)model.textures.size()) {
			const tinygltf::Texture &colorTexture = model.textures[textureIndex];
			if (colorTexture.source >= 0 && colorTexture.source < (int)model.images.size()) {
				if (colorTexture.sampler >= 0 && colorTexture.sampler < (int)model.samplers.size()) {
					const tinygltf::Sampler &sampler = model.samplers[colorTexture.sampler];
					Log::debug("Sampler: %s, wrapS: %i, wrapT: %i", sampler.name.c_str(), sampler.wrapS, sampler.wrapT);
					wrapS = convertTextureWrap(sampler.wrapS);
					wrapT = convertTextureWrap(sampler.wrapT);
				}
				const tinygltf::Image &image = model.images[colorTexture.source];
				Log::debug("Image components: %i, width: %i, height: %i, bits: %i", image.component, image.width,
						   image.height, image.bits);
				if (image.uri.empty()) {
					if (image.bufferView >= 0 && image.bufferView < (int)model.bufferViews.size()) {
						const tinygltf::BufferView &imgBufferView = model.bufferViews[image.bufferView];
						if (imgBufferView.buffer >= 0 && imgBufferView.buffer < (int)model.buffers.size()) {
							const tinygltf::Buffer &imgBuffer = model.buffers[imgBufferView.buffer];
							const size_t offset = imgBufferView.byteOffset;
							const uint8_t *buf = imgBuffer.data.data() + offset;
							image::ImagePtr tex = image::createEmptyImage(image.name.c_str());
							if (!tex->load(buf, (int)imgBufferView.byteLength)) {
								Log::warn("Failed to load embedded image %s", image.name.c_str());
							} else {
								diffuseTexture = image.name.c_str();
								textures.emplace(diffuseTexture, core::move(tex));
							}
						} else {
							Log::warn("Invalid buffer index for image: %i", imgBufferView.buffer);
						}
					} else if (!image.image.empty()) {
						if (image.component == 4) {
							core::String name = image.name.c_str();
							if (name.empty()) {
								name = core::string::format("image%i", colorTexture.source);
							}
							image::ImagePtr tex = image::createEmptyImage(name);
							core_assert(image.image.size() == (size_t)(image.width * image.height * image.component));
							tex->loadRGBA(image.image.data(), image.width, image.height);
							Log::debug("Use image %s", name.c_str());
							diffuseTexture = name.c_str();
							textures.emplace(diffuseTexture, core::move(tex));
							texCoordIndex = textureInfo.texCoord;
						} else {
							Log::warn("Failed to load image with %i components", image.component);
						}
					} else {
						Log::warn("Invalid buffer view index for image: %i", image.bufferView);
					}
				} else {
					core::String name = image.uri.c_str();
					if (!textures.hasKey(name)) {
						name = lookupTexture(filename, name);
						image::ImagePtr tex = image::loadImage(name);
						if (tex->isLoaded()) {
							Log::debug("Use image %s", name.c_str());
							diffuseTexture = image.uri.c_str();
							textures.emplace(diffuseTexture, core::move(tex));
							texCoordIndex = textureInfo.texCoord;
						} else {
							Log::warn("Failed to load %s", name.c_str());
						}
					} else {
						diffuseTexture = name;
					}
				}
			} else {
				Log::debug("Invalid image index given %i", colorTexture.source);
			}
		} else {
			Log::debug("Invalid texture index given %i", textureIndex);
		}
	}

	const core::String texCoordAttribute = core::string::format("TEXCOORD_%i", texCoordIndex);
	Log::debug("Texcoords: %s", texCoordAttribute.c_str());

	bool foundPosition = false;
	size_t verticesOffset = vertices.size();
	for (auto &attrIter : primitive.attributes) {
		const std::string &attrType = attrIter.first;
		const tinygltf::Accessor *attributeAccessor = getGltfAccessor(model, attrIter.second);
		if (attributeAccessor == nullptr) {
			Log::warn("Could not get accessor for %s", attrType.c_str());
			continue;
		}
		if (verticesOffset + attributeAccessor->count > vertices.size()) {
			vertices.resize(verticesOffset + attributeAccessor->count);
		}
		const size_t size = getGltfAccessorSize(*attributeAccessor);
		const tinygltf::BufferView &attributeBufferView = model.bufferViews[attributeAccessor->bufferView];
		const size_t stride = attributeBufferView.byteStride ? attributeBufferView.byteStride : size;
		const tinygltf::Buffer &attributeBuffer = model.buffers[attributeBufferView.buffer];
		const size_t offset = attributeAccessor->byteOffset + attributeBufferView.byteOffset;
		Log::debug("%s: %i (offset: %i, stride: %i)", attrType.c_str(), (int)attributeAccessor->count, (int)offset, (int)stride);
		const uint8_t *buf = attributeBuffer.data.data() + offset;
		if (attrType == "POSITION") {
			if (attributeAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				Log::debug("Skip non float type for %s", attrType.c_str());
				continue;
			}
			foundPosition = true;
			core_assert(attributeAccessor->type == TINYGLTF_TYPE_VEC3);
			for (size_t i = 0; i < attributeAccessor->count; i++) {
				const float *posData = (const float *)buf;
				vertices[verticesOffset + i].pos = glm::vec3(posData[0], posData[1], posData[2]);
				vertices[verticesOffset + i].texture = diffuseTexture;
				buf += stride;
			}
		} else if (attrType == texCoordAttribute.c_str()) {
			if (attributeAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				Log::debug("Skip non float type (%i) for %s", attributeAccessor->componentType, attrType.c_str());
				continue;
			}
			core_assert(attributeAccessor->type == TINYGLTF_TYPE_VEC2);
			for (size_t i = 0; i < attributeAccessor->count; i++) {
				const float *uvData = (const float *)buf;
				vertices[verticesOffset + i].uv = glm::vec2(uvData[0], uvData[1]);
				vertices[verticesOffset + i].wrapS = wrapS;
				vertices[verticesOffset + i].wrapT = wrapT;
				buf += stride;
			}
		} else if (core::string::startsWith(attrType.c_str(), "COLOR")) {
			if (attributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
				for (size_t i = 0; i < attributeAccessor->count; i++) {
					const float *colorData = (const float *)(buf);
					const float alpha = attributeAccessor->type == TINYGLTF_TYPE_VEC4 ? colorData[3] : 1.0f;
					vertices[verticesOffset + i].color = core::Color::getRGBA(glm::vec4(colorData[0], colorData[1], colorData[2], alpha));
					buf += stride;
				}
			} else if (attributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
				for (size_t i = 0; i < attributeAccessor->count; i++) {
					const uint8_t *colorData = buf;
					const uint8_t alpha = attributeAccessor->type == TINYGLTF_TYPE_VEC4 ? colorData[3] : 255u;
					vertices[verticesOffset + i].color = core::RGBA(colorData[0], colorData[1], colorData[2], alpha);
					buf += stride;
				}
			} else if (attributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
				for (size_t i = 0; i < attributeAccessor->count; i++) {
					const uint16_t *colorData = (const uint16_t *)buf;
					const uint8_t alpha = attributeAccessor->type == TINYGLTF_TYPE_VEC4 ? colorData[3] / 256u : 255u;
					vertices[verticesOffset + i].color = core::RGBA(colorData[0] / 256, colorData[1] / 256, colorData[2] / 256, alpha);
					buf += stride;
				}
			} else {
				Log::warn("Skip unknown type for vertex colors (%i) for %s", attributeAccessor->componentType, attrType.c_str());
				continue;
			}
		} else {
			Log::debug("Skip unhandled attribute %s", attrType.c_str());
		}
	}
	return foundPosition;
}

bool GLTFFormat::subdivideShape(scenegraph::SceneGraphNode &node,
								const TriCollection &tris,
								const glm::vec3 &offset,
								bool axisAlignedMesh) const {
	auto func = [&offset, &axisAlignedMesh](Tri tri) {
		for (size_t i = 0; i < 3; ++i) {
			tri.vertices[i] -= offset;
		}
		TriCollection subdivided;

		if (axisAlignedMesh) {
			subdivided.push_back(tri);
		} else {
			subdivideTri(tri, subdivided);
		}

		return core::move(subdivided);
	};

	core::ThreadPool &threadPool = app::App::getInstance()->threadPool();
	const bool fillHollow = core::Var::getSafe(cfg::VoxformatFillHollow)->boolVal();
	core::DynamicArray<std::future<TriCollection>> futures;
	futures.reserve(tris.size());
	threadPool.reserve(futures.size());
	for (const Tri &tri : tris) {
		futures.emplace_back(threadPool.enqueue(func, tri));
	}

	voxel::Palette palette;
	voxel::RawVolume *volume = node.volume();
	int n = 0;
	for (auto &f : futures) {
		const TriCollection &tris = f.get();
		if (!tris.empty()) {
			PosMap posMap;

			if (axisAlignedMesh) {
				const glm::vec3 &triMins = glm::round(tris[0].mins());
				const glm::vec3 &triMaxs = glm::round(tris[0].maxs());
				const glm::ivec3 &triDimensions = glm::max(glm::abs(triMaxs - triMins), 1.0f);
				posMap = PosMap(triDimensions.x * triDimensions.y * triDimensions.z);
				transformTrisAxisAligned(tris, posMap);
			} else {
				posMap = PosMap((int)tris.size() * 3);
				transformTris(tris, posMap);
			}

			for (const auto &entry : posMap) {
				const PosSampling &pos = entry->second;
				const core::RGBA color = pos.avgColor(_flattenFactor);
				palette.addColorToPalette(color, true);
				const voxel::Voxel voxel = voxel::createVoxel(palette, palette.getClosestMatch(color));
				volume->setVoxel(entry->first, voxel);
			}
		}
		++n;
		Log::debug("step: %i, tris: %i", n, (int)tris.size());
	}

	Log::debug("colors: %i", (int)palette.size());
	if (palette.colorCount() == 1) {
		if (palette.colors()[0].a == 0) {
			palette.color(0).a = 255;
		}
	}
	node.setPalette(palette);
	if (fillHollow) {
		Log::debug("fill hollows");
		voxel::RawVolumeWrapper wrapper(volume);
		voxelutil::fillHollow(wrapper, voxel::Voxel(voxel::VoxelType::Generic, 2));
	}
	return true;
}

bool GLTFFormat::loadGltfNode_r(const core::String &filename, scenegraph::SceneGraph &sceneGraph, core::StringMap<image::ImagePtr> &textures, const tinygltf::Model &model, int gltfNodeIdx, int parentNodeId) const {
	const tinygltf::Node &gltfNode = model.nodes[gltfNodeIdx];
	Log::debug("Found node with name '%s'", gltfNode.name.c_str());
	Log::debug(" - camera: %i", gltfNode.camera);
	Log::debug(" - mesh: %i", gltfNode.mesh);
	Log::debug(" - skin: %i", gltfNode.skin);
	Log::debug(" - children: %i", (int)gltfNode.children.size());

	if (gltfNode.camera != -1) {
		const scenegraph::SceneGraphTransform &transform = loadGltfTransform(gltfNode);
		if (gltfNode.camera < 0 || gltfNode.camera >= (int)model.cameras.size()) {
			Log::debug("Skip invalid camera node %i", gltfNode.camera);
			for (int childId : gltfNode.children) {
				loadGltfNode_r(filename, sceneGraph, textures, model, childId, parentNodeId);
			}
			return true;
		}
		Log::debug("Camera node %i", gltfNodeIdx);
		const tinygltf::Camera &cam = model.cameras[gltfNode.camera];
		scenegraph::SceneGraphNodeCamera node;
		if (!cam.name.empty()) {
			node.setName(cam.name.c_str());
		} else {
			node.setName(gltfNode.name.c_str());
		}
		const scenegraph::KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);
		if (cam.type == "orthographic") {
			node.setOrthographic();
			node.setWidth((int)(cam.orthographic.xmag * 2.0));
			node.setHeight((int)(cam.orthographic.ymag * 2.0));
			node.setFarPlane((float)cam.orthographic.zfar);
			node.setNearPlane((float)cam.orthographic.znear);
		} else if (cam.type == "perspective") {
			node.setPerspective();
			node.setAspectRatio((float)cam.perspective.aspectRatio);
			node.setFieldOfView((int)glm::degrees(cam.perspective.yfov)); // Field Of View in Y-direction in radians
			node.setFarPlane((float)cam.perspective.zfar);
			node.setNearPlane((float)cam.perspective.znear);
		}
		const int cameraId = sceneGraph.emplace(core::move(node), parentNodeId);
		for (int childId : gltfNode.children) {
			loadGltfNode_r(filename, sceneGraph, textures, model, childId, cameraId);
		}
		return true;
	}

	if (gltfNode.mesh < 0 || gltfNode.mesh >= (int)model.meshes.size()) {
		const scenegraph::SceneGraphTransform &transform = loadGltfTransform(gltfNode);
		Log::debug("No mesh node (%i) - add a group %i", gltfNode.mesh, gltfNodeIdx);
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
		node.setName(gltfNode.name.c_str());
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);
		int groupId = sceneGraph.emplace(core::move(node), parentNodeId);
		if (groupId == -1) {
			groupId = parentNodeId;
		}
		for (int childId : gltfNode.children) {
			loadGltfNode_r(filename, sceneGraph, textures, model, childId, groupId);
		}
		return true;
	}

	Log::debug("Mesh node %i", gltfNodeIdx);
	const tinygltf::Mesh &mesh = model.meshes[gltfNode.mesh];
	core::DynamicArray<uint32_t> indices;
	core::DynamicArray<GltfVertex> vertices;
	Log::debug("Primitives: %i in mesh %i", (int)mesh.primitives.size(), gltfNode.mesh);
	for (const tinygltf::Primitive &primitive : mesh.primitives) {
		const size_t indicesStart = vertices.size();
		if (!loadGlftAttributes(filename, textures, model, primitive, vertices)) {
			Log::warn("Failed to load vertices");
			continue;
		}
		if (primitive.indices == -1) {
			if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
				const size_t indicedEnd = vertices.size();
				indices.clear();
				for (size_t i = indicesStart; i < indicedEnd; ++i) {
					indices.push_back(i);
				}
			} else {
				Log::warn("Unexpected primitive mode for assembling the indices: %i", primitive.mode);
				return false;
			}
		} else if (!loadGltfIndices(model, primitive, indices, indicesStart)) {
			Log::warn("Failed to load indices");
			return false;
		}
	}
	if (indices.empty() || vertices.empty()) {
		Log::error("No indices (%i) or vertices (%i) found for mesh %i", (int)indices.size(), (int)vertices.size(), gltfNode.mesh);
		for (int childId : gltfNode.children) {
			loadGltfNode_r(filename, sceneGraph, textures, model, childId, parentNodeId);
		}
		return false;
	}
	Log::debug("Indices (%i) or vertices (%i) found for mesh %i", (int)indices.size(), (int)vertices.size(), gltfNode.mesh);

	if (indices.size() % 3 != 0) {
		Log::error("Unexpected amount of indices %i", (int)indices.size());
		return false;
	}

	TriCollection tris;
	const size_t maxIndices = indices.size();
	tris.reserve(maxIndices / 3);
	const glm::vec3 &scale = getScale();
	for (size_t indexOffset = 0; indexOffset < maxIndices; indexOffset += 3) {
		Tri tri;
		for (size_t i = 0; i < 3; ++i) {
			const size_t idx = indices[i + indexOffset];
			tri.vertices[i] = vertices[idx].pos * scale;
			tri.uv[i] = vertices[idx].uv;
			tri.color[i] = vertices[idx].color;
		}
		const size_t textureIdx = indices[indexOffset];
		const GltfVertex &v = vertices[textureIdx];
		tri.wrapS = v.wrapS;
		tri.wrapT = v.wrapT;
		if (!v.texture.empty()) {
			auto textureIter = textures.find(v.texture);
			if (textureIter != textures.end()) {
				tri.texture = textureIter->second.get();
			} else {
				Log::warn("Texture %s not found", v.texture.c_str());
			}
		} else {
			Log::trace("No texture for vertex found");
		}
		tris.push_back(tri);
	}

	glm::vec3 mins;
	glm::vec3 maxs;
	calculateAABB(tris, mins, maxs);

	glm::vec3 regionOffset(0.0f);
	glm::ivec3 imins;
	glm::ivec3 imaxs;

	const bool axisAligned = isVoxelMesh(tris);
	if (axisAligned) {
		const glm::vec3 aabbCenterDelta = (mins + maxs) / 2.0f;

		mins -= aabbCenterDelta;
		maxs -= aabbCenterDelta;

		imins = glm::round(mins);
		const glm::vec3 minsDelta = glm::vec3(imins) - mins;

		imaxs = glm::round(maxs + minsDelta);
		imaxs -= 1;

		regionOffset = aabbCenterDelta - minsDelta;
	} else {
		imins = glm::floor(mins);
		imaxs = glm::ceil(maxs);
	}

	const voxel::Region region(imins, imaxs);
	if (!region.isValid()) {
		Log::error("Invalid region found %s", region.toString().c_str());
		return false;
	}
	const glm::ivec3 &vdim = region.getDimensionsInVoxels();
	if (glm::any(glm::greaterThan(vdim, glm::ivec3(512)))) {
		Log::warn("Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh! (%i:%i:%i)",
				  vdim.x, vdim.y, vdim.z);
	}
	Log::debug("region mins(%i:%i:%i)/maxs(%i:%i:%i)", imins.x, imins.y, imins.z, imaxs.x, imaxs.y, imaxs.z);

	scenegraph::SceneGraphNode node;
	scenegraph::SceneGraphTransform transform = loadGltfTransform(gltfNode);
	node.setName(gltfNode.name.c_str());
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx++, transform);

	// keyframes https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_007_Animations.md
	const size_t animCnt = model.animations.size();
	for (size_t animIdx = 0; animIdx < animCnt; ++animIdx) {
		const tinygltf::Animation &animation = model.animations[animIdx];
		const core::String animationName = animation.name.c_str();
		if (!animationName.empty()) {
			sceneGraph.addAnimation(animationName);
			node.setAnimation(animationName);
		}

		const std::vector<tinygltf::AnimationChannel> &channels = animation.channels;
		for (const tinygltf::AnimationChannel& channel : channels) {
			const int nodeId = channel.target_node;
			if (nodeId != gltfNodeIdx) {
				continue;
			}
			const tinygltf::AnimationSampler &sampler = animation.samplers[channel.sampler];
			const std::string& type = channel.target_path;
			scenegraph::InterpolationType interpolation = scenegraph::InterpolationType::Linear;
			if (sampler.interpolation == "LINEAR") {
				interpolation = scenegraph::InterpolationType::Linear;
			} else if (sampler.interpolation == "STEP") {
				interpolation = scenegraph::InterpolationType::Instant;
			// } else if (sampler.interpolation == "CUBICSPLINE") {
				// TODO: implement easing for this type
				// interpolation = InterpolationType::Linear;
			}

			// get the key frame seconds (float)
			{
				const tinygltf::Accessor *accessor = getGltfAccessor(model, sampler.input);
				if (accessor == nullptr || accessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor->type != TINYGLTF_TYPE_SCALAR) {
					Log::warn("Could not get accessor for samplers");
					continue;
				}
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor->bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				const size_t stride = bufferView.byteStride ? bufferView.byteStride : 4;

				const size_t offset = accessor->byteOffset + bufferView.byteOffset;
				const uint8_t *buf = buffer.data.data() + offset;
				for (size_t i = 0; i < accessor->count; ++i) {
					const float seconds = *(const float*)buf;
					node.addKeyFrame((scenegraph::FrameIndex)(seconds * _priv::FPS));
					buf += stride;
				}
			}

			// get the key frame values (xyz for translation and scale and xyzw for the rotation)
			{
				const tinygltf::Accessor *accessor = getGltfAccessor(model, sampler.output);
				if (accessor == nullptr) {
					Log::warn("Could not get accessor for samplers");
					continue;
				}

				const size_t size = getGltfAccessorSize(*accessor);
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor->bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				const size_t stride = bufferView.byteStride ? bufferView.byteStride : size;

				const size_t offset = accessor->byteOffset + bufferView.byteOffset;
				const uint8_t *transformBuf = buffer.data.data() + offset;

				if (accessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
					Log::warn("Skip non float type for sampler output");
					continue;
				}
				for (size_t keyFrameIdx = 0; keyFrameIdx < accessor->count; ++keyFrameIdx) {
					const float *buf = (const float *)transformBuf;
					transformBuf += stride;
					scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame((scenegraph::KeyFrameIndex)keyFrameIdx);
					keyFrame.interpolation = interpolation;
					scenegraph::SceneGraphTransform &transform = keyFrame.transform();
					if (type == "translation") {
						core_assert(accessor->type == TINYGLTF_TYPE_VEC3);
						glm::vec3 v(buf[0], buf[1], buf[2]);
						transform.setLocalTranslation(v);
					} else if (type == "rotation") {
						core_assert(accessor->type == TINYGLTF_TYPE_VEC4);
						glm::quat orientation(buf[0], buf[1], buf[2], buf[3]);
						transform.setLocalOrientation(orientation);
					} else if (type == "scale") {
						core_assert(accessor->type == TINYGLTF_TYPE_VEC3);
						glm::vec3 v(buf[0], buf[1], buf[2]);
						transform.setLocalScale(v);
					} else if (type == "weights") {
						// TODO: not supported yet
						break;
					}
				}
			}
		}
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	node.setVolume(volume, true);
	node.setPivot(-regionOffset / glm::vec3(vdim));
	int newParent = parentNodeId;
	// TODO: use voxelizeNode here and remove subdivideShape
	if (!subdivideShape(node, tris, regionOffset * scale, axisAligned)) {
		Log::error("Failed to subdivide node %i", gltfNodeIdx);
	} else {
		newParent = sceneGraph.emplace(core::move(node), parentNodeId);
		if (newParent == -1) {
			Log::error("Failed to add node");
			newParent = parentNodeId;
		}
	}
	for (int childId : gltfNode.children) {
		loadGltfNode_r(filename, sceneGraph, textures, model, childId, newParent);
	}
	return true;
}

bool GLTFFormat::voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	uint32_t magic;
	stream.peekUInt32(magic);
	const int64_t size = stream.size();
	uint8_t* data = (uint8_t*)core_malloc(size);
	if (stream.read(data, size) == -1) {
		Log::error("Failed to read gltf stream for %s of size %i", filename.c_str(), (int)size);
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
	const int parentNodeId = sceneGraph.root().id();

	scenegraph::SceneGraphNode& root = sceneGraph.node(parentNodeId);
	if (!model.asset.generator.empty()) {
		root.setProperty("Generator", model.asset.generator.c_str());
	}
	if (!model.asset.copyright.empty()) {
		root.setProperty("Copyright", model.asset.copyright.c_str());
	}
	if (!model.asset.version.empty()) {
		root.setProperty("Version", model.asset.version.c_str());
	}

	for (const tinygltf::Scene &gltfScene : model.scenes) {
		Log::debug("Found %i nodes in scene %s", (int)gltfScene.nodes.size(), gltfScene.name.c_str());
		for (int gltfNodeIdx : gltfScene.nodes) {
			loadGltfNode_r(filename, sceneGraph, textures, model, gltfNodeIdx, parentNodeId);
		}
	}
	return true;
}

} // namespace voxelformat
