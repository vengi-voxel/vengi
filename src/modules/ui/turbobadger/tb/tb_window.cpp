/**
 * @file
 */

#include "tb_window.h"
#include "core/Assert.h"

namespace tb {

TBWindow::TBWindow() : m_settings(WINDOW_SETTINGS_DEFAULT) {
	setSkinBg(TBIDC("TBWindow"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	addChild(&m_mover);
	addChild(&m_resizer);
	m_mover.setSkinBg(TBIDC("TBWindow.mover"));
	m_mover.addChild(&m_textfield);
	m_textfield.setIgnoreInput(true);
	m_mover.addChild(&m_close_button);
	m_close_button.setSkinBg(TBIDC("TBWindow.close"));
	m_close_button.setIsFocusable(false);
	m_close_button.setID(TBIDC("TBWindow.close"));
	setIsGroupRoot(true);
}

TBWindow::~TBWindow() {
	m_resizer.removeFromParent();
	m_mover.removeFromParent();
	m_close_button.removeFromParent();
	m_textfield.removeFromParent();
}

TBRect TBWindow::getResizeToFitContentRect(RESIZE_FIT fit) {
	PreferredSize ps = getPreferredSize();
	int new_w = ps.pref_w;
	int new_h = ps.pref_h;
	if (fit == RESIZE_FIT_MinIMAL) {
		new_w = ps.min_w;
		new_h = ps.min_h;
	} else if (fit == RESIZE_FIT_CURRENT_OR_NEEDED) {
		new_w = Clamp(getRect().w, ps.min_w, ps.max_w);
		new_h = Clamp(getRect().h, ps.min_h, ps.max_h);
	}
	if (getParent()) {
		new_w = Min(new_w, getParent()->getRect().w);
		new_h = Min(new_h, getParent()->getRect().h);
	}
	return TBRect(getRect().x, getRect().y, new_w, new_h);
}

void TBWindow::resizeToFitContent(RESIZE_FIT fit) {
	setRect(getResizeToFitContentRect(fit));
}

void TBWindow::close() {
	die();
}

bool TBWindow::isActive() const {
	return getState(WIDGET_STATE_SELECTED);
}

TBWindow *TBWindow::getTopMostOtherWindow(bool onlyActivableWindows) {
	TBWindow *other_window = nullptr;
	TBWidget *sibling = getParent()->getLastChild();
	while (sibling && !other_window) {
		if (sibling != this)
			other_window = TBSafeCast<TBWindow>(sibling);

		if (onlyActivableWindows && other_window && !(other_window->m_settings & WINDOW_SETTINGS_CAN_ACTIVATE))
			other_window = nullptr;

		sibling = sibling->getPrev();
	}
	return other_window;
}

void TBWindow::activate() {
	if (!getParent() || !(m_settings & WINDOW_SETTINGS_CAN_ACTIVATE))
		return;
	if (isActive()) {
		// Already active, but we may still have lost focus,
		// so ensure it comes back to us.
		ensureFocus();
		return;
	}

	// Deactivate currently active window
	TBWindow *active_window = getTopMostOtherWindow(true);
	if (active_window)
		active_window->deActivate();

	// Activate this window

	setZ(WIDGET_Z_TOP);
	setWindowActiveState(true);
	ensureFocus();
}

bool TBWindow::ensureFocus() {
	// If we already have focus, we're done.
	if (focused_widget && isAncestorOf(focused_widget))
		return true;

	// Focus last focused widget (if we have one)
	bool success = false;
	if (m_last_focus.get())
		success = m_last_focus.get()->setFocus(WIDGET_FOCUS_REASON_UNKNOWN);
	// We didn't have one or failed, so try focus any child.
	if (!success)
		success = setFocusRecursive(WIDGET_FOCUS_REASON_UNKNOWN);
	return success;
}

void TBWindow::deActivate() {
	if (!isActive())
		return;
	setWindowActiveState(false);
}

void TBWindow::setWindowActiveState(bool active) {
	setState(WIDGET_STATE_SELECTED, active);
	m_mover.setState(WIDGET_STATE_SELECTED, active);
}

void TBWindow::setSettings(WINDOW_SETTINGS settings) {
	if (settings == m_settings)
		return;
	m_settings = settings;

	if (settings & WINDOW_SETTINGS_TITLEBAR) {
		if (!m_mover.getParent())
			addChild(&m_mover);
	} else if (!(settings & WINDOW_SETTINGS_TITLEBAR)) {
		m_mover.removeFromParent();
	}
	if (settings & WINDOW_SETTINGS_RESIZABLE) {
		if (!m_resizer.getParent())
			addChild(&m_resizer);
	} else if (!(settings & WINDOW_SETTINGS_RESIZABLE)) {
		m_resizer.removeFromParent();
	}
	if (settings & WINDOW_SETTINGS_CLOSE_BUTTON) {
		if (!m_close_button.getParent())
			m_mover.addChild(&m_close_button);
	} else if (!(settings & WINDOW_SETTINGS_CLOSE_BUTTON)) {
		m_close_button.removeFromParent();
	}

	// FIX: invalidate layout / resize window!
	invalidate();
}

int TBWindow::getTitleHeight() {
	if (m_settings & WINDOW_SETTINGS_TITLEBAR)
		return m_mover.getPreferredSize().pref_h;
	return 0;
}

TBRect TBWindow::getPaddingRect() {
	TBRect padding_rect = TBWidget::getPaddingRect();
	int title_height = getTitleHeight();
	padding_rect.y += title_height;
	padding_rect.h -= title_height;
	return padding_rect;
}

PreferredSize TBWindow::onCalculatePreferredSize(const SizeConstraints &constraints) {
	PreferredSize ps = onCalculatePreferredContentSize(constraints);

	// Add window skin padding
	if (TBSkinElement *e = getSkinBgElement()) {
		ps.min_w += e->padding_left + e->padding_right;
		ps.pref_w += e->padding_left + e->padding_right;
		ps.min_h += e->padding_top + e->padding_bottom;
		ps.pref_h += e->padding_top + e->padding_bottom;
	}
	// Add window title bar height
	int title_height = getTitleHeight();
	ps.min_h += title_height;
	ps.pref_h += title_height;
	return ps;
}

bool TBWindow::onEvent(const TBWidgetEvent &ev) {
	if (ev.target == &m_close_button) {
		if (ev.type == EVENT_TYPE_CLICK)
			close();
		return true;
	}
	return false;
}

void TBWindow::onAdded() {
	// If we was added last, call Activate to update status etc.
	if (getParent()->getLastChild() == this)
		activate();
}

void TBWindow::onRemove() {
	deActivate();

	// Active the top most other window
	if (TBWindow *active_window = getTopMostOtherWindow(true))
		active_window->activate();
}

void TBWindow::onChildAdded(TBWidget *child) {
	m_resizer.setZ(WIDGET_Z_TOP);
}

void TBWindow::onResized(int oldW, int oldH) {
	// Apply gravity on children
	TBWidget::onResized(oldW, oldH);
	// Manually move our own decoration children
	// FIX: Put a layout in the TBMover so we can add things there nicely.
	const int title_height = getTitleHeight();
	m_mover.setRect(TBRect(0, 0, getRect().w, title_height));

	PreferredSize ps = m_resizer.getPreferredSize();
	m_resizer.setRect(TBRect(getRect().w - ps.pref_w, getRect().h - ps.pref_h, ps.pref_w, ps.pref_h));

	const TBRect mover_rect = m_mover.getRect();
	const TBRect mover_padding_rect = m_mover.getPaddingRect();
	const int mover_padding_right = mover_rect.x + mover_rect.w - (mover_padding_rect.x + mover_padding_rect.w);
	const int button_w = m_close_button.getPreferredSize().pref_w;
	const int button_h = Max(m_close_button.getPreferredSize().pref_h, mover_padding_rect.h);
	m_close_button.setRect(
		TBRect(mover_padding_rect.x + mover_padding_rect.w - button_w, mover_padding_rect.y, button_w, button_h));

	TBRect title_rect = mover_padding_rect;
	if (m_settings & WINDOW_SETTINGS_CLOSE_BUTTON)
		title_rect.w -= mover_padding_right + button_w;
	m_textfield.setRect(title_rect);
}

} // namespace tb
