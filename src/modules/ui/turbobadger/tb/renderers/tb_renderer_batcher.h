/**
 * @file
 */

#pragma once

#include "tb_renderer.h"

namespace tb {

#define VERTEX_BATCH_SIZE 6 * 2048

/** TBRendererBatcher is a helper class that implements batching of draw operations for a TBRenderer.
	If you do not want to do your own batching you can subclass this class instead of TBRenderer.
	If overriding any function in this class, make sure to call the base class too. */
class TBRendererBatcher : public TBRenderer {
public:
	/** Vertex stored in a Batch */
	struct Vertex {
		float x, y;
		float u, v;
		union {
			struct {
				unsigned char r, g, b, a;
			};
			uint32_t col;
		};
	};
	/** A batch which should be rendered. */
	class Batch {
	public:
		Batch() : vertex_count(0), bitmap(nullptr), fragment(nullptr), batch_id(0), is_flushing(false) {
		}
		void flush(TBRendererBatcher *batch_renderer);
		Vertex *reserve(TBRendererBatcher *batch_renderer, int count);

		alignas(16) Vertex vertex[VERTEX_BATCH_SIZE];
		int vertex_count;

		TBBitmap *bitmap;
		TBBitmapFragment *fragment;

		uint32_t batch_id;
		bool is_flushing;
	};

	TBRendererBatcher();
	virtual ~TBRendererBatcher();

	virtual void beginPaint(int render_target_w, int render_target_h);
	virtual void endPaint();

	virtual void translate(int dx, int dy);

	virtual void setOpacity(float opacity);
	virtual float getOpacity();

	virtual TBRect setClipRect(const TBRect &rect, bool add_to_current);
	virtual TBRect getClipRect();

	virtual void drawBitmap(const TBRect &dst_rect, const TBRect &src_rect, TBBitmapFragment *bitmap_fragment);
	virtual void drawBitmap(const TBRect &dst_rect, const TBRect &src_rect, TBBitmap *bitmap);
	virtual void drawBitmapColored(const TBRect &dst_rect, const TBRect &src_rect, const TBColor &color,
								   TBBitmapFragment *bitmap_fragment);
	virtual void drawBitmapColored(const TBRect &dst_rect, const TBRect &src_rect, const TBColor &color,
								   TBBitmap *bitmap);
	virtual void drawBitmapTile(const TBRect &dst_rect, TBBitmap *bitmap);
	virtual void flushBitmap(TBBitmap *bitmap);
	virtual void flushBitmapFragment(TBBitmapFragment *bitmap_fragment);

	virtual void beginBatchHint(TBRenderer::BATCH_HINT hint) {
	}
	virtual void endBatchHint() {
	}

	// == Methods that need implementation in subclasses ================================
	virtual TBBitmap *createBitmap(int width, int height, uint32_t *data) = 0;
	virtual void renderBatch(Batch *batch) = 0;
	virtual void setClipRect(const TBRect &rect) = 0;

protected:
	uint8_t m_opacity;
	TBRect m_screen_rect;
	TBRect m_clip_rect;
	int m_translation_x;
	int m_translation_y;

	float m_u, m_v, m_uu, m_vv; ///< Some temp variables
	Batch batch;				///< The one and only batch. this should be improved.

	void addQuadInternal(const TBRect &dst_rect, const TBRect &src_rect, uint32_t color, TBBitmap *bitmap,
						 TBBitmapFragment *fragment);
	void flushAllInternal();
};

} // namespace tb
