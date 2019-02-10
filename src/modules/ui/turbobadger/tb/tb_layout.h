/**
 * @file
 */

#pragma once

#include "tb_widgets.h"

namespace tb {

/** This means the spacing should be the default, read from the skin. */
#define SPACING_FROM_SKIN TB_INVALID_DIMENSION

/** Specifies which height widgets in a AXIS_X layout should have,
	or which width widgets in a AXIS_Y layout should have.
	No matter what, it will still prioritize minimum and maximum for each widget. */
enum LAYOUT_SIZE {
	LAYOUT_SIZE_GRAVITY,	///< Sizes depend on the gravity for each widget. (If the widget pulls
							///< towards both directions, it should grow to all available space)
	LAYOUT_SIZE_PREFERRED,	///< Size will be the preferred so each widget may be sized differently. [default]
	LAYOUT_SIZE_AVAILABLE	///< Size should grow to all available space
};

/** Specifies which y position widgets in a AXIS_X layout should have,
	or which x position widgets in a AXIS_Y layout should have. */
enum LAYOUT_POSITION {
	LAYOUT_POSITION_CENTER,			///< Position is centered. [default]
	LAYOUT_POSITION_LEFT_TOP,		///< Position is to the left for AXIS_Y layout and top for AXIS_X layout.
	LAYOUT_POSITION_RIGHT_BOTTOM,	///< Position is to the right for AXIS_Y layout and bottom for AXIS_X layout.
	LAYOUT_POSITION_GRAVITY,		///< Position depend on the gravity for each widget. (If the widget pulls
									///< towards both directions, it will be centered)
};

/** Specifies which width widgets in a AXIS_X layout should have,
	or which height widgets in a AXIS_Y layout should have. */
enum LAYOUT_DISTRIBUTION {
	LAYOUT_DISTRIBUTION_PREFERRED,	///< Size will be the preferred so each widget may be sized differently. [default]
	LAYOUT_DISTRIBUTION_AVAILABLE,	///< Size should grow to all available space
	LAYOUT_DISTRIBUTION_GRAVITY		///< Sizes depend on the gravity for each widget. (If the widget pulls
									///< towards both directions, it should grow to all available space)
};

/** Specifies how widgets should be moved horizontally in a AXIS_X
	layout (or vertically in a AXIS_Y layout) if there is extra space
	available. */
enum LAYOUT_DISTRIBUTION_POSITION {
	LAYOUT_DISTRIBUTION_POSITION_CENTER,		///< Position centered. [default]
	LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP,		///< Position to the upper left.
	LAYOUT_DISTRIBUTION_POSITION_RIGHT_BOTTOM	///< Position to the lower right.
};

/** Layout order parameter for TBLayout::setLayoutOrder. */
enum LAYOUT_ORDER {
	LAYOUT_ORDER_BOTTOM_TO_TOP,	///< From bottom to top widget (default creation order).
	LAYOUT_ORDER_TOP_TO_BOTTOM	///< From top to bottom widget.
};

/** Specifies what happens when there is not enough room for the layout, even
	when all the children have been shrunk to their minimum size. */
enum LAYOUT_OVERFLOW {
	LAYOUT_OVERFLOW_CLIP,	///< Clip the chilren widgtes. [default]
	LAYOUT_OVERFLOW_SCROLL	///< Create a scroller.
	//LAYOUT_OVERFLOW_WRAP
};

/** TBLayout layouts its children along the given axis.

	Each widgets size depend on its preferred size (See TBWidget::getPreferredSize),
	gravity, and the specified layout settings (See SetLayoutSize, SetLayoutPosition
	SetLayoutOverflow, SetLayoutDistribution, SetLayoutDistributionPosition), and
	the available size.

	Each widget is also separated by the specified spacing (See SetSpacing).
*/

class TBLayout : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBLayout, TBWidget);

	TBLayout(AXIS axis = AXIS_X);

	/** Set along which axis the content should be layouted */
	virtual void setAxis(AXIS axis) override;
	virtual AXIS getAxis() const override { return m_axis; }

	/** Set the spacing between widgets in this layout. Setting the default (SPACING_FROM_SKIN)
		will make it use the spacing specified in the skin. */
	void setSpacing(int spacing);
	int getSpacing() const { return m_spacing; }

	/** Set the overflow scroll. If there is not enough room for all children in this layout,
		it can scroll in the axis it's laid out. It does so automatically by wheel or panning also
		for other LAYOUT_OVERFLOW than LAYOUT_OVERFLOW_SCROLL. */
	void setOverflowScroll(int overflow_scroll);
	int getOverflowScroll() const { return m_overflow_scroll; }

	/** Set if a fadeout should be painter where the layout overflows or not. */
	void setPaintOverflowFadeout(bool paint_fadeout) { m_packed.paint_overflow_fadeout = paint_fadeout; }

	/** Set the layout size mode. See LAYOUT_SIZE. */
	void setLayoutSize(LAYOUT_SIZE size);

	/** Set the layout position mode. See LAYOUT_POSITION. */
	void setLayoutPosition(LAYOUT_POSITION pos);

	/** Set the layout size mode. See LAYOUT_OVERFLOW. */
	void setLayoutOverflow(LAYOUT_OVERFLOW overflow);

	/** Set the layout distribution mode. See LAYOUT_DISTRIBUTION. */
	void setLayoutDistribution(LAYOUT_DISTRIBUTION distribution);

	/** Set the layout distribution position mode. See LAYOUT_DISTRIBUTION_POSITION. */
	void setLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION distribution_pos);

	/** Set the layout order. The default is LAYOUT_ORDER_BOTTOM_TO_TOP, which begins
		from bottom to top (default creation order). */
	void setLayoutOrder(LAYOUT_ORDER order);

	virtual void invalidateLayout(INVALIDATE_LAYOUT il) override;

	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints) override;

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onPaintChildren(const PaintProps &paint_props) override;
	virtual void onProcess() override;
	virtual void onResized(int old_w, int old_h) override;
	virtual void onInflateChild(TBWidget *child) override;
	virtual void getChildTranslation(int &x, int &y) const override;
	virtual void scrollTo(int x, int y) override;
	virtual TBWidget::ScrollInfo getScrollInfo() override;
protected:
	AXIS m_axis;
	int m_spacing;
	int m_overflow;
	int m_overflow_scroll;
	union {
		struct {
			uint32_t layout_is_invalid		: 1;
			uint32_t layout_mode_size			: 4;
			uint32_t layout_mode_pos			: 4;
			uint32_t layout_mode_overflow		: 4;
			uint32_t layout_mode_dist			: 4;
			uint32_t layout_mode_dist_pos		: 4;
			uint32_t mode_reverse_order		: 1;
			uint32_t paint_overflow_fadeout	: 1;
		} m_packed;
		uint32_t m_packed_init;
	};
	void validateLayout(const SizeConstraints &constraints, PreferredSize *calculate_ps = nullptr);
	/** Can this TBLayout expand in its direction? */
	bool qualifyForExpansion(WIDGET_GRAVITY gravity) const;
	int getWantedHeight(WIDGET_GRAVITY gravity, const PreferredSize &ps, int available_height) const;
	TBWidget *getNextNonCollapsedWidget(TBWidget *child) const;
	int getTrailingSpace(TBWidget *child, int spacing) const;
	int calculateSpacing();
	TBWidget *getFirstInLayoutOrder() const;
	TBWidget *getNextInLayoutOrder(TBWidget *child) const;
};

}
