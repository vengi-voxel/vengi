/**
 * @file
 */

#include "tb_layout.h"
#include "tb_system.h"
#include "tb_skin_util.h"
#include "core/Assert.h"

namespace tb {

TBLayout::TBLayout(AXIS axis)
	: m_axis(axis)
	, m_spacing(SPACING_FROM_SKIN)
	, m_overflow(0)
	, m_overflow_scroll(0)
	, m_packed_init(0)
{
	m_packed.layout_mode_size = LAYOUT_SIZE_GRAVITY;
	m_packed.layout_mode_pos = LAYOUT_POSITION_CENTER;
	m_packed.layout_mode_overflow = LAYOUT_OVERFLOW_CLIP;
	m_packed.layout_mode_dist = LAYOUT_DISTRIBUTION_PREFERRED;
	m_packed.layout_mode_dist_pos = LAYOUT_DISTRIBUTION_POSITION_CENTER;
	m_packed.paint_overflow_fadeout = 1;
}

void TBLayout::setAxis(AXIS axis)
{
	if (axis == m_axis)
		return;
	m_axis = axis;
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	invalidateSkinStates();
}

void TBLayout::setSpacing(int spacing)
{
	if (spacing == m_spacing)
		return;
	m_spacing = spacing;
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
}

void TBLayout::setOverflowScroll(int overflowScroll)
{
	overflowScroll = Min(overflowScroll, m_overflow);
	overflowScroll = Max(overflowScroll, 0);
	if (overflowScroll == m_overflow_scroll)
		return;
	m_overflow_scroll = overflowScroll;
	invalidate();
	if (m_axis == AXIS_X)
		onScroll(m_overflow_scroll, 0);
	else
		onScroll(0, m_overflow_scroll);
}

void TBLayout::setLayoutSize(LAYOUT_SIZE size)
{
	if (size == m_packed.layout_mode_size)
		return;
	m_packed.layout_mode_size = size;
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::setLayoutPosition(LAYOUT_POSITION pos)
{
	if (pos == m_packed.layout_mode_pos)
		return;
	m_packed.layout_mode_pos = pos;
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::setLayoutOverflow(LAYOUT_OVERFLOW overflow)
{
	if (overflow == m_packed.layout_mode_overflow)
		return;
	m_packed.layout_mode_overflow = overflow;
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::setLayoutDistribution(LAYOUT_DISTRIBUTION distribution)
{
	if (distribution == m_packed.layout_mode_dist)
		return;
	m_packed.layout_mode_dist = distribution;
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::setLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION distributionPos)
{
	if (distributionPos == m_packed.layout_mode_dist_pos)
		return;
	m_packed.layout_mode_dist_pos = distributionPos;
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::setLayoutOrder(LAYOUT_ORDER order)
{
	bool reversed = (order == LAYOUT_ORDER_TOP_TO_BOTTOM);
	if (reversed == m_packed.mode_reverse_order)
		return;
	m_packed.mode_reverse_order = reversed;
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBLayout::invalidateLayout(INVALIDATE_LAYOUT il)
{
	m_packed.layout_is_invalid = 1;
	// Continue invalidating parents (depending on il)
	TBWidget::invalidateLayout(il);
}

PreferredSize RotPreferredSize(const PreferredSize &ps, AXIS axis)
{
	if (axis == AXIS_X)
		return ps;
	PreferredSize psr;
	psr.max_w = ps.max_h;
	psr.max_h = ps.max_w;
	psr.min_w = ps.min_h;
	psr.min_h = ps.min_w;
	psr.pref_w = ps.pref_h;
	psr.pref_h = ps.pref_w;
	psr.size_dependency =
		((ps.size_dependency & SIZE_DEP_WIDTH_DEPEND_ON_HEIGHT) ?
				SIZE_DEP_HEIGHT_DEPEND_ON_WIDTH : SIZE_DEP_NONE) |
		((ps.size_dependency & SIZE_DEP_HEIGHT_DEPEND_ON_WIDTH) ?
				SIZE_DEP_WIDTH_DEPEND_ON_HEIGHT : SIZE_DEP_NONE);
	return psr;
}

SizeConstraints RotSizeConstraints(const SizeConstraints &sc, AXIS axis)
{
	return axis == AXIS_X ? sc : SizeConstraints(sc.available_h, sc.available_w);
}

TBRect RotRect(const TBRect &rect, AXIS axis)
{
	if (axis == AXIS_X)
		return rect;
	return TBRect(rect.y, rect.x, rect.h, rect.w);
}

WIDGET_GRAVITY RotGravity(WIDGET_GRAVITY gravity, AXIS axis)
{
	if (axis == AXIS_X)
		return gravity;
	WIDGET_GRAVITY r = WIDGET_GRAVITY_NONE;
	r |= (gravity & WIDGET_GRAVITY_LEFT) ? WIDGET_GRAVITY_TOP : WIDGET_GRAVITY_NONE;
	r |= (gravity & WIDGET_GRAVITY_TOP) ? WIDGET_GRAVITY_LEFT : WIDGET_GRAVITY_NONE;
	r |= (gravity & WIDGET_GRAVITY_RIGHT) ? WIDGET_GRAVITY_BOTTOM : WIDGET_GRAVITY_NONE;
	r |= (gravity & WIDGET_GRAVITY_BOTTOM) ? WIDGET_GRAVITY_RIGHT : WIDGET_GRAVITY_NONE;
	return r;
}

bool TBLayout::qualifyForExpansion(WIDGET_GRAVITY gravity) const
{
	if (m_packed.layout_mode_dist == LAYOUT_DISTRIBUTION_AVAILABLE)
		return true;
	if (m_packed.layout_mode_dist == LAYOUT_DISTRIBUTION_GRAVITY &&
		((gravity & WIDGET_GRAVITY_LEFT) && (gravity & WIDGET_GRAVITY_RIGHT)))
		return true;
	return false;
}

int TBLayout::getWantedHeight(WIDGET_GRAVITY gravity, const PreferredSize &ps, int availableHeight) const
{
	int height = 0;
	switch (m_packed.layout_mode_size)
	{
	case LAYOUT_SIZE_GRAVITY:
		height = ((gravity & WIDGET_GRAVITY_TOP) && (gravity & WIDGET_GRAVITY_BOTTOM)) ?
									availableHeight : Min(availableHeight, ps.pref_h);
		break;
	case LAYOUT_SIZE_PREFERRED:
		height = Min(availableHeight, ps.pref_h);
		break;
	case LAYOUT_SIZE_AVAILABLE:
		height = Min(availableHeight, ps.max_h);
		break;
	}
	height = Min(height, ps.max_h);
	return height;
}


TBWidget *TBLayout::getNextNonCollapsedWidget(TBWidget *child) const
{
	TBWidget *next = getNextInLayoutOrder(child);
	while (next && next->getVisibility() == WIDGET_VISIBILITY_GONE)
		next = getNextInLayoutOrder(next);
	return next;
}

int TBLayout::getTrailingSpace(TBWidget *child, int spacing) const
{
	if (spacing == 0)
		return 0;
	if (!getNextNonCollapsedWidget(child))
		return 0;
	return spacing;
}

int TBLayout::calculateSpacing()
{
	// Get spacing from skin, if not specified
	int spacing = m_spacing;
	if (spacing == SPACING_FROM_SKIN)
	{
		if (TBSkinElement *e = getSkinBgElement())
			spacing = e->spacing;

		core_assert(SPACING_FROM_SKIN == SKIN_VALUE_NOT_SPECIFIED);
		if (spacing == SPACING_FROM_SKIN /*|| spacing == SKIN_VALUE_NOT_SPECIFIED*/)
			spacing = g_tb_skin->getDefaultSpacing();
	}
	return spacing;
}

TBWidget *TBLayout::getFirstInLayoutOrder() const
{
	return m_packed.mode_reverse_order ? getLastChild() : getFirstChild();
}

TBWidget *TBLayout::getNextInLayoutOrder(TBWidget *child) const
{
	return m_packed.mode_reverse_order ? child->getPrev() : child->getNext();
}

void TBLayout::validateLayout(const SizeConstraints &constraints, PreferredSize *calculatePs)
{
	// Layout notes:
	// -All layout code is written for AXIS_X layout.
	//  Instead of duplicating the layout code for both AXIS_X and AXIS_Y, we simply
	//  rotate the in data (rect, gravity, preferred size) and the outdata (rect).

	if (!calculatePs)
	{
		if (!m_packed.layout_is_invalid)
			return;
		m_packed.layout_is_invalid = 0;
	}
	else
	{
		// Maximum size will grow below depending of the childrens maximum size
		calculatePs->max_w = calculatePs->max_h = 0;
	}

	const int spacing = calculateSpacing();
	const TBRect padding_rect = getPaddingRect();
	const TBRect layout_rect = RotRect(padding_rect, m_axis);

	const SizeConstraints inner_sc = constraints.constrainByPadding(getRect().w - padding_rect.w,
																	getRect().h - padding_rect.h);

	// Calculate totals for minimum and preferred width that we need for layout.
	int total_preferred_w = 0;
	int total_min_pref_diff_w = 0;
	int total_max_pref_diff_w = 0;
	for (TBWidget *child = getFirstInLayoutOrder(); child; child = getNextInLayoutOrder(child))
	{
		if (child->getVisibility() == WIDGET_VISIBILITY_GONE)
			continue;

		const int ending_space = getTrailingSpace(child, spacing);
		const PreferredSize ps = RotPreferredSize(child->getPreferredSize(inner_sc), m_axis);
		const WIDGET_GRAVITY gravity = RotGravity(child->getGravity(), m_axis);

		total_preferred_w += ps.pref_w + ending_space;
		total_min_pref_diff_w += ps.pref_w - ps.min_w;

		if (qualifyForExpansion(gravity))
		{
			int capped_max_w = Min(layout_rect.w, ps.max_w);
			total_max_pref_diff_w += capped_max_w - ps.pref_w;
		}

		if (calculatePs)
		{
			calculatePs->min_h = Max(calculatePs->min_h, ps.min_h);
			calculatePs->pref_h = Max(calculatePs->pref_h, ps.pref_h);
			calculatePs->min_w += ps.min_w + ending_space;
			calculatePs->pref_w += ps.pref_w + ending_space;
			calculatePs->max_w += ps.max_w + ending_space;

			// The widget height depends on layout and widget properties, so get what
			// it would actually use if it was given max_h as available height.
			// If we just used its max_h, that could increase the whole layout size
			// even if the widget wouldn't actually use it.
			int height = getWantedHeight(gravity, ps, ps.max_h);
			calculatePs->max_h = Max(calculatePs->max_h, height);

			calculatePs->size_dependency |= ps.size_dependency;
		}
	}

	if (calculatePs)
	{
		// We just wanted to calculate preferred size, so return without layouting.
		*calculatePs = RotPreferredSize(*calculatePs, m_axis);
		return;
	}

	TB_IF_DEBUG_SETTING(LAYOUT_PS_DEBUGGING, last_layout_time = TBSystem::getTimeMS());

	// Pre Layout step (calculate distribution position)
	int missing_space = Max(total_preferred_w - layout_rect.w, 0);
	int extra_space = Max(layout_rect.w - total_preferred_w, 0);

	int offset = layout_rect.x;
	if (extra_space && m_packed.layout_mode_dist_pos != LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP)
	{
		// To calculate the offset we need to predict the used space. We can do that by checking
		// the distribution mode and total_max_pref_diff_w. That's how much the widgets could possible
		// expand in the layout below.

		int used_space = total_preferred_w;
		if (m_packed.layout_mode_dist != LAYOUT_DISTRIBUTION_PREFERRED)
			used_space += Min(extra_space, total_max_pref_diff_w);

		if (m_packed.layout_mode_dist_pos == LAYOUT_DISTRIBUTION_POSITION_CENTER)
			offset += (layout_rect.w - used_space) / 2;
		else // LAYOUT_DISTRIBUTION_POSITION_RIGHT_BOTTOM
			offset += layout_rect.w - used_space;
	}

	// Layout
	int used_space = 0;
	for (TBWidget *child = getFirstInLayoutOrder(); child; child = getNextInLayoutOrder(child))
	{
		if (child->getVisibility() == WIDGET_VISIBILITY_GONE)
			continue;

		const int ending_space = getTrailingSpace(child, spacing);
		const PreferredSize ps = RotPreferredSize(child->getPreferredSize(inner_sc), m_axis);
		const WIDGET_GRAVITY gravity = RotGravity(child->getGravity(), m_axis);

		// Calculate width. May shrink if space is missing, or grow if we have extra space.
		int width = ps.pref_w;
		if (missing_space && total_min_pref_diff_w)
		{
			int diff_w = ps.pref_w - ps.min_w;
			float factor = (float)diff_w / (float)total_min_pref_diff_w;
			int removed = (int)(missing_space * factor);
			removed = Min(removed, diff_w);
			width -= removed;

			total_min_pref_diff_w -= diff_w;
			missing_space -= removed;
		}
		else if (extra_space && total_max_pref_diff_w && qualifyForExpansion(gravity))
		{
			int capped_max_w = Min(layout_rect.w, ps.max_w);
			int diff_w = capped_max_w - ps.pref_w;
			float factor = (float)diff_w / (float)total_max_pref_diff_w;
			int added = (int)(extra_space * factor);
			added = Min(added, diff_w);
			width += added;

			total_max_pref_diff_w -= capped_max_w - ps.pref_w;
			extra_space -= added;
		}

		// Calculate height
		int available_height = layout_rect.h;
		int height = getWantedHeight(gravity, ps, available_height);

		// Calculate position
		int pos = layout_rect.y;
		switch (m_packed.layout_mode_pos)
		{
		case LAYOUT_POSITION_CENTER:
			pos += (available_height - height) / 2;
			break;
		case LAYOUT_POSITION_RIGHT_BOTTOM:
			pos += available_height - height;
			break;
		case LAYOUT_POSITION_GRAVITY:
			if ((gravity & WIDGET_GRAVITY_TOP) && (gravity & WIDGET_GRAVITY_BOTTOM))
				pos += (available_height - height) / 2;
			else if (gravity & WIDGET_GRAVITY_BOTTOM)
				pos += available_height - height;
			break;
		default: // LAYOUT_POSITION_LEFT_TOP
			break;
		}

		// Done! Set rect and increase used space
		TBRect rect(used_space + offset, pos, width, height);
		used_space += width + ending_space;

		child->setRect(RotRect(rect, m_axis));
	}
	// Update overflow and overflow scroll
	m_overflow = Max(0, used_space - layout_rect.w);
	setOverflowScroll(m_overflow_scroll);
}

PreferredSize TBLayout::onCalculatePreferredContentSize(const SizeConstraints &constraints)
{
	// Do a layout pass (without layouting) to check childrens preferences.
	PreferredSize ps;
	validateLayout(constraints, &ps);
	return ps;
}

bool TBLayout::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_WHEEL && ev.modifierkeys == TB_MODIFIER_NONE)
	{
		int old_scroll = getOverflowScroll();
		setOverflowScroll(m_overflow_scroll + ev.delta_y * TBSystem::getPixelsPerLine());
		return m_overflow_scroll != old_scroll;
	}
	return false;
}

void TBLayout::onPaintChildren(const PaintProps &paintProps)
{
	TBRect padding_rect = getPaddingRect();
	if (padding_rect.isEmpty())
		return;

	// If we overflow the layout, apply clipping when painting children
	TBRect old_clip_rect;
	if (m_overflow)
	{
		// We only want clipping in one axis (the overflowing one) so we
		// don't damage any expanded skins on the other axis. Add some fluff.
		TBRect clip_rect = padding_rect;
		const int fluff = 100;

		if (m_axis == AXIS_X)
			clip_rect = clip_rect.expand(m_overflow_scroll == 0 ? fluff : 0, fluff,
										m_overflow_scroll == m_overflow ? fluff : 0, fluff);
		else
			clip_rect = clip_rect.expand(fluff, m_overflow_scroll == 0 ? fluff : 0,
										fluff, m_overflow_scroll == m_overflow ? fluff : 0);

		old_clip_rect = g_renderer->setClipRect(clip_rect, true);

		TB_IF_DEBUG_SETTING(LAYOUT_CLIPPING, g_tb_skin->paintRect(clip_rect, TBColor(255, 0, 0, 200), 1));
	}

	// Paint children
	TBWidget::onPaintChildren(paintProps);

	// Paint fadeout image over the overflowed edges
	// to the indicate to used that it's overflowed.
	if (m_overflow && m_packed.paint_overflow_fadeout)
	{
		TBID skin_x, skin_y;
		if (m_axis == AXIS_X)
			skin_x = TBIDC("TBLayout.fadeout_x");
		else
			skin_y = TBIDC("TBLayout.fadeout_y");

		drawEdgeFadeout(padding_rect, skin_x, skin_y,
			m_overflow_scroll,
			m_overflow_scroll,
			m_overflow - m_overflow_scroll,
			m_overflow - m_overflow_scroll);
	}

	// Restore clipping
	if (m_overflow)
		g_renderer->setClipRect(old_clip_rect, false);
}

void TBLayout::onProcess()
{
	SizeConstraints sc(getRect().w, getRect().h);
	validateLayout(sc);
}

void TBLayout::onResized(int oldW, int oldH)
{
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
	SizeConstraints sc(getRect().w, getRect().h);
	validateLayout(sc);
}

void TBLayout::onInflateChild(TBWidget *child)
{
	// Do nothing since we're going to layout the child soon.
}

void TBLayout::getChildTranslation(int &x, int &y) const
{
	if (m_axis == AXIS_X)
	{
		x = -m_overflow_scroll;
		y = 0;
	}
	else
	{
		x = 0;
		y = -m_overflow_scroll;
	}
}

void TBLayout::scrollTo(int x, int y)
{
	setOverflowScroll(m_axis == AXIS_X ? x : y);
}

TBWidget::ScrollInfo TBLayout::getScrollInfo()
{
	ScrollInfo info;
	if (m_axis == AXIS_X)
	{
		info.max_x = m_overflow;
		info.x = m_overflow_scroll;
	}
	else
	{
		info.max_y = m_overflow;
		info.y = m_overflow_scroll;
	}
	return info;
}

} // namespace tb
