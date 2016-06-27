/**
 * @file
 */

#pragma once

#include "video/GLFunc.h"
#include "UiShaders.h"

#include <renderers/tb_renderer_batcher.h>

namespace tb {

class UIRendererGL;

class UIBitmapGL: public TBBitmap {
public:
	UIBitmapGL(UIRendererGL *renderer);
	~UIBitmapGL();bool Init(int width, int height, uint32 *data);
	virtual int Width() {
		return m_w;
	}
	virtual int Height() {
		return m_h;
	}
	virtual void SetData(uint32 *data);
public:
	UIRendererGL *m_renderer;
	int m_w, m_h;
	GLuint m_texture;
};

class UIRendererGL: public TBRendererBatcher {
private:
	shader::UiShader _shader;
	GLuint _buffer = 0u;
	GLuint _vao = 0u;
public:
	UIRendererGL();

	bool init();
	void shutdown();

	virtual void BeginPaint(int render_target_w, int render_target_h);
	virtual void EndPaint();

	virtual TBBitmap *CreateBitmap(int width, int height, uint32 *data);

	virtual void RenderBatch(Batch *batch);
	virtual void SetClipRect(const TBRect &rect);
};

}
