/**
 * @file
 */

#include "tb_widgets_common.h"
#include "core/Assert.h"
#include "core/Var.h"
#include "core/command/Command.h"
#include "core/command/CommandHandler.h"
#include "tb_font_renderer.h"
#include "tb_system.h"
#include "tb_widgets_listener.h"

namespace tb {

TBWidgetString::TBWidgetString() : m_text_align(TB_TEXT_ALIGN_CENTER), m_width(0), m_height(0) {
}

void TBWidgetString::validatCachedSize(TBWidget *widget) {
	const TBFontDescription fd = widget->getCalculatedFontDescription();
	if ((m_height == 0) || fd != m_fd) {
		m_fd = fd;
		TBFontFace *font = g_font_manager->getFontFace(fd);
		m_width = font->getStringWidth(m_text);
		m_height = font->getHeight();
	}
}

int TBWidgetString::getWidth(TBWidget *widget) {
	validatCachedSize(widget);
	return m_width;
}

int TBWidgetString::getHeight(TBWidget *widget) {
	validatCachedSize(widget);
	return m_height;
}

bool TBWidgetString::setText(const char *text) {
	// Invalidate cache
	m_height = 0;
	return m_text.set(text);
}

void TBWidgetString::paint(TBWidget *widget, const TBRect &rect, const TBColor &color) {
	validatCachedSize(widget);
	TBFontFace *font = widget->getFont();

	int x = rect.x;
	if (m_text_align == TB_TEXT_ALIGN_RIGHT) {
		x += rect.w - m_width;
	} else if (m_text_align == TB_TEXT_ALIGN_CENTER) {
		x += Max(0, (rect.w - m_width) / 2);
	}
	int y = rect.y + (rect.h - m_height) / 2;

	if (m_width <= rect.w) {
		font->drawString(x, y, color, m_text);
	} else {
		// There's not enough room for the entire string
		// so cut it off and end with ellipsis (...)

		// const char *end = "…"; // 2026 HORIZONTAL ELLIPSIS
		// Some fonts seem to render ellipsis a lot uglier than three dots.
		const char *end = "...";

		int endw = font->getStringWidth(end);
		int startw = 0;
		int startlen = 0;
		while (m_text.c_str()[startlen] != 0) {
			int new_startw = font->getStringWidth(m_text, startlen);
			if (new_startw + endw > rect.w) {
				break;
			}
			startw = new_startw;
			startlen++;
		}
		startlen = Max(0, startlen - 1);
		font->drawString(x, y, color, m_text, startlen);
		font->drawString(x + startw, y, color, end);
	}
}

/** This value on m_cached_text_width means it needs to be updated again. */
#define UPDATE_TEXT_WIDTH_CACHE -1

TBTextField::TBTextField() : m_cached_text_width(UPDATE_TEXT_WIDTH_CACHE), m_squeezable(false) {
	setSkinBg(TBIDC("TBTextField"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
}

bool TBTextField::setText(const char *text) {
	if (m_text.equals(text)) {
		return true;
	}
	m_cached_text_width = UPDATE_TEXT_WIDTH_CACHE;
	invalidate();
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	return m_text.setText(text);
}

void TBTextField::setSqueezable(bool squeezable) {
	if (squeezable == m_squeezable) {
		return;
	}
	m_squeezable = squeezable;
	invalidate();
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
}

PreferredSize TBTextField::onCalculatePreferredContentSize(const SizeConstraints &constraints) {
	PreferredSize ps;
	if (m_cached_text_width == UPDATE_TEXT_WIDTH_CACHE) {
		m_cached_text_width = m_text.getWidth(this);
	}
	ps.pref_w = m_cached_text_width;
	ps.pref_h = ps.min_h = m_text.getHeight(this);
	// If gravity pull both up and down, use default max_h (grow as much as possible).
	// Otherwise it makes sense to only accept one line height.
	if (!(((getGravity() & WIDGET_GRAVITY_TOP) != 0U) && ((getGravity() & WIDGET_GRAVITY_BOTTOM) != 0U))) {
		ps.max_h = ps.pref_h;
	}
	if (!m_squeezable) {
		ps.min_w = ps.pref_w;
	}
	return ps;
}

void TBTextField::onFontChanged() {
	m_cached_text_width = UPDATE_TEXT_WIDTH_CACHE;
	invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
}

void TBTextField::onPaint(const PaintProps &paintProps) {
	m_text.paint(this, getPaddingRect(), paintProps.text_color);
}

const int auto_click_first_delay = 500;
const int auto_click_repeat_delay = 100;

TBButton::TBButton() : m_auto_repeat_click(false), m_toggle_mode(false) {
	setIsFocusable(true);
	setClickByKey(true);
	setSkinBg(TBIDC("TBButton"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	addChild(&m_layout);
	// Set the textfield gravity to all, even though it would display the same with default gravity.
	// This will make the buttons layout expand if there is space available, without forcing the parent
	// layout to grow to make the space available.
	m_textfield.setGravity(WIDGET_GRAVITY_ALL);
	m_layout.addChild(&m_textfield);
	m_layout.setRect(getPaddingRect());
	m_layout.setGravity(WIDGET_GRAVITY_ALL);
	m_layout.setPaintOverflowFadeout(false);
}

TBButton::~TBButton() {
	m_layout.removeChild(&m_textfield);
	removeChild(&m_layout);
}

bool TBButton::setText(const char *text) {
	bool ret = m_textfield.setText(text);
	updateTextFieldVisibility();
	return ret;
}

void TBButton::setValue(int value) {
	if (value == getValue()) {
		return;
	}
	setState(WIDGET_STATE_PRESSED, value != 0);

	if (canToggle()) {
		// Invoke a changed event.
		TBWidgetEvent ev(EVENT_TYPE_CHANGED);
		invokeEvent(ev);
	}

	if (value != 0 && getGroupID() != 0U) {
		TBRadioCheckBox::updateGroupWidgets(this);
	}
}

int TBButton::getValue() const {
	return static_cast<int>(getState(WIDGET_STATE_PRESSED));
}

void TBButton::onCaptureChanged(bool captured) {
	if (captured && m_auto_repeat_click) {
		postMessageDelayed(TBIDC("auto_click"), nullptr, auto_click_first_delay);
	} else if (!captured) {
		if (TBMessage *msg = getMessageByID(TBIDC("auto_click"))) {
			deleteMessage(msg);
		}
	}
}

void TBButton::onProcess() {
	TBWidget::onProcess();
	if (!_var || !_var->isDirty()) {
		return;
	}
	setValue(_var->intVal());
	_var->markClean();
}

void TBButton::onSkinChanged() {
	m_layout.setRect(getPaddingRect());
}

bool TBButton::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_CLICK && ev.target == this) {
		if (canToggle()) {
			TBWidgetSafePointer this_widget(this);

			// Toggle the value, if it's not a grouped widget with value on.
			if (!((getGroupID() != 0U) && (getValue() != 0))) {
				setValue(static_cast<int>(static_cast<int>(getValue()) == 0));
			}

			if (this_widget.get() == nullptr) {
				return true; // We got removed so we actually handled this event.
			}

			// Intentionally don't return true for this event. We want it to continue propagating.
		}
		if (_var) {
			_var->setVal(getValue());
		}
		if (!_command.isEmpty()) {
			if (canToggle()) {
				execute("%s %i", _command.c_str(), getValue());
			} else {
				execute("%s", _command.c_str());
			}
		}
	}
	return TBWidget::onEvent(ev);
}

void TBButton::onMessageReceived(TBMessage *msg) {
	if (msg->message == TBIDC("auto_click")) {
		core_assert(captured_widget == this);
		if (!cancel_click && (getHitStatus(pointer_move_widget_x, pointer_move_widget_y) != 0U)) {
			TBWidgetEvent ev(EVENT_TYPE_CLICK, pointer_move_widget_x, pointer_move_widget_y, TB_TOUCH);
			captured_widget->invokeEvent(ev);
		}
		if (auto_click_repeat_delay != 0) {
			postMessageDelayed(TBIDC("auto_click"), nullptr, auto_click_repeat_delay);
		}
	}
}

WIDGET_HIT_STATUS TBButton::getHitStatus(int x, int y) {
	// Never hit any of the children to the button. We always want to the button itself.
	return TBWidget::getHitStatus(x, y) != 0U ? WIDGET_HIT_STATUS_HIT_NO_CHILDREN : WIDGET_HIT_STATUS_NO_HIT;
}

void TBButton::updateTextFieldVisibility() {
	// Auto-collapse the textfield if the text is empty and there are other
	// widgets added apart from the textfield. This removes the extra spacing
	// added between the textfield and the other widget.
	bool collapse_textfield = m_textfield.isEmpty() && m_layout.getFirstChild() != m_layout.getLastChild();
	m_textfield.setVisibility(collapse_textfield ? WIDGET_VISIBILITY_GONE : WIDGET_VISIBILITY_VISIBLE);
}

void TBButton::ButtonLayout::onChildAdded(TBWidget *child) {
	static_cast<TBButton *>(getParent())->updateTextFieldVisibility();
}

void TBButton::ButtonLayout::onChildRemove(TBWidget *child) {
	static_cast<TBButton *>(getParent())->updateTextFieldVisibility();
}

TBClickLabel::TBClickLabel() {
	addChild(&m_layout);
	m_layout.addChild(&m_textfield);
	m_layout.setRect(getPaddingRect());
	m_layout.setGravity(WIDGET_GRAVITY_ALL);
	m_layout.setLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
}

TBClickLabel::~TBClickLabel() {
	m_layout.removeChild(&m_textfield);
	removeChild(&m_layout);
}

bool TBClickLabel::onEvent(const TBWidgetEvent &ev) {
	// Get a widget from the layout that isn't the textfield, or just bail out
	// if we only have the textfield.
	if (m_layout.getFirstChild() == m_layout.getLastChild()) {
		return false;
	}
	TBWidget *click_target =
		(m_layout.getFirstChild() == &m_textfield ? m_layout.getLastChild() : m_layout.getFirstChild());
	// Invoke the event on it, as if it was invoked on the target itself.
	if ((click_target != nullptr) && ev.target != click_target) {
		// Focus the target if we clicked the label.
		if (ev.type == EVENT_TYPE_CLICK) {
			click_target->setFocus(WIDGET_FOCUS_REASON_POINTER);
		}

		// Sync our pressed state with the click target. Special case for when we're just about to
		// lose it ourself (pointer is being released).
		bool pressed_state = (ev.target->getAutoState() & WIDGET_STATE_PRESSED) != 0U;
		if (ev.type == EVENT_TYPE_POINTER_UP || ev.type == EVENT_TYPE_CLICK) {
			pressed_state = false;
		}

		click_target->setState(WIDGET_STATE_PRESSED, pressed_state);

		TBWidgetEvent target_ev(ev.type, ev.target_x - click_target->getRect().x,
								ev.target_y - click_target->getRect().y, ev.button_type, ev.modifierkeys);
		return click_target->invokeEvent(target_ev);
	}
	return false;
}

PreferredSize TBSkinImage::onCalculatePreferredSize(const SizeConstraints &constraints) {
	PreferredSize ps = TBWidget::onCalculatePreferredSize(constraints);
	// FIX: Make it stretched proportionally if shrunk.
	ps.max_w = ps.pref_w;
	ps.max_h = ps.pref_h;
	return ps;
}

TBSeparator::TBSeparator() {
	setSkinBg(TBIDC("TBSeparator"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	setState(WIDGET_STATE_DISABLED, true);
}

// FIX: Add spin_speed to skin!
// FIX: Make it post messages only if visible
const int spin_speed = 1000 / 30; ///< How fast should the spinner animation animate.

TBProgressSpinner::TBProgressSpinner() : m_value(0), m_frame(0) {
	setSkinBg(TBIDC("TBProgressSpinner"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	m_skin_fg.set(TBIDC("TBProgressSpinner.fg"));
}

void TBProgressSpinner::setValue(int value) {
	if (value == m_value) {
		return;
	}
	invalidateSkinStates();
	core_assert(value >= 0); // If this happens, you probably have unballanced Begin/End calls.
	m_value = value;
	if (value > 0) {
		// Start animation
		if (getMessageByID(TBID(1)) == nullptr) {
			m_frame = 0;
			postMessageDelayed(TBID(1), nullptr, spin_speed);
		}
	} else {
		// Stop animation
		if (TBMessage *msg = getMessageByID(TBID(1))) {
			deleteMessage(msg);
		}
	}
}

void TBProgressSpinner::onPaint(const PaintProps &paintProps) {
	if (isRunning()) {
		TBSkinElement *e = g_tb_skin->getSkinElement(m_skin_fg);
		if ((e != nullptr) && (e->bitmap != nullptr)) {
			int size = e->bitmap->height();
			int num_frames = e->bitmap->width() / e->bitmap->height();
			int current_frame = m_frame % num_frames;
			g_renderer->drawBitmap(getPaddingRect(), TBRect(current_frame * size, 0, size, size), e->bitmap);
		}
	}
}

void TBProgressSpinner::onMessageReceived(TBMessage *msg) {
	m_frame++;
	invalidate();
	// Keep animation running
	postMessageDelayed(TBID(1), nullptr, spin_speed);
}

TBRadioCheckBox::TBRadioCheckBox() : m_value(0) {
	setIsFocusable(true);
	setClickByKey(true);
}

// static
void TBRadioCheckBox::updateGroupWidgets(TBWidget *newLeader) {
	core_assert(newLeader->getValue() && newLeader->getGroupID());

	// Find the group root widget.
	TBWidget *group = newLeader;
	while ((group != nullptr) && !group->getIsGroupRoot() && (group->getParent() != nullptr)) {
		group = group->getParent();
	}

	for (TBWidget *child = group; child != nullptr; child = child->getNextDeep(group)) {
		if (child != newLeader && child->getGroupID() == newLeader->getGroupID()) {
			child->setValue(0);
		}
	}
}

void TBRadioCheckBox::setValue(int value) {
	if (m_value == value) {
		return;
	}
	m_value = value;
	if (_var) {
		_var->setVal(value != 0);
	}
	if (!_command.isEmpty()) {
		execute("%s %i", _command.c_str(), m_value);
	}
	setState(WIDGET_STATE_SELECTED, value != 0);

	if (value != 0 && getGroupID() != 0U) {
		updateGroupWidgets(this);
	}

	TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	invokeEvent(ev);
}

PreferredSize TBRadioCheckBox::onCalculatePreferredSize(const SizeConstraints &constraints) {
	PreferredSize ps = TBWidget::onCalculatePreferredSize(constraints);
	ps.min_w = ps.max_w = ps.pref_w;
	ps.min_h = ps.max_h = ps.pref_h;
	return ps;
}

void TBRadioCheckBox::onProcess() {
	TBWidget::onProcess();
	if (!_var || !_var->isDirty()) {
		return;
	}
	setValue(_var->intVal());
	_var->markClean();
}

bool TBRadioCheckBox::onEvent(const TBWidgetEvent &ev) {
	if (ev.target == this && ev.type == EVENT_TYPE_CLICK) {
		// Toggle the value, if it's not a grouped widget with value on.
		if (!((getGroupID() != 0U) && (getValue() != 0))) {
			setValue(static_cast<int>(static_cast<int>(getValue()) == 0));
		}
	}
	return false;
}

TBScrollBar::TBScrollBar()
	: m_axis(AXIS_Y) ///< Make setAxis below always succeed and set the skin
	  ,
	  m_value(0), m_min(0), m_max(1), m_visible(1), m_to_pixel_factor(0) {
	setAxis(AXIS_X);
	addChild(&m_handle);
}

TBScrollBar::~TBScrollBar() {
	removeChild(&m_handle);
}

void TBScrollBar::setAxis(AXIS axis) {
	if (axis == m_axis) {
		return;
	}
	m_axis = axis;
	if (axis == AXIS_X) {
		setSkinBg(TBIDC("TBScrollBarBgX"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
		m_handle.setSkinBg(TBIDC("TBScrollBarFgX"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	} else {
		setSkinBg(TBIDC("TBScrollBarBgY"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
		m_handle.setSkinBg(TBIDC("TBScrollBarFgY"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	}
	invalidate();
}

void TBScrollBar::setLimits(double min, double max, double visible) {
	max = Max(min, max);
	visible = Max(visible, 0.0);
	if (min == m_min && max == m_max && m_visible == visible) {
		return;
	}
	m_min = min;
	m_max = max;
	m_visible = visible;
	setValueDouble(m_value);

	// If we're currently dragging the scrollbar handle, convert the down point
	// to root and then back after the applying the new limit.
	// This prevents sudden jumps to unexpected positions when scrolling.
	if (captured_widget == &m_handle) {
		m_handle.convertToRoot(pointer_down_widget_x, pointer_down_widget_y);
	}

	updateHandle();

	if (captured_widget == &m_handle) {
		m_handle.convertFromRoot(pointer_down_widget_x, pointer_down_widget_y);
	}
}

void TBScrollBar::setValueDouble(double value) {
	value = Clamp(value, m_min, m_max);
	if (value == m_value) {
		return;
	}
	m_value = value;

	updateHandle();
	TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	invokeEvent(ev);
}

bool TBScrollBar::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_POINTER_MOVE && captured_widget == &m_handle) {
		if (m_to_pixel_factor > 0) {
			int dx = ev.target_x - pointer_down_widget_x;
			int dy = ev.target_y - pointer_down_widget_y;
			double delta_val = (m_axis == AXIS_X ? dx : dy) / m_to_pixel_factor;
			setValueDouble(m_value + delta_val);
		}
		return true;
	}
	if (ev.type == EVENT_TYPE_POINTER_MOVE && ev.target == this) {
		return true;
	}
	if (ev.type == EVENT_TYPE_POINTER_DOWN && ev.target == this) {
		bool after_handle =
			(m_axis == AXIS_X ? ev.target_x > m_handle.getRect().x : ev.target_y > m_handle.getRect().y);
		setValueDouble(m_value + (after_handle ? m_visible : -m_visible));
		return true;
	}
	if (ev.type == EVENT_TYPE_WHEEL) {
		double old_val = m_value;
		setValueDouble(m_value + ev.delta_y * TBSystem::getPixelsPerLine());
		return m_value != old_val;
	}
	return false;
}

void TBScrollBar::updateHandle() {
	// Calculate the mover size and position
	bool horizontal = m_axis == AXIS_X;
	int available_pixels = horizontal ? getRect().w : getRect().h;
	int min_thickness_pixels = Min(getRect().h, getRect().w);

	int visible_pixels = available_pixels;

	if (m_max - m_min > 0 && m_visible > 0) {
		double visible_proportion = m_visible / (m_visible + m_max - m_min);
		visible_pixels = (int)(visible_proportion * available_pixels);

		// Limit the size of the indicator to the slider thickness so that it doesn't
		// become too tiny when the visible proportion is very small.
		visible_pixels = Max(visible_pixels, min_thickness_pixels);

		m_to_pixel_factor = (double)(available_pixels - visible_pixels) / (m_max - m_min) /*+ 0.5*/;
	} else {
		m_to_pixel_factor = 0;

		// If we can't scroll anything, make the handle invisible
		visible_pixels = 0;
	}

	int pixel_pos = (int)(m_value * m_to_pixel_factor);

	TBRect rect;
	if (horizontal) {
		rect.set(pixel_pos, 0, visible_pixels, getRect().h);
	} else {
		rect.set(0, pixel_pos, getRect().w, visible_pixels);
	}

	m_handle.setRect(rect);
}

void TBScrollBar::onResized(int oldW, int oldH) {
	updateHandle();
}

TBSlider::TBSlider()
	: m_axis(AXIS_Y) ///< Make setAxis below always succeed and set the skin
	  ,
	  m_value(0), m_min(0), m_max(1), m_to_pixel_factor(0) {
	setIsFocusable(true);
	setAxis(AXIS_X);
	addChild(&m_handle);
}

TBSlider::~TBSlider() {
	removeChild(&m_handle);
}

void TBSlider::setAxis(AXIS axis) {
	if (axis == m_axis) {
		return;
	}
	m_axis = axis;
	if (axis == AXIS_X) {
		setSkinBg(TBIDC("TBSliderBgX"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
		m_handle.setSkinBg(TBIDC("TBSliderFgX"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	} else {
		setSkinBg(TBIDC("TBSliderBgY"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
		m_handle.setSkinBg(TBIDC("TBSliderFgY"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	}
	invalidate();
}

void TBSlider::setLimits(double min, double max) {
	min = Min(min, max);
	if (min == m_min && max == m_max) {
		return;
	}
	m_min = min;
	m_max = max;
	setValueDouble(m_value);
	updateHandle();
}

void TBSlider::onProcess() {
	TBWidget::onProcess();
	if (!_var || !_var->isDirty()) {
		return;
	}
	setValue(_var->intVal());
	_var->markClean();
}

void TBSlider::setValueDouble(double value) {
	value = Clamp(value, m_min, m_max);
	if (value == m_value) {
		return;
	}
	m_value = value;
	if (_var) {
		_var->setVal((float)value);
	}
	if (!_command.isEmpty()) {
		execute("%s %f", _command.c_str(), m_value);
	}

	updateHandle();
	TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	invokeEvent(ev);
}

bool TBSlider::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_POINTER_MOVE && captured_widget == &m_handle) {
		if (m_to_pixel_factor > 0) {
			int dx = ev.target_x - pointer_down_widget_x;
			int dy = ev.target_y - pointer_down_widget_y;
			double delta_val = (m_axis == AXIS_X ? dx : -dy) / m_to_pixel_factor;
			setValueDouble(m_value + delta_val);
		}
		return true;
	}
	if (ev.type == EVENT_TYPE_WHEEL) {
		double old_val = m_value;
		double step = (m_axis == AXIS_X ? getSmallStep() : -getSmallStep());
		setValueDouble(m_value + step * ev.delta_y);
		return m_value != old_val;
	}
	if (ev.type == EVENT_TYPE_KEY_DOWN) {
		double step = (m_axis == AXIS_X ? getSmallStep() : -getSmallStep());
		if (ev.special_key == TB_KEY_LEFT || ev.special_key == TB_KEY_UP) {
			setValueDouble(getValueDouble() - step);
		} else if (ev.special_key == TB_KEY_RIGHT || ev.special_key == TB_KEY_DOWN) {
			setValueDouble(getValueDouble() + step);
		} else {
			return false;
		}
		return true;
	}
	if (ev.type == EVENT_TYPE_KEY_UP) {
		if (ev.special_key == TB_KEY_LEFT || ev.special_key == TB_KEY_UP || ev.special_key == TB_KEY_RIGHT ||
			ev.special_key == TB_KEY_DOWN) {
			return true;
		}
	}
	return TBWidget::onEvent(ev);
}

void TBSlider::updateHandle() {
	// Calculate the handle position
	bool horizontal = m_axis == AXIS_X;
	int available_pixels = horizontal ? getRect().w : getRect().h;

	TBRect rect;
	if (m_max - m_min > 0) {
		PreferredSize ps = m_handle.getPreferredSize();
		int handle_pixels = horizontal ? ps.pref_w : ps.pref_h;
		m_to_pixel_factor = (double)(available_pixels - handle_pixels) / (m_max - m_min) /*+ 0.5*/;

		int pixel_pos = (int)((m_value - m_min) * m_to_pixel_factor);

		if (horizontal) {
			rect.set(pixel_pos, (getRect().h - ps.pref_h) / 2, ps.pref_w, ps.pref_h);
		} else {
			rect.set((getRect().w - ps.pref_w) / 2, getRect().h - handle_pixels - pixel_pos, ps.pref_w, ps.pref_h);
		}
	} else {
		m_to_pixel_factor = 0;
	}

	m_handle.setRect(rect);
}

void TBSlider::onResized(int oldW, int oldH) {
	updateHandle();
}

TBContainer::TBContainer() {
	setSkinBg(TBIDC("TBContainer"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
}

TBMover::TBMover() {
	setSkinBg(TBIDC("TBMover"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
}

bool TBMover::onEvent(const TBWidgetEvent &ev) {
	TBWidget *target = getParent();
	if (target == nullptr) {
		return false;
	}
	if (ev.type == EVENT_TYPE_POINTER_MOVE && captured_widget == this) {
		int dx = ev.target_x - pointer_down_widget_x;
		int dy = ev.target_y - pointer_down_widget_y;
		TBRect rect = target->getRect().offset(dx, dy);
		if (target->getParent() != nullptr) {
			// Apply limit.
			rect.x = Clamp(rect.x, -pointer_down_widget_x, target->getParent()->getRect().w - pointer_down_widget_x);
			rect.y = Clamp(rect.y, -pointer_down_widget_y, target->getParent()->getRect().h - pointer_down_widget_y);
		}
		target->setRect(rect);
		return true;
	}
	return false;
}

TBResizer::TBResizer() {
	setSkinBg(TBIDC("TBResizer"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
}

WIDGET_HIT_STATUS TBResizer::getHitStatus(int x, int y) {
	// Shave off some of the upper left diagonal half from the hit area.
	const int extra_hit_area = 3;
	if (x < getRect().w - y - extra_hit_area) {
		return WIDGET_HIT_STATUS_NO_HIT;
	}
	return TBWidget::getHitStatus(x, y);
}

bool TBResizer::onEvent(const TBWidgetEvent &ev) {
	TBWidget *target = getParent();
	if (target == nullptr) {
		return false;
	}
	if (ev.type == EVENT_TYPE_POINTER_MOVE && captured_widget == this) {
		int dx = ev.target_x - pointer_down_widget_x;
		int dy = ev.target_y - pointer_down_widget_y;
		TBRect rect = target->getRect();
		rect.w += dx;
		rect.h += dy;
		// Apply limit. We should not use minimum size since we can squeeze
		// the layout much more, and provide scroll/pan when smaller.
		rect.w = Max(rect.w, 50);
		rect.h = Max(rect.h, 50);
		target->setRect(rect);
	} else {
		return false;
	}
	return true;
}

TBDimmer::TBDimmer() {
	setSkinBg(TBIDC("TBDimmer"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	setGravity(WIDGET_GRAVITY_ALL);
}

void TBDimmer::onAdded() {
	setRect(TBRect(0, 0, getParent()->getRect().w, getParent()->getRect().h));
}

} // namespace tb
