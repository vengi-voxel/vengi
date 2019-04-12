/**
 * @file
 */

#pragma once

#include "tb_widgets_common.h"

namespace tb {

enum SCROLL_MODE {
	SCROLL_MODE_X_Y,		   ///< X and Y always			scroll-mode: xy
	SCROLL_MODE_Y,			   ///< Y always (X never)		scroll-mode: y
	SCROLL_MODE_Y_AUTO,		   ///< Y auto (X never)		scroll-mode: y-auto
	SCROLL_MODE_X_AUTO_Y_AUTO, ///< X auto, Y auto			scroll-mode: auto
	SCROLL_MODE_OFF			   ///< X any Y never			scroll-mode: off
};

/** TBScrollContainerRoot - Internal for TBScrollContainer */
class TBScrollContainerRoot : public TBWidget {
private: // May only be used by TBScrollContainer.
	friend class TBScrollContainer;
	TBScrollContainerRoot() {
	}

public:
	virtual void onPaintChildren(const PaintProps &paint_props) override;
	virtual void getChildTranslation(int &x, int &y) const override;
};

/** TBScrollBarVisibility - Helper for TBScrollContainer or any other scrollable
	container that needs to solve scrollbar visibility according to SCROLL_MODE. */
class TBScrollBarVisibility {
public:
	TBScrollBarVisibility() : x_on(false), y_on(false), visible_w(0), visible_h(0) {
	}

	static TBScrollBarVisibility solve(SCROLL_MODE mode, int contentW, int contentH, int availableW, int availableH,
									   int scrollbarXH, int scrollbarYW);
	static bool isAlwaysOnX(SCROLL_MODE mode) {
		return mode == SCROLL_MODE_X_Y;
	}
	static bool isAlwaysOnY(SCROLL_MODE mode) {
		return mode == SCROLL_MODE_X_Y || mode == SCROLL_MODE_Y;
	}

public:
	bool x_on, y_on;
	int visible_w, visible_h;
};

/** TBScrollContainer - A container with scrollbars that can scroll its children. */
class TBScrollContainer : public TBWidget {
	friend class TBScrollContainerRoot;

public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBScrollContainer, TBWidget);

	TBScrollContainer();
	~TBScrollContainer();

	/** Set to true if the preferred size of this container should adapt to the preferred
		size of the content. This is disabled by default. */
	void setAdaptToContentSize(bool adapt);
	bool getAdaptToContentSize() {
		return m_adapt_to_content_size;
	}

	/** Set to true if the content should adapt to the available size of this container
		when it's larger than the preferred size. */
	void setAdaptContentSize(bool adapt);
	bool getAdaptContentSize() {
		return m_adapt_content_size;
	}

	void setScrollMode(SCROLL_MODE mode);
	SCROLL_MODE getScrollMode() {
		return m_mode;
	}

	virtual void scrollTo(int x, int y) override;
	virtual TBWidget::ScrollInfo getScrollInfo() override;
	virtual TBWidget *getScrollRoot() override {
		return &m_root;
	}

	virtual void invalidateLayout(INVALIDATE_LAYOUT il) override;

	virtual TBRect getPaddingRect() override;
	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints) override;

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onProcess() override;
	virtual void onResized(int oldW, int oldH) override;

	virtual TBWidget *getContentRoot() override {
		return &m_root;
	}

protected:
	TBScrollBar m_scrollbar_x;
	TBScrollBar m_scrollbar_y;
	TBScrollContainerRoot m_root;
	bool m_adapt_to_content_size = false;
	bool m_adapt_content_size = false;
	bool m_layout_is_invalid = false;
	SCROLL_MODE m_mode = SCROLL_MODE_X_Y;
	void validateLayout(const SizeConstraints &constraints);
};

} // namespace tb
