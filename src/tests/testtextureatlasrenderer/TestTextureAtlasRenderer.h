/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxelformat/MeshCache.h"
#include "voxelrender/CachedMeshRenderer.h"
#include "video/TextureAtlasRenderer.h"
#include "video/Buffer.h"
#include "RenderShaders.h"

class TestTextureAtlasRenderer: public TestApp {
private:
	struct Vertex {
		union {
			glm::vec2 pos;
			struct {
				float x;
				float y;
			};
		};
		union {
			glm::vec2 uv;
			struct {
				float u;
				float v;
			};
		};
		union {
			uint32_t color;
			struct {
				uint8_t r;
				uint8_t g;
				uint8_t b;
				uint8_t a;
			};
		};
	};
	alignas(16) Vertex _vertices[6];
	int _bufIdx = -1;

	using Super = TestApp;
	voxelrender::CachedMeshRenderer _meshRenderer;
	video::TextureAtlasRenderer _atlasRenderer;
	shader::TextureShader _textureShader;
	video::Buffer _vbo;
	int _modelIndex = -1;

	void doRender() override;
public:
	TestTextureAtlasRenderer(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
