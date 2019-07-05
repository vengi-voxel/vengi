/**
 * @file
 */

#pragma once

#include "video/Renderer.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "RenderShaders.h"

#include <renderers/tb_renderer_batcher.h>

namespace tb {

class UIRendererGL;

class UIBitmapGL: public TBBitmap {
public:
	UIBitmapGL(UIRendererGL *renderer);
	~UIBitmapGL();

	bool init(int width, int height, uint32_t *data);

	bool init(int width, int height, video::Id texture);

	void bind(video::TextureUnit unit = video::TextureUnit::Zero);

	void shutdown();

	virtual int width() override {
		return _w;
	}

	virtual int height() override {
		return _h;
	}

	virtual void setData(uint32_t *data) override;
public:
	UIRendererGL *_renderer;
	int _w = 0;
	int _h = 0;
	video::Id _texture = video::InvalidId;
	video::TextureConfig _textureConfig;
	bool _destroy = false;
};

class UIRendererGL: public TBRendererBatcher {
private:
	UIBitmapGL _white;
	shader::TextureShader _shader;
	video::Camera _camera;
	video::Buffer _vbo;
	int32_t _bufferIndex = -1;

	void bindBitmap(TBBitmap *bitmap);

public:
	UIRendererGL();

	bool init(const glm::ivec2& pixelDimensions, const glm::ivec2& screenDimensions);
	void shutdown();
	void onWindowResize(const glm::ivec2& pixelDimensions, const glm::ivec2& screenDimensions);

	virtual void beginPaint(int pixelWidth, int pixelHeight) override;
	virtual void endPaint() override;
	virtual void translate(int dx, int dy) override;

	virtual TBBitmap *createBitmap(int width, int height, uint32_t *data) override;

	virtual void renderBatch(Batch *batch) override;
	virtual void setClipRect(const TBRect &rect) override;
};

}
