#include "WorldRenderer.h"
#include "video/GLFunc.h"
#include "video/Color.h"
#include "voxel/Spiral.h"
#include "core/App.h"
#include "noise/SimplexNoise.h"
#include <SDL.h>
#include <voxel/polyvox/CubicSurfaceExtractor.h>
#include <voxel/polyvox/RawVolume.h>

constexpr int MinCullingDistance = 500;

namespace frontend {

WorldRenderer::WorldRenderer(const voxel::WorldPtr& world) :
		_world(world) {
}

WorldRenderer::~WorldRenderer() {
}

void WorldRenderer::reset() {
	for (const video::GLMeshData& meshData : _meshData) {
		glDeleteBuffers(meshData.numLods, meshData.vertexBuffer);
		glDeleteBuffers(meshData.numLods, meshData.indexBuffer);
		glDeleteVertexArrays(meshData.numLods, meshData.vertexArrayObject);
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
	core_trace_gl_scoped(WorldRendererDeleteMesh);
	const glm::ivec3& p = _world->getGridPos(pos);
	for (auto i = _meshData.begin(); i != _meshData.end(); ++i) {
		const video::GLMeshData& meshData = *i;
		if (meshData.translation != p) {
			continue;
		}
		_meshData.erase(i);
		glDeleteBuffers(meshData.numLods, meshData.vertexBuffer);
		glDeleteBuffers(meshData.numLods, meshData.indexBuffer);
		glDeleteVertexArrays(meshData.numLods, meshData.vertexArrayObject);
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
	core_trace_gl_scoped(WorldRendererHandleMeshQueue);
	for (video::GLMeshData& m : _meshData) {
		if (m.translation == mesh.translation) {
			core_assert(m.numLods == mesh.numLods);
			for (int i = 0; i < m.numLods; ++i) {
				updateMesh(mesh.mesh[i], m, i);
			}
			return;
		}
	}
	// Now add the mesh to the list of meshes to render.
	_meshData.push_back(createMesh(shader, mesh));
}

int WorldRenderer::renderWorld(video::Shader& shader, const video::Camera& camera, const glm::mat4& projection) {
	handleMeshQueue(shader);

	core_trace_gl_scoped(WorldRendererRenderWorld);
	int drawCallsWorld = 0;

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
		const float distance = getDistance2(meshData.translation);
		if (isDistanceCulled(distance, true)) {
			_world->allowReExtraction(meshData.translation);
			glDeleteBuffers(meshData.numLods, meshData.vertexBuffer);
			glDeleteBuffers(meshData.numLods, meshData.indexBuffer);
			glDeleteVertexArrays(meshData.numLods, meshData.vertexArrayObject);
			i = _meshData.erase(i);
			continue;
		}
		const int lod = 0;
		const glm::vec3 mins(meshData.translation);
		const glm::vec3 maxs = glm::vec3(meshData.translation) + bboxSize;
		if (camera.testFrustum(mins, maxs) == video::FrustumResult::Outside) {
			++i;
			continue;
		}
		const glm::mat4& model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(meshData.translation)), glm::vec3(meshData.scale[lod]));
		shader.setUniformMatrix("u_model", model, false);
		glBindVertexArray(meshData.vertexArrayObject[lod]);
		glDrawElements(GL_TRIANGLES, meshData.noOfIndices[lod], meshData.indexType[lod], 0);
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

void WorldRenderer::updateMesh(voxel::DecodedMesh& surfaceMesh, video::GLMeshData& meshData, int lod) {
	core_trace_gl_scoped(WorldRendererUpdateMesh);
	const uint32_t* vecIndices = surfaceMesh.getRawIndexData();
	const uint32_t numIndices = surfaceMesh.getNoOfIndices();
	const voxel::Vertex* vecVertices = surfaceMesh.getRawVertexData();
	const uint32_t numVertices = surfaceMesh.getNoOfVertices();

	core_assert(meshData.vertexBuffer[lod] > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer[lod]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::Vertex), vecVertices, GL_STATIC_DRAW);

	core_assert(meshData.indexBuffer[lod] > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer[lod]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint32_t), vecIndices, GL_STATIC_DRAW);

	meshData.noOfIndices[lod] = numIndices;
}

// TODO: generate bigger buffers and use glBufferSubData
video::GLMeshData WorldRenderer::createMesh(video::Shader& shader, voxel::DecodedMeshData& mesh) {
	core_trace_gl_scoped(WorldRendererCreateMesh);
	// This struct holds the OpenGL properties (buffer handles, etc) which will be used
	// to render our mesh. We copy the data from the PolyVox mesh into this structure.
	video::GLMeshData meshData;
	meshData.translation = mesh.translation;
	meshData.numLods = glm::clamp(mesh.numLods, 1, video::MAX_LODS);

	// Create the VAOs for the meshes
	glGenVertexArrays(meshData.numLods, meshData.vertexArrayObject);

	// The GL_ARRAY_BUFFER will contain the list of vertex positions
	glGenBuffers(meshData.numLods, meshData.vertexBuffer);

	// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
	glGenBuffers(meshData.numLods, meshData.indexBuffer);

	for (int i = 0; i < meshData.numLods; ++i) {
		core_assert(meshData.vertexArrayObject[i] > 0);
		glBindVertexArray(meshData.vertexArrayObject[i]);

		updateMesh(mesh.mesh[i], meshData, i);

		const int posLoc = shader.enableVertexAttribute("a_pos");
		glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(voxel::Vertex),
				GL_OFFSET_CAST(offsetof(voxel::Vertex, position)));

		const int matLoc = shader.enableVertexAttribute("a_material");
		// our material and density is encoded as 8 bits material and 8 bits density
		core_assert(sizeof(voxel::Voxel) == sizeof(uint8_t));
		glVertexAttribIPointer(matLoc, sizeof(voxel::Voxel), GL_UNSIGNED_BYTE, sizeof(voxel::Vertex),
				GL_OFFSET_CAST(offsetof(voxel::Vertex, data)));

		meshData.scale[i] = 1 << i;
		meshData.indexType[i] = GL_UNSIGNED_INT;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return meshData;
}

void WorldRenderer::onSpawn(const glm::vec3& pos, int initialExtractionRadius) {
	core_trace_scoped(WorldRendererOnSpawn);
	_viewDistance = 1.0f;
	_lastGridPosition = _world->getGridPos(pos);
	extractMeshAroundCamera(initialExtractionRadius);
}

int WorldRenderer::renderEntities(const video::ShaderPtr& shader, const video::Camera& camera, const glm::mat4& projection) {
	core_trace_gl_scoped(WorldRendererRenderEntities);
	if (_entities.empty()) {
		return 0;
	}

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
		if (!mesh->initMesh(shader)) {
			continue;
		}
		const glm::mat4& translate = glm::translate(glm::mat4(1.0f), ent->position());
		const glm::mat4& scale = glm::scale(translate, glm::vec3(ent->scale()));
		const glm::mat4& model = glm::rotate(scale, ent->orientation(), glm::vec3(0.0, 1.0, 0.0));
		shader->setUniformMatrix("u_model", model, false);
		drawCallsEntities += mesh->render();
		GL_checkError();
	}
	shader->deactivate();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();
	return drawCallsEntities;
}

void WorldRenderer::extractNewMeshes(const glm::vec3& position, bool force) {
	core_trace_scoped(WorldRendererExtractNewMeshes);
	if (force) {
		deleteMesh(position);
		_world->allowReExtraction(position);
		_world->scheduleMeshExtraction(position);
		return;
	}
	const glm::ivec3& camXYZ = _world->getGridPos(position);
	const glm::vec3 diff = _lastGridPosition - camXYZ;
	if (glm::length(diff.x) >= 1 || glm::length(diff.y) >= 1 || glm::length(diff.z) >= 1) {
		_lastGridPosition = camXYZ;
		extractMeshAroundCamera(1);
	}
}

void WorldRenderer::extractMeshAroundCamera(int radius) {
	core_trace_scoped(WorldRendererExtractAroundCamera);
	const int sideLength = radius * 2 + 1;
	const int amount = sideLength * (sideLength - 1) + sideLength;
	const int chunkSize = _world->getChunkSize();
	const glm::ivec3& cameraPos = _lastGridPosition;
	glm::ivec3 pos = cameraPos;
	pos.y = 0;
	voxel::Spiral o;
	for (int i = 0; i < amount; ++i) {
		const float distance = getDistance2(pos);
		if (!isDistanceCulled(distance, false)) {
			_world->scheduleMeshExtraction(pos);
		}
		o.next();
		pos.x = cameraPos.x + o.x() * chunkSize;
		pos.z = cameraPos.z + o.y() * chunkSize;
	}
}

void WorldRenderer::onInit() {
	core_trace_scoped(WorldRendererOnInit);
	_noiseFuture.push_back(core::App::getInstance()->threadPool().enqueue([] () {
		const int ColorTextureSize = 256;
		const int ColorTextureOctaves = 2;
		const int ColorTextureDepth = 3;
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		noise::Simplex::SeamlessNoise2DRGB(colorTexture, ColorTextureSize, ColorTextureOctaves, 0.3f, 0.7f, 1.0f);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, ColorTextureDepth);
	}));
	_colorTexture = video::createTexture("**colortexture**");
}

void WorldRenderer::onRunning(long dt) {
	core_trace_scoped(WorldRendererOnRunning);
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
		const float advance = _world->getChunkSize() * (dt / 1000.0f);
		_viewDistance += advance;
	}
}

int WorldRenderer::getDistance2(const glm::ivec3& pos) const {
	const glm::ivec3 dist = pos - _lastGridPosition;
	const int distance = dist.x * dist.x + dist.z * dist.z;
	return distance;
}

bool WorldRenderer::isDistanceCulled(int distance2, bool queryForRendering) const {
	const float cullingThreshold = _world->getChunkSize() * 3;
	const int maxAllowedDistance = glm::pow(_viewDistance + cullingThreshold, 2);
	if ((!queryForRendering && distance2 > glm::pow(MinCullingDistance, 2)) && distance2 >= maxAllowedDistance) {
		return true;
	}
	return false;
}

}
