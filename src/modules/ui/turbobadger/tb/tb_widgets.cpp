/**
 * @file
 */

#include "tb_widgets.h"
#include "core/Assert.h"
#include "core/command/CommandHandler.h"
#include "tb_font_renderer.h"
#include "tb_renderer.h"
#include "tb_scroller.h"
#include "tb_system.h"
#include "tb_widget_skin_condition_context.h"
#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"
#include "tb_window.h"
#ifdef TB_ALWAYS_SHOW_EDIT_FOCUS
#include "tb_editfield.h"
#endif // TB_ALWAYS_SHOW_EDIT_FOCUS

namespace tb {

// static data
TBWidget *TBWidget::hovered_widget = nullptr;
TBWidget *TBWidget::captured_widget = nullptr;
TBWidget *TBWidget::focused_widget = nullptr;
int TBWidget::pointer_down_widget_x = 0;
int TBWidget::pointer_down_widget_y = 0;
int TBWidget::pointer_move_widget_x = 0;
int TBWidget::pointer_move_widget_y = 0;
bool TBWidget::cancel_click = false;
bool TBWidget::update_widget_states = true;
bool TBWidget::update_skin_states = true;
bool TBWidget::show_focus_state = false;

static TBHashTableAutoDeleteOf<TBWidget::TOUCH_INFO> s_touch_info;

TBWidget::TOUCH_INFO *TBWidget::getTouchInfo(uint32_t id) {
	return s_touch_info.get(id);
}

static TBWidget::TOUCH_INFO *newTouchInfo(uint32_t id) {
	core_assert(!s_touch_info.get(id));
	TBWidget::TOUCH_INFO *ti = new TBWidget::TOUCH_INFO;
	memset(ti, 0, sizeof(TBWidget::TOUCH_INFO));
	s_touch_info.add(id, ti);
	return ti;
}

static void DeleteTouchInfo(uint32_t id) {
	s_touch_info.deleteKey(id);
}

// == TBLongClickTimer ==================================================================

/** One shot timer for long click event */
class TBLongClickTimer : private TBMessageHandler {
public:
	TBLongClickTimer(TBWidget *widget, BUTTON_TYPE type) : m_widget(widget), m_type(type) {
		postMessageDelayed(TBIDC("TBLongClickTimer"), nullptr, TBSystem::getLongClickDelayMS());
	}
	virtual void onMessageReceived(TBMessage *msg) {
		core_assert(msg->message == TBIDC("TBLongClickTimer"));
		m_widget->maybeInvokeLongClickOrContextMenu(m_type);
	}

private:
	TBWidget *m_widget;
	BUTTON_TYPE m_type;
};

// == TBWidget::PaintProps ==============================================================

TBWidget::PaintProps::PaintProps() {
	// Set the default properties, used for the root widgets
	// calling InvokePaint. The base values for all inheritance.
	text_color = g_tb_skin->getDefaultTextColor();
}

// == TBWidget ==========================================================================

TBWidget::TBWidget()
	: m_parent(nullptr), m_opacity(1.F), m_state(WIDGET_STATE_NONE), m_gravity(WIDGET_GRAVITY_DEFAULT),
	  m_layout_params(nullptr), m_scroller(nullptr), m_long_click_timer(nullptr), m_packed_init(0) {
#ifdef TB_RUNTIME_DEBUG_INFO
	last_measure_time = 0;
	last_layout_time = 0;
#endif // TB_RUNTIME_DEBUG_INFO
}

TBWidget::~TBWidget() {
	core_assert(!m_parent); ///< A widget must be removed from parent before deleted
	m_packed.is_dying = true;

	// Unreference from pointer capture
	if (this == hovered_widget) {
		hovered_widget = nullptr;
	}
	if (this == captured_widget) {
		captured_widget = nullptr;
	}
	if (this == focused_widget) {
		focused_widget = nullptr;
	}

	// Unreference from touch info
	TBHashTableIteratorOf<TOUCH_INFO> it(&s_touch_info);
	while (TOUCH_INFO *ti = it.getNextContent()) {
		if (this == ti->hovered_widget) {
			ti->hovered_widget = nullptr;
		}
		if (this == ti->captured_widget) {
			ti->captured_widget = nullptr;
		}
	}

	TBWidgetListener::invokeWidgetDelete(this);
	deleteAllChildren();

	delete m_scroller;
	delete m_layout_params;

	stopLongClickTimer();

	core_assert(!m_listeners.hasLinks()); // There's still listeners added to this widget!
}

void TBWidget::setRect(const TBRect &rect) {
	if (m_rect.equals(rect)) {
		return;
	}

	TBRect old_rect = m_rect;
	m_rect = rect;

	if (old_rect.w != m_rect.w || old_rect.h != m_rect.h) {
		onResized(old_rect.w, old_rect.h);
	}

	invalidate();
}

void TBWidget::invalidate() {
	if (!getVisibilityCombined() && !m_rect.isEmpty()) {
		return;
	}
	TBWidget *tmp = this;
	while (tmp != nullptr) {
		tmp->onInvalid();
		tmp = tmp->m_parent;
	}
}

void TBWidget::invalidateStates() {
	update_widget_states = true;
	invalidateSkinStates();
}

void TBWidget::invalidateSkinStates() {
	update_skin_states = true;
}

void TBWidget::die() {
	if (m_packed.is_dying) {
		return;
	}
	m_packed.is_dying = true;
	onDie();
	if (!TBWidgetListener::invokeWidgetDying(this)) {
		// No one was interested, so die immediately.
		removeFromParent();
		delete this;
	}
}

TBWidget *TBWidget::getWidgetByIDInternal(const TBID &id, const TB_TYPE_ID typeId) {
	if (m_id == id && ((typeId == nullptr) || isOfTypeId(typeId))) {
		return this;
	}
	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		if (TBWidget *sub_child = child->getWidgetByIDInternal(id, typeId)) {
			return sub_child;
		}
	}
	return nullptr;
}

TBStr TBWidget::getTextByID(const TBID &id) {
	if (TBWidget *widget = getWidgetByID(id)) {
		return widget->getText();
	}
	return "";
}

int TBWidget::getValueByID(const TBID &id) {
	if (TBWidget *widget = getWidgetByID(id)) {
		return widget->getValue();
	}
	return 0;
}

void TBWidget::setID(const TBID &id) {
	m_id = id;
	invalidateSkinStates();
}

void TBWidget::setStateRaw(WIDGET_STATE state) {
	if (m_state == state) {
		return;
	}
	m_state = state;
	invalidate();
	invalidateSkinStates();
}

void TBWidget::setState(WIDGET_STATE state, bool on) {
	setStateRaw(on ? m_state | state : m_state & ~state);
}

WIDGET_STATE TBWidget::getAutoState() const {
	WIDGET_STATE state = m_state;
	bool add_pressed_state = !cancel_click && this == captured_widget && this == hovered_widget;
	if (add_pressed_state) {
		state |= WIDGET_STATE_PRESSED;
	}
	if (this == hovered_widget && (!m_packed.no_automatic_hover_state || add_pressed_state)) {
		state |= WIDGET_STATE_HOVERED;
	}
	if (this == focused_widget && show_focus_state) {
		state |= WIDGET_STATE_FOCUSED;
	}
#ifdef TB_ALWAYS_SHOW_EDIT_FOCUS
	else if (this == focused_widget && IsOfType<TBEditField>())
		state |= WIDGET_STATE_FOCUSED;
#endif
	return state;
}

// static
void TBWidget::setAutoFocusState(bool on) {
	if (show_focus_state == on) {
		return;
	}
	show_focus_state = on;
	if (focused_widget != nullptr) {
		focused_widget->invalidate();
	}
}

void TBWidget::setOpacity(float opacity) {
	opacity = Clamp(opacity, 0.F, 1.F);
	if (m_opacity == opacity) {
		return;
	}
	if (opacity == 0) { // Invalidate after setting opacity 0 will do nothing.
		invalidate();
	}
	m_opacity = opacity;
	invalidate();
}

void TBWidget::setVisibility(WIDGET_VISIBILITY vis) {
	if (m_packed.visibility == vis) {
		return;
	}

	// Invalidate after making it invisible will do nothing.
	if (vis != WIDGET_VISIBILITY_VISIBLE) {
		invalidate();
	}
	if (vis == WIDGET_VISIBILITY_GONE) {
		invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	}

	WIDGET_VISIBILITY old_vis = getVisibility();
	m_packed.visibility = vis;

	invalidate();
	if (old_vis == WIDGET_VISIBILITY_GONE) {
		invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	}

	onVisibilityChanged();
}

WIDGET_VISIBILITY TBWidget::getVisibility() const {
	return static_cast<WIDGET_VISIBILITY>(m_packed.visibility);
}

bool TBWidget::getVisibilityCombined() const {
	const TBWidget *tmp = this;
	while (tmp != nullptr) {
		if (tmp->getOpacity() == 0 || tmp->getVisibility() != WIDGET_VISIBILITY_VISIBLE) {
			return false;
		}
		tmp = tmp->m_parent;
	}
	return true;
}

bool TBWidget::getDisabled() const {
	const TBWidget *tmp = this;
	while (tmp != nullptr) {
		if (tmp->getState(WIDGET_STATE_DISABLED)) {
			return true;
		}
		tmp = tmp->m_parent;
	}
	return false;
}

void TBWidget::addChild(TBWidget *child, WIDGET_Z z, WIDGET_INVOKE_INFO info) {
	addChildRelative(child, z == WIDGET_Z_TOP ? WIDGET_Z_REL_AFTER : WIDGET_Z_REL_BEFORE, nullptr, info);
}

void TBWidget::addChildRelative(TBWidget *child, WIDGET_Z_REL z, TBWidget *reference, WIDGET_INVOKE_INFO info) {
	core_assert(!child->m_parent);
	child->m_parent = this;

	if (reference != nullptr) {
		if (z == WIDGET_Z_REL_BEFORE) {
			m_children.addBefore(child, reference);
		} else {
			m_children.addAfter(child, reference);
		}
	} else // If there is no reference widget, before means first and after means last.
	{
		if (z == WIDGET_Z_REL_BEFORE) {
			m_children.addFirst(child);
		} else {
			m_children.addLast(child);
		}
	}

	if (info == WIDGET_INVOKE_INFO_NORMAL) {
		onChildAdded(child);
		child->onAdded();
		TBWidgetListener::invokeWidgetAdded(this, child);
	}
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	invalidate();
	invalidateSkinStates();
}

void TBWidget::removeChild(TBWidget *child, WIDGET_INVOKE_INFO info) {
	core_assert(child->m_parent);

	if (info == WIDGET_INVOKE_INFO_NORMAL) {
		// If we're not being deleted and delete the focused widget, try
		// to keep the focus in this widget by moving it to the next widget.
		if (!m_packed.is_dying && child == focused_widget) {
			child->getEventDestination()->setFocusRecursive();
		}

		onChildRemove(child);
		child->onRemove();
		TBWidgetListener::invokeWidgetRemove(this, child);
	}

	m_children.remove(child);
	child->m_parent = nullptr;

	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	invalidate();
	invalidateSkinStates();
}

void TBWidget::deleteAllChildren() {
	while (TBWidget *child = getFirstChild()) {
		removeChild(child);
		delete child;
	}
}

void TBWidget::setZ(WIDGET_Z z) {
	if (m_parent == nullptr) {
		return;
	}
	if (z == WIDGET_Z_TOP && this == m_parent->m_children.getLast()) {
		return; // Already at the top
	}
	if (z == WIDGET_Z_BOTTOM && this == m_parent->m_children.getFirst()) {
		return; // Already at the top
	}
	TBWidget *parent = m_parent;
	parent->removeChild(this, WIDGET_INVOKE_INFO_NO_CALLBACKS);
	parent->addChild(this, z, WIDGET_INVOKE_INFO_NO_CALLBACKS);
}

void TBWidget::setGravity(WIDGET_GRAVITY g) {
	if (m_gravity == g) {
		return;
	}
	m_gravity = g;
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
}

void TBWidget::setSkinBg(const TBID &skinBg, WIDGET_INVOKE_INFO info) {
	if (skinBg == m_skin_bg) {
		return;
	}

	// Set the skin and m_skin_bg_expected. During InvokeProcess, we will detect
	// if any widget gets a different element due to conditions and strong override.
	// If that happens, OnSkinChanged will be called and m_skin_bg_expected updated to
	// match that override.
	m_skin_bg = skinBg;
	m_skin_bg_expected = skinBg;

	invalidate();
	invalidateSkinStates();
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);

	if (info == WIDGET_INVOKE_INFO_NORMAL) {
		onSkinChanged();
	}
}

TBSkinElement *TBWidget::getSkinBgElement() {
	TBWidgetSkinConditionContext context(this);
	WIDGET_STATE state = getAutoState();
	return g_tb_skin->getSkinElementStrongOverride(m_skin_bg, static_cast<SKIN_STATE>(state), context);
}

TBWidget *TBWidget::findScrollableWidget(bool scrollX, bool scrollY) {
	TBWidget *candidate = this;
	while (candidate != nullptr) {
		ScrollInfo scroll_info = candidate->getScrollInfo();
		if ((scrollX && scroll_info.canScrollX()) || (scrollY && scroll_info.canScrollY())) {
			return candidate;
		}
		candidate = candidate->getParent();
	}
	return nullptr;
}

TBScroller *TBWidget::findStartedScroller() {
	TBWidget *candidate = this;
	while (candidate != nullptr) {
		if ((candidate->m_scroller != nullptr) && candidate->m_scroller->isStarted()) {
			return candidate->m_scroller;
		}
		candidate = candidate->getParent();
	}
	return nullptr;
}

TBScroller *TBWidget::getReadyScroller(bool scrollX, bool scrollY) {
	if (TBScroller *scroller = findStartedScroller()) {
		return scroller;
	}
	// We didn't have any active scroller, so create one for the nearest scrollable parent.
	if (TBWidget *scrollable_widget = findScrollableWidget(scrollX, scrollY)) {
		return scrollable_widget->getScroller();
	}
	return nullptr;
}

TBScroller *TBWidget::getScroller() {
	if (m_scroller == nullptr) {
		m_scroller = new TBScroller(this);
	}
	return m_scroller;
}

void TBWidget::scrollToSmooth(int x, int y) {
	ScrollInfo info = getScrollInfo();
	int dx = x - info.x;
	int dy = y - info.y;
	if (TBScroller *scroller = getReadyScroller(dx != 0, dy != 0)) {
		scroller->onScrollBy(dx, dy, false);
	}
}

void TBWidget::scrollBySmooth(int dx, int dy) {
	// Clip the values to the scroll limits, so we don't
	// scroll any parents.
	// int x = Clamp(info.x + dx, info.min_x, info.max_x);
	// int y = Clamp(info.y + dy, info.min_y, info.max_y);
	// dx = x - info.x;
	// dy = y - info.y;
	if ((dx == 0) && (dy == 0)) {
		return;
	}

	if (TBScroller *scroller = getReadyScroller(dx != 0, dy != 0)) {
		scroller->onScrollBy(dx, dy, true);
	}
}

void TBWidget::scrollBy(int dx, int dy) {
	ScrollInfo info = getScrollInfo();
	scrollTo(info.x + dx, info.y + dy);
}

void TBWidget::scrollByRecursive(int &dx, int &dy) {
	TBWidget *tmp = this;
	while (tmp != nullptr) {
		ScrollInfo old_info = tmp->getScrollInfo();
		tmp->scrollTo(old_info.x + dx, old_info.y + dy);
		ScrollInfo new_info = tmp->getScrollInfo();
		dx -= new_info.x - old_info.x;
		dy -= new_info.y - old_info.y;
		if ((dx == 0) && (dy == 0)) {
			break;
		}
		tmp = tmp->m_parent;
	}
}

void TBWidget::scrollIntoViewRecursive() {
	TBRect scroll_to_rect = m_rect;
	TBWidget *tmp = this;
	while (tmp->m_parent != nullptr) {
		tmp->m_parent->scrollIntoView(scroll_to_rect);
		scroll_to_rect.x += tmp->m_parent->m_rect.x;
		scroll_to_rect.y += tmp->m_parent->m_rect.y;
		tmp = tmp->m_parent;
	}
}

void TBWidget::scrollIntoView(const TBRect &rect) {
	const ScrollInfo info = getScrollInfo();
	int new_x = info.x;
	int new_y = info.y;

	const TBRect visible_rect = getPaddingRect().offset(info.x, info.y);

	if (rect.y <= visible_rect.y) {
		new_y = rect.y;
	} else if (rect.y + rect.h > visible_rect.y + visible_rect.h) {
		new_y = rect.y + rect.h - visible_rect.h;
	}

	if (rect.x <= visible_rect.x) {
		new_x = rect.x;
	} else if (rect.x + rect.w > visible_rect.x + visible_rect.w) {
		new_x = rect.x + rect.w - visible_rect.w;
	}

	scrollTo(new_x, new_y);
}

bool TBWidget::setFocus(WIDGET_FOCUS_REASON reason, WIDGET_INVOKE_INFO info) {
	if (focused_widget == this) {
		return true;
	}
	if (getDisabled() || !getIsFocusable() || !getVisibilityCombined() || getIsDying()) {
		return false;
	}

	// Update windows last focus
	TBWindow *window = getParentWindow();
	if (window != nullptr) {
		window->setLastFocus(this);
		// If not active, just return. We should get focus when the window is activated.
		// Exception for windows that doesn't activate. They may contain focusable widgets.
		if (!window->isActive() && ((window->getSettings() & WINDOW_SETTINGS_CAN_ACTIVATE) != 0U)) {
			return true;
		}
	}

	if (focused_widget != nullptr) {
		focused_widget->invalidate();
		focused_widget->invalidateSkinStates();
	}

	TBWidgetSafePointer old_focus(focused_widget);
	focused_widget = this;

	invalidate();
	invalidateSkinStates();

	if (reason == WIDGET_FOCUS_REASON_NAVIGATION) {
		scrollIntoViewRecursive();
	}

	if (info == WIDGET_INVOKE_INFO_NORMAL) {
		// A lot of weird bugs could happen if people mess with focus from OnFocusChanged.
		// Take some precaution and detect if it change again after OnFocusChanged(false).
		if (TBWidget *old = old_focus.get()) {
			// The currently focused widget still has the pressed state set by the emulated click
			// (By keyboard), so unset it before we unfocus it so it's not stuck in pressed state.
			if (old->m_packed.has_key_pressed_state) {
				old->setState(WIDGET_STATE_PRESSED, false);
				old->m_packed.has_key_pressed_state = false;
			}
			old->onFocusChanged(false);
		}
		if (old_focus.get() != nullptr) {
			TBWidgetListener::invokeWidgetFocusChanged(old_focus.get(), false);
		}
		if ((focused_widget != nullptr) && focused_widget == this) {
			focused_widget->onFocusChanged(true);
		}
		if ((focused_widget != nullptr) && focused_widget == this) {
			TBWidgetListener::invokeWidgetFocusChanged(focused_widget, true);
		}
	}
	return true;
}

bool TBWidget::setFocusRecursive(WIDGET_FOCUS_REASON reason) {
	// Search for a child widget that accepts focus
	TBWidget *child = getFirstChild();
	while (child != nullptr) {
		if (child->setFocus(WIDGET_FOCUS_REASON_UNKNOWN)) {
			return true;
		}
		child = child->getNextDeep(this);
	}
	return false;
}

bool TBWidget::moveFocus(bool forward) {
	TBWidget *origin = focused_widget;
	if (origin == nullptr) {
		origin = this;
	}

	TBWidget *root = origin->getParentWindow();
	if (root == nullptr) {
		root = origin->getParentRoot();
	}

	TBWidget *current = origin;
	while (current != nullptr) {
		current = forward ? current->getNextDeep(root) : current->getPrevDeep();
		// Wrap around if we reach the end/beginning
		if ((current == nullptr) || !root->isAncestorOf(current)) {
			current = forward ? root : root->getLastLeaf();
		}
		// Break if we reached the origin again (we're not finding anything else)
		if (current == origin) {
			break;
		}
		// Try to focus what we found
		if ((current != nullptr) && current->setFocus(WIDGET_FOCUS_REASON_NAVIGATION)) {
			return true;
		}
	}
	return false;
}

TBWidget *TBWidget::getNextDeep(const TBWidget *boundingAncestor) const {
	if (m_children.getFirst() != nullptr) {
		return getFirstChild();
	}
	for (const TBWidget *widget = this; widget != boundingAncestor; widget = widget->m_parent) {
		if (widget->next != nullptr) {
			return widget->getNext();
		}
	}
	return nullptr;
}

TBWidget *TBWidget::getPrevDeep() const {
	if (prev == nullptr) {
		return m_parent;
	}
	TBWidget *widget = getPrev();
	while (widget->m_children.getLast() != nullptr) {
		widget = widget->getLastChild();
	}
	return widget;
}

TBWidget *TBWidget::getLastLeaf() const {
	if (TBWidget *widget = getLastChild()) {
		while (widget->getLastChild() != nullptr) {
			widget = widget->getLastChild();
		}
		return widget;
	}
	return nullptr;
}

bool TBWidget::getIsInteractable() const {
	return !(m_opacity == 0 || getIgnoreInput() || getState(WIDGET_STATE_DISABLED) || getIsDying() ||
			 getVisibility() != WIDGET_VISIBILITY_VISIBLE);
}

WIDGET_HIT_STATUS TBWidget::getHitStatus(int x, int y) {
	if (!getIsInteractable()) {
		return WIDGET_HIT_STATUS_NO_HIT;
	}
	if (x < 0 || y < 0) {
		return WIDGET_HIT_STATUS_NO_HIT;
	}
	if (x >= m_rect.w || y >= m_rect.h) {
		return WIDGET_HIT_STATUS_NO_HIT;
	}
	return WIDGET_HIT_STATUS_HIT;
}

TBWidget *TBWidget::getWidgetAt(int x, int y, bool includeChildren) const {
	int child_translation_x;
	int child_translation_y;
	getChildTranslation(child_translation_x, child_translation_y);
	x -= child_translation_x;
	y -= child_translation_y;

	TBWidget *tmp = getFirstChild();
	TBWidget *last_match = nullptr;
	while (tmp != nullptr) {
		WIDGET_HIT_STATUS hit_status = tmp->getHitStatus(x - tmp->m_rect.x, y - tmp->m_rect.y);
		if (hit_status != 0U) {
			if (includeChildren && hit_status != WIDGET_HIT_STATUS_HIT_NO_CHILDREN) {
				last_match = tmp->getWidgetAt(x - tmp->m_rect.x, y - tmp->m_rect.y, includeChildren);
				if (last_match == nullptr) {
					last_match = tmp;
				}
			} else {
				last_match = tmp;
			}
		}
		tmp = tmp->getNext();
	}
	return last_match;
}

TBWidget *TBWidget::getChildFromIndex(int index) const {
	int i = 0;
	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		if (i++ == index) {
			return child;
		}
	}
	return nullptr;
}

int TBWidget::getIndexFromChild(TBWidget *child) const {
	core_assert(child->getParent() == this);
	int i = 0;
	for (TBWidget *tmp = getFirstChild(); tmp != nullptr; tmp = tmp->getNext(), i++) {
		if (tmp == child) {
			return i;
		}
	}
	return -1; ///< Should not happen!
}

bool TBWidget::isAncestorOf(TBWidget *otherWidget) const {
	while (otherWidget != nullptr) {
		if (otherWidget == this) {
			return true;
		}
		otherWidget = otherWidget->m_parent;
	}
	return false;
}

bool TBWidget::isEventDestinationFor(TBWidget *otherWidget) const {
	while (otherWidget != nullptr) {
		if (otherWidget == this) {
			return true;
		}
		otherWidget = otherWidget->getEventDestination();
	}
	return false;
}

TBWidget *TBWidget::getParentRoot() {
	TBWidget *tmp = this;
	while (tmp->m_parent != nullptr) {
		tmp = tmp->m_parent;
	}
	return tmp;
}

TBWindow *TBWidget::getParentWindow() {
	TBWidget *tmp = this;
	while ((tmp != nullptr) && !tmp->isOfType<TBWindow>()) {
		tmp = tmp->m_parent;
	}
	return static_cast<TBWindow *>(tmp);
}

void TBWidget::addListener(TBWidgetListener *listener) {
	m_listeners.addLast(listener);
}

void TBWidget::removeListener(TBWidgetListener *listener) {
	m_listeners.remove(listener);
}

bool TBWidget::hasListener(TBWidgetListener *listener) const {
	return m_listeners.containsLink(listener);
}

void TBWidget::onPaintChildren(const PaintProps &paintProps) {
	if (m_children.getFirst() == nullptr) {
		return;
	}

	// Translate renderer with child translation
	int child_translation_x;
	int child_translation_y;
	getChildTranslation(child_translation_x, child_translation_y);
	g_renderer->translate(child_translation_x, child_translation_y);

	TBRect clip_rect = g_renderer->getClipRect();

	// Invoke paint on all children that are in the current visible rect.
	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		if (clip_rect.intersects(child->m_rect)) {
			child->invokePaint(paintProps);
		}
	}

	// Invoke paint of overlay elements on all children that are in the current visible rect.
	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		if (clip_rect.intersects(child->m_rect) && child->getVisibility() == WIDGET_VISIBILITY_VISIBLE) {
			TBSkinElement *skin_element = child->getSkinBgElement();
			if ((skin_element != nullptr) && skin_element->hasOverlayElements()) {
				// Update the renderer with the widgets opacity
				WIDGET_STATE state = child->getAutoState();
				float old_opacity = g_renderer->getOpacity();
				float opacity = old_opacity * child->calculateOpacityInternal(state, skin_element);
				if (opacity > 0) {
					g_renderer->setOpacity(opacity);

					TBWidgetSkinConditionContext context(child);
					g_tb_skin->paintSkinOverlay(child->m_rect, skin_element, static_cast<SKIN_STATE>(state), context);

					g_renderer->setOpacity(old_opacity);
				}
			}
		}
	}

	// Draw generic focus skin if the focused widget is one of the children, and the skin
	// doesn't have a skin state for focus which would already be painted.
	if ((focused_widget != nullptr) && focused_widget->m_parent == this) {
		TBWidgetSkinConditionContext context(focused_widget);
		TBSkinElement *skin_element = focused_widget->getSkinBgElement();
		if ((skin_element == nullptr) || !skin_element->hasState(SKIN_STATE_FOCUSED, context)) {
			WIDGET_STATE state = focused_widget->getAutoState();
			if ((state & SKIN_STATE_FOCUSED) != 0) {
				g_tb_skin->paintSkin(focused_widget->m_rect, TBIDC("generic_focus"), static_cast<SKIN_STATE>(state),
									 context);
			}
		}
	}

	g_renderer->translate(-child_translation_x, -child_translation_y);
}

void TBWidget::onResized(int oldW, int oldH) {
	int dw = m_rect.w - oldW;
	int dh = m_rect.h - oldH;
	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		if (child->getVisibility() == WIDGET_VISIBILITY_GONE) {
			continue;
		}
		TBRect rect = child->m_rect;
		if (((child->m_gravity & WIDGET_GRAVITY_LEFT) != 0U) && ((child->m_gravity & WIDGET_GRAVITY_RIGHT) != 0U)) {
			rect.w += dw;
		} else if ((child->m_gravity & WIDGET_GRAVITY_RIGHT) != 0U) {
			rect.x += dw;
		}
		if (((child->m_gravity & WIDGET_GRAVITY_TOP) != 0U) && ((child->m_gravity & WIDGET_GRAVITY_BOTTOM) != 0U)) {
			rect.h += dh;
		} else if ((child->m_gravity & WIDGET_GRAVITY_BOTTOM) != 0U) {
			rect.y += dh;
		}
		child->setRect(rect);
	}
}

void TBWidget::onInflateChild(TBWidget *child) {
	if (child->getVisibility() == WIDGET_VISIBILITY_GONE) {
		return;
	}

	// If the child pull towards only one edge (per axis), stick to that edge
	// and use the preferred size. Otherwise fill up all available space.
	TBRect padding_rect = getPaddingRect();
	TBRect child_rect = padding_rect;
	WIDGET_GRAVITY gravity = child->getGravity();
	bool fill_x = ((gravity & WIDGET_GRAVITY_LEFT) != 0U) && ((gravity & WIDGET_GRAVITY_RIGHT) != 0U);
	bool fill_y = ((gravity & WIDGET_GRAVITY_TOP) != 0U) && ((gravity & WIDGET_GRAVITY_BOTTOM) != 0U);
	if (!fill_x || !fill_y) {
		PreferredSize ps = child->getPreferredSize();
		if (!fill_x) {
			child_rect.w = ps.pref_w;
			if ((gravity & WIDGET_GRAVITY_RIGHT) != 0U) {
				child_rect.x = padding_rect.x + padding_rect.w - child_rect.w;
			}
		}
		if (!fill_y) {
			child_rect.h = ps.pref_h;
			if ((gravity & WIDGET_GRAVITY_BOTTOM) != 0U) {
				child_rect.y = padding_rect.y + padding_rect.h - child_rect.h;
			}
		}
	}
	child->setRect(child_rect);
}

TBRect TBWidget::getPaddingRect() {
	TBRect padding_rect(0, 0, m_rect.w, m_rect.h);
	if (TBSkinElement *e = getSkinBgElement()) {
		padding_rect.x += e->padding_left;
		padding_rect.y += e->padding_top;
		padding_rect.w -= e->padding_left + e->padding_right;
		padding_rect.h -= e->padding_top + e->padding_bottom;
	}
	return padding_rect;
}

PreferredSize TBWidget::onCalculatePreferredContentSize(const SizeConstraints &constraints) {
	// The default preferred size is calculated to satisfy the children
	// in the best way. Since this is the default, it's probably not a
	// layout widget and children are resized purely by gravity.

	// Allow this widget a larger maximum if our gravity wants both ways,
	// otherwise don't grow more than the largest child.
	bool apply_max_w = !(((m_gravity & WIDGET_GRAVITY_LEFT) != 0U) && ((m_gravity & WIDGET_GRAVITY_RIGHT) != 0U));
	bool apply_max_h = !(((m_gravity & WIDGET_GRAVITY_TOP) != 0U) && ((m_gravity & WIDGET_GRAVITY_BOTTOM) != 0U));
	bool has_layouting_children = false;
	PreferredSize ps;

	TBSkinElement *bg_skin = getSkinBgElement();
	int horizontal_padding = bg_skin != nullptr ? bg_skin->padding_left + bg_skin->padding_right : 0;
	int vertical_padding = bg_skin != nullptr ? bg_skin->padding_top + bg_skin->padding_bottom : 0;
	SizeConstraints inner_sc = constraints.constrainByPadding(horizontal_padding, vertical_padding);

	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		if (child->getVisibility() == WIDGET_VISIBILITY_GONE) {
			continue;
		}
		if (!has_layouting_children) {
			has_layouting_children = true;
			if (apply_max_w) {
				ps.max_w = 0;
			}
			if (apply_max_h) {
				ps.max_h = 0;
			}
		}
		PreferredSize child_ps = child->getPreferredSize(inner_sc);
		ps.pref_w = Max(ps.pref_w, child_ps.pref_w);
		ps.pref_h = Max(ps.pref_h, child_ps.pref_h);
		ps.min_w = Max(ps.min_w, child_ps.min_w);
		ps.min_h = Max(ps.min_h, child_ps.min_h);
		if (apply_max_w) {
			ps.max_w = Max(ps.max_w, child_ps.max_w);
		}
		if (apply_max_h) {
			ps.max_h = Max(ps.max_h, child_ps.max_h);
		}
		ps.size_dependency |= child_ps.size_dependency;
	}

	return ps;
}

PreferredSize TBWidget::onCalculatePreferredSize(const SizeConstraints &constraints) {
	PreferredSize ps = onCalculatePreferredContentSize(constraints);
	core_assert(ps.pref_w >= ps.min_w);
	core_assert(ps.pref_h >= ps.min_h);

	if (TBSkinElement *e = getSkinBgElement()) {
		// Override the widgets preferences with skin attributes that has been specified.
		// If not set by the widget, calculate based on the intrinsic size of the skin.

		const int skin_intrinsic_w = e->getIntrinsicWidth();
		if (e->getPrefWidth() != SKIN_VALUE_NOT_SPECIFIED) {
			ps.pref_w = e->getPrefWidth();
		} else if (ps.pref_w == 0 && skin_intrinsic_w != SKIN_VALUE_NOT_SPECIFIED) {
			ps.pref_w = skin_intrinsic_w;
		} else {
			// Grow by padding to get the preferred size from preferred content size.
			ps.min_w += e->padding_left + e->padding_right;
			ps.pref_w += e->padding_left + e->padding_right;
		}

		const int skin_intrinsic_h = e->getIntrinsicHeight();
		if (e->getPrefHeight() != SKIN_VALUE_NOT_SPECIFIED) {
			ps.pref_h = e->getPrefHeight();
		} else if (ps.pref_h == 0 && skin_intrinsic_h != SKIN_VALUE_NOT_SPECIFIED) {
			ps.pref_h = skin_intrinsic_h;
		} else {
			// Grow by padding to get the preferred size from preferred content size.
			ps.min_h += e->padding_top + e->padding_bottom;
			ps.pref_h += e->padding_top + e->padding_bottom;
		}

		if (e->getMinWidth() != SKIN_VALUE_NOT_SPECIFIED) {
			ps.min_w = e->getMinWidth();
		} else {
			ps.min_w = Max(ps.min_w, e->getIntrinsicMinWidth());
		}

		if (e->getMinHeight() != SKIN_VALUE_NOT_SPECIFIED) {
			ps.min_h = e->getMinHeight();
		} else {
			ps.min_h = Max(ps.min_h, e->getIntrinsicMinHeight());
		}

		if (e->getMaxWidth() != SKIN_VALUE_NOT_SPECIFIED) {
			ps.max_w = e->getMaxWidth();
		} else {
			ps.max_w += e->padding_left + e->padding_right;
		}

		if (e->getMaxHeight() != SKIN_VALUE_NOT_SPECIFIED) {
			ps.max_h = e->getMaxHeight();
		} else {
			ps.max_h += e->padding_top + e->padding_bottom;
		}

		// Sanitize result
		ps.pref_w = Max(ps.pref_w, ps.min_w);
		ps.pref_h = Max(ps.pref_h, ps.min_h);
	}
	return ps;
}

PreferredSize TBWidget::getPreferredSize(const SizeConstraints &inConstraints) {
	SizeConstraints constraints(inConstraints);
	if (m_layout_params != nullptr) {
		constraints = constraints.constrainByLayoutParams(*m_layout_params);
	}

	// Returned cached result if valid and the constraints are the same.
	if (m_packed.is_cached_ps_valid) {
		if (m_cached_sc == constraints ||
			m_cached_ps.size_dependency == SIZE_DEP_NONE /*||
			// FIX: These optimizations would probably be good. Keeping
			//      disabled for now because it needs testing.
			// If *only* width depend on height, only the height matter
			(m_cached_ps.size_dependency == SIZE_DEP_WIDTH_DEPEND_ON_HEIGHT &&
			m_cached_sc.available_h == constraints.available_h) ||
			// If *only* height depend on width, only the width matter
			(m_cached_ps.size_dependency == SIZE_DEP_HEIGHT_DEPEND_ON_WIDTH &&
			m_cached_sc.available_w == constraints.available_w)*/)
		{
			return m_cached_ps;
		}
	}

	// Measure and save to cache
	TB_IF_DEBUG_SETTING(LAYOUT_PS_DEBUGGING, last_measure_time = TBSystem::getTimeMS());
	m_packed.is_cached_ps_valid = 1;
	m_cached_ps = onCalculatePreferredSize(constraints);
	m_cached_sc = constraints;

	// Override the calculated ps with any specified layout parameter.
	if (m_layout_params != nullptr) {
#define LP_OVERRIDE(param)                                                                                             \
	if (m_layout_params->param != LayoutParams::UNSPECIFIED)                                                           \
		m_cached_ps.param = m_layout_params->param;
		LP_OVERRIDE(min_w);
		LP_OVERRIDE(min_h);
		LP_OVERRIDE(max_w);
		LP_OVERRIDE(max_h);
		LP_OVERRIDE(pref_w);
		LP_OVERRIDE(pref_h);

		// Sanitize results
		m_cached_ps.max_w = Max(m_cached_ps.max_w, m_cached_ps.min_w);
		m_cached_ps.max_h = Max(m_cached_ps.max_h, m_cached_ps.min_h);
		m_cached_ps.pref_w = Max(m_cached_ps.pref_w, m_cached_ps.min_w);
		m_cached_ps.pref_h = Max(m_cached_ps.pref_h, m_cached_ps.min_h);
	}
	return m_cached_ps;
}

void TBWidget::setLayoutParams(const LayoutParams &lp) {
	if (m_layout_params == nullptr) {
		m_layout_params = new LayoutParams;
	}
	if (m_layout_params == nullptr) {
		return;
	}
	*m_layout_params = lp;
	m_packed.is_cached_ps_valid = 0;
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
}

void TBWidget::invalidateLayout(INVALIDATE_LAYOUT il) {
	m_packed.is_cached_ps_valid = 0;
	if (getVisibility() == WIDGET_VISIBILITY_GONE) {
		return;
	}
	invalidate();
	if (il == INVALIDATE_LAYOUT_RECURSIVE && (m_parent != nullptr)) {
		m_parent->invalidateLayout(il);
	}
}

void TBWidget::invokeProcess() {
	invokeSkinUpdatesInternal(false);
	invokeProcessInternal();
}

void TBWidget::invokeSkinUpdatesInternal(bool forceUpdate) {
	if (!update_skin_states && !forceUpdate) {
		return;
	}
	update_skin_states = false;

	// Check if the skin we get is different from what we expect. That might happen
	// if the skin has some strong override dependant a condition that has changed.
	// If that happens, call OnSkinChanged so the widget can react to that, and
	// invalidate layout to apply new skin properties.
	if (TBSkinElement *skin_elm = getSkinBgElement()) {
		if (skin_elm->id != m_skin_bg_expected) {
			onSkinChanged();
			m_skin_bg_expected = skin_elm->id;
			invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
		}
	}

	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		child->invokeSkinUpdatesInternal(true);
	}
}

void TBWidget::invokeProcessInternal() {
	onProcess();

	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		child->invokeProcessInternal();
	}

	onProcessAfterChildren();
}

void TBWidget::invokeProcessStates(bool forceUpdate) {
	if (!update_widget_states && !forceUpdate) {
		return;
	}
	update_widget_states = false;

	onProcessStates();

	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		child->invokeProcessStates(true);
	}
}

float TBWidget::calculateOpacityInternal(WIDGET_STATE state, TBSkinElement *skinElement) const {
	float opacity = m_opacity;
	if (skinElement != nullptr) {
		opacity *= skinElement->opacity;
	}
	if ((state & WIDGET_STATE_DISABLED) != 0U) {
		opacity *= g_tb_skin->getDefaultDisabledOpacity();
	}
	return Clamp(opacity, 0.F, 1.F);
}

void TBWidget::invokePaint(const PaintProps &parentPaintProps) {
	// Don't paint invisible widgets
	if (m_opacity == 0 || m_rect.isEmpty() || getVisibility() != WIDGET_VISIBILITY_VISIBLE) {
		return;
	}

	WIDGET_STATE state = getAutoState();
	TBSkinElement *skin_element = getSkinBgElement();

	// Multiply current opacity with widget opacity, skin opacity and state opacity.
	float old_opacity = g_renderer->getOpacity();
	float opacity = old_opacity * calculateOpacityInternal(state, skin_element);
	if (opacity == 0) {
		return;
	}

	// FIX: This does not give the correct result! Must use a new render target!
	g_renderer->setOpacity(opacity);

	int trns_x = m_rect.x;
	int trns_y = m_rect.y;
	g_renderer->translate(trns_x, trns_y);

	// Paint background skin
	TBRect local_rect(0, 0, m_rect.w, m_rect.h);
	TBWidgetSkinConditionContext context(this);
	TBSkinElement *used_element =
		g_tb_skin->paintSkin(local_rect, skin_element, static_cast<SKIN_STATE>(state), context);
	core_assert(!!used_element == !!skin_element);

	TB_IF_DEBUG_SETTING(LAYOUT_BOUNDS, g_tb_skin->paintRect(local_rect, TBColor(255, 255, 255, 50), 1));

	// Inherit properties from parent if not specified in the used skin for this widget.
	PaintProps paint_props = parentPaintProps;
	if ((used_element != nullptr) && used_element->text_color != 0) {
		paint_props.text_color = used_element->text_color;
	}

	// Paint content
	onPaint(paint_props);

	if (used_element != nullptr) {
		g_renderer->translate(used_element->content_ofs_x, used_element->content_ofs_y);
	}

	// Paint children
	onPaintChildren(paint_props);

#ifdef TB_RUNTIME_DEBUG_INFO
	if (TB_DEBUG_SETTING(LAYOUT_PS_DEBUGGING)) {
		// Layout debug painting. Paint recently layouted widgets with red and
		// recently measured widgets with yellow.
		// Invalidate to keep repainting until we've timed out (so it's removed).
		const double debug_time = 300;
		const double now = TBSystem::getTimeMS();
		if (now < last_layout_time + debug_time) {
			g_tb_skin->paintRect(local_rect, TBColor(255, 30, 30, 200), 1);
			invalidate();
		}
		if (now < last_measure_time + debug_time) {
			g_tb_skin->paintRect(local_rect.shrink(1, 1), TBColor(255, 255, 30, 200), 1);
			invalidate();
		}
	}
#endif // TB_RUNTIME_DEBUG_INFO

	if (used_element != nullptr) {
		g_renderer->translate(-used_element->content_ofs_x, -used_element->content_ofs_y);
	}

	g_renderer->translate(-trns_x, -trns_y);
	g_renderer->setOpacity(old_opacity);
}

bool TBWidget::invokeEvent(TBWidgetEvent &ev) {
	ev.target = this;

	// First call the global listener about this event.
	// Who knows, maybe some listener will block the event or cause us
	// to be deleted.
	TBWidgetSafePointer this_widget(this);
	if (TBWidgetListener::invokeWidgetInvokeEvent(this, ev)) {
		return true;
	}

	if (this_widget.get() == nullptr) {
		return true; // We got removed so we actually handled this event.
	}

	if (ev.type == EVENT_TYPE_CHANGED) {
		invalidateSkinStates();
		m_connection.syncFromWidget(this);
	}

	if (this_widget.get() == nullptr) {
		return true; // We got removed so we actually handled this event.
	}

	// Always update states after some event types.
	switch (ev.type) {
	case EVENT_TYPE_CLICK:
	case EVENT_TYPE_LONG_CLICK:
	case EVENT_TYPE_CHANGED:
	case EVENT_TYPE_KEY_DOWN:
	case EVENT_TYPE_KEY_UP:
		invalidateStates();
		break;
	default:
		break;
	}

	// Call onEvent on this widgets and travel up through its parents if not handled.
	bool handled = false;
	TBWidget *tmp = this;
	while ((tmp != nullptr) && !(handled = tmp->onEvent(ev))) {
		tmp = tmp->getEventDestination();
	}
	return handled;
}

void TBWidget::execute(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[4096];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';

	tb::TBWidgetEvent ev(tb::EVENT_TYPE_COMMAND);
	ev.string = buf;
	invokeEvent(ev);

	core::executeCommands(core::String(buf));
	va_end(args);
}

void TBWidget::startLongClickTimer(BUTTON_TYPE type) {
	stopLongClickTimer();
	m_long_click_timer = new TBLongClickTimer(this, type);
}

void TBWidget::stopLongClickTimer() {
	if (m_long_click_timer == nullptr) {
		return;
	}
	delete m_long_click_timer;
	m_long_click_timer = nullptr;
}

bool TBWidget::invokePointerDown(int x, int y, int clickCount, MODIFIER_KEYS modifierkeys, BUTTON_TYPE type) {
	if (captured_widget == nullptr) {
		setCapturedWidget(getWidgetAt(x, y, true));
		setHoveredWidget(captured_widget, type != 0U);
		// captured_button = button;

		// Hide focus when we use the pointer, if it's not on the focused widget.
		if (focused_widget != captured_widget) {
			setAutoFocusState(false);
		}

		// Start long click timer. Only for touch events for now.
		if (type == TB_TOUCH && (captured_widget != nullptr) && captured_widget->getWantLongClick()) {
			captured_widget->startLongClickTimer(type);
		}

		// Get the closest parent window and bring it to the top
		TBWindow *window = captured_widget != nullptr ? captured_widget->getParentWindow() : nullptr;
		if (window != nullptr) {
			window->activate();
		}
	}
	if (captured_widget != nullptr) {
		// Check if there's any started scroller that should be stopped.
		TBWidget *tmp = captured_widget;
		while (tmp != nullptr) {
			if ((tmp->m_scroller != nullptr) && tmp->m_scroller->isStarted()) {
				// When we touch down to stop a scroller, we don't
				// want the touch to end up causing a click.
				cancel_click = true;
				tmp->m_scroller->stop();
				break;
			}
			tmp = tmp->getParent();
		}

		// Focus the captured widget or the closest
		// focusable parent if it isn't focusable.
		TBWidget *focus_target = captured_widget;
		while (focus_target != nullptr) {
			if (focus_target->setFocus(WIDGET_FOCUS_REASON_POINTER)) {
				break;
			}
			focus_target = focus_target->m_parent;
		}
	}
	if (captured_widget != nullptr) {
		captured_widget->convertFromRoot(x, y);
		pointer_move_widget_x = pointer_down_widget_x = x;
		pointer_move_widget_y = pointer_down_widget_y = y;
		TBWidgetEvent ev(EVENT_TYPE_POINTER_DOWN, x, y, type, modifierkeys);
		ev.count = clickCount;
		captured_widget->invokeEvent(ev);

		// Return true when captured instead of InvokeEvent result. If a widget is
		// hit is more interesting for callers than if the event was handled or not.
		return true;
	}
	return false;
}

bool TBWidget::invokePointerUp(int x, int y, MODIFIER_KEYS modifierkeys, BUTTON_TYPE type) {
	if (captured_widget != nullptr) {
		captured_widget->convertFromRoot(x, y);
		TBWidgetEvent ev_up(EVENT_TYPE_POINTER_UP, x, y, type, modifierkeys);
		captured_widget->invokeEvent(ev_up);
		if (captured_widget != nullptr) {
			if (!cancel_click) {
				if (captured_widget->getHitStatus(x, y) != WIDGET_HIT_STATUS_NO_HIT) {
					TBWidgetEvent ev_click(EVENT_TYPE_CLICK, x, y, type, modifierkeys);
					captured_widget->invokeEvent(ev_click);
				}
			}
			if (captured_widget != nullptr) { // && button == captured_button
				captured_widget->releaseCapture();
			}
		}

		// Return true when captured instead of invokeEvent result. If a widget is
		// hit is more interesting for callers than if the event was handled or not.
		return true;
	}
	return false;
}

void TBWidget::maybeInvokeLongClickOrContextMenu(BUTTON_TYPE type) {
	stopLongClickTimer();
	if (captured_widget == this && !cancel_click &&
		(captured_widget->getHitStatus(pointer_move_widget_x, pointer_move_widget_y) != 0U)) {
		// Invoke long click
		TBWidgetEvent ev_long_click(EVENT_TYPE_LONG_CLICK, pointer_move_widget_x, pointer_move_widget_y, type,
									TB_MODIFIER_NONE);
		bool handled = captured_widget->invokeEvent(ev_long_click);
		if (!handled) {
			// Long click not handled so invoke a context menu event instead
			TBWidgetEvent ev_context_menu(EVENT_TYPE_CONTEXT_MENU, pointer_move_widget_x, pointer_move_widget_y, type,
										  TB_MODIFIER_NONE);
			handled = captured_widget->invokeEvent(ev_context_menu);
		}
		// If any event was handled, suppress click when releasing pointer.
		if (handled) {
			cancel_click = true;
		}
	}
}

void TBWidget::invokePointerMove(int x, int y, MODIFIER_KEYS modifierkeys, BUTTON_TYPE type) {
	const bool touch = type == TB_TOUCH;
	setHoveredWidget(getWidgetAt(x, y, true), touch);

	TBWidget *target = captured_widget != nullptr ? captured_widget : hovered_widget;
	if (target != nullptr) {
		target->convertFromRoot(x, y);
		pointer_move_widget_x = x;
		pointer_move_widget_y = y;

		TBWidgetEvent ev(EVENT_TYPE_POINTER_MOVE, x, y, type, modifierkeys);

		if (target->invokeEvent(ev)) {
			return;
		}

		// The move event was not handled, so handle panning of scrollable widgets.
		handlePanningOnMove(x, y);
	}
}

void TBWidget::handlePanningOnMove(int x, int y) {
	if (captured_widget == nullptr) {
		return;
	}

	// Check pointer movement
	const int dx = pointer_down_widget_x - x;
	const int dy = pointer_down_widget_y - y;
	const int threshold = TBSystem::getPanThreshold();
	const bool maybe_start_panning_x = Abs(dx) >= threshold;
	const bool maybe_start_panning_y = Abs(dy) >= threshold;

	// Do panning, or attempt starting panning (we don't know if any widget is scrollable yet)
	if (captured_widget->m_packed.is_panning || maybe_start_panning_x || maybe_start_panning_y) {
		// The threshold is met for not invoking any long click
		captured_widget->stopLongClickTimer();

		int start_compensation_x = 0;
		int start_compensation_y = 0;
		if (!captured_widget->m_packed.is_panning) {
			// When we start panning, deduct the extra distance caused by the
			// start threshold from the delta so we don't start with a sudden jump.
			int extra = threshold - 1;
			if (maybe_start_panning_x) {
				start_compensation_x = dx < 0 ? extra : -extra;
			}
			if (maybe_start_panning_y) {
				start_compensation_y = dy < 0 ? extra : -extra;
			}
		}

		// Get any active scroller and feed it with pan actions.
		TBScroller *scroller = captured_widget->getReadyScroller(dx != 0, dy != 0);
		if (scroller == nullptr) {
			return;
		}

		int old_translation_x = 0;
		int old_translation_y = 0;
		captured_widget->getScrollRoot()->getChildTranslation(old_translation_x, old_translation_y);

		if (scroller->onPan(dx + start_compensation_x, dy + start_compensation_y)) {
			// Scroll delta changed, so we are now panning!
			captured_widget->m_packed.is_panning = true;
			cancel_click = true;

			// If the captured widget (or its scroll root) has panned, we have to compensate the
			// pointer down coordinates so we won't accumulate the difference the following pan.
			int new_translation_x = 0;
			int new_translation_y = 0;
			captured_widget->getScrollRoot()->getChildTranslation(new_translation_x, new_translation_y);
			pointer_down_widget_x += new_translation_x - old_translation_x + start_compensation_x;
			pointer_down_widget_y += new_translation_y - old_translation_y + start_compensation_y;
		}
	}
}

void TBWidget::invokePointerCancel() {
	if (captured_widget != nullptr) {
		captured_widget->releaseCapture();
	}
}

bool TBWidget::invokeTouchDown(int x, int y, uint32_t id, int clickCount, MODIFIER_KEYS modifierkeys) {
	if (id == 0) {
		return invokePointerDown(x, y, clickCount, modifierkeys, TB_TOUCH);
	}

	TOUCH_INFO *ti = newTouchInfo(id);
	if (ti == nullptr) {
		return false;
	}

	if (ti->captured_widget == nullptr) {
		ti->captured_widget = getWidgetAt(x, y, true);
	}
	if ((ti->captured_widget != nullptr) && !ti->captured_widget->getState(WIDGET_STATE_DISABLED)) {
		ti->hovered_widget = ti->captured_widget;
	}

	if (ti->captured_widget != nullptr) {
		ti->captured_widget->convertFromRoot(x, y);
		ti->move_widget_x = ti->down_widget_x = x;
		ti->move_widget_y = ti->down_widget_y = y;
		TBWidgetEvent ev(EVENT_TYPE_TOUCH_DOWN, x, y, TB_TOUCH, modifierkeys);
		ev.count = clickCount;
		ev.ref_id = id;
		ti->captured_widget->invokeEvent(ev);
		return true;
	}
	return false;
}

bool TBWidget::invokeTouchUp(int x, int y, uint32_t id, MODIFIER_KEYS modifierkeys) {
	if (id == 0) {
		return invokePointerUp(x, y, modifierkeys, TB_TOUCH);
	}
	TOUCH_INFO *ti = getTouchInfo(id);
	if ((ti != nullptr) && (ti->captured_widget != nullptr)) {
		ti->captured_widget->convertFromRoot(x, y);
		TBWidgetEvent ev(EVENT_TYPE_TOUCH_UP, x, y, TB_TOUCH, modifierkeys);
		ev.ref_id = id;
		ti->captured_widget->invokeEvent(ev);
		DeleteTouchInfo(id);
		return true;
	}
	return false;
}

void TBWidget::invokeTouchMove(int x, int y, uint32_t id, MODIFIER_KEYS modifierkeys) {
	if (id == 0) {
		return invokePointerMove(x, y, modifierkeys, TB_TOUCH);
	}

	TOUCH_INFO *ti = getTouchInfo(id);
	if (ti == nullptr) {
		return;
	}

	ti->hovered_widget = getWidgetAt(x, y, true);
	if (ti->captured_widget != nullptr) {
		ti->captured_widget->convertFromRoot(x, y);
		ti->move_widget_x = x;
		ti->move_widget_y = y;
		TBWidgetEvent ev(EVENT_TYPE_TOUCH_MOVE, x, y, TB_TOUCH, modifierkeys);
		ev.ref_id = id;
		if (ti->captured_widget->invokeEvent(ev)) {
			return;
		}
	}
}

void TBWidget::invokeTouchCancel(uint32_t id) {
	if (id == 0) {
		return invokePointerCancel();
	}

	TOUCH_INFO *ti = getTouchInfo(id);
	if (ti != nullptr) {
		if (ti->captured_widget != nullptr) {
			TBWidgetEvent ev(EVENT_TYPE_TOUCH_CANCEL, 0, 0, TB_TOUCH);
			ev.ref_id = id;
			ti->captured_widget->invokeEvent(ev);
		}
		DeleteTouchInfo(id);
	}
}

bool TBWidget::invokeWheel(int x, int y, int deltaX, int deltaY, MODIFIER_KEYS modifierkeys) {
	setHoveredWidget(getWidgetAt(x, y, true), true);

	TBWidget *target = captured_widget != nullptr ? captured_widget : hovered_widget;
	if (target != nullptr) {
		target->convertFromRoot(x, y);
		pointer_move_widget_x = x;
		pointer_move_widget_y = y;
		TBWidgetEvent ev(EVENT_TYPE_WHEEL, x, y, TB_TOUCH, modifierkeys);
		ev.delta_x = deltaX;
		ev.delta_y = deltaY;
		target->invokeEvent(ev);

		// Return true when we have a target instead of InvokeEvent result. If a widget is
		// hit is more interesting for callers than if the event was handled or not.
		return true;
	}

	return false;
}

bool TBWidget::invokeKey(int key, SPECIAL_KEY specialKey, MODIFIER_KEYS modifierkeys, bool down) {
	bool handled = false;
	if (focused_widget != nullptr) {
		// Emulate a click on the focused widget when pressing space or enter
		if ((modifierkeys == 0U) && focused_widget->getClickByKey() && !focused_widget->getDisabled() &&
			!focused_widget->getIsDying() && (specialKey == TB_KEY_ENTER || key == ' ')) {
			// Set the pressed state while the key is down, if it
			// didn't already have the pressed state.
			static bool check_pressed_state = true;
			static bool had_pressed_state = false;
			if (down && check_pressed_state) {
				had_pressed_state = focused_widget->getState(WIDGET_STATE_PRESSED);
				check_pressed_state = false;
			}
			if (!down) {
				check_pressed_state = true;
			}

			if (!had_pressed_state) {
				focused_widget->setState(WIDGET_STATE_PRESSED, down);
				focused_widget->m_packed.has_key_pressed_state = down;
			}

			// Invoke the click event
			if (!down) {
				TBWidgetEvent ev(EVENT_TYPE_CLICK, m_rect.w / 2, m_rect.h / 2, TB_TOUCH);
				focused_widget->invokeEvent(ev);
			}
			handled = true;
		} else {
			// Invoke the key event on the focused widget
			TBWidgetEvent ev(down ? EVENT_TYPE_KEY_DOWN : EVENT_TYPE_KEY_UP);
			ev.key = key;
			ev.special_key = specialKey;
			ev.modifierkeys = modifierkeys;
			handled = focused_widget->invokeEvent(ev);
		}
	}

	// Move focus between widgets
	if (down && !handled && specialKey == TB_KEY_TAB) {
		handled = moveFocus((modifierkeys & TB_SHIFT) == 0U);

		// Show the focus when we move it by keyboard
		if (handled) {
			setAutoFocusState(true);
		}
	}
	return handled;
}

void TBWidget::releaseCapture() {
	if (this == captured_widget) {
		setCapturedWidget(nullptr);
	}
}

void TBWidget::convertToRoot(int &x, int &y) const {
	const TBWidget *tmp = this;
	while (tmp->m_parent != nullptr) {
		x += tmp->m_rect.x;
		y += tmp->m_rect.y;
		tmp = tmp->m_parent;

		if (tmp != nullptr) {
			int child_translation_x;
			int child_translation_y;
			tmp->getChildTranslation(child_translation_x, child_translation_y);
			x += child_translation_x;
			y += child_translation_y;
		}
	}
}

void TBWidget::convertFromRoot(int &x, int &y) const {
	const TBWidget *tmp = this;
	while (tmp->m_parent != nullptr) {
		x -= tmp->m_rect.x;
		y -= tmp->m_rect.y;
		tmp = tmp->m_parent;

		if (tmp != nullptr) {
			int child_translation_x;
			int child_translation_y;
			tmp->getChildTranslation(child_translation_x, child_translation_y);
			x -= child_translation_x;
			y -= child_translation_y;
		}
	}
}

// static
void TBWidget::setHoveredWidget(TBWidget *widget, bool touch) {
	if (TBWidget::hovered_widget == widget) {
		return;
	}
	if ((widget != nullptr) && widget->getState(WIDGET_STATE_DISABLED)) {
		return;
	}

	// We may apply hover state automatically so the widget might need to be updated.
	if (TBWidget::hovered_widget != nullptr) {
		TBWidget::hovered_widget->invalidate();
		TBWidget::hovered_widget->invalidateSkinStates();
	}

	TBWidget::hovered_widget = widget;

	if (widget && widget->getWantCaptureOnHover()) {
		setCapturedWidget(widget);
	}

	if (widget && widget->getWantFocusOnHover()) {
		widget->setFocus(WIDGET_FOCUS_REASON_POINTER);
	}

	if (TBWidget::hovered_widget != nullptr) {
		TBWidget::hovered_widget->invalidate();
		TBWidget::hovered_widget->invalidateSkinStates();

		// Cursor based movement should set hover state automatically, but touch
		// events should not (since touch doesn't really move unless pressed).
		TBWidget::hovered_widget->m_packed.no_automatic_hover_state = touch;
	}
}

// static
void TBWidget::setCapturedWidget(TBWidget *widget) {
	if (TBWidget::captured_widget == widget) {
		return;
	}
	if ((widget != nullptr) && widget->getState(WIDGET_STATE_DISABLED)) {
		return;
	}

	if (TBWidget::captured_widget != nullptr) {
		// Stop panning when capture change (most likely changing to nullptr because of InvokePointerUp)
		// Notify any active scroller so it may begin scrolling.
		if (TBScroller *scroller = TBWidget::captured_widget->findStartedScroller()) {
			if (TBWidget::captured_widget->m_packed.is_panning) {
				scroller->onPanReleased();
			} else {
				scroller->stop();
			}
		}
		TBWidget::captured_widget->m_packed.is_panning = false;

		// We apply pressed state automatically so the widget might need to be updated.
		TBWidget::captured_widget->invalidate();
		TBWidget::captured_widget->invalidateSkinStates();

		TBWidget::captured_widget->stopLongClickTimer();
	}
	cancel_click = false;

	TBWidget *old_capture = TBWidget::captured_widget;

	TBWidget::captured_widget = widget;

	if (old_capture != nullptr) {
		old_capture->onCaptureChanged(false);
	}

	if (TBWidget::captured_widget != nullptr) {
		TBWidget::captured_widget->invalidate();
		TBWidget::captured_widget->invalidateSkinStates();
		TBWidget::captured_widget->onCaptureChanged(true);
	}
}

bool TBWidget::setFontDescription(const TBFontDescription &fontDesc) {
	if (m_font_desc == fontDesc) {
		return true;
	}

	// Set the font description only if we have a matching font, or succeed creating one.
	if (g_font_manager->hasFontFace(fontDesc)) {
		m_font_desc = fontDesc;
	} else if (g_font_manager->createFontFace(fontDesc) != nullptr) {
		m_font_desc = fontDesc;
	} else {
		return false;
	}

	invokeFontChanged();
	return true;
}

void TBWidget::invokeFontChanged() {
	onFontChanged();

	// Recurse to children that inherit the font
	for (TBWidget *child = getFirstChild(); child != nullptr; child = child->getNext()) {
		if (child->m_font_desc.getFontFaceID() == 0) {
			child->invokeFontChanged();
		}
	}
}

TBFontDescription TBWidget::getCalculatedFontDescription() const {
	const TBWidget *tmp = this;
	while (tmp != nullptr) {
		if (tmp->m_font_desc.getFontFaceID() != 0) {
			return tmp->m_font_desc;
		}
		tmp = tmp->m_parent;
	}
	return g_font_manager->getDefaultFontDescription();
}

TBFontFace *TBWidget::getFont() const {
	return g_font_manager->getFontFace(getCalculatedFontDescription());
}

} // namespace tb
