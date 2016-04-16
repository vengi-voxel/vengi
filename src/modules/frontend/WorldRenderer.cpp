#include "WorldRenderer.h"
#include "video/GLFunc.h"
#include "video/Color.h"
#include "voxel/Spiral.h"
#include "core/App.h"
#include "noise/SimplexNoise.h"
#include "cubiquity/PolyVox/CubicSurfaceExtractor.h"
#include "cubiquity/PolyVox/RawVolume.h"
#include "cubiquity/CubiquityC.h"
#include <SDL.h>

constexpr int MinCullingDistance = 500;

namespace frontend {

WorldRenderer::WorldRenderer(const voxel::WorldPtr& world) :
		_world(world) {
}

WorldRenderer::~WorldRenderer() {
	if (_volumeHandle != 0) {
		// Delete the volume from memory (doesn't delete from disk).
		const int rc = cuDeleteVolume(_volumeHandle);
		if (rc != CU_OK) {
			Log::error("%s : %s", cuGetErrorCodeAsString(rc), cuGetLastErrorMessage());
		}
	}
	delete _rootOpenGLOctreeNode;
}

void WorldRenderer::reset() {
	for (const video::GLMeshData& meshData : _meshData) {
		glDeleteBuffers(1, &meshData.vertexBuffer);
		glDeleteBuffers(1, &meshData.indexBuffer);
		glDeleteVertexArrays(1, &meshData.vertexArrayObject);
	}
	_meshData.clear();
	_entities.clear();
	_fogRange = 0.0f;
	_viewDistance = 1.0f;
	_now = 0l;
}

void WorldRenderer::onCleanup() {
	reset();
}

ClientEntityPtr WorldRenderer::getEntity(ClientEntityId id) {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		return ClientEntityPtr();
	}
	return i->second;
}

bool WorldRenderer::addEntity(const ClientEntityPtr& entity) {
	auto i = _entities.find(entity->id());
	if (i != _entities.end()) {
		return false;
	}
	_entities[entity->id()] = entity;
	return true;
}

void WorldRenderer::deleteMesh(const glm::ivec3& pos) {
	const glm::ivec3& p = _world->getGridPos(pos);
	for (auto i = _meshData.begin(); i != _meshData.end(); ++i) {
		const video::GLMeshData& meshData = *i;
		if (meshData.translation != p) {
			continue;
		}
		_meshData.erase(i);
		glDeleteBuffers(1, &meshData.vertexBuffer);
		glDeleteBuffers(1, &meshData.indexBuffer);
		glDeleteVertexArrays(1, &meshData.vertexArrayObject);
		return;
	}
}

bool WorldRenderer::removeEntity(ClientEntityId id) {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		return false;
	}
	_entities.erase(i);
	return true;
}

void WorldRenderer::handleMeshQueue(video::Shader& shader) {
	voxel::DecodedMeshData mesh;
	if (!_world->pop(mesh)) {
		return;
	}
	for (video::GLMeshData& m : _meshData) {
		if (m.translation == mesh.translation) {
			updateMesh(mesh.mesh, m);
			return;
		}
	}
	// Now add the mesh to the list of meshes to render.
	_meshData.push_back(createMesh(shader, mesh.mesh, mesh.translation, 1.0f));
}

int WorldRenderer::renderWorld(video::Shader& shader, const video::Camera& camera, const glm::mat4& projection) {
	handleMeshQueue(shader);

	int drawCallsWorld = 0;

	// TODO: use polyvox VolumeResampler to create a minimap of your volume
	// RawVolume<uint8_t> volDataLowLOD(PolyVox::Region(Vector3DInt32(0, 0, 0), Vector3DInt32(15, 31, 31)));
	// VolumeResampler< RawVolume<uint8_t>, RawVolume<uint8_t> > volumeResampler(&volData, PolyVox::Region(Vector3DInt32(0, 0, 0), Vector3DInt32(31, 63, 63)), &volDataLowLOD, volDataLowLOD.getEnclosingRegion());
	// volumeResampler.execute();
	// auto meshLowLOD = extractMarchingCubesMesh(&volDataLowLOD, volDataLowLOD.getEnclosingRegion());
	// auto decodedMeshLowLOD = decodeMesh(meshLowLOD);

	glClear(GL_DEPTH_BUFFER_BIT);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles whose normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GL_checkError();

	const glm::mat4& view = camera.getViewMatrix();

	static const glm::vec4 materialColors[] = {
		video::Color::LightBlue,	// air
		video::Color::Lime,			// grass
		video::Color::Brown,		// wood
		// leaves
		video::Color::DarkGreen,
		video::Color::Green,
		video::Color::Purple,
		video::Color::Cyan,
		video::Color::Olive,
		video::Color::Orange,
		video::Color::Red,
		video::Color::Yellow,
		video::Color::LightRed,
		video::Color::Blue,
		// leaves end
		video::Color::DarkGray,		// rock
		video::Color::White,		// clouds
		video::Color::Blue,			// water
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink
	};
	static_assert(SDL_arraysize(materialColors) == 32, "amount of colors doesn't match shader uniform");

	shader.activate();
	shader.setUniformMatrix("u_view", view, false);
	shader.setUniformMatrix("u_projection", projection, false);
	shader.setUniformf("u_fogrange", _fogRange);
	shader.setUniformf("u_viewdistance", _viewDistance);
	shader.setUniformi("u_texture", 0);
	shader.setUniformVec3("u_lightpos", _lightPos);
	shader.setUniformVec4v("u_materialcolor[0]", materialColors, SDL_arraysize(materialColors));
	_colorTexture->bind();
	const float chunkSize = (float)_world->getChunkSize();
	const glm::vec3 bboxSize(chunkSize, chunkSize, chunkSize);
	for (auto i = _meshData.begin(); i != _meshData.end();) {
		const video::GLMeshData& meshData = *i;
		if (isDistanceCulled(meshData.translation, true)) {
			_world->allowReExtraction(meshData.translation);
			glDeleteBuffers(1, &meshData.vertexBuffer);
			glDeleteBuffers(1, &meshData.indexBuffer);
			glDeleteVertexArrays(1, &meshData.vertexArrayObject);
			i = _meshData.erase(i);
			continue;
		}
		const glm::vec3 mins(meshData.translation);
		const glm::vec3 maxs = glm::vec3(meshData.translation) + bboxSize;
		if (camera.testFrustum(mins, maxs) == video::FrustumResult::Outside) {
			++i;
			continue;
		}
		const glm::mat4& model = glm::translate(glm::mat4(1.0f), glm::vec3(meshData.translation));
		shader.setUniformMatrix("u_model", model, false);
		glBindVertexArray(meshData.vertexArrayObject);
		glDrawElements(GL_TRIANGLES, meshData.noOfIndices, meshData.indexType, 0);
		GL_checkError();
		++drawCallsWorld;
		++i;
	}
	shader.deactivate();
	GL_checkError();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);

	GL_checkError();
	return drawCallsWorld;
}

void WorldRenderer::updateMesh(voxel::DecodedMesh& surfaceMesh, video::GLMeshData& meshData) {
	const uint32_t* vecIndices = surfaceMesh.getRawIndexData();
	const uint32_t numIndices = surfaceMesh.getNoOfIndices();
	const voxel::VoxelVertexDecoded* vecVertices = surfaceMesh.getRawVertexData();
	const uint32_t numVertices = surfaceMesh.getNoOfVertices();

	core_assert(meshData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::VoxelVertexDecoded), vecVertices, GL_STATIC_DRAW);

	core_assert(meshData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(typename voxel::DecodedMesh::IndexType), vecIndices, GL_STATIC_DRAW);

	meshData.noOfIndices = numIndices;
}

// TODO: generate bigger buffers and use glBufferSubData
video::GLMeshData WorldRenderer::createMesh(video::Shader& shader, voxel::DecodedMesh& surfaceMesh, const glm::ivec3& translation, float scale) {
	const uint32_t numIndices = surfaceMesh.getNoOfIndices();
	const uint32_t numVertices = surfaceMesh.getNoOfVertices();

	// This struct holds the OpenGL properties (buffer handles, etc) which will be used
	// to render our mesh. We copy the data from the PolyVox mesh into this structure.
	video::GLMeshData meshData;

	// Create the VAO for the mesh
	glGenVertexArrays(1, &meshData.vertexArrayObject);
	core_assert(meshData.vertexArrayObject > 0);
	glBindVertexArray(meshData.vertexArrayObject);

	// The GL_ARRAY_BUFFER will contain the list of vertex positions
	glGenBuffers(1, &meshData.vertexBuffer);

	// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
	glGenBuffers(1, &meshData.indexBuffer);

	updateMesh(surfaceMesh, meshData);

	const int posLoc = shader.enableVertexAttribute("a_pos");
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(voxel::VoxelVertexDecoded),
			GL_OFFSET_CAST(offsetof(voxel::VoxelVertexDecoded, position)));

	const int matLoc = shader.enableVertexAttribute("a_materialdensity");
	// our material and density is encoded as 8 bits material and 8 bits density
	core_assert(sizeof(voxel::Voxel) == sizeof(uint16_t));
	glVertexAttribIPointer(matLoc, sizeof(voxel::Voxel), GL_UNSIGNED_BYTE, sizeof(voxel::VoxelVertexDecoded),
			GL_OFFSET_CAST(offsetof(voxel::VoxelVertexDecoded, data)));

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Log::trace("mesh information:\n- mesh indices: %i, vertices: %i\n- position: %i:%i:%i", numIndices, numVertices, translation.x,
			translation.y, translation.z);

	meshData.translation = translation;
	meshData.scale = scale;
	meshData.indexType = GL_UNSIGNED_INT;
	return meshData;
}

void WorldRenderer::onSpawn(const glm::vec3& pos, int initialExtractionRadius) {
	_viewDistance = 1.0f;
	_lastCameraPosition = _world->getGridPos(pos);
	extractMeshAroundCamera(initialExtractionRadius);
}

int WorldRenderer::renderEntities(const video::ShaderPtr& shader, const video::Camera& camera, const glm::mat4& projection) {
	if (_entities.empty())
		return 0;

	int drawCallsEntities = 0;

	const glm::mat4& view = camera.getViewMatrix();

	shader->activate();
	shader->setUniformMatrix("u_view", view, false);
	shader->setUniformMatrix("u_projection", projection, false);
	shader->setUniformVec3("u_lightpos", _lightPos);
	shader->setUniformf("u_fogrange", _fogRange);
	shader->setUniformf("u_viewdistance", _viewDistance);
	shader->setUniformi("u_texture", 0);
	for (const auto& e : _entities) {
		const frontend::ClientEntityPtr& ent = e.second;
		ent->update(_now);
		if (camera.testFrustum(ent->position()) == video::FrustumResult::Outside) {
			continue;
		}
		const video::MeshPtr& mesh = ent->mesh();
		if (!mesh->initMesh(shader))
			continue;
		const glm::mat4& translate = glm::translate(glm::mat4(1.0f), ent->position());
		const glm::mat4& scale = glm::scale(translate, glm::vec3(0.01f));
		const glm::mat4& model = glm::rotate(scale, ent->orientation(), glm::vec3(0.0, 1.0, 0.0));
		shader->setUniformMatrix("u_model", model, false);
		mesh->render();
		GL_checkError();
		++drawCallsEntities;
	}
	shader->deactivate();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();
	return drawCallsEntities;
}

void WorldRenderer::extractNewMeshes(const glm::vec3& position, bool force) {
	if (force) {
		deleteMesh(position);
		_world->allowReExtraction(position);
		_world->scheduleMeshExtraction(position);
		return;
	}
	const glm::ivec3& camXYZ = _world->getGridPos(position);
	const glm::vec3 diff = _lastCameraPosition - camXYZ;
	if (glm::length(diff.x) >= 1 || glm::length(diff.y) >= 1 || glm::length(diff.z) >= 1) {
		_lastCameraPosition = camXYZ;
		extractMeshAroundCamera(1);
	}
}

void WorldRenderer::extractMeshAroundCamera(int radius) {
	const int sideLength = radius * 2 + 1;
	const int amount = sideLength * (sideLength - 1) + sideLength;
	const int chunkSize = _world->getChunkSize();
	const glm::ivec3& cameraPos = _lastCameraPosition;
	const int maxChunks = MAX_HEIGHT / chunkSize;
	glm::ivec3 pos = cameraPos;
	voxel::Spiral o;
	for (int i = 0; i < amount; ++i) {
		if (!isDistanceCulled(pos, false)) {
			glm::ivec3 regionPos = pos;
			regionPos.y = 0;
			for (int i = 0; i < maxChunks; ++i) {
				_world->scheduleMeshExtraction(regionPos);
				regionPos.y = chunkSize;
			}
		}
		o.next();
		pos.x = cameraPos.x + o.x() * chunkSize;
		pos.z = cameraPos.z + o.y() * chunkSize;
	}
}

void WorldRenderer::onInit() {
	_noiseFuture.push_back(core::App::getInstance()->threadPool().enqueue([] () {
		const int ColorTextureSize = 256;
		const int ColorTextureDepth = 3;
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		noise::Simplex::SeamlessNoise2DRGB(colorTexture, ColorTextureSize, ColorTextureDepth, 0.3f, 0.7f);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, ColorTextureDepth);
	}));
	_colorTexture = video::createTexture("**colortexture**");
}

void WorldRenderer::onRunning(long dt) {
	_now += dt;
	if (!_noiseFuture.empty()) {
		NoiseFuture& future = _noiseFuture.back();
		if (future.valid()) {
			NoiseGenerationTask c = future.get();
			Log::info("Noise texture ready - upload it");
			_colorTexture->upload(c.buffer, c.width, c.height, c.depth);
			_colorTexture->unbind();
			delete[] c.buffer;
			_noiseFuture.erase(_noiseFuture.end());
		}
	}

	if (_viewDistance < MinCullingDistance) {
		const float advance = _world->getChunkSize() * (dt / 1000.0f) * 0.1f;
		_viewDistance += advance;
	}
}

bool WorldRenderer::isDistanceCulled(const glm::ivec3& pos, bool queryForRendering) const {
	const glm::ivec3 dist = pos - _lastCameraPosition;
	const int distance = glm::sqrt(dist.x * dist.x + dist.z * dist.z);
	const float cullingThreshold = _world->getChunkSize() * 3;
	const int maxAllowedDistance = _viewDistance + cullingThreshold;
	if ((!queryForRendering && distance > MinCullingDistance) && distance >= maxAllowedDistance) {
		return true;
	}
	return false;
}

void WorldRenderer::renderOctree(video::Shader& shader, const glm::mat4& view, float aspect) {
	// The framework we're using here doesn't seem to provide easy access to the camera position. The following lines compute it.
	glm::vec4 eyeSpaceEyePos(0.0, 0.0, 0.0, 1.0);
	glm::mat4 inverseViewMatrix = glm::inverse(view);
	glm::vec4 worldSpaceEyePos = inverseViewMatrix * eyeSpaceEyePos;
	worldSpaceEyePos /= worldSpaceEyePos.w;

	uint32_t isUpToDate;
	int rc = cuUpdateVolume(_volumeHandle, worldSpaceEyePos[0], worldSpaceEyePos[1], worldSpaceEyePos[2], 1.0f, &isUpToDate);
	if (rc != CU_OK) {
		Log::info("%s - %s", cuGetErrorCodeAsString(rc), cuGetLastErrorMessage());
	}

	uint32_t hasRootNode;
	rc = cuHasRootOctreeNode(_volumeHandle, &hasRootNode);
	if (rc != CU_OK) {
		Log::info("%s - %s", cuGetErrorCodeAsString(rc), cuGetLastErrorMessage());
	}
	if (hasRootNode == 1) {
		// FIXME - Maybe it's easier if there is always a root node?
		if (!_rootOpenGLOctreeNode) {
			_rootOpenGLOctreeNode = new OpenGLOctreeNode(0);
		}

		uint32_t octreeNodeHandle;
		cuGetRootOctreeNode(_volumeHandle, &octreeNodeHandle);
		processOctreeNodeStructure(shader, octreeNodeHandle, _rootOpenGLOctreeNode);
	} else {
		if (_rootOpenGLOctreeNode) {
			delete _rootOpenGLOctreeNode;
			_rootOpenGLOctreeNode = nullptr;
		}
	}

	glClear(GL_DEPTH_BUFFER_BIT);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles whose normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GL_checkError();

	const glm::mat4& projection = glm::perspective(45.0f, aspect, 0.1f, 1000.0f);

	// Use our shader
	shader.activate();
	shader.setUniformMatrix("viewMatrix", view, false);
	shader.setUniformMatrix("projectionMatrix", projection, false);
	_colorTexture->bind();

	if (_rootOpenGLOctreeNode) {
		renderOpenGLOctreeNode(shader, _rootOpenGLOctreeNode);
	}

	shader.deactivate();
	GL_checkError();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);

	GL_checkError();
}

void WorldRenderer::processOctreeNodeStructure(video::Shader& shader, uint32_t octreeNodeHandle, frontend::WorldRenderer::OpenGLOctreeNode* openGLOctreeNode) {
	CuOctreeNode octreeNode;
	int rc = cuGetOctreeNode(octreeNodeHandle, &octreeNode);
	if (rc != CU_OK) {
		Log::info("%s - %s", cuGetErrorCodeAsString(rc), cuGetLastErrorMessage());
	}

	if (octreeNode.nodeOrChildrenLastChanged > openGLOctreeNode->nodeAndChildrenLastSynced) {
		if (octreeNode.propertiesLastChanged > openGLOctreeNode->propertiesLastSynced) {
			Log::debug("Resynced properties at %i", openGLOctreeNode->propertiesLastSynced);
			openGLOctreeNode->height = octreeNode.height;
			openGLOctreeNode->renderThisNode = octreeNode.renderThisNode;
			cuGetCurrentTime(&(openGLOctreeNode->propertiesLastSynced));
		}

		if (octreeNode.meshLastChanged > openGLOctreeNode->meshLastSynced) {
			if (octreeNode.hasMesh == 1) {
				// These will point to the index and vertex data
				uint32_t noOfIndices;
				uint16_t* indices;
				uint16_t noOfVertices;
				void* vertices;

				// Get the index and vertex data
				rc = cuGetMesh(octreeNodeHandle, &noOfVertices, &vertices, &noOfIndices, &indices);
				if (rc != CU_OK) {
					Log::info("%s - %s", cuGetErrorCodeAsString(rc), cuGetLastErrorMessage());
				}

				uint32_t volumeType;
				rc = cuGetVolumeType(octreeNodeHandle, &volumeType);
				if (rc != CU_OK) {
					Log::info("%s - %s", cuGetErrorCodeAsString(rc), cuGetLastErrorMessage());
				}

				// Pass it to the OpenGL node.
				openGLOctreeNode->posX = octreeNode.posX;
				openGLOctreeNode->posY = octreeNode.posY;
				openGLOctreeNode->posZ = octreeNode.posZ;

				openGLOctreeNode->noOfIndices = noOfIndices;

				glBindVertexArray(openGLOctreeNode->vertexArrayObject);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openGLOctreeNode->indexBuffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * noOfIndices, indices, GL_STATIC_DRAW);

				glBindBuffer(GL_ARRAY_BUFFER, openGLOctreeNode->vertexBuffer);

				if (volumeType == CU_COLORED_CUBES) {
					glBufferData(GL_ARRAY_BUFFER, sizeof(CuColoredCubesVertex) * noOfVertices, vertices, GL_STATIC_DRAW);

					// We pack the encoded position and the encoded normal into a single
					// vertex attribute to save space: http://stackoverflow.com/a/21680009
					const int posLoc = shader.enableVertexAttribute("encodedPosition");
					glVertexAttribIPointer(posLoc, 4, GL_UNSIGNED_BYTE, sizeof(CuColoredCubesVertex),
							(GLvoid*)(offsetof(CuColoredCubesVertex, encodedPosX)));

					const int colorLoc = shader.enableVertexAttribute("quantizedColor");
					glVertexAttribIPointer(colorLoc, 1, GL_UNSIGNED_INT, sizeof(CuColoredCubesVertex),
							(GLvoid*)(offsetof(CuColoredCubesVertex, data)));
				} else if (volumeType == CU_TERRAIN) {
					glBufferData(GL_ARRAY_BUFFER, sizeof(CuTerrainVertex) * noOfVertices, vertices, GL_STATIC_DRAW);

					// We pack the encoded position and the encoded normal into a single
					// vertex attribute to save space: http://stackoverflow.com/a/21680009
					const int posLoc = shader.enableVertexAttribute("encodedPositionAndNormal");
					glVertexAttribIPointer(posLoc, 4, GL_UNSIGNED_SHORT, sizeof(CuTerrainVertex),
							GL_OFFSET_CAST(offsetof(CuTerrainVertex, encodedPosX)));

					const int materialLoc = shader.enableVertexAttribute("materialWeightsAsUBytes");
					glVertexAttribIPointer(materialLoc, 4, GL_UNSIGNED_BYTE, sizeof(CuTerrainVertex),
							GL_OFFSET_CAST(offsetof(CuTerrainVertex, material0)));
				}

				glBindVertexArray(0);
			} else {
				core_assert(openGLOctreeNode->noOfIndices == 0);
			}

			cuGetCurrentTime(&(openGLOctreeNode->meshLastSynced));
			Log::debug("Resynced mesh at %i", openGLOctreeNode->meshLastSynced);
		}

		if (octreeNode.structureLastChanged > openGLOctreeNode->structureLastSynced) {
			for (uint32_t z = 0; z < 2; z++) {
				for (uint32_t y = 0; y < 2; y++) {
					for (uint32_t x = 0; x < 2; x++) {
						// TODO: use a pool for these allocations
						if (octreeNode.childHandles[x][y][z] != CU_UNKNOWN) {
							if (!openGLOctreeNode->children[x][y][z]) {
								openGLOctreeNode->children[x][y][z] = new OpenGLOctreeNode(openGLOctreeNode);
							}
						} else {
							if (openGLOctreeNode->children[x][y][z]) {
								delete openGLOctreeNode->children[x][y][z];
								openGLOctreeNode->children[x][y][z] = nullptr;
							}
						}
					}
				}
			}

			cuGetCurrentTime(&(openGLOctreeNode->structureLastSynced));
			Log::debug("Resynced structure at %i", openGLOctreeNode->structureLastSynced);
		}

		for (uint32_t z = 0; z < 2; z++) {
			for (uint32_t y = 0; y < 2; y++) {
				for (uint32_t x = 0; x < 2; x++) {
					if (octreeNode.childHandles[x][y][z] != CU_UNKNOWN) {
						// Recursively call the octree traversal
						processOctreeNodeStructure(shader, octreeNode.childHandles[x][y][z], openGLOctreeNode->children[x][y][z]);
					}
				}
			}
		}

		cuGetCurrentTime(&(openGLOctreeNode->nodeAndChildrenLastSynced));
	}
}

void WorldRenderer::renderOpenGLOctreeNode(video::Shader& shader, OpenGLOctreeNode* openGLOctreeNode) {
	if (openGLOctreeNode->noOfIndices > 0 && openGLOctreeNode->renderThisNode) {
		const glm::vec3 translate(openGLOctreeNode->posX, openGLOctreeNode->posY, openGLOctreeNode->posZ);
		const glm::mat4 model = glm::translate(glm::mat4(1.0f), translate);
		shader.setUniformMatrix("modelMatrix", model);
		if (shader.hasUniform("height")) {
			shader.setUniformui("height", openGLOctreeNode->height);
		}
		glBindVertexArray(openGLOctreeNode->vertexArrayObject);
		glDrawElements(GL_TRIANGLES, openGLOctreeNode->noOfIndices, GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);
	}

	for (uint32_t z = 0; z < 2; z++) {
		for (uint32_t y = 0; y < 2; y++) {
			for (uint32_t x = 0; x < 2; x++) {
				if (openGLOctreeNode->children[x][y][z]) {
					renderOpenGLOctreeNode(shader, openGLOctreeNode->children[x][y][z]);
				}
			}
		}
	}
}

WorldRenderer::OpenGLOctreeNode::OpenGLOctreeNode(OpenGLOctreeNode* parent) {
	noOfIndices = 0;
	indexBuffer = 0;
	vertexBuffer = 0;
	vertexArrayObject = 0;

	posX = 0;
	posY = 0;
	posZ = 0;

	structureLastSynced = 0;
	propertiesLastSynced = 0;
	meshLastSynced = 0;
	nodeAndChildrenLastSynced = 0;

	renderThisNode = false;

	height = 0;

	this->parent = parent;

	for (uint32_t z = 0; z < 2; z++) {
		for (uint32_t y = 0; y < 2; y++) {
			for (uint32_t x = 0; x < 2; x++) {
				children[x][y][z] = 0;
			}
		}
	}

	glGenVertexArrays(1, &vertexArrayObject);

	glGenBuffers(1, &indexBuffer);
	glGenBuffers(1, &vertexBuffer);
}

WorldRenderer::OpenGLOctreeNode::~OpenGLOctreeNode() {
	for (uint32_t z = 0; z < 2; z++) {
		for (uint32_t y = 0; y < 2; y++) {
			for (uint32_t x = 0; x < 2; x++) {
				delete children[x][y][z];
				children[x][y][z] = 0;
			}
		}
	}

	glDeleteBuffers(1, &indexBuffer);
	glDeleteBuffers(1, &vertexBuffer);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vertexArrayObject);
}

}
