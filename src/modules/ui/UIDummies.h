#pragma once

class DummyBitmap: public tb::TBBitmap {
public:
	bool Init(int width, int height, uint32_t *data) {
		m_w = width;
		m_h = height;
		SetData(data);
		return true;
	}

	virtual int Width() override {
		return m_w;
	}

	virtual int Height() override {
		return m_h;
	}

	virtual void SetData(uint32_t *data) override {
	}
public:
	int m_w = 0, m_h = 0;
};

class DummyRenderer: public tb::TBRendererBatcher {
private:
	tb::TBRect _clipRect;
public:
	void BeginPaint(int, int) override {
	}

	void EndPaint() override {
	}

	tb::TBBitmap *CreateBitmap(int width, int height, uint32_t* data) override {
		DummyBitmap *bitmap = new DummyBitmap();
		if (!bitmap || !bitmap->Init(width, height, data)) {
			delete bitmap;
			return nullptr;
		}
		return bitmap;
	}

	void RenderBatch(Batch*) override {
	}

	void SetClipRect(const tb::TBRect&) override {
	}
};
