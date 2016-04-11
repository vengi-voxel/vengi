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
	_viewDistance = 0.0f;
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

void WorldRenderer::deleteMesh(const glm::ivec2& pos) {
	const glm::ivec2& p = _world->getGridPos(pos);
	for (auto i = _meshData.begin(); i != _meshData.end(); ++i) {
		const video::GLMeshData& meshData = *i;
		if (meshData.translation.x != p.x || meshData.translation.y != p.y) {
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

int WorldRenderer::renderWorld(video::Shader& shader, const glm::mat4& view, float aspect) {
	int drawCallsWorld = 0;
	voxel::DecodedMeshData mesh;
	if (_world->pop(mesh)) {
		// Now add the mesh to the list of meshes to render.
		_meshData.push_back(createMesh(shader, mesh.mesh, mesh.translation, 1.0f));
	}

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

	const glm::mat4& projection = glm::perspective(45.0f, aspect, 0.1f, 1000.0f);

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
	for (auto i = _meshData.begin(); i != _meshData.end();) {
		const video::GLMeshData& meshData = *i;
		// TODO: proper culling - distance and frustum
		if (isDistanceCulled(meshData.translation, true)) {
			_world->allowReExtraction(meshData.translation);
			glDeleteBuffers(1, &meshData.vertexBuffer);
			glDeleteBuffers(1, &meshData.indexBuffer);
			glDeleteVertexArrays(1, &meshData.vertexArrayObject);
			i = _meshData.erase(i);
			continue;
		}
		const glm::mat4& model = glm::translate(glm::mat4(1.0f), glm::vec3(meshData.translation.x, 0, meshData.translation.y));
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

// TODO: generate bigger buffers and use glBufferSubData
video::GLMeshData WorldRenderer::createMesh(video::Shader& shader, voxel::DecodedMesh& surfaceMesh, const glm::ivec2& translation, float scale) {
	// Convenient access to the vertices and indices
	const uint32_t* vecIndices = surfaceMesh.getRawIndexData();
	const uint32_t numIndices = surfaceMesh.getNoOfIndices();
	const voxel::VoxelVertexDecoded* vecVertices = surfaceMesh.getRawVertexData();
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
	core_assert(meshData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::VoxelVertexDecoded), vecVertices, GL_STATIC_DRAW);

	// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
	glGenBuffers(1, &meshData.indexBuffer);
	core_assert(meshData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(typename voxel::DecodedMesh::IndexType), vecIndices, GL_STATIC_DRAW);

	const int posLoc = shader.enableVertexAttribute("a_pos");
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(voxel::VoxelVertexDecoded),
			(GLvoid*)(offsetof(voxel::VoxelVertexDecoded, position)));

	const int matLoc = shader.enableVertexAttribute("a_materialdensity");
	// our material and density is encoded as 8 bits material and 8 bits density
	core_assert(sizeof(voxel::Voxel) == sizeof(uint16_t));
	glVertexAttribIPointer(matLoc, sizeof(voxel::Voxel), GL_UNSIGNED_BYTE, sizeof(voxel::VoxelVertexDecoded),
			(GLvoid*)(offsetof(voxel::VoxelVertexDecoded, data)));

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Log::trace("mesh information:\n- mesh indices: %i, vertices: %i\n- position: %i:%i", numIndices, numVertices, translation.x,
			translation.y);

	meshData.noOfIndices = numIndices;
	meshData.translation = translation;
	meshData.scale = scale;
	meshData.indexType = GL_UNSIGNED_INT;
	return meshData;
}

void WorldRenderer::onSpawn(const glm::vec3& pos, int initialExtractionRadius) {
	_viewDistance = _world->getChunkSize() * (initialExtractionRadius + 1);
	_fogRange = _viewDistance;
	_lastCameraPosition = _world->getGridPos(pos);
	extractMeshAroundCamera(initialExtractionRadius);
}

int WorldRenderer::renderEntities(const video::ShaderPtr& shader, const glm::mat4& view, float aspect) {
	if (_entities.empty())
		return 0;

	int drawCallsEntities = 0;
	const glm::mat4& projection = glm::perspective(45.0f, aspect, 0.1f, 1000.0f);

	shader->activate();
	shader->setUniformMatrix("u_view", view, false);
	shader->setUniformMatrix("u_projection", projection, false);
	shader->setUniformVec3("u_lightpos", _lightPos);
	shader->setUniformf("u_fogrange", _fogRange);
	shader->setUniformf("u_viewdistance", _viewDistance);
	shader->setUniformi("u_texture", 0);
	// TODO: culling (not needed for server controlled entities - because the server is handling the vis)
	for (const auto& e : _entities) {
		const frontend::ClientEntityPtr& ent = e.second;
		ent->update(_now);
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
		const glm::ivec2& gridPos = _world->getGridPos(position);
		deleteMesh(gridPos);
		_world->allowReExtraction(gridPos);
		_world->scheduleMeshExtraction(gridPos);
		return;
	}
	const glm::ivec2& camXZ = _world->getGridPos(position);
	const glm::vec2 diff = _lastCameraPosition - camXZ;
	if (glm::length(diff.x) >= 1 || glm::length(diff.y) >= 1) {
		_lastCameraPosition = camXZ;
		extractMeshAroundCamera(1);
	}
}

void WorldRenderer::extractMeshAroundCamera(int radius) {
	const int sideLength = radius * 2 + 1;
	const int amount = sideLength * (sideLength - 1) + sideLength;
	const int size = _world->getChunkSize();
	const glm::ivec2& cameraPos = _lastCameraPosition;
	glm::ivec2 pos = cameraPos;
	voxel::Spiral o;
	for (int i = 0; i < amount; ++i) {
		if (!isDistanceCulled(pos, false)) {
			_world->scheduleMeshExtraction(pos);
		}
		o.next();
		pos.x = cameraPos.x + o.x() * size;
		pos.y = cameraPos.y + o.y() * size;
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

void WorldRenderer::onRunning(long now) {
	_now = now;
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

	// TODO: properly lerp this
	if (_viewDistance < MinCullingDistance) {
		const int advance = _world->getChunkSize() / 16;
		_viewDistance += advance;
		_fogRange += advance / 2;
	}
}

bool WorldRenderer::isDistanceCulled(const glm::ivec2& pos, bool queryForRendering) const {
	const glm::ivec2 dist = pos - _lastCameraPosition;
	const int distance = glm::sqrt(dist.x * dist.x + dist.y * dist.y);
	const float cullingThreshold = _world->getChunkSize();
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
							(GLvoid*)(offsetof(CuTerrainVertex, encodedPosX)));

					const int materialLoc = shader.enableVertexAttribute("materialWeightsAsUBytes");
					glVertexAttribIPointer(materialLoc, 4, GL_UNSIGNED_BYTE, sizeof(CuTerrainVertex),
							(GLvoid*)(offsetof(CuTerrainVertex, material0)));
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
						if (octreeNode.childHandles[x][y][z] != 0xFFFFFFFF) {
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
					if (octreeNode.childHandles[x][y][z] != 0xFFFFFFFF) {
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
