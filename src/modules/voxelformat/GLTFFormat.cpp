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
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/StdStreamBuf.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/Palette.h"
#include "voxel/VoxelVertex.h"
#include "voxelutil/VoxelUtil.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits.h>

#define TINYGLTF_IMPLEMENTATION
// #define TINYGLTF_NO_FS // TODO: use our own file abstraction
#define JSON_HAS_CPP_11
#include "external/tiny_gltf.h"

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
	if (gltfAttributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
		const float *colorData = (const float *)(buf);
		const float alpha = gltfAttributeAccessor->type == TINYGLTF_TYPE_VEC4 ? colorData[3] : 1.0f;
		return core::Color::getRGBA(glm::vec4(colorData[0], colorData[1], colorData[2], alpha));
	} else if (gltfAttributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
		const uint8_t *colorData = buf;
		const uint8_t alpha = gltfAttributeAccessor->type == TINYGLTF_TYPE_VEC4 ? colorData[3] : 255u;
		return core::RGBA(colorData[0], colorData[1], colorData[2], alpha);
	} else if (gltfAttributeAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
		const uint16_t *colorData = (const uint16_t *)buf;
		const uint8_t alpha = gltfAttributeAccessor->type == TINYGLTF_TYPE_VEC4 ? colorData[3] / 256u : 255u;
		return core::RGBA(colorData[0] / 256, colorData[1] / 256, colorData[2] / 256, alpha);
	} else {
		Log::warn("Skip unknown type for vertex colors (%i)", gltfAttributeAccessor->componentType);
	}
	return core::RGBA(0, 0, 0, 255);
}

template <typename T>
void copyGltfIndices(const uint8_t *data, size_t count, size_t stride, core::DynamicArray<uint32_t> &indices,
					 uint32_t offset) {
	for (size_t i = 0; i < count; i++) {
		indices.push_back((uint32_t)(*(const T *)data) + offset);
		data += stride;
	}
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

} // namespace _priv

void GLTFFormat::saveGltfNode(core::Map<int, int> &nodeMapping, tinygltf::Model &gltfModel, tinygltf::Node &gltfNode,
							  tinygltf::Scene &gltfScene, const scenegraph::SceneGraphNode &node, Stack &stack,
							  const scenegraph::SceneGraph &sceneGraph, const glm::vec3 &scale, bool exportAnimations) {
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

			gltfNode.matrix = nodeMatrixArray;
		}
	}

	gltfModel.nodes.push_back(gltfNode);
	nodeMapping.put(node.id(), idx);

	if (stack.back().second != -1) {
		gltfModel.nodes[stack.back().second].children.push_back(idx);
	} else {
		gltfScene.nodes.push_back(idx);
	}

	stack.pop();

	const scenegraph::SceneGraphNodeChildren &nodeChildren = node.children();

	for (int i = (int)nodeChildren.size() - 1; i >= 0; i--) {
		stack.emplace_back(nodeChildren[i], idx);
	}
}

bool GLTFFormat::saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
							const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
							const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	const core::String &ext = core::string::extractExtension(filename);
	const bool writeBinary = ext == "glb";

	tinygltf::TinyGLTF gltf;
	tinygltf::Model gltfModel;
	tinygltf::Scene gltfScene;

	const size_t modelNodes = meshes.size();
	const core::String &appname = app::App::getInstance()->appname();
	const core::String &generator = core::string::format("%s " PROJECT_VERSION, appname.c_str());
	// Define the asset. The version is required
	gltfModel.asset.generator = generator.c_str();
	gltfModel.asset.version = "2.0";
	gltfModel.asset.copyright = sceneGraph.root().property("Copyright").c_str();
	gltfModel.accessors.reserve(modelNodes * 4 + sceneGraph.animations().size() * 4);

	Stack stack;
	stack.emplace_back(0, -1);

	const bool exportAnimations = sceneGraph.hasAnimations();

	core::Map<uint64_t, int> paletteMaterialIndices((int)sceneGraph.size());
	core::Map<int, int> nodeMapping((int)sceneGraph.nodeSize());
	while (!stack.empty()) {
		const int nodeId = stack.back().first;
		const scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
		const voxel::Palette &palette = node.palette();

		int materialId = -1;
		int texcoordIndex = 0;
		if (node.type() == scenegraph::SceneGraphNodeType::Model) {
			const auto palTexIter = paletteMaterialIndices.find(palette.hash());
			if (palTexIter != paletteMaterialIndices.end()) {
				materialId = palTexIter->second;
				Log::debug("Re-use material id %i for hash %" PRIu64, materialId, palette.hash());
			} else {
				const core::String hashId = core::String::format("%" PRIu64, palette.hash());

				const int imageIndex = (int)gltfModel.images.size();
				{
					tinygltf::Image gltfPaletteImage;
					image::Image image("pal");
					image.loadRGBA((const unsigned char *)palette.colors(), voxel::PaletteMaxColors, 1);
					const core::String &pal64 = image.pngBase64();
					gltfPaletteImage.uri = "data:image/png;base64,";
					gltfPaletteImage.width = voxel::PaletteMaxColors;
					gltfPaletteImage.height = 1;
					gltfPaletteImage.component = 4;
					gltfPaletteImage.bits = 32;
					gltfPaletteImage.uri += pal64.c_str();
					gltfModel.images.emplace_back(core::move(gltfPaletteImage));
				}

				const int textureIndex = (int)gltfModel.textures.size();
				{
					tinygltf::Texture gltfPaletteTexture;
					gltfPaletteTexture.source = imageIndex;
					gltfModel.textures.emplace_back(core::move(gltfPaletteTexture));
				}
				// TODO: save emissiveTexture

				{
					tinygltf::Material gltfMaterial;
					if (withTexCoords) {
						gltfMaterial.pbrMetallicRoughness.baseColorTexture.index = textureIndex;
						gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord = texcoordIndex;
					} else if (withColor) {
						gltfMaterial.pbrMetallicRoughness.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
					}

					gltfMaterial.name = hashId.c_str();
					gltfMaterial.pbrMetallicRoughness.roughnessFactor = 1.0;
					gltfMaterial.pbrMetallicRoughness.metallicFactor = 0.0;
					gltfMaterial.doubleSided = false;

					materialId = (int)gltfModel.materials.size();
					gltfModel.materials.emplace_back(core::move(gltfMaterial));
				}
				paletteMaterialIndices.put(palette.hash(), materialId);
				Log::debug("New material id %i for hash %" PRIu64, materialId, palette.hash());
			}
		}

		if (meshIdxNodeMap.find(nodeId) == meshIdxNodeMap.end()) {
			tinygltf::Node gltfNode;
			saveGltfNode(nodeMapping, gltfModel, gltfNode, gltfScene, node, stack, sceneGraph, scale, false);
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
			const voxel::NormalArray &normals = mesh->getNormalVector();
			const voxel::IndexArray &indices = mesh->getIndexVector();
			const char *objectName = meshExt.name.c_str();
			const bool exportNormals = !normals.empty();
			if (exportNormals) {
				Log::debug("Export normals for mesh %i", i);
			}

			if (objectName[0] == '\0') {
				objectName = "Noname";
			}

			tinygltf::Mesh gltfMesh;
			const size_t expectedSize = (size_t)ni * sizeof(voxel::IndexType) + (size_t)nv * 10 * sizeof(float);
			io::BufferedReadWriteStream os((int64_t)expectedSize);

			gltfMesh.name = std::string(objectName);

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

				if (exportNormals) {
					for (int coordIndex = 0; coordIndex < glm::vec3::length(); coordIndex++) {
						os.writeFloat(normals[j][coordIndex]);
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

			tinygltf::BufferView gltfIndicesBufferView;
			gltfIndicesBufferView.buffer = (int)gltfModel.buffers.size();
			gltfIndicesBufferView.byteOffset = 0;
			gltfIndicesBufferView.byteLength = FLOAT_BUFFER_OFFSET;
			gltfIndicesBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

			tinygltf::BufferView gltfVerticesBufferView;
			gltfVerticesBufferView.buffer = (int)gltfModel.buffers.size();
			gltfVerticesBufferView.byteOffset = FLOAT_BUFFER_OFFSET;
			gltfVerticesBufferView.byteLength = os.size() - FLOAT_BUFFER_OFFSET;
			gltfVerticesBufferView.byteStride = sizeof(glm::vec3);
			if (exportNormals) {
				gltfVerticesBufferView.byteStride += sizeof(glm::vec3);
			}
			if (withTexCoords) {
				gltfVerticesBufferView.byteStride += sizeof(glm::vec2);
			} else if (withColor) {
				gltfVerticesBufferView.byteStride += sizeof(glm::vec4);
			}
			gltfVerticesBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

			// Describe the layout of indicesBufferView, the indices of the vertices
			tinygltf::Accessor gltfIndicesAccessor;
			gltfIndicesAccessor.bufferView = (int)gltfModel.bufferViews.size();
			gltfIndicesAccessor.byteOffset = 0;
			gltfIndicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
			gltfIndicesAccessor.count = ni;
			gltfIndicesAccessor.type = TINYGLTF_TYPE_SCALAR;
			gltfIndicesAccessor.maxValues.push_back(maxIndex);
			gltfIndicesAccessor.minValues.push_back(minIndex);

			// Describe the layout of verticesUvBufferView, the vertices themself
			tinygltf::Accessor gltfVerticesAccessor;
			gltfVerticesAccessor.bufferView = (int)gltfModel.bufferViews.size() + 1;
			gltfVerticesAccessor.byteOffset = 0;
			gltfVerticesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			gltfVerticesAccessor.count = nv;
			gltfVerticesAccessor.type = TINYGLTF_TYPE_VEC3;
			gltfVerticesAccessor.maxValues = {maxVertex[0], maxVertex[1], maxVertex[2]};
			gltfVerticesAccessor.minValues = {minVertex[0], minVertex[1], minVertex[2]};

			// Describe the layout of normals - they are followed
			tinygltf::Accessor gltfNormalAccessor;
			gltfNormalAccessor.bufferView = (int)gltfModel.bufferViews.size() + 1;
			gltfNormalAccessor.byteOffset = sizeof(glm::vec3);
			gltfNormalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			gltfNormalAccessor.count = nv;
			gltfNormalAccessor.type = TINYGLTF_TYPE_VEC3;

			tinygltf::Accessor gltfColorAccessor;
			if (withTexCoords) {
				gltfColorAccessor.bufferView = (int)gltfModel.bufferViews.size() + 1;
				gltfColorAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				gltfColorAccessor.count = nv;
				gltfColorAccessor.byteOffset = (exportNormals ? 2 : 1) * sizeof(glm::vec3);
				gltfColorAccessor.type = TINYGLTF_TYPE_VEC2;
			} else if (withColor) {
				gltfColorAccessor.bufferView = (int)gltfModel.bufferViews.size() + 1;
				gltfColorAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				gltfColorAccessor.count = nv;
				gltfColorAccessor.byteOffset = (exportNormals ? 2 : 1) * sizeof(glm::vec3);
				gltfColorAccessor.type = TINYGLTF_TYPE_VEC4;
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
				gltfMeshPrimitive.material = materialId;
				gltfMeshPrimitive.mode = TINYGLTF_MODE_TRIANGLES;
				gltfMesh.primitives.emplace_back(core::move(gltfMeshPrimitive));
			}

			{
				tinygltf::Node gltfNode;
				gltfNode.mesh = (int)gltfModel.meshes.size();
				saveGltfNode(nodeMapping, gltfModel, gltfNode, gltfScene, node, stack, sceneGraph, scale,
							 exportAnimations);
			}

			{
				// indices and vertices
				tinygltf::Buffer gltfBuffer;
				gltfBuffer.data.insert(gltfBuffer.data.end(), os.getBuffer(), os.getBuffer() + os.size());
				gltfModel.buffers.emplace_back(core::move(gltfBuffer));
			}

			gltfModel.meshes.emplace_back(core::move(gltfMesh));
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
		tinygltf::Camera gltfCamera = _priv::processCamera(toCameraNode(*iter));
		if (gltfCamera.type.empty()) {
			continue;
		}
		gltfModel.cameras.push_back(gltfCamera);
	}

	io::StdOStreamBuf buf(stream);
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
	transform.setLocalScale(transform.localScale() / getScale());
	return transform;
}

bool GLTFFormat::loadIndices(const tinygltf::Model &gltfModel, const tinygltf::Primitive &gltfPrimitive,
							 core::DynamicArray<uint32_t> &indices, size_t indicesOffset) const {
	if (gltfPrimitive.mode != TINYGLTF_MODE_TRIANGLES) {
		Log::warn("Unexpected primitive mode: %i", gltfPrimitive.mode);
		return false;
	}
	const tinygltf::Accessor *accessor = getAccessor(gltfModel, gltfPrimitive.indices);
	if (accessor == nullptr) {
		Log::warn("Could not get accessor for indices");
		return false;
	}
	const size_t size = accessorSize(*accessor);
	const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[accessor->bufferView];
	const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];
	const size_t stride = gltfBufferView.byteStride ? gltfBufferView.byteStride : size;

	const size_t offset = accessor->byteOffset + gltfBufferView.byteOffset;
	const uint8_t *indexBuf = gltfBuffer.data.data() + offset;

	Log::debug("indicesOffset: %i", (int)indicesOffset);

	switch (accessor->componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		_priv::copyGltfIndices<uint8_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		_priv::copyGltfIndices<int8_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		_priv::copyGltfIndices<uint16_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		_priv::copyGltfIndices<int16_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		_priv::copyGltfIndices<uint32_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		_priv::copyGltfIndices<int32_t>(indexBuf, accessor->count, stride, indices, indicesOffset);
		break;
	default:
		Log::error("Unknown component type for indices: %i", accessor->componentType);
		break;
	}
	return true;
}

bool GLTFFormat::loadTextures(const core::String &filename, core::StringMap<image::ImagePtr> &textures,
							  const tinygltf::Model &gltfModel, const tinygltf::Primitive &gltfPrimitive,
							  GltfTextureData &textureData) const {
	Log::debug("Primitive material: %i", gltfPrimitive.material);
	Log::debug("Primitive mode: %i", gltfPrimitive.mode);
	int texCoordIndex = 0;
	if (gltfPrimitive.material >= 0 && gltfPrimitive.material < (int)gltfModel.materials.size()) {
		const tinygltf::Material *gltfMaterial = &gltfModel.materials[gltfPrimitive.material];
		// TODO: load emissiveTexture
		const tinygltf::TextureInfo &gltfTextureInfo = gltfMaterial->pbrMetallicRoughness.baseColorTexture;
		const int textureIndex = gltfTextureInfo.index;
		if (textureIndex != -1 && textureIndex < (int)gltfModel.textures.size()) {
			const tinygltf::Texture &gltfTexture = gltfModel.textures[textureIndex];
			if (gltfTexture.source >= 0 && gltfTexture.source < (int)gltfModel.images.size()) {
				if (gltfTexture.sampler >= 0 && gltfTexture.sampler < (int)gltfModel.samplers.size()) {
					const tinygltf::Sampler &gltfTextureSampler = gltfModel.samplers[gltfTexture.sampler];
					Log::debug("Sampler: %s, wrapS: %i, wrapT: %i", gltfTextureSampler.name.c_str(),
							   gltfTextureSampler.wrapS, gltfTextureSampler.wrapT);
					textureData.wrapS = _priv::convertTextureWrap(gltfTextureSampler.wrapS);
					textureData.wrapT = _priv::convertTextureWrap(gltfTextureSampler.wrapT);
				}
				const tinygltf::Image &gltfImage = gltfModel.images[gltfTexture.source];
				Log::debug("Image components: %i, width: %i, height: %i, bits: %i", gltfImage.component,
						   gltfImage.width, gltfImage.height, gltfImage.bits);
				if (gltfImage.uri.empty()) {
					if (gltfImage.bufferView >= 0 && gltfImage.bufferView < (int)gltfModel.bufferViews.size()) {
						const tinygltf::BufferView &gltfImgBufferView = gltfModel.bufferViews[gltfImage.bufferView];
						if (gltfImgBufferView.buffer >= 0 && gltfImgBufferView.buffer < (int)gltfModel.buffers.size()) {
							const tinygltf::Buffer &gltfImgBuffer = gltfModel.buffers[gltfImgBufferView.buffer];
							const size_t offset = gltfImgBufferView.byteOffset;
							const uint8_t *buf = gltfImgBuffer.data.data() + offset;
							image::ImagePtr tex = image::createEmptyImage(gltfImage.name.c_str());
							if (!tex->load(buf, (int)gltfImgBufferView.byteLength)) {
								Log::warn("Failed to load embedded image %s", gltfImage.name.c_str());
							} else {
								textureData.diffuseTexture = gltfImage.name.c_str();
								textures.emplace(textureData.diffuseTexture, core::move(tex));
							}
						} else {
							Log::warn("Invalid buffer index for image: %i", gltfImgBufferView.buffer);
						}
					} else if (!gltfImage.image.empty()) {
						if (gltfImage.component == 4) {
							core::String name = gltfImage.name.c_str();
							if (name.empty()) {
								name = core::string::format("image%i", gltfTexture.source);
							}
							image::ImagePtr tex = image::createEmptyImage(name);
							core_assert(gltfImage.image.size() ==
										(size_t)(gltfImage.width * gltfImage.height * gltfImage.component));
							tex->loadRGBA(gltfImage.image.data(), gltfImage.width, gltfImage.height);
							Log::debug("Use image %s", name.c_str());
							textureData.diffuseTexture = name.c_str();
							textures.emplace(textureData.diffuseTexture, core::move(tex));
							texCoordIndex = gltfTextureInfo.texCoord;
						} else {
							Log::warn("Failed to load image with %i components", gltfImage.component);
						}
					} else {
						Log::warn("Invalid buffer view index for image: %i", gltfImage.bufferView);
					}
				} else {
					core::String name = gltfImage.uri.c_str();
					if (!textures.hasKey(name)) {
						name = lookupTexture(filename, name);
						image::ImagePtr tex = image::loadImage(name);
						if (tex->isLoaded()) {
							Log::debug("Use image %s", name.c_str());
							textureData.diffuseTexture = gltfImage.uri.c_str();
							textures.emplace(textureData.diffuseTexture, core::move(tex));
							texCoordIndex = gltfTextureInfo.texCoord;
						} else {
							Log::warn("Failed to load %s", name.c_str());
						}
					} else {
						textureData.diffuseTexture = name;
					}
				}
			} else {
				Log::debug("Invalid image index given %i", gltfTexture.source);
			}
		} else {
			Log::debug("Invalid texture index given %i", textureIndex);
		}
	}

	textureData.texCoordAttribute = core::string::format("TEXCOORD_%i", texCoordIndex);
	Log::debug("Texcoords: %s", textureData.texCoordAttribute.c_str());

	return true;
}

bool GLTFFormat::loadAttributes(const core::String &filename, core::StringMap<image::ImagePtr> &textures,
								const tinygltf::Model &gltfModel, const tinygltf::Primitive &gltfPrimitive,
								core::DynamicArray<GltfVertex> &vertices) const {
	GltfTextureData textureData;
	if (!loadTextures(filename, textures, gltfModel, gltfPrimitive, textureData)) {
		return false;
	}

	bool foundPosition = false;
	size_t verticesOffset = vertices.size();
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
			foundPosition = true;
			core_assert(gltfAttributeAccessor->type == TINYGLTF_TYPE_VEC3);
			for (size_t i = 0; i < gltfAttributeAccessor->count; i++) {
				const float *posData = (const float *)buf;
				vertices[verticesOffset + i].pos = glm::vec3(posData[0], posData[1], posData[2]);
				vertices[verticesOffset + i].texture = textureData.diffuseTexture;
				buf += stride;
			}
		} else if (attrType == textureData.texCoordAttribute.c_str()) {
			if (gltfAttributeAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				Log::debug("Skip non float type (%i) for %s", gltfAttributeAccessor->componentType, attrType.c_str());
				continue;
			}
			core_assert(gltfAttributeAccessor->type == TINYGLTF_TYPE_VEC2);
			for (size_t i = 0; i < gltfAttributeAccessor->count; i++) {
				const float *uvData = (const float *)buf;
				vertices[verticesOffset + i].uv = glm::vec2(uvData[0], uvData[1]);
				vertices[verticesOffset + i].wrapS = textureData.wrapS;
				vertices[verticesOffset + i].wrapT = textureData.wrapT;
				buf += stride;
			}
		} else if (core::string::startsWith(attrType.c_str(), "COLOR")) {
			for (size_t i = 0; i < gltfAttributeAccessor->count; i++) {
				vertices[verticesOffset + i].color = _priv::toColor(gltfAttributeAccessor, buf);
				buf += stride;
			}
		} else {
			Log::debug("Skip unhandled attribute %s", attrType.c_str());
		}
	}
	return foundPosition;
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
		// } else if (sampler.interpolation == "CUBICSPLINE") {
		// TODO: implement easing for this type
		// interpolation = InterpolationType::Linear;
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
		const uint8_t *buf = gltfBuffer.data.data() + offset;
		for (size_t i = 0; i < gltfFrameTimeAccessor->count; ++i) {
			const float seconds = *(const float *)buf;
			if (node.addKeyFrame((scenegraph::FrameIndex)(seconds * _priv::FPS)) == InvalidKeyFrame) {
				Log::debug("Failed to add keyframe for %f seconds (%i) for node %s", seconds,
						   (int)gltfFrameTimeAccessor->count, node.name().c_str());
			} else {
				Log::debug("Added keyframe for %f seconds (%i) for node %s", seconds, (int)gltfFrameTimeAccessor->count,
						   node.name().c_str());
			}
			buf += stride;
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
			const float *buf = (const float *)transformBuf;
			transformBuf += stride;
			scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
			keyFrame.interpolation = interpolation;
			scenegraph::SceneGraphTransform &transform = keyFrame.transform();
			if (gltfAnimChannel.target_path == "translation") {
				core_assert(gltfTransformAccessor->type == TINYGLTF_TYPE_VEC3);
				glm::vec3 v(buf[0], buf[1], buf[2]);
				transform.setLocalTranslation(v);
			} else if (gltfAnimChannel.target_path == "rotation") {
				core_assert(gltfTransformAccessor->type == TINYGLTF_TYPE_VEC4);
				glm::quat orientation(buf[3], buf[0], buf[1], buf[2]);
				transform.setLocalOrientation(orientation);
			} else if (gltfAnimChannel.target_path == "scale") {
				core_assert(gltfTransformAccessor->type == TINYGLTF_TYPE_VEC3);
				glm::vec3 v(buf[0], buf[1], buf[2]);
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
			animationName = core::string::format("animation %i", (int)animIdx);
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
							core::StringMap<image::ImagePtr> &textures, const tinygltf::Model &gltfModel,
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
				loadNode_r(filename, sceneGraph, textures, gltfModel, childId, parentNodeId);
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
			loadNode_r(filename, sceneGraph, textures, gltfModel, childId, cameraId);
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
			loadNode_r(filename, sceneGraph, textures, gltfModel, childId, groupId);
		}
		return true;
	}

	Log::debug("Mesh node %i", gltfNodeIdx);
	const tinygltf::Mesh &gltfMesh = gltfModel.meshes[gltfNode.mesh];
	// TODO: directly fill the tris, don't create the vertices first - would save a lot of memory
	core::DynamicArray<uint32_t> indices;
	core::DynamicArray<GltfVertex> vertices;
	Log::debug("Primitives: %i in mesh %i", (int)gltfMesh.primitives.size(), gltfNode.mesh);
	for (const tinygltf::Primitive &primitive : gltfMesh.primitives) {
		const size_t indicesStart = vertices.size();
		if (!loadAttributes(filename, textures, gltfModel, primitive, vertices)) {
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
		} else if (!loadIndices(gltfModel, primitive, indices, indicesStart)) {
			Log::warn("Failed to load indices");
			return false;
		}
	}
	if (indices.empty() || vertices.empty()) {
		Log::error("No indices (%i) or vertices (%i) found for mesh %i", (int)indices.size(), (int)vertices.size(),
				   gltfNode.mesh);
		for (int childId : gltfNode.children) {
			loadNode_r(filename, sceneGraph, textures, gltfModel, childId, parentNodeId);
		}
		return false;
	}
	Log::debug("Indices (%i) or vertices (%i) found for mesh %i", (int)indices.size(), (int)vertices.size(),
			   gltfNode.mesh);

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

	const int nodeId = voxelizeNode(gltfNode.name.c_str(), sceneGraph, tris, parentNodeId, false);
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
		loadNode_r(filename, sceneGraph, textures, gltfModel, childId, nodeId);
	}
	return true;
}

bool GLTFFormat::voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream,
								scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	uint32_t magic;
	stream.peekUInt32(magic);
	const int64_t size = stream.size();
	uint8_t *data = (uint8_t *)core_malloc(size);
	if (stream.read(data, size) == -1) {
		Log::error("Failed to read gltf stream for %s of size %i", filename.c_str(), (int)size);
		core_free(data);
		return false;
	}

	std::string err;
	bool state;

	const core::String filePath = core::string::extractPath(filename);
	tinygltf::TinyGLTF gltfLoader;
	tinygltf::Model gltfModel;
	if (magic == FourCC('g', 'l', 'T', 'F')) {
		Log::debug("Detected binary gltf stream");
		state = gltfLoader.LoadBinaryFromMemory(&gltfModel, &err, nullptr, data, size, filePath.c_str(),
												tinygltf::SectionCheck::NO_REQUIRE);
		if (!state) {
			Log::error("Failed to load binary gltf file: %s", err.c_str());
		}
	} else {
		Log::debug("Detected ascii gltf stream");
		state = gltfLoader.LoadASCIIFromString(&gltfModel, &err, nullptr, (const char *)data, size, filePath.c_str(),
											   tinygltf::SectionCheck::NO_REQUIRE);
		if (!state) {
			Log::error("Failed to load ascii gltf file: %s", err.c_str());
		}
	}
	core_free(data);
	if (!state) {
		return false;
	}

	core::StringMap<image::ImagePtr> textures;

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

	scenegraph::SceneGraphNode &root = sceneGraph.node(parentNodeId);
	if (!gltfModel.asset.generator.empty()) {
		root.setProperty("Generator", gltfModel.asset.generator.c_str());
	}
	if (!gltfModel.asset.copyright.empty()) {
		root.setProperty("Copyright", gltfModel.asset.copyright.c_str());
	}
	if (!gltfModel.asset.version.empty()) {
		root.setProperty("Version", gltfModel.asset.version.c_str());
	}

	for (const tinygltf::Scene &gltfScene : gltfModel.scenes) {
		Log::debug("Found %i nodes in scene %s", (int)gltfScene.nodes.size(), gltfScene.name.c_str());
		for (int gltfNodeIdx : gltfScene.nodes) {
			loadNode_r(filename, sceneGraph, textures, gltfModel, gltfNodeIdx, parentNodeId);
		}
	}
	return true;
}

} // namespace voxelformat
