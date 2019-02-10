/**
 * @file
 */

#pragma once

class DummyBitmap: public tb::TBBitmap {
public:
	bool init(int width, int height, uint32_t *data) {
		m_w = width;
		m_h = height;
		setData(data);
		return true;
	}

	virtual int width() override {
		return m_w;
	}

	virtual int height() override {
		return m_h;
	}

	virtual void setData(uint32_t *data) override {
	}
public:
	int m_w = 0, m_h = 0;
};

class DummyRenderer: public tb::TBRendererBatcher {
private:
	tb::TBRect _clipRect;
public:
	void beginPaint(int, int) override {
	}

	void endPaint() override {
	}

	tb::TBBitmap *createBitmap(int width, int height, uint32_t* data) override {
		DummyBitmap *bitmap = new DummyBitmap();
		if (!bitmap || !bitmap->init(width, height, data)) {
			delete bitmap;
			return nullptr;
		}
		return bitmap;
	}

	void renderBatch(Batch*) override {
	}

	void setClipRect(const tb::TBRect&) override {
	}
};
