/**
 * @file
 */

#include "tb_scroll_container.h"
#include "core/Assert.h"
#include "tb_system.h"

namespace tb {

TBScrollBarVisibility TBScrollBarVisibility::solve(SCROLL_MODE mode, int contentW, int contentH, int availableW,
												   int availableH, int scrollbarXH, int scrollbarYW) {
	TBScrollBarVisibility visibility;
	visibility.visible_w = availableW;
	visibility.visible_h = availableH;

	if (mode == SCROLL_MODE_X_Y) {
		visibility.y_on = true;
		visibility.x_on = true;
		visibility.visible_w -= scrollbarYW;
		visibility.visible_h -= scrollbarXH;
	} else if (mode == SCROLL_MODE_OFF) {
	} else if (mode == SCROLL_MODE_Y) {
		visibility.y_on = true;
		visibility.visible_w -= scrollbarYW;
	} else if (mode == SCROLL_MODE_Y_AUTO) {
		if (contentH > availableH) {
			visibility.y_on = true;
			visibility.visible_w -= scrollbarYW;
		}
	} else if (mode == SCROLL_MODE_X_AUTO_Y_AUTO) {
		if (contentW > visibility.visible_w) {
			visibility.x_on = true;
			visibility.visible_h = availableH - scrollbarXH;
		}
		if (contentH > visibility.visible_h) {
			visibility.y_on = true;
			visibility.visible_w = availableW - scrollbarYW;
		}
		if (contentW > visibility.visible_w) {
			visibility.x_on = true;
			visibility.visible_h = availableH - scrollbarXH;
		}
	}
	return visibility;
}

void TBScrollContainerRoot::onPaintChildren(const PaintProps &paintProps) {
	// We only want clipping in one axis (the overflowing one) so we
	// don't damage any expanded skins on the other axis. Add some fluff.
	const int fluff = 100;
	TBScrollContainer *sc = static_cast<TBScrollContainer *>(getParent());
	TBRect clip_rect = getPaddingRect().expand(
		sc->m_scrollbar_x.canScrollNegative() ? 0 : fluff, sc->m_scrollbar_y.canScrollNegative() ? 0 : fluff,
		sc->m_scrollbar_x.canScrollPositive() ? 0 : fluff, sc->m_scrollbar_y.canScrollPositive() ? 0 : fluff);

	TBRect old_clip_rect = g_renderer->setClipRect(clip_rect, true);

	TB_IF_DEBUG_SETTING(LAYOUT_CLIPPING, g_tb_skin->paintRect(clip_rect, TBColor(255, 0, 0, 200), 1));

	TBWidget::onPaintChildren(paintProps);

	g_renderer->setClipRect(old_clip_rect, false);
}

void TBScrollContainerRoot::getChildTranslation(int &x, int &y) const {
	TBScrollContainer *sc = static_cast<TBScrollContainer *>(getParent());
	x = (int)-sc->m_scrollbar_x.getValue();
	y = (int)-sc->m_scrollbar_y.getValue();
}

TBScrollContainer::TBScrollContainer()
	: m_adapt_to_content_size(false), m_adapt_content_size(false), m_layout_is_invalid(false), m_mode(SCROLL_MODE_X_Y) {
	addChild(&m_scrollbar_x);
	addChild(&m_scrollbar_y);
	addChild(&m_root);
	m_scrollbar_y.setAxis(AXIS_Y);
}

TBScrollContainer::~TBScrollContainer() {
	removeChild(&m_root);
	removeChild(&m_scrollbar_y);
	removeChild(&m_scrollbar_x);
}

void TBScrollContainer::setAdaptToContentSize(bool adapt) {
	if (m_adapt_to_content_size == adapt) {
		return;
	}
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	m_adapt_to_content_size = adapt;
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
}

void TBScrollContainer::setAdaptContentSize(bool adapt) {
	if (m_adapt_content_size == adapt) {
		return;
	}
	m_adapt_content_size = adapt;
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBScrollContainer::setScrollMode(SCROLL_MODE mode) {
	if (mode == m_mode) {
		return;
	}
	m_mode = mode;
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
}

void TBScrollContainer::scrollTo(int x, int y) {
	int old_x = m_scrollbar_x.getValue();
	int old_y = m_scrollbar_y.getValue();
	m_scrollbar_x.setValue(x);
	m_scrollbar_y.setValue(y);
	if (old_x != m_scrollbar_x.getValue() || old_y != m_scrollbar_y.getValue()) {
		invalidate();
	}
}

TBWidget::ScrollInfo TBScrollContainer::getScrollInfo() {
	ScrollInfo info;
	info.min_x = static_cast<int>(m_scrollbar_x.getMinValue());
	info.min_y = static_cast<int>(m_scrollbar_y.getMinValue());
	info.max_x = static_cast<int>(m_scrollbar_x.getMaxValue());
	info.max_y = static_cast<int>(m_scrollbar_y.getMaxValue());
	info.x = m_scrollbar_x.getValue();
	info.y = m_scrollbar_y.getValue();
	return info;
}

void TBScrollContainer::invalidateLayout(INVALIDATE_LAYOUT il) {
	m_layout_is_invalid = true;
	// No recursion up to parents here unless we adapt to content size.
	if (m_adapt_to_content_size) {
		TBWidget::invalidateLayout(il);
	}
}

TBRect TBScrollContainer::getPaddingRect() {
	int visible_w = getRect().w;
	int visible_h = getRect().h;
	if (m_scrollbar_x.getOpacity() != 0.0F) {
		visible_h -= m_scrollbar_x.getPreferredSize().pref_h;
	}
	if (m_scrollbar_y.getOpacity() != 0.0F) {
		visible_w -= m_scrollbar_y.getPreferredSize().pref_w;
	}
	return TBRect(0, 0, visible_w, visible_h);
}

PreferredSize TBScrollContainer::onCalculatePreferredContentSize(const SizeConstraints &constraints) {
	PreferredSize ps;
	ps.pref_w = ps.pref_h = 100;
	ps.min_w = ps.min_h = 50;
	if (m_adapt_to_content_size) {
		if (TBWidget *content_child = m_root.getFirstChild()) {
			ps = content_child->getPreferredSize(constraints);
			int scrollbar_y_w = m_scrollbar_y.getPreferredSize().pref_w;
			int scrollbar_x_h = m_scrollbar_x.getPreferredSize().pref_h;

			ps.pref_w += scrollbar_y_w;
			ps.max_w += scrollbar_y_w;

			if (m_mode == SCROLL_MODE_X_Y || m_mode == SCROLL_MODE_X_AUTO_Y_AUTO) {
				ps.pref_h += scrollbar_x_h;
				ps.max_h += scrollbar_x_h;
			}
		}
	}
	return ps;
}

bool TBScrollContainer::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_CHANGED && (ev.target == &m_scrollbar_x || ev.target == &m_scrollbar_y)) {
		invalidate();
		onScroll(m_scrollbar_x.getValue(), m_scrollbar_y.getValue());
		return true;
	}
	if (ev.type == EVENT_TYPE_WHEEL && ev.modifierkeys == TB_MODIFIER_NONE) {
		double old_val_y = m_scrollbar_y.getValueDouble();
		m_scrollbar_y.setValueDouble(old_val_y + ev.delta_y * TBSystem::getPixelsPerLine());
		double old_val_x = m_scrollbar_x.getValueDouble();
		m_scrollbar_x.setValueDouble(old_val_x + ev.delta_x * TBSystem::getPixelsPerLine());
		return (m_scrollbar_x.getValueDouble() != old_val_x || m_scrollbar_y.getValueDouble() != old_val_y);
	}
	if (ev.type == EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == TB_KEY_LEFT && m_scrollbar_x.canScrollNegative()) {
			scrollBySmooth(-TBSystem::getPixelsPerLine(), 0);
		} else if (ev.special_key == TB_KEY_RIGHT && m_scrollbar_x.canScrollPositive()) {
			scrollBySmooth(TBSystem::getPixelsPerLine(), 0);
		} else if (ev.special_key == TB_KEY_UP && m_scrollbar_y.canScrollNegative()) {
			scrollBySmooth(0, -TBSystem::getPixelsPerLine());
		} else if (ev.special_key == TB_KEY_DOWN && m_scrollbar_y.canScrollPositive()) {
			scrollBySmooth(0, TBSystem::getPixelsPerLine());
		} else if (ev.special_key == TB_KEY_PAGE_UP && m_scrollbar_y.canScrollNegative()) {
			scrollBySmooth(0, -getPaddingRect().h);
		} else if (ev.special_key == TB_KEY_PAGE_DOWN && m_scrollbar_y.canScrollPositive()) {
			scrollBySmooth(0, getPaddingRect().h);
		} else if (ev.special_key == TB_KEY_HOME) {
			scrollToSmooth(m_scrollbar_x.getValue(), 0);
		} else if (ev.special_key == TB_KEY_END) {
			scrollToSmooth(m_scrollbar_x.getValue(), (int)m_scrollbar_y.getMaxValue());
		} else {
			return false;
		}
		return true;
	}
	return false;
}

void TBScrollContainer::onProcess() {
	SizeConstraints sc(getRect().w, getRect().h);
	validateLayout(sc);
}

void TBScrollContainer::validateLayout(const SizeConstraints &constraints) {
	if (!m_layout_is_invalid) {
		return;
	}
	m_layout_is_invalid = false;

	// Layout scrollbars (no matter if they are visible or not)
	int scrollbar_y_w = m_scrollbar_y.getPreferredSize().pref_w;
	int scrollbar_x_h = m_scrollbar_x.getPreferredSize().pref_h;
	m_scrollbar_x.setRect(TBRect(0, getRect().h - scrollbar_x_h, getRect().w - scrollbar_y_w, scrollbar_x_h));
	m_scrollbar_y.setRect(TBRect(getRect().w - scrollbar_y_w, 0, scrollbar_y_w, getRect().h));

	if (TBWidget *content_child = m_root.getFirstChild()) {
		int horizontal_padding = TBScrollBarVisibility::isAlwaysOnY(m_mode) ? scrollbar_y_w : 0;
		int vertical_padding = TBScrollBarVisibility::isAlwaysOnX(m_mode) ? scrollbar_x_h : 0;

		SizeConstraints inner_sc = constraints.constrainByPadding(horizontal_padding, vertical_padding);

		PreferredSize ps = content_child->getPreferredSize(inner_sc);

		TBScrollBarVisibility visibility = TBScrollBarVisibility::solve(m_mode, ps.pref_w, ps.pref_h, getRect().w,
																		getRect().h, scrollbar_x_h, scrollbar_y_w);
		m_scrollbar_x.setOpacity(visibility.x_on ? 1.F : 0.F);
		m_scrollbar_y.setOpacity(visibility.y_on ? 1.F : 0.F);
		m_root.setRect(TBRect(0, 0, visibility.visible_w, visibility.visible_h));

		int content_w;
		int content_h;
		if (m_adapt_content_size) {
			content_w = Max(ps.pref_w, m_root.getRect().w);
			content_h = Max(ps.pref_h, m_root.getRect().h);
			if (!visibility.x_on && m_root.getRect().w < ps.pref_w) {
				content_w = Min(ps.pref_w, m_root.getRect().w);
			}
		} else {
			content_w = ps.pref_w;
			content_h = ps.pref_h;
		}

		content_child->setRect(TBRect(0, 0, content_w, content_h));
		double limit_max_w = Max(0, content_w - m_root.getRect().w);
		double limit_max_h = Max(0, content_h - m_root.getRect().h);
		m_scrollbar_x.setLimits(0, limit_max_w, m_root.getRect().w);
		m_scrollbar_y.setLimits(0, limit_max_h, m_root.getRect().h);
	}
}

void TBScrollContainer::onResized(int oldW, int oldH) {
	invalidateLayout(INVALIDATE_LAYOUT_TARGET_ONLY);
	SizeConstraints sc(getRect().w, getRect().h);
	validateLayout(sc);
}

} // namespace tb
