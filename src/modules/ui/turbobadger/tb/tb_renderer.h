/**
 * @file
 */

#pragma once

#include "tb_color.h"
#include "tb_core.h"
#include "tb_geometry.h"
#include "tb_linklist.h"
#include "video/Camera.h"

namespace tb {

class TBBitmapFragment;

/** TBRendererListener is a listener for TBRenderer. */
class TBRendererListener : public TBLinkOf<TBRendererListener> {
public:
	virtual ~TBRendererListener() {
	}

	/** Called when the context has been lost and all TBBitmaps need to be deleted.
		NOTE: Only do cleanup here. It's not safe to do work on any bitmap since the
		context is already lost. */
	virtual void onContextLost() = 0;

	/** Called when the context has been restored again, and new TBBitmaps can be created
		again. */
	virtual void onContextRestored() = 0;
};

/** TBBitmap is a minimal interface for bitmap to be painted by TBRenderer. */

class TBBitmap {
public:
	/** Note: Implementations for batched renderers should call TBRenderer::FlushBitmap
		to make sure any active batch is being flushed before the bitmap is deleted. */
	virtual ~TBBitmap() {
	}

	virtual int width() = 0;
	virtual int height() = 0;

	/** Update the bitmap with the given data (in BGRA32 format).
		Note: Implementations for batched renderers should call TBRenderer::FlushBitmap
		to make sure any active batch is being flushed before the bitmap is changed. */
	virtual void setData(uint32_t *data) = 0;
};

/** TBRenderer is a minimal interface for painting strings and bitmaps. */

class TBRenderer {
protected:
	video::Camera _camera;
public:
	TBRenderer() : _camera(video::CameraType::FirstPerson, video::CameraMode::Orthogonal) {}

	virtual ~TBRenderer() {
	}

	/** Should be called before invoking paint on any widget.
		render_target_w and render_target_h should be the size of the render target
		that the renderer renders to. I.e window size, screen size or frame buffer object. */
	virtual void beginPaint(int renderTargetW, int renderTargetH) = 0;
	virtual void endPaint() = 0;

	/** Translate all drawing with the given offset */
	virtual void translate(int dx, int dy) = 0;

	/** Set the current opacity that should apply to all drawing (0.f-1.f). */
	virtual void setOpacity(float opacity) = 0;
	virtual float getOpacity() = 0;

	/** Set a clip rect to the renderer. add_to_current should be true when
		pushing a new cliprect that should clip inside the last clip rect,
		and false when restoring.
		It will return the clip rect that was in use before this call. */
	virtual TBRect setClipRect(const TBRect &rect, bool addToCurrent) = 0;

	/** Get the current clip rect. Note: This may be different from the rect
		sent to SetClipRect, due to intersecting with the previous cliprect! */
	virtual TBRect getClipRect() = 0;

	/** Draw the src_rect part of the fragment stretched to dst_rect.
		dst_rect or src_rect can have negative width and height to achieve horizontal and vertical flip. */
	virtual void drawBitmap(const TBRect &dstRect, const TBRect &srcRect, TBBitmapFragment *bitmapFragment) = 0;

	/** Draw the src_rect part of the bitmap stretched to dst_rect.
		dst_rect or src_rect can have negative width and height to achieve horizontal and vertical flip. */
	virtual void drawBitmap(const TBRect &dstRect, const TBRect &srcRect, TBBitmap *bitmap) = 0;

	/** Draw the src_rect part of the fragment stretched to dst_rect.
		The bitmap will be used as a mask for the color.
		dst_rect or src_rect can have negative width and height to achieve horizontal and vertical flip. */
	virtual void drawBitmapColored(const TBRect &dstRect, const TBRect &srcRect, const TBColor &color,
								   TBBitmapFragment *bitmapFragment) = 0;

	/** Draw the src_rect part of the bitmap stretched to dst_rect.
		The bitmap will be used as a mask for the color.
		dst_rect or src_rect can have negative width and height to achieve horizontal and vertical flip. */
	virtual void drawBitmapColored(const TBRect &dstRect, const TBRect &srcRect, const TBColor &color,
								   TBBitmap *bitmap) = 0;

	/** Draw the bitmap tiled into dst_rect. */
	virtual void drawBitmapTile(const TBRect &dstRect, TBBitmap *bitmap) = 0;

	/** Make sure the given bitmap fragment is flushed from any batching, because it may
		be changed or deleted after this call. */
	virtual void flushBitmapFragment(TBBitmapFragment *bitmapFragment) = 0;

	/** Create a new TBBitmap from the given data (in BGRA32 format).
		Width and height must be a power of two.
		Return nullptr if fail. */
	virtual TBBitmap *createBitmap(int width, int height, uint32_t *data) = 0;

	/** Add a listener to this renderer. Does not take ownership. */
	void addListener(TBRendererListener *listener) {
		m_listeners.addLast(listener);
	}

	/** Remove a listener from this renderer. */
	void removeListener(TBRendererListener *listener) {
		m_listeners.remove(listener);
	}

	/** Invoke OnContextLost on all listeners.
		Call when bitmaps should be forgotten. */
	void invokeContextLost();

	/** Invoke OnContextRestored on all listeners.
		Call when bitmaps can safely be restored. */
	void invokeContextRestored();

	/** Defines the hint given to BeginBatchHint. */
	enum BATCH_HINT {
		/** All calls are either DrawBitmap or DrawBitmapColored with the same bitmap
			fragment. */
		BATCH_HINT_DRAW_BITMAP_FRAGMENT
	};

	const video::Camera& camera() const {
		return _camera;
	}

	/** A hint to batching renderers that the following set of draw calls are of the
		same type so batching might be optimized.
		The hint defines what operations are allowed between BeginBatchHint
		until EndBatchHint is called. All other draw operations are invalid.
		It's not valid to nest calls to BeginBatchHint. */
	virtual void beginBatchHint(BATCH_HINT hint) {
	}

	/** End the hint scope started with BeginBatchHint. */
	virtual void endBatchHint() {
	}

	virtual void flush() {
	}

private:
	TBLinkListOf<TBRendererListener> m_listeners;
};

} // namespace tb
