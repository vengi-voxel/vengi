/**
 * @file
 */

#pragma once

#include "video/GLFunc.h"
#include "video/Camera.h"
#include "video/VertexBuffer.h"
#include "UiShaders.h"

#include <renderers/tb_renderer_batcher.h>

namespace tb {

class UIRendererGL;

class UIBitmapGL: public TBBitmap {
public:
	UIBitmapGL(UIRendererGL *renderer);
	~UIBitmapGL();

	bool Init(int width, int height, uint32 *data);

	bool Init(int width, int height, GLuint texture);

	void bind();

	virtual int Width() override {
		return _w;
	}

	virtual int Height() override {
		return _h;
	}

	virtual void SetData(uint32 *data) override;
public:
	static GLuint g_current_texture;
	UIRendererGL *_renderer;
	int _w, _h;
	GLuint _texture;
	bool _destroy = true;
};

class UIRendererGL: public TBRendererBatcher {
private:
	UIBitmapGL _white;
	shader::TextureShader _shader;
	video::Camera _camera;
	video::VertexBuffer _vbo;
	int32_t _bufferIndex = -1;

	void bindBitmap(TBBitmap *bitmap);

public:
	UIRendererGL();

	bool init(const glm::ivec2& dimensions);
	void shutdown();
	void onWindowResize(const glm::ivec2& dimensions);

	virtual void BeginPaint(int renderTargetW, int renderTargetH) override;
	virtual void EndPaint() override;

	virtual TBBitmap *CreateBitmap(int width, int height, uint32 *data) override;

	virtual void RenderBatch(Batch *batch) override;
	virtual void SetClipRect(const TBRect &rect) override;
};

}
