/**
 * @file
 */

#include "WorldRenderer.h"
#include "video/GLFunc.h"
#include "voxel/Spiral.h"
#include "core/App.h"
#include "noise/SimplexNoise.h"
#include "core/Var.h"
#include <SDL.h>
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/polyvox/RawVolume.h"
#include "MaterialColor.h"

#define GBUFFER 0

constexpr int MinCullingDistance = 500;
constexpr int MinExtractionCullingDistance = 1000;

namespace frontend {

WorldRenderer::WorldRenderer(const voxel::WorldPtr& world) :
		_world(world) {
}

WorldRenderer::~WorldRenderer() {
}

void WorldRenderer::reset() {
	for (const video::GLMeshData& meshData : _meshDataOpaque) {
		glDeleteBuffers(1, &meshData.vertexBuffer);
		glDeleteBuffers(1, &meshData.indexBuffer);
		glDeleteBuffers(1, &meshData.offsetBuffer);
		glDeleteVertexArrays(1, &meshData.vertexArrayObject);
	}
	for (const video::GLMeshData& meshData : _meshDataWater) {
		glDeleteBuffers(1, &meshData.vertexBuffer);
		glDeleteBuffers(1, &meshData.indexBuffer);
		glDeleteBuffers(1, &meshData.offsetBuffer);
		glDeleteVertexArrays(1, &meshData.vertexArrayObject);
	}
	_meshDataOpaque.clear();
	_meshDataWater.clear();
	_entities.clear();
	_fogRange = 0.0f;
	_viewDistance = 1.0f;
	_now = 0l;
}

void WorldRenderer::shutdown() {
#if GBUFFER
	_gbuffer.shutdown();
#endif
	reset();
	_colorTexture = video::TexturePtr();
	_entities.clear();

	for (const video::GLMeshData& meshData : _meshDataPlant) {
		glDeleteBuffers(1, &meshData.vertexBuffer);
		glDeleteBuffers(1, &meshData.indexBuffer);
		glDeleteBuffers(1, &meshData.offsetBuffer);
		glDeleteVertexArrays(1, &meshData.vertexArrayObject);
	}
	_meshDataPlant.clear();
	_noiseFuture.clear();
	_plantGenerator.shutdown();
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
	const glm::ivec3& p = _world->getMeshPos(pos);
	for (auto i = _meshDataOpaque.begin(); i != _meshDataOpaque.end(); ++i) {
		const video::GLMeshData& meshData = *i;
		if (meshData.translation != p) {
			continue;
		}
		_meshDataOpaque.erase(i);
		glDeleteBuffers(1, &meshData.vertexBuffer);
		glDeleteBuffers(1, &meshData.indexBuffer);
		glDeleteBuffers(1, &meshData.offsetBuffer);
		glDeleteVertexArrays(1, &meshData.vertexArrayObject);
		break;
	}
	for (auto i = _meshDataWater.begin(); i != _meshDataWater.end(); ++i) {
		const video::GLMeshData& meshData = *i;
		if (meshData.translation != p) {
			continue;
		}
		_meshDataWater.erase(i);
		glDeleteBuffers(1, &meshData.vertexBuffer);
		glDeleteBuffers(1, &meshData.indexBuffer);
		glDeleteBuffers(1, &meshData.offsetBuffer);
		glDeleteVertexArrays(1, &meshData.vertexArrayObject);
		break;
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

void WorldRenderer::distributePlants(int amount, const glm::ivec3& meshGridPos, core::Random& random, std::vector<glm::vec3>& translations) {
	core_trace_scoped(WorldRendererDistributePlants);
	const int size = _world->getMeshSize();
	const voxel::BiomeManager& biomeMgr = _world->getBiomeManager();
	for (;;) {
		if (amount-- <= 0) {
			return;
		}
		const int lx = random.random(1, size - 1);
		const int nx = meshGridPos.x + lx;
		const int lz = random.random(1, size - 1);
		const int nz = meshGridPos.z + lz;
		const int y = _world->findFloor(nx, nz);
		if (y == -1) {
			continue;
		}
		const glm::ivec3 translation(nx, y, nz);
		if (!biomeMgr.hasPlants(translation)) {
			continue;
		}

		translations.push_back(translation);
		Log::trace("plant at %i:%i:%i (%i)", nx, y, nz, (int)translations.size());
	}
}

// redistribute the plants on the meshes that are already extracted
void WorldRenderer::fillPlantPositionsFromMeshes() {
	const int plantMeshAmount = _meshDataPlant.size();
	if (plantMeshAmount == 0) {
		return;
	}
	for (video::GLMeshData& mp : _meshDataPlant) {
		mp.instancedPositions.clear();
	}
	for (const video::GLMeshData& data : _meshDataOpaque) {
		if (data.instancedPositions.empty()) {
			continue;
		}
		std::vector<glm::vec3> p = data.instancedPositions;
		core::Random rnd(_world->seed() + data.translation.x + data.translation.y + data.translation.z);
		rnd.shuffle(p.begin(), p.end());
		const int plantMeshes = p.size() / plantMeshAmount;
		int delta = p.size() - plantMeshes * plantMeshAmount;
		for (video::GLMeshData& mp : _meshDataPlant) {
			auto it = std::next(p.begin(), plantMeshes + delta);
			std::move(p.begin(), it, std::back_inserter(mp.instancedPositions));
			p.erase(p.begin(), it);
			delta = 0;
		}
	}
}

void WorldRenderer::handleMeshQueue(video::Shader& shader) {
	voxel::ChunkMeshData mesh(0, 0);
	if (!_world->pop(mesh)) {
		return;
	}
	core_trace_gl_scoped(WorldRendererHandleMeshQueue);
	for (video::GLMeshData& m : _meshDataOpaque) {
		if (m.translation == mesh.opaqueMesh.getOffset()) {
			updateMesh(mesh.opaqueMesh, m);
			return;
		}
	}
	for (video::GLMeshData& m : _meshDataWater) {
		if (m.translation == mesh.waterMesh.getOffset()) {
			updateMesh(mesh.waterMesh, m);
			return;
		}
	}
	// Now add the mesh to the list of meshes to render.
	video::GLMeshData meshDataOpaque = createMesh(shader, mesh.opaqueMesh);
	if (meshDataOpaque.noOfIndices > 0) {
		core::Random rnd(_world->seed() + meshDataOpaque.translation.x + meshDataOpaque.translation.y + meshDataOpaque.translation.z);
		distributePlants(100, meshDataOpaque.translation, rnd, meshDataOpaque.instancedPositions);
		_meshDataOpaque.push_back(meshDataOpaque);
		fillPlantPositionsFromMeshes();
	}
	const video::GLMeshData& meshDataWater = createMesh(shader, mesh.waterMesh);
	if (meshDataWater.noOfIndices > 0) {
		_meshDataWater.push_back(meshDataWater);
	}
}

int WorldRenderer::renderWorldMeshes(video::Shader& shader, const video::Camera& camera, GLMeshDatas& meshes, int* vertices, bool culling) {
	const glm::mat4& view = camera.viewMatrix();

	const MaterialColorArray& materialColors = getMaterialColors();

	shader.activate();
	shader.setUniformMatrix("u_view", view, false);
	shader.setUniformMatrix("u_projection", camera.projectionMatrix(), false);
	shader.setUniformf("u_fogrange", _fogRange);
	shader.setUniformf("u_viewdistance", _viewDistance);
	shader.setUniformi("u_texture", 0);
	shader.setUniformVec3("u_lightpos", _lightPos + camera.position());
	shader.setUniformVec3("u_diffuse_color", _diffuseColor);
	shader.setUniformVec4v("u_materialcolor[0]", &materialColors[0], materialColors.size());
	shader.setUniformf("u_debug_color", 1.0);

	const float chunkSize = (float)_world->getMeshSize();
	const glm::vec3 bboxSize(chunkSize, chunkSize, chunkSize);
	const bool debugGeometry = _debugGeometry->boolVal();
	int drawCallsWorld = 0;
	for (auto i = meshes.begin(); i != meshes.end();) {
		const video::GLMeshData& meshData = *i;
		const float distance = getDistance2(meshData.translation);
		if (culling && isDistanceCulled(distance, true)) {
			_world->allowReExtraction(meshData.translation);
			glDeleteBuffers(1, &meshData.vertexBuffer);
			glDeleteBuffers(1, &meshData.indexBuffer);
			glDeleteBuffers(1, &meshData.offsetBuffer);
			glDeleteVertexArrays(1, &meshData.vertexArrayObject);
			i = meshes.erase(i);
			continue;
		}
		const glm::vec3 mins(meshData.translation);
		const glm::vec3 maxs = glm::vec3(meshData.translation) + bboxSize;
		if (culling && camera.testFrustum(mins, maxs) == video::FrustumResult::Outside) {
			++i;
			continue;
		}
		const glm::mat4& translate = glm::translate(glm::mat4(1.0f), glm::vec3(meshData.translation));
		const glm::mat4& model = translate; //glm::scale(translate, meshData.scale);
		shader.setUniformMatrix("u_model", model, false);
		glBindVertexArray(meshData.vertexArrayObject);

		if (debugGeometry) {
			shader.setUniformf("u_debug_color", 1.0);
		}
		if (meshData.amount == 1) {
			glDrawElements(GL_TRIANGLES, meshData.noOfIndices, meshData.indexType, nullptr);
		} else {
			const int amount = (int)meshData.instancedPositions.size();
			glBindBuffer(GL_ARRAY_BUFFER, meshData.offsetBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * amount, &meshData.instancedPositions[0], GL_DYNAMIC_DRAW);
			glDrawElementsInstanced(GL_TRIANGLES, meshData.noOfIndices, meshData.indexType, nullptr, amount);
		}
		if (vertices != nullptr) {
			*vertices += meshData.noOfVertices;
		}
		GL_checkError();

		if (debugGeometry) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glEnable(GL_POLYGON_OFFSET_LINE);
			glEnable(GL_LINE_SMOOTH);
			glLineWidth(2);
			glPolygonOffset(-2, -2);
			shader.setUniformf("u_debug_color", 0.0);
			glDrawElements(GL_TRIANGLES, meshData.noOfIndices, meshData.indexType, 0);
			glDisable(GL_LINE_SMOOTH);
			glDisable(GL_POLYGON_OFFSET_LINE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			GL_checkError();
		}

		++drawCallsWorld;
		++i;
	}

	shader.deactivate();

	return drawCallsWorld;
}

int WorldRenderer::renderWorld(video::Shader& opaqueShader, video::Shader& plantShader, video::Shader& waterShader, const video::Camera& camera, int* vertices) {
	handleMeshQueue(opaqueShader);

	if (_meshDataOpaque.empty()) {
		return 0;
	}

	core_trace_gl_scoped(WorldRendererRenderWorld);
	int drawCallsWorld = 0;

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles whose normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GL_checkError();

	_colorTexture->bind();

#if GBUFFER
	_gbuffer.bindForWriting();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

	drawCallsWorld  = renderWorldMeshes(opaqueShader, camera, _meshDataOpaque, vertices);
	drawCallsWorld += renderWorldMeshes(waterShader,  camera, _meshDataWater,  vertices);
	drawCallsWorld += renderWorldMeshes(plantShader,  camera, _meshDataPlant,  vertices, false);

#if GBUFFER
	const int width = camera.getWidth();
	const int height = camera.getHeight();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	_gbuffer.bindForReading();

	const GLsizei halfWidth = (GLsizei) (width / 2.0f);
	const GLsizei halfHeight = (GLsizei) (height / 2.0f);

	_gbuffer.setReadBuffer(video::GBuffer::GBUFFER_TEXTURE_TYPE_POSITION);
	glBlitFramebuffer(0, 0, width, height, 0, 0, halfWidth, halfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	_gbuffer.setReadBuffer(video::GBuffer::GBUFFER_TEXTURE_TYPE_DIFFUSE);
	glBlitFramebuffer(0, 0, width, height, 0, halfHeight, halfWidth, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	_gbuffer.setReadBuffer(video::GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
	glBlitFramebuffer(0, 0, width, height, halfWidth, halfHeight, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	_gbuffer.setReadBuffer(video::GBuffer::GBUFFER_TEXTURE_TYPE_TEXCOORD);
	glBlitFramebuffer(0, 0, width, height, halfWidth, 0, width, halfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	GL_checkError();
#endif

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);

	GL_checkError();
	return drawCallsWorld;
}

void WorldRenderer::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) {
	Log::debug("set voxel to %i at %i:%i:%i", voxel.getMaterial(), pos.x, pos.y, pos.z);
	_world->setVoxel(pos, voxel);
	extractNewMeshes(pos, true);
}

void WorldRenderer::updateMesh(voxel::Mesh& surfaceMesh, video::GLMeshData& meshData) {
	core_trace_gl_scoped(WorldRendererUpdateMesh);
	const voxel::IndexType* vecIndices = surfaceMesh.getRawIndexData();
	const uint32_t numIndices = surfaceMesh.getNoOfIndices();
	const voxel::Vertex* vecVertices = surfaceMesh.getRawVertexData();
	const uint32_t numVertices = surfaceMesh.getNoOfVertices();

	core_assert(meshData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::Vertex), vecVertices, GL_STATIC_DRAW);

	core_assert(meshData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(voxel::IndexType), vecIndices, GL_STATIC_DRAW);

	meshData.noOfVertices = numVertices;
	meshData.noOfIndices = numIndices;
}

video::GLMeshData WorldRenderer::createMeshInternal(video::Shader& shader, voxel::Mesh &mesh, int buffers) {
	core_trace_gl_scoped(WorldRendererCreateMesh);

	// This struct holds the OpenGL properties (buffer handles, etc) which will be used
	// to render our mesh. We copy the data from the PolyVox mesh into this structure.
	video::GLMeshData meshData;
	meshData.translation = mesh.getOffset();
	meshData.scale = glm::vec3(1.0f);

	static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
	meshData.indexType = GL_UNSIGNED_INT;

	if (mesh.getNoOfIndices() == 0) {
		return meshData;
	}

	// Create the VAOs for the meshes
	glGenVertexArrays(1, &meshData.vertexArrayObject);

	// The GL_ARRAY_BUFFER will contain the list of vertex positions
	// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
	// and GL_ARRAY_BUFFER will contain the offsets for instanced rendering
	core_assert(buffers == 2 || buffers == 3);
	glGenBuffers(buffers, &meshData.indexBuffer);
	core_assert(buffers == 2 || meshData.offsetBuffer > 0);

	core_assert(meshData.vertexArrayObject > 0);
	glBindVertexArray(meshData.vertexArrayObject);

	updateMesh(mesh, meshData);

	const int posLoc = shader.enableVertexAttribute("a_pos");
	glVertexAttribIPointer(posLoc, 3, GL_UNSIGNED_BYTE, sizeof(voxel::Vertex),
			GL_OFFSET_CAST(offsetof(voxel::Vertex, position)));

	static_assert(sizeof(voxel::Voxel) == sizeof(uint8_t), "Voxel type doesn't match");
	const int locationInfo = shader.enableVertexAttribute("a_info");
	glVertexAttribIPointer(locationInfo, 2, GL_UNSIGNED_BYTE, sizeof(voxel::Vertex),
			GL_OFFSET_CAST(offsetof(voxel::Vertex, ambientOcclusion)));
	GL_checkError();

	return meshData;
}

// TODO: generate bigger buffers and use glBufferSubData
video::GLMeshData WorldRenderer::createMesh(video::Shader& shader, voxel::Mesh &mesh) {
	const video::GLMeshData& meshData = createMeshInternal(shader, mesh, 2);
	if (mesh.getNoOfIndices() == 0) {
		return meshData;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return meshData;
}

video::GLMeshData WorldRenderer::createInstancedMesh(video::Shader& shader, voxel::Mesh &mesh, int amount) {
	video::GLMeshData meshData = createMeshInternal(shader, mesh, 3);
	if (mesh.getNoOfIndices() == 0) {
		return meshData;
	}

	meshData.amount = amount;

	core_assert(meshData.offsetBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.offsetBuffer);

	const int offsetLoc = shader.enableVertexAttribute("a_offset");
	glVertexAttribPointer(offsetLoc, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
			GL_OFFSET_CAST(offsetof(glm::vec3, x)));
	glVertexAttribDivisor(offsetLoc, 1);
	GL_checkError();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return meshData;
}

void WorldRenderer::onSpawn(const glm::vec3& pos, int initialExtractionRadius) {
	core_trace_scoped(WorldRendererOnSpawn);
	_viewDistance = 1.0f;
	extractMeshAroundCamera(_world->getMeshPos(pos), initialExtractionRadius);
}

int WorldRenderer::renderEntities(const video::ShaderPtr& shader, const video::Camera& camera) {
	if (_entities.empty()) {
		return 0;
	}
	core_trace_gl_scoped(WorldRendererRenderEntities);

	int drawCallsEntities = 0;

	const glm::mat4& view = camera.viewMatrix();

	shader->activate();
	shader->setUniformMatrix("u_view", view, false);
	shader->setUniformMatrix("u_projection", camera.projectionMatrix(), false);
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

bool WorldRenderer::extractNewMeshes(const glm::vec3& position, bool force) {
	core_trace_scoped(WorldRendererExtractNewMeshes);
	if (force) {
		_world->allowReExtraction(position);
		return _world->scheduleMeshExtraction(position);
	}
	const glm::ivec3& meshGridPos = _world->getMeshPos(position);
	const glm::ivec3& diff = _lastGridPosition - meshGridPos;
	if (glm::abs(diff.x) >= 1 || glm::abs(diff.y) >= 1 || glm::abs(diff.z) >= 1) {
		const int chunks = MinCullingDistance / _world->getMeshSize() + 1;
		extractMeshAroundCamera(meshGridPos, chunks);
		return true;
	}
	return false;
}

void WorldRenderer::extractMeshAroundCamera(const glm::ivec3& meshGridPos, int radius) {
	core_trace_scoped(WorldRendererExtractAroundCamera);
	const int sideLength = radius * 2 + 1;
	const int amount = sideLength * (sideLength - 1) + sideLength;
	const int meshSize = _world->getMeshSize();
	if (meshGridPos == _lastGridPosition) {
		return;
	}
	_lastGridPosition = meshGridPos;
	glm::ivec3 pos = meshGridPos;
	pos.y = 0;
	voxel::Spiral o;
	for (int i = 0; i < amount; ++i) {
		const float distance = getDistance2(pos);
		if (!isDistanceCulled(distance, false)) {
			_world->scheduleMeshExtraction(pos);
		}
		o.next();
		pos.x = meshGridPos.x + o.x() * meshSize;
		pos.z = meshGridPos.z + o.z() * meshSize;
	}
}

void WorldRenderer::stats(int& meshes, int& extracted, int& pending) const {
	_world->stats(meshes, extracted, pending);
}

void WorldRenderer::onInit(video::Shader& plantShader, int width, int height) {
	_debugGeometry = core::Var::get(cfg::ClientDebugGeometry, "false", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugAmbientOcclusion, "false", core::CV_SHADER);
	core_trace_scoped(WorldRendererOnInit);
	_noiseFuture.push_back(core::App::getInstance()->threadPool().enqueue([] () {
		const int ColorTextureSize = 256;
		const int ColorTextureOctaves = 2;
		const int ColorTextureDepth = 3;
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		const float persistence = 0.3f;
		const float frequency = 0.7f;
		const float amplitude = 1.0f;
		noise::Simplex::SeamlessNoise2DRGB(colorTexture, ColorTextureSize, ColorTextureOctaves, persistence, frequency, amplitude);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, ColorTextureDepth);
	}));
	_colorTexture = video::createTexture("**colortexture**");
	_plantGenerator.generateAll();

	for (int i = 0; i < voxel::MaxPlantTypes; ++i) {
		voxel::Mesh* mesh = _plantGenerator.getMesh((voxel::PlantType)i);
		video::GLMeshData meshDataPlant = createInstancedMesh(plantShader, *mesh, 40);
		if (meshDataPlant.noOfIndices > 0) {
			meshDataPlant.scale = glm::vec3(0.4f, 0.4f, 0.4f);
			_meshDataPlant.push_back(meshDataPlant);
		}
	}

#if GBUFFER
	core_assert(_gbuffer.init(width, height));
#endif
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
		const float advance = _world->getMeshSize() * (dt / 1000.0f);
		_viewDistance += advance;
	}
}

int WorldRenderer::getDistance2(const glm::ivec3& pos) const {
	const glm::ivec3 dist = pos - _lastGridPosition;
	const int distance = dist.x * dist.x + dist.z * dist.z;
	return distance;
}

bool WorldRenderer::isDistanceCulled(int distance2, bool queryForRendering) const {
	const float cullingThreshold = _world->getMeshSize() * 3;
	const int maxAllowedDistance = glm::pow(_viewDistance + cullingThreshold, 2);
	if ((!queryForRendering && distance2 > glm::pow(MinExtractionCullingDistance, 2)) && distance2 >= maxAllowedDistance) {
		return true;
	}
	return false;
}

}
