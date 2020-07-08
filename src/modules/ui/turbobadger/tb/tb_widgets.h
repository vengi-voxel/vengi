/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "tb_core.h"
#include "tb_font_desc.h"
#include "tb_geometry.h"
#include "tb_linklist.h"
#include "tb_object.h"
#include "tb_skin.h"
#include "tb_widget_value.h"

namespace tb {

class TBWindow;
class TBWidget;
class TBFontFace;
class TBScroller;
class TBWidgetListener;
class TBLongClickTimer;
struct INFLATE_INFO;

// == Generic widget stuff =================================================

enum TB_ALIGN {
	TB_ALIGN_LEFT,  ///< Align to the left side
	TB_ALIGN_TOP,   ///< Align to the top (above)
	TB_ALIGN_RIGHT, ///< Align to the right side
	TB_ALIGN_BOTTOM ///< Align to the bottom (below)
};

enum EVENT_TYPE {
	/** Click event is what should be used to trig actions in almost all cases.

		It is invoked on a widget after POINTER_UP if the pointer is still inside
		its hit area. It can also be invoked by keyboard on some clickable widgets
		(see TBWidget::setClickByKey).

		If panning of scrollable widgets start while the pointer is down, CLICK
		won't be invoked when releasing the pointer (since that should stop panning). */
	EVENT_TYPE_CLICK,

	/** Long click event is sent when the pointer has been down for some time
		without moving much.

		It is invoked on a widget that has enabled it (TBWidget::setWantLongClick
		If this event isn't handled, the widget will invoke a CONTEXT_MENU event.
		If any of those are handled, the CLICK event that would normally be
		invoked after the pending POINTER_UP will be suppressed. */
	EVENT_TYPE_LONG_CLICK,
	EVENT_TYPE_POINTER_DOWN,
	EVENT_TYPE_POINTER_UP,
	EVENT_TYPE_POINTER_MOVE,
	EVENT_TYPE_TOUCH_DOWN,
	EVENT_TYPE_TOUCH_UP,
	EVENT_TYPE_TOUCH_MOVE,
	EVENT_TYPE_TOUCH_CANCEL,
	EVENT_TYPE_WHEEL,

	/** Invoked after changing text in a TBTextField, changing selected item
		in a TBSelectList etc. Invoking this event trigs synchronization with
		connected TBWidgetValue and other widgets connected to it. */
	EVENT_TYPE_CHANGED,
	EVENT_TYPE_KEY_DOWN,
	EVENT_TYPE_KEY_UP,

	/** Invoked by the platform when a standard keyboard shortcut is pressed.
		It's called before InvokeKeyDown (EVENT_TYPE_KEY_DOWN) and if the event
		is handled (returns true), the KeyDown is canceled.
		The ref_id will be set to one of the following:
			"cut", "copy", "paste", "selectall", "undo", "redo", "new", "open", "save". */
	EVENT_TYPE_SHORTCUT,
	/**
	 * Invoked when a command was executed
	 */
	EVENT_TYPE_COMMAND,

	/** Invoked when a context menu should be opened at the event x and y coordinates.
		It may be invoked automatically for a widget on long click, if nothing handles
		the long click event. */
	EVENT_TYPE_CONTEXT_MENU,

	/** Invoked by the platform when one or multiple files has been dropped on
		the widget. The event is guaranteed to be a TBWidgetEventFileDrop. */
	EVENT_TYPE_FILE_DROP,

	/** Custom event. Not used internally. ref_id may be used for additional type info. */
	EVENT_TYPE_CUSTOM
};

enum MODIFIER_KEYS { TB_MODIFIER_NONE = 0, TB_CTRL = 1, TB_SHIFT = 2, TB_ALT = 4, TB_SUPER = 8 };
MAKE_ENUM_FLAG_COMBO(MODIFIER_KEYS);

enum BUTTON_TYPE { TB_LEFT = 0, TB_RIGHT = 1, TB_MIDDLE = 2, TB_TOUCH = 4, TB_UNKNOWN = 8 };
MAKE_ENUM_FLAG_COMBO(BUTTON_TYPE);

enum SPECIAL_KEY {
	TB_KEY_UNDEFINED = 0,
	TB_KEY_UP,
	TB_KEY_DOWN,
	TB_KEY_LEFT,
	TB_KEY_RIGHT,
	TB_KEY_PAGE_UP,
	TB_KEY_PAGE_DOWN,
	TB_KEY_HOME,
	TB_KEY_END,
	TB_KEY_TAB,
	TB_KEY_BACKSPACE,
	TB_KEY_INSERT,
	TB_KEY_DELETE,
	TB_KEY_ENTER,
	TB_KEY_ESC,
	TB_KEY_F1,
	TB_KEY_F2,
	TB_KEY_F3,
	TB_KEY_F4,
	TB_KEY_F5,
	TB_KEY_F6,
	TB_KEY_F7,
	TB_KEY_F8,
	TB_KEY_F9,
	TB_KEY_F10,
	TB_KEY_F11,
	TB_KEY_F12,
	TB_KEY_SHIFT,
	TB_KEY_ALT,
	TB_KEY_CTRL,
	TB_KEY_GUI,
	TB_KEY_MODE
};

class TBWidgetEvent : public TBTypedObject {
public:
	TBWidget *target; ///< The widget that invoked the event
	EVENT_TYPE type;  ///< Which type of event
	int target_x;	 ///< X position in target widget. Set for all pointer events, click and wheel.
	int target_y;	 ///< Y position in target widget. Set for all pointer events, click and wheel.
	int delta_x;	  ///< Set for EVENT_TYPE_WHEEL. Positive is a turn right.
	int delta_y;	  ///< Set for EVENT_TYPE_WHEEL. Positive is a turn against the user.
	int count; ///< 1 for all events, but increased for POINTER_DOWN event to 2 for doubleclick, 3 for tripleclick and
			   ///< so on.
	int key;
	SPECIAL_KEY special_key;
	MODIFIER_KEYS modifierkeys;
	TBID ref_id;			 ///< Sometimes (when documented) events have a ref_id (The id that caused this event)
	BUTTON_TYPE button_type; ///< Set for pointer events. True if the event is a touch event (finger or pen on screen)
							 ///< False if mouse or other cursor input.
	const char* string = nullptr;

	TBOBJECT_SUBCLASS(TBWidgetEvent, TBTypedObject);

	TBWidgetEvent(EVENT_TYPE type)
		: target(nullptr), type(type), target_x(0), target_y(0), delta_x(0), delta_y(0), count(1), key(0),
		  special_key(TB_KEY_UNDEFINED), modifierkeys(TB_MODIFIER_NONE), button_type(TB_UNKNOWN) {
	}

	TBWidgetEvent(EVENT_TYPE type, int x, int y, BUTTON_TYPE buttonType, MODIFIER_KEYS modifierkeys = TB_MODIFIER_NONE)
		: target(nullptr), type(type), target_x(x), target_y(y), delta_x(0), delta_y(0), count(1), key(0),
		  special_key(TB_KEY_UNDEFINED), modifierkeys(modifierkeys), button_type(buttonType) {
	}

	/** The count value may be 1 to infinity. If you f.ex want to see which count it is for something
		handling click and double click, call GetCountCycle(2). If you also handle triple click, call
		GetCountCycle(3) and so on. That way you'll get a count that always cycle in the range you need. */
	int getCountCycle(int max) {
		return ((count - 1) % max) + 1;
	}

	bool isPointerEvent() const {
		return type == EVENT_TYPE_POINTER_DOWN || type == EVENT_TYPE_POINTER_UP || type == EVENT_TYPE_POINTER_MOVE;
	}
	bool isTouchEvent() const {
		return type == EVENT_TYPE_TOUCH_DOWN || type == EVENT_TYPE_TOUCH_UP || type == EVENT_TYPE_TOUCH_MOVE ||
			   type == EVENT_TYPE_TOUCH_CANCEL;
	}
	bool isKeyEvent() const {
		return type == EVENT_TYPE_KEY_DOWN || type == EVENT_TYPE_KEY_UP;
	}
	bool isAny(const tb::TBID &id) const;
};

/** TBWidgetEventFileDrop is a event of type EVENT_TYPE_FILE_DROP.
	It contains a list of filenames of the files that was dropped. */
class TBWidgetEventFileDrop : public TBWidgetEvent {
public:
	TBListAutoDeleteOf<TBStr> files;

	TBOBJECT_SUBCLASS(TBWidgetEventFileDrop, TBWidgetEvent);

	TBWidgetEventFileDrop() : TBWidgetEvent(EVENT_TYPE_FILE_DROP) {
	}
};

/** TBWidget state types (may be combined).
	NOTE: This should exactly match SKIN_STATE in tb_skin.h! */
enum WIDGET_STATE : uint8_t {
	WIDGET_STATE_NONE = 0,
	WIDGET_STATE_DISABLED = 1,
	WIDGET_STATE_FOCUSED = 2,
	WIDGET_STATE_PRESSED = 4,
	WIDGET_STATE_SELECTED = 8,
	WIDGET_STATE_HOVERED = 16,

	WIDGET_STATE_ALL = WIDGET_STATE_DISABLED | WIDGET_STATE_FOCUSED | WIDGET_STATE_PRESSED | WIDGET_STATE_SELECTED |
					   WIDGET_STATE_HOVERED,
	WIDGET_STATE_ENSURESIZE = 0xFFU
};
MAKE_ENUM_FLAG_COMBO(WIDGET_STATE);

/** TBWidget gravity (may be combined).
	Gravity gives hints about positioning and sizing preferences. */
enum WIDGET_GRAVITY {
	WIDGET_GRAVITY_NONE = 0,
	WIDGET_GRAVITY_LEFT = 1,
	WIDGET_GRAVITY_RIGHT = 2,
	WIDGET_GRAVITY_TOP = 4,
	WIDGET_GRAVITY_BOTTOM = 8,

	WIDGET_GRAVITY_LEFT_RIGHT = WIDGET_GRAVITY_LEFT | WIDGET_GRAVITY_RIGHT,
	WIDGET_GRAVITY_TOP_BOTTOM = WIDGET_GRAVITY_TOP | WIDGET_GRAVITY_BOTTOM,
	WIDGET_GRAVITY_ALL = WIDGET_GRAVITY_LEFT_RIGHT | WIDGET_GRAVITY_TOP_BOTTOM,
	WIDGET_GRAVITY_DEFAULT = WIDGET_GRAVITY_LEFT | WIDGET_GRAVITY_TOP,
	WIDGET_GRAVITY_ENSURESIZE = 0xFFU
};
MAKE_ENUM_FLAG_COMBO(WIDGET_GRAVITY);

enum AXIS {
	AXIS_X, ///< Horizontal layout
	AXIS_Y, ///< Vertical layout
	AXIS_Z ///< Depth
};

/** Defines how the size in one axis depend on the other axis when a widgets size is
	affected by constraints. */
enum SIZE_DEP {
	/** No dependency (Faster layout). */
	SIZE_DEP_NONE = 0,
	/** The width is dependant on the height. Additional layout pass may be required. */
	SIZE_DEP_WIDTH_DEPEND_ON_HEIGHT = 1,
	/** The height is dependant on the width. Additional layout pass may be required. */
	SIZE_DEP_HEIGHT_DEPEND_ON_WIDTH = 2,
	/** Both width and height are dependant on each other. Additional layout pass may
		be required. */
	SIZE_DEP_BOTH = SIZE_DEP_WIDTH_DEPEND_ON_HEIGHT | SIZE_DEP_HEIGHT_DEPEND_ON_WIDTH
};
MAKE_ENUM_FLAG_COMBO(SIZE_DEP);

/** PreferredSize contains size preferences for a TBWidget.
	This is calculated during layout for each widget from
	the current skin, widget preferences and LayoutParams. */

class PreferredSize {
public:
	PreferredSize()
		: min_w(0), min_h(0), max_w(10000), max_h(10000), pref_w(0), pref_h(0), size_dependency(SIZE_DEP_NONE) {
	}
	PreferredSize(int w, int h)
		: min_w(w), min_h(h), max_w(w), max_h(h), pref_w(w), pref_h(h), size_dependency(SIZE_DEP_NONE) {
	}

	int min_w, min_h;		  ///< The minimal preferred width and height.
	int max_w, max_h;		  ///< The maximum preferred width and height.
	int pref_w, pref_h;		  ///< The preferred width and height.
	SIZE_DEP size_dependency; ///< The size dependency when size is affected by constraints.
};

/** LayoutParams defines size preferences for a TBWidget that
	are set on the widget to override size preferences from
	skin and widget. */
class LayoutParams {
public:
	static const int UNSPECIFIED = TB_INVALID_DIMENSION;
	LayoutParams()
		: min_w(UNSPECIFIED), min_h(UNSPECIFIED), max_w(UNSPECIFIED), max_h(UNSPECIFIED), pref_w(UNSPECIFIED),
		  pref_h(UNSPECIFIED) {
	}
	LayoutParams(int w, int h) : min_w(w), min_h(h), max_w(w), max_h(h), pref_w(w), pref_h(h) {
	}

	/** Set both min max and preferred width to the given width. */
	void setWidth(int width) {
		min_w = max_w = pref_w = width;
	}

	/** Set both min max and preferred height to the given height. */
	void setHeight(int height) {
		min_h = max_h = pref_h = height;
	}

	int min_w, min_h;   ///< The minimal preferred width and height.
	int max_w, max_h;   ///< The maximum preferred width and height.
	int pref_w, pref_h; ///< The preferred width and height.
};

/** Specifies size constraints used during size calculations. */
class SizeConstraints {
public:
	static const int NO_RESTRICTION = 10000;

	/** The available width and height. May be NO_RESTRICTION which is a large value. */
	int available_w, available_h;

	/** Constrain to the given width and height. */
	SizeConstraints(int w, int h) : available_w(w), available_h(h) {
	}

	/** No constraints. */
	SizeConstraints() : available_w(NO_RESTRICTION), available_h(NO_RESTRICTION) {
	}

	/** Return new constraints reduced by the given padding. */
	SizeConstraints constrainByPadding(int horizontalPadding, int verticalPadding) const {
		return SizeConstraints(available_w == NO_RESTRICTION ? NO_RESTRICTION : available_w - horizontalPadding,
							   available_h == NO_RESTRICTION ? NO_RESTRICTION : available_h - verticalPadding);
	}

	/** Return new constraints that are constrained by LayoutParams. */
	SizeConstraints constrainByLayoutParams(const LayoutParams &lp) const {
		return SizeConstraints(constrainByLPMax(available_w, lp.min_w, lp.max_w),
							   constrainByLPMax(available_h, lp.min_h, lp.max_h));
	}

	bool operator==(const SizeConstraints &sc) const {
		return available_w == sc.available_w && available_h == sc.available_h;
	}

private:
	int constrainByLPMax(int constraint, int lpMin, int lpMax) const {
		if (constraint == NO_RESTRICTION) {
			return lpMax != LayoutParams::UNSPECIFIED ? lpMax : NO_RESTRICTION;
		}
		int ret = constraint;
		if (lpMin != LayoutParams::UNSPECIFIED) {
			ret = Max(ret, lpMin);
		}
		if (lpMax != LayoutParams::UNSPECIFIED) {
			ret = Min(ret, lpMax);
		}
		return ret;
	}
};

/** Defines widget z level, used with TBWidget::setZ, TBWidget::AddChild. */
enum WIDGET_Z {
	WIDGET_Z_TOP,   ///< The toplevel (Visually drawn on top of everything else).
	WIDGET_Z_BOTTOM ///< The bottomlevel (Visually drawn behind everything else).
};

/** Defines widget z level relative to another widget, used with TBWidget::AddChildRelative. */
enum WIDGET_Z_REL {
	WIDGET_Z_REL_BEFORE, ///< Before the reference widget (visually behind reference).
	WIDGET_Z_REL_AFTER   ///< After the reference widget (visually above reference).
};

/** Defines widget visibility, used with TBWidget::setVisibility. */
enum WIDGET_VISIBILITY {
	WIDGET_VISIBILITY_VISIBLE,   ///< Visible (default)
	WIDGET_VISIBILITY_INVISIBLE, ///< Invisible, but layouted. Interaction disabled.
	WIDGET_VISIBILITY_GONE		 ///< Invisible and no layout. Interaction disabled.
};

enum WIDGET_INVOKE_INFO { WIDGET_INVOKE_INFO_NORMAL, WIDGET_INVOKE_INFO_NO_CALLBACKS };

enum WIDGET_FOCUS_REASON {
	WIDGET_FOCUS_REASON_NAVIGATION, ///< Set focus by navigation (i.e. keyboard tab). This will
									///< scroll to the widget if needed.
	WIDGET_FOCUS_REASON_POINTER,	///< Set focus by pointer (i.e. clicking)
	WIDGET_FOCUS_REASON_UNKNOWN		///< Set focus by anything else.
};

/** Hit status return value for TBWidget::getHitStatus */
enum WIDGET_HIT_STATUS {
	WIDGET_HIT_STATUS_NO_HIT = 0,	 ///< The widget was not hit
	WIDGET_HIT_STATUS_HIT,			  ///< The widget was hit, any child may be hit too.
	WIDGET_HIT_STATUS_HIT_NO_CHILDREN ///< The widget was hit, no children should be hit.
};

/** The base TBWidget class.
	Make a subclass to implement UI controls.
	Each widget has a background skin (no skin specified by default) which will be used to
	calculate the default size preferences and padding around the preferred content size.

	Note: When you subclass a widget, use the TBOBJECT_SUBCLASS macro to define the type
	casting functions instead of implementing those manually. */

class TBWidget : public TBTypedObject, public TBLinkOf<TBWidget> {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBWidget, TBTypedObject);

	TBWidget();
	virtual ~TBWidget();

	/** Set the rect for this widget in its parent. The rect is relative to the parent widget.
		The skin may expand outside this rect to draw f.ex shadows. */
	void setRect(const TBRect &rect);
	inline const TBRect& getRect() const {
		return m_rect;
	}

	/** Set position of this widget in its parent. The position is relative to the parent widget. */
	void setPosition(const TBPoint &pos) {
		setRect(TBRect(pos.x, pos.y, m_rect.w, m_rect.h));
	}

	/** Set size of this widget. */
	void setSize(int width, int height) {
		int dw = width - m_rect.w;
		int dh = height - m_rect.h;
		TBRect rect(m_rect.x, m_rect.y, width, height);
		// If the widget is anchored to an edge, move its origin appropriately
		if ((m_gravity & WIDGET_GRAVITY_RIGHT) && !((m_gravity & WIDGET_GRAVITY_LEFT)))
			rect.x -= dw;
		if ((m_gravity & WIDGET_GRAVITY_BOTTOM) && !((m_gravity & WIDGET_GRAVITY_TOP)))
			rect.y -= dh;
		setRect(rect);
	}

	void execute(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);

	/** Invalidate should be called if the widget need to be repainted,
		to make sure the renderer repaints it and its children next frame. */
	void invalidate();

	/** Call if something changes that might need other widgets to update their state.
		F.ex if a action availability changes, some widget might have to become enabled/disabled.
		Calling this will result in a later call to OnProcessStates().
		This is done automatically for all invoked events of type:
			EVENT_TYPE_CLICK, EVENT_TYPE_LONG_CLICK, EVENT_TYPE_CHANGED, EVENT_TYPE_KEYDOWN,
			EVENT_TYPE_KEYUP. */
	void invalidateStates();

	/** Call if something changes that might cause any skin to change due to different state
		or conditions. This is called automatically from InvalidateStates(), when event
		EVENT_TYPE_CHANGED is invoked, and in various other situations. */
	void invalidateSkinStates();

	/** Delete the widget with the possibility for some extended life during animations.

		If any widget listener responds true to onWidgetDying it will be kept as a child and live
		until the animations are done, but the widgets and all its children are marked as dying.
		Dying widgets get no input or focus.

		If no widget listener responded, it will be deleted immediately. */
	void die();

	/** Return true if this widget or any of its parents is dying. */
	bool getIsDying() const {
		return m_packed.is_dying || ((m_parent != nullptr) && m_parent->getIsDying());
	}

	/** Set the id reference for this widgets. This id is 0 by default.
		You can use this id to receive the widget from GetWidgetByID (or
		preferable TBSafeGetByID to avoid dangerous casts). */
	void setID(const TBID &id);
	TBID &getID() {
		return m_id;
	}

	/** Set the group id reference for this widgets. This id is 0 by default.
		All widgets with the same group id under the same group root will
		be automatically changed when one change its value. */
	void setGroupID(const TBID &id) {
		m_group_id = id;
	}
	TBID &getGroupID() {
		return m_group_id;
	}

	/** Get this widget or any child widget with a matching id, or nullptr if none is found. */
	TBWidget *getWidgetByID(const TBID &id) {
		return getWidgetByIDInternal(id);
	}

	/** Get this widget or any child widget with a matching id and type, or nullptr if none is found. */
	template <class T> T *getWidgetByIDAndType(const TBID &id) {
		return (T *)getWidgetByIDInternal(id, getTypeId<T>());
	}

	/** Enable or disable the given state(s). The state affects which skin state is used when drawing.
		Some states are set automatically on interaction. See GetAutoState(). */
	void setState(WIDGET_STATE state, bool on);

	/** Get status of the given state(s). Returns true if the given state combination is set. */
	bool getState(WIDGET_STATE state) const {
		return (m_state & state) != 0U;
	}

	/** Set the widget state. Like SetState but setting the entire state as given, instead
		of toggling individual states. See SetState for more info on states. */
	void setStateRaw(WIDGET_STATE state);

	/** Get the widget state. */
	WIDGET_STATE getStateRaw() const {
		return m_state;
	}

	/** Return the current combined state for this widget. It will also add some
		automatic states, such as hovered (if the widget is currently hovered), or pressed etc.

		Automatic states: WIDGET_STATE_PRESSED, WIDGET_STATE_HOVERED, WIDGET_STATE_FOCUSED.

		Remarks for WIDGET_STATE_FOCUSED: May also be controlled by calling SetAutoFocusState and
		the define TB_ALWAYS_SHOW_EDIT_FOCUS. */
	WIDGET_STATE getAutoState() const;

	/** Set if the state WIDGET_STATE_FOCUSED should be set automatically for the focused widget.
		This value is set to true when moving focus by keyboard, and set to off when clicking
		with the pointer. */
	static void setAutoFocusState(bool on);

	/** Set opacity for this widget and its children from 0.0 - 1.0.
		If opacity is 0 (invisible), the widget won't receive any input. */
	void setOpacity(float opacity);
	float getOpacity() const {
		return m_opacity;
	}

	/** Set visibility for this widget and its children.
		If visibility is not WIDGET_VISIBILITY_VISIBLE, the widget won't receive any input. */
	void setVisibility(WIDGET_VISIBILITY vis);
	WIDGET_VISIBILITY getVisibility() const;

	/** Return true if this widget and all its ancestors are visible
		(has a opacity > 0 and visibility WIDGET_VISIBILITY_VISIBLE) */
	bool getVisibilityCombined() const;

	/** Return true if this widget or any of its parents are disabled (has state WIDGET_STATE_DISABLED). */
	bool getDisabled() const;

	/** Add the child to this widget. The child widget will automatically be deleted when
		this widget is deleted. (If the child isn't removed again with RemoveChild.) */
	void addChild(TBWidget *child, WIDGET_Z z = WIDGET_Z_TOP, WIDGET_INVOKE_INFO info = WIDGET_INVOKE_INFO_NORMAL);

	/** Add the child to this widget. See AddChild for adding a child to the top or bottom.
		This takes a relative Z and insert the child before or after the given reference widget.*/
	void addChildRelative(TBWidget *child, WIDGET_Z_REL z, TBWidget *reference,
						  WIDGET_INVOKE_INFO info = WIDGET_INVOKE_INFO_NORMAL);

	/** Remove child from this widget without deleting it. */
	void removeChild(TBWidget *child, WIDGET_INVOKE_INFO info = WIDGET_INVOKE_INFO_NORMAL);

	/** Remove this widget from parent if it has one. */
	void removeFromParent() {
		if (m_parent != nullptr) {
			m_parent->removeChild(this);
		}
	}

	/** Remove and delete all children in this widget.
		Note: This won't invoke Die so there's no chance for widgets to survive or
		animate. They will be instantly removed and deleted. */
	void deleteAllChildren();

	/** Sets the z-order of this widget related to its siblings. When a widget is added with AddChild, it will be
		placed at the top in the parent (Above previously added widget). SetZ can be used to change the order. */
	void setZ(WIDGET_Z z);

	/** Set the z order in which children are added during resource loading. */
	void setZInflate(WIDGET_Z z) {
		m_packed.inflate_child_z = z;
	}
	WIDGET_Z getZInflate() const {
		return (WIDGET_Z)m_packed.inflate_child_z;
	}

	/** Set the widget gravity (any combination of WIDGET_GRAVITY).
		For child widgets in a layout, the gravity affects how the layout is done depending on the layout settings.
		For child widgets in a non layout widget, it will do some basic resizing/moving:
			-left && right: Widget resize horizontally when parent resize.
			-!left && right: Widget follows the right edge when parent resize.
			-top && bottom: Widget resize vertically when parent resize.
			-!top && bottom: Widget follows the bottom edge when parent resize. */
	void setGravity(WIDGET_GRAVITY g);
	WIDGET_GRAVITY getGravity() const {
		return m_gravity;
	}

	/** Set the skin background for this widget and call onSkinChanged if it changed.

		The skin background is used for calculating padding, preferred size
		etc. if the widget doesn't have any preferences itself.

		The skin will be painted according to the current widget state (WIDGET_STATE).
		If there is no special skin state for WIDGET_STATE_FOCUSED, it will paint the skin
		element called "generic_focus" (if it exist) after painting all widget children.

		It's possible to omit the OnSkinChanged callback using WIDGET_INVOKE_INFO_NO_CALLBACKS. */
	void setSkinBg(const TBID &skin_bg, WIDGET_INVOKE_INFO info = WIDGET_INVOKE_INFO_NORMAL);

	/** Return the current skin background, as set by SetSkinBg. */
	TBID getSkinBg() const {
		return m_skin_bg;
	}

	/** Return the skin background element, or nullptr. */
	TBSkinElement *getSkinBgElement();

	/** Set if this widget is a group root. Grouped widgets (such as TBRadioButton) will toggle all other
		widgets with the same group_id under the nearest parent group root. TBWindow is a group root by default. */
	void setIsGroupRoot(bool groupRoot) {
		m_packed.is_group_root = groupRoot;
	}
	bool getIsGroupRoot() const {
		return m_packed.is_group_root;
	}

	/** Set if this widget should be able to receive focus or not. */
	void setIsFocusable(bool focusable) {
		m_packed.is_focusable = focusable;
	}
	bool getIsFocusable() const {
		return m_packed.is_focusable;
	}

	void setWantCaptureOnHover(bool want_capture_on_hover) {
		m_packed.want_capture_on_hover = want_capture_on_hover;
	}
	bool getWantCaptureOnHover() const {
		return m_packed.want_capture_on_hover;
	}

	void setWantFocusOnHover(bool want_focus_on_hover) {
		m_packed.want_focus_on_hover = want_focus_on_hover;
	}
	bool getWantFocusOnHover() const {
		return m_packed.want_focus_on_hover;
	}

	/** Set if this widget should emulate a click when it's focused and pressing enter or space. */
	void setClickByKey(bool clickByKey) {
		m_packed.click_by_key = clickByKey;
	}
	bool getClickByKey() const {
		return m_packed.click_by_key;
	}

	/** Set if this widget should generate long-click event (or context menu event if nothing
		handles the long click event). The default is false. */
	void setWantLongClick(bool wantLongClick) {
		m_packed.want_long_click = wantLongClick;
	}
	bool getWantLongClick() const {
		return m_packed.want_long_click;
	}

	/** Set if this widget should ignore input, as if it didn't exist. */
	void setIgnoreInput(bool ignoreInput) {
		m_packed.ignore_input = ignoreInput;
	}
	bool getIgnoreInput() const {
		return m_packed.ignore_input;
	}

	/** Get if this widget wants interaction depending on various states.
		Cares about zero opacity, visibility, flag set by SetIgnoreInput, disabled state,
		and if the widget is currently dying. */
	bool getIsInteractable() const;

	/** Set this widget to be the focused widget. It will be the one receiving keyboard input.
		Widgets can be focused only after enabling it (See SetIsFocusable(true)).
		Invisible or disabled widgets can not be focused.

		If SetFocus is called on a widget in a inactive window, it will succeed (return true),
		but it won't actually become focused until that window is activated (See TBWindow::setLastFocus).

		Returns true if successfully focused, or if set as last focus in its window. */
	bool setFocus(WIDGET_FOCUS_REASON reason, WIDGET_INVOKE_INFO info = WIDGET_INVOKE_INFO_NORMAL);
	bool getIsFocused() const {
		return focused_widget == this;
	}

	/** Call SetFocus on all children and their children, until a widget is found that accepts it.
		Returns true if some child was successfully focused. */
	bool setFocusRecursive(WIDGET_FOCUS_REASON reason = WIDGET_FOCUS_REASON_UNKNOWN);

	/** Move focus from the currently focused widget to another focusable widget. It will search
		for a focusable widget in the same TBWindow (or top root if there is no window) forward or
		backwards in the widget order. */
	bool moveFocus(bool forward);

	/** Returns the child widget that contains the coordinate or nullptr if no one does. If include_children
		is true, the search will recurse into the childrens children. */
	TBWidget *getWidgetAt(int x, int y, bool include_children) const;

	/** Get the child at the given index, or nullptr if there was no child at that index.
		Note: Avoid calling this in loops since it does iteration. Consider iterating
		the widgets directly instead! */
	TBWidget *getChildFromIndex(int index) const;

	/** Get the child index of the given widget (that must be a child of this widget).
		Note: Avoid calling this in loops since it does iteration. Consider iterating
		the widgets directly instead! */
	int getIndexFromChild(TBWidget *child) const;

	/** Get the text of a child widget with the given id, or an empty string if there was
		no widget with that id. */
	TBStr getTextByID(const TBID &id);

	/** Get the value of a child widget with the given id, or 0 if there was no widget
		with that id. */
	int getValueByID(const TBID &id);

	TBWidget *getNextDeep(const TBWidget *bounding_ancestor = nullptr) const;
	TBWidget *getPrevDeep() const;
	TBWidget *getLastLeaf() const;
	inline TBWidget *getFirstChild() const {
		return m_children.getFirst();
	}
	inline TBWidget *getLastChild() const {
		return m_children.getLast();
	}
	TBLinkListOf<TBWidget>::Iterator getIteratorForward() {
		return m_children.iterateForward();
	}
	TBLinkListOf<TBWidget>::Iterator getIteratorBackward() {
		return m_children.iterateBackward();
	}

	/** Return true if this widget is the same or a ancestor of other_widget. */
	bool isAncestorOf(TBWidget *other_widget) const;

	/** Return true if this widget is the same as other_widget or if other_widget
		events are going through this widget (See GetEventDestination()) */
	bool isEventDestinationFor(TBWidget *other_widget) const;

	// == Callbacks ==============================================

	/** Add a listener to this widget. It should be removed again with
		RemoveListener before the widget is deleted. */
	void addListener(TBWidgetListener *listener);
	void removeListener(TBWidgetListener *listener);
	bool hasListener(TBWidgetListener *listener) const;

	/** Callback for handling events.
		Return true if the event is handled and should not
		continue to be handled by any parent widgets. */
	virtual bool onEvent(const TBWidgetEvent &ev) {
		return false;
	}

	/** Callback for doing anything that might be needed before paint.
		F.ex Updating invalid layout, formatting text etc. */
	virtual void onProcess() {
	}

	/** Callback for doing anything that might be needed before paint.
		This is called after OnProcess has been called on this widgets children. */
	virtual void onProcessAfterChildren() {
	}

	/** Callback for doing state updates that depend on your application state.
		F.ex setting the disabled state on a widget whose action is currently not
		available. This callback is called for all widgets before OnProcess if
		something has called InvalidateStates().*/
	virtual void onProcessStates() {
	}

	/** PaintProps contains properties needed for painting a widget.
		Properties may be inherited from the parent widget if not specified
		in the skin. */
	class PaintProps {
	public:
		PaintProps();

		/** Text color as specified in the skin element, or inherited from parent. */
		TBColor text_color;
	};

	/** Callback for painting this widget.
		The skin is already painted and the opacity set to reflect this widgets.
		This is only called for widgets with a opacity > 0 */
	virtual void onPaint(const PaintProps &paintProps) {
	}

	/** Callback for painting child widgets.
		The default implementation is painting all children. */
	virtual void onPaintChildren(const PaintProps &paint_props);

	/** Callback for when this widget or any of its children have
		called Invalidate() */
	virtual void onInvalid() {
	}

	/** Called when the background skin changes by calling SetSkinBg(), or when the skin
		has changed indirectly after a skin condition changes in a way that may affect layout.

		For indirect skin changes, OnSkinChanged is called before validation of layouts is about
		to happen in InvokeProcess().
	*/
	virtual void onSkinChanged() {
	}

	/** Called when the font has changed. */
	virtual void onFontChanged() {
	}

	/** Called when the focus has changed. */
	virtual void onFocusChanged(bool focused) {
	}

	/** Called when the visibility has changed.
		Note: This is not called when combined visibility change, so it may change visibility
		because of ancestors without this being called. */
	virtual void onVisibilityChanged() {
	}

	/** Called when the capture has changed. */
	virtual void onCaptureChanged(bool captured) {
	}

	/** Called when a child widget has been added to this widget (before calling OnAdded on child). */
	virtual void onChildAdded(TBWidget *child) {
	}

	/** Called when a child widget is about to be removed from this widget (before calling OnRemove on child). */
	virtual void onChildRemove(TBWidget *child) {
	}

	/** Called when this widget has been added to a parent (after calling OnChildAdded on parent). */
	virtual void onAdded() {
	}

	/** Called when a this widget has been removed from its parent (after calling OnChildRemove on parent). */
	virtual void onRemove() {
	}

	/** Called when Die() is called on this widget. Note: Not called for children to the widget Die() was
		invoked on even though they are also dying. */
	virtual void onDie() {
	}

	/** Called when this widget has been resized.
		The default implementation move and resize all children according to their gravity. */
	virtual void onResized(int oldW, int oldH);

	/** Called when this widget has been scrolled. */
	virtual void onScroll(int scrollX, int scrollY) {
	}

	/** Called just after a child have been inflated into this widget.
		The default implementation will resize the child to it's preferred size
		and position it according to the gravity. If you implement a layouting
		widget, you should override this to prevent doing unnecessary measuring. */
	virtual void onInflateChild(TBWidget *child);

	/** Called when this widget is inflated from resources, before any children
		have been inflated. This will read generic widget properties and add the
		widget to the hierarchy if it's not already added. If overridden, you
		must call the super implementation. */
	virtual void onInflate(const INFLATE_INFO &info);

	/** Get hit status tests if this widget should be hit at the given coordinate.
		The default implementation checks the visibility, ignored input flag, rectangle,
		and disabled status. */
	virtual WIDGET_HIT_STATUS getHitStatus(int x, int y);

	/** Get if skin condition applies to this widget. This is called when a skin condition has the property
		PROPERTY_CUSTOM (not a generic one known by skin and the default widget condition context).
		This can be used to extend the skin conditions support with properties specific to different widgets. */
	virtual bool getCustomSkinCondition(const TBSkinCondition::CONDITION_INFO &info) {
		return false;
	}

	/** Get this widget or a child widget that should be root for other children. This is useful
		for widgets having multiple children by default, to specify which one that should get the children. */
	virtual TBWidget *getContentRoot() {
		return this;
	}

	/** Get this widget or a parent widget that is the absolute root parent. */
	TBWidget *getParentRoot();

	/** Get the closest parent widget that is a TBWindow or nullptr if there is none.
		If this widget is a window itself, this will be returned. */
	TBWindow *getParentWindow();

	/** Get the parent widget, or nullptr if this widget is not added. */
	inline TBWidget *getParent() const {
		return m_parent;
	}

	/** Get the widget that should receive the events this widget invoke. By default the parent. */
	virtual TBWidget *getEventDestination() const {
		return m_parent;
	}

	/** Return translation the children should have. Any scrolling of child widgets
		should be done with this method, by returning the wanted translation.

		When reimplementing this, you must also implement ScrollTo and GetScrollInfo
		so focus-scroll and panning will work automatically when dragging this or
		any child widget. Note: You can apply the translation on one widget and
		implement those methods on a parent, by returning this widget from the
		parents GetScrollRoot(). */
	virtual void getChildTranslation(int &x, int &y) const {
		x = y = 0;
	}

	/** If this is a widget that scroll children (see GetChildTranslation), it should
		scroll to the coordinates x, y.
		This must result in calling OnScroll if scrolling occured. */
	virtual void scrollTo(int x, int y) {
	}

	/** Start the TBScroller for this widget and scroll it to the given position.
		Will cancel any on going smooth scroll operation. */
	void scrollToSmooth(int x, int y);

	/** If this is a widget that scroll children (see GetChildTranslation), it will
		scroll by delta dx, dy relative to its current position. */
	void scrollBy(int dx, int dy);

	/** Start the TBScroller for this widget and scroll it by the given delta.
		Consecutive calls will accumulate the scroll speed. */
	void scrollBySmooth(int dx, int dy);

	/** Information about scrolling for a widget at the time of calling GetScrollInfo. */
	class ScrollInfo {
	public:
		ScrollInfo() : min_x(0), min_y(0), max_x(0), max_y(0), x(0), y(0) {
		}
		bool canScrollX() const {
			return max_x > min_x;
		}
		bool canScrollY() const {
			return max_y > min_y;
		}
		bool canScrollLeft() const {
			return x > min_x;
		}
		bool canScrollRight() const {
			return x < max_x;
		}
		bool canScrollUp() const {
			return y > min_y;
		}
		bool canScrollDown() const {
			return y < max_y;
		}
		bool canScroll() const {
			return canScrollX() || canScrollY();
		}
		int min_x, min_y; ///< Minimum x and y scroll position.
		int max_x, max_y; ///< Maximum x and y scroll position.
		int x, y;		  ///< Current x and y scroll position.
	};

	/** If this is a widget that scroll children (see GetChildTranslation),
		it should return the current scroll information. */
	virtual ScrollInfo getScrollInfo() {
		return ScrollInfo();
	}

	/** If this widget is implementing ScrollTo and GetScrollInfo but
		the corresponding GetChildTranslation is implemented on a child,
		you should return that child from this method. */
	virtual TBWidget *getScrollRoot() {
		return this;
	}

	/** Scroll this widget and/or any parent widgets by the given delta.
		dx and dy will be reduced by the amount that was successfully
		scrolled. */
	void scrollByRecursive(int &dx, int &dy);

	/** Make this widget visible by calling ScrollIntoView on all parent widgets */
	void scrollIntoViewRecursive();

	/** If this is a widget that scroll children (see GetChildTranslation), it will
		scroll so that rect is visible. Rect is relative to this widget. */
	void scrollIntoView(const TBRect &rect);

	/** Return the TBScroller set up for this widget, or nullptr if creation failed. */
	TBScroller *getScroller();

	// == Setter shared for many types of widgets ============

	/** Set along which axis the content should be layouted. */
	virtual void setAxis(AXIS axis) {
	}
	virtual AXIS getAxis() const {
		return AXIS_X;
	}

	/** Set the value of this widget. Implemented by most widgets (that has a value).
		Note: Some widgets also provide special setters with other types (such as double). */
	virtual void setValue(int value) {
	}
	virtual int getValue() const {
		return 0;
	}

	/** Set the value in double precision. It only makes sense to use this instead
		of setValue() on widgets that store the value as double. F.ex TBScrollBar, TBSlider. */
	virtual void setValueDouble(double value) {
		setValue((int)value);
	}

	/** Return the value in double precision. It only makes sense to use this instead
		of getValue() on widgets that store the value as double. F.ex TBScrollBar, TBSlider. */
	virtual double getValueDouble() const {
		return (double)getValue();
	}

	/** Set the text of this widget. Implemented by most widgets (that has text). */
	virtual bool setText(const char *text) {
		return true;
	}

	/** Get the text of this widget. Implemented by most widgets (that has text).
		returns false if it failed. */
	virtual bool getText(TBStr &text) {
		text.clear();
		return true;
	}

	/** Get the text of this widget. Implemented by most widgets (that has text). */
	TBStr getText() {
		TBStr str;
		getText(str);
		return str;
	}

	/** Connect this widget to a widget value.

		When this widget invoke EVENT_TYPE_CHANGED, it will automatically update the
		connected widget value, and any other widgets that may be connected to it.

		On connection, the value of this widget will be updated to the value of the
		given TBWidgetValue. */
	void connect(TBWidgetValue *value) {
		m_connection.connect(value, this);
	}

	/** Unconnect, if this widget is connected to a TBWidgetValue. */
	void unconnect() {
		m_connection.unconnect();
	}

	/** Get the rectangle inside any padding, relative to this widget. This is the
		rectangle in which the content should be rendered.

		This may be overridden to f.ex deduct space allocated by visible scrollbars
		managed by this widget. Anything that removes space from the content area. */
	virtual TBRect getPaddingRect();

	/** Calculate the preferred content size for this widget. This is the size of the actual
		content. Don't care about padding or other decoration. */
	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints);

	/** Calculate the preferred size for this widget. This is the full size of the widget,
		content + padding + eventual other decoration (but not skin expansion).
		This is the size that should be used for layouting a widget.
		The returned PreferredSize also contains minimal size and maximum size. */
	virtual PreferredSize onCalculatePreferredSize(const SizeConstraints &constraints);

	/** Get the PreferredSize for this widget.
		This returns cached data if valid, or calls OnCalculatePreferredSize if needed. */
	PreferredSize getPreferredSize(const SizeConstraints &constraints);
	PreferredSize getPreferredSize() {
		return getPreferredSize(SizeConstraints());
	}

	/** Type used for InvalidateLayout */
	enum INVALIDATE_LAYOUT {
		INVALIDATE_LAYOUT_TARGET_ONLY, ///< InvalidateLayout should not be recursively called on parents.
		INVALIDATE_LAYOUT_RECURSIVE	///< InvalidateLayout should recursively be called on parents too.
	};

	/** Invalidate layout for this widget so it will be scheduled for relayout.
		Any change to the size preferences for a widget should call this to let parent layout adjust to the change.

		Remarks for layout widgets:
		- When a layout widget get this, it should mark its layout as invalid and do the layout later
		  (in GetPreferredContentSize/GetPreferredSize are called). If a layout knows that no parents will
		  be affected, it may stop recursion to parents to avoid unnecessary relayout.
		- When setting the size of a layout widget (typically from another layout widget or from a OnResize),
		  it should be called with INVALIDATE_LAYOUT_TARGET_ONLY to avoid recursing back up to parents when
		  already recursing down, to avoid unnecessary computation.
		*/
	virtual void invalidateLayout(INVALIDATE_LAYOUT il);

	/** Set layout params. Calls InvalidateLayout. */
	void setLayoutParams(const LayoutParams &lp);

	/** Get layout params, or nullptr if not specified.
		Note: The layout params has already been applied to the PreferredSize returned
		from GetPreferredSize so you normally don't need to check these params. */
	const LayoutParams *getLayoutParams() const {
		return m_layout_params;
	}

	// == Misc methods for invoking events. Should normally be called only on the root widget ===============

	/** Invoke OnProcess and OnProcessAfterChildren on this widget and its children. */
	void invokeProcess();

	/** Invoke OnProcessStates on all child widgets, if state processing
		is needed (InvalidateStates() has been called) */
	void invokeProcessStates(bool force_update = false);

	/** Invoke paint on this widget and all its children */
	void invokePaint(const PaintProps &parent_paint_props);

	/** Invoke OnFontChanged on this widget and recursively on any children that inherit the font. */
	void invokeFontChanged();

	/** Invoke a event on this widget.

		This will first check with all registered TBWidgetListener if the event should be dispatched.

		If the widgets onEvent returns false (event not handled), it will continue traversing to
		GetEventDestination (by default the parent) until a widget handles the event.

		Note: When invoking event EVENT_TYPE_CHANGED, this will update the value of other widgets connected
			  to the same group.

		Note: Some event types will automatically invalidate states (See InvalidateStates(), InvalidateSkinStates())

		Note: Remember that this widgets may be deleted after this call! So if you really must do something after
		this call and are not sure what the event will cause, use TBWidgetSafePointer to detect self deletion. */
	bool invokeEvent(TBWidgetEvent &ev);

	bool invokePointerDown(int x, int y, int click_count, MODIFIER_KEYS modifierkeys, BUTTON_TYPE type);
	bool invokePointerUp(int x, int y, MODIFIER_KEYS modifierkeys, BUTTON_TYPE type);
	void invokePointerMove(int x, int y, MODIFIER_KEYS modifierkeys, BUTTON_TYPE type);
	void invokePointerCancel();

	/** Invoke touch events with ref_id set as the given id.
		Note: For id 0, it will invoke EVENT_TYPE_POINTER_DOWN (with touch flag set to true), and EVENT_TYPE_TOUCH_DOWN
		for other id > 0. This results in normal interaction for first finger, and optional handling of other
		simultaneous interaction. GetTouchInfo(id) can be used to get additional interaction info. */
	bool invokeTouchDown(int x, int y, uint32_t id, int click_count, MODIFIER_KEYS modifierkeys);
	bool invokeTouchUp(int x, int y, uint32_t id, MODIFIER_KEYS modifierkeys);
	void invokeTouchMove(int x, int y, uint32_t id, MODIFIER_KEYS modifierkeys);
	void invokeTouchCancel(uint32_t id);

	bool invokeWheel(int x, int y, int deltaX, int deltaY, MODIFIER_KEYS modifierkeys);

	/** Invoke the EVENT_TYPE_KEY_DOWN and EVENT_TYPE_KEY_UP events on the currently focused widget.
		This will also do some generic key handling, such as cycling focus on tab etc. */
	bool invokeKey(int key, SPECIAL_KEY special_key, MODIFIER_KEYS modifierkeys, bool down);

	/** A widget that receive a EVENT_TYPE_POINTER_DOWN event, will stay "captured" until EVENT_TYPE_POINTER_UP
		is received. While captured, all EVENT_TYPE_POINTER_MOVE are sent to it. This method can force release the
	   capture, which may happen f.ex if the TBWidget is removed while captured. */
	void releaseCapture();

	/** Make x and y (relative to this widget) relative to the upper left corner of the root widget. */
	void convertToRoot(int &x, int &y) const;

	/** Make x and y (relative to the upper left corner of the root widget) relative to this widget. */
	void convertFromRoot(int &x, int &y) const;

	/** Set the font description for this widget and any children that inherit the font.

		Setting a unspecified TBFontDescription (no changes made since construction) means
		it will be inherited from parent (the default).

		Returns true and invokes OnFontChanged on all affected widgets, if the
		font was successfully set.

		Returns false and keep the font onchanged if it no matching font exists or fails creation. */
	bool setFontDescription(const TBFontDescription &font_desc);

	/** Get the font description as set with SetFontDescription. Use GetCalculatedFontDescription()
		to get the calculated font description (Inherit from parent widget etc.) */
	TBFontDescription getFontDescription() const {
		return m_font_desc;
	}

	/** Calculate the font description for this widget. If this widget have unspecified font
		description, it will be inheritted from parent. If no parent specify any font,
		the default font description will be returned. */
	TBFontDescription getCalculatedFontDescription() const;

	/** Get the TBFontFace for this widget from the current font description (calculated
		by GetCalculatedFontDescription) */
	TBFontFace *getFont() const;

private:
	friend class TBWidgetListener;				///< It does iteration of m_listeners for us.
	TBWidget *m_parent;							///< The parent of this widget
	TBRect m_rect;								///< The rectangle of this widget, relative to the parent. See SetRect.
	TBID m_id;									///< ID for GetWidgetByID and others.
	TBID m_group_id;							///< ID for button groups (such as TBRadioButton)
	TBID m_skin_bg;								///< ID for the background skin (0 for no skin).
	TBID m_skin_bg_expected;					///< ID for the background skin after strong override,
												///< used to indirect skin changes because of condition changes.
	TBLinkListOf<TBWidget> m_children;			///< List of child widgets
	TBWidgetValueConnection m_connection;		///< TBWidget value connection
	TBLinkListOf<TBWidgetListener> m_listeners; ///< List of listeners
	float m_opacity;							///< Opacity 0-1. See SetOpacity.
	WIDGET_STATE m_state;						///< The widget state (excluding any auto states)
	WIDGET_GRAVITY m_gravity;					///< The layout gravity setting.
	TBFontDescription m_font_desc;				///< The font description.
	PreferredSize m_cached_ps;					///< Cached preferred size.
	SizeConstraints m_cached_sc;				///< Cached size constraints.
	LayoutParams *m_layout_params;				///< Layout params, or nullptr.
	TBScroller *m_scroller;
	TBLongClickTimer *m_long_click_timer;
	union {
		struct {
			uint16_t is_group_root : 1;
			uint16_t is_focusable : 1;
			uint16_t want_capture_on_hover : 1;
			uint16_t want_focus_on_hover : 1;
			uint16_t click_by_key : 1;
			uint16_t has_key_pressed_state : 1;
			uint16_t ignore_input : 1;
			uint16_t is_dying : 1;
			uint16_t is_cached_ps_valid : 1;
			uint16_t no_automatic_hover_state : 1;
			uint16_t is_panning : 1;
			uint16_t want_long_click : 1;
			uint16_t visibility : 2;
			uint16_t inflate_child_z : 1; // Should have enough bits to hold WIDGET_Z values.
		} m_packed;
		uint16_t m_packed_init;
	};

public:
	/** This value is free to use for anything. It's not used by TBWidget itself. Initially TYPE_NULL. */
	TBValue data;

	// Debugging
#ifdef TB_RUNTIME_DEBUG_INFO
	double last_measure_time;
	double last_layout_time;
#endif // TB_RUNTIME_DEBUG_INFO

	// TBWidget related globals
	static TBWidget *hovered_widget;  ///< The currently hovered widget, or nullptr.
	static TBWidget *captured_widget; ///< The currently captured widget, or nullptr.
	static TBWidget *focused_widget;  ///< The currently focused widget, or nullptr.
	static int pointer_down_widget_x; ///< Pointer x position on down event, relative to the captured widget.
	static int pointer_down_widget_y; ///< Pointer y position on down event, relative to the captured widget.
	static int pointer_move_widget_x; ///< Pointer x position on last pointer event, relative to the captured widget (if
									  ///< any) or hovered widget.
	static int pointer_move_widget_y; ///< Pointer y position on last pointer event, relative to the captured widget (if
									  ///< any) or hovered widget.
	static bool cancel_click;		  ///< true if the pointer up event should not generate a click event.
	static bool
		update_widget_states; ///< true if something has called invalidateStates() and it still hasn't been updated.
	static bool
		update_skin_states; ///< true if something has called invalidateSkinStates() and skin still hasn't been updated.
	static bool show_focus_state; ///< true if the focused state should be painted automatically.
	struct TOUCH_INFO {
		TBWidget *hovered_widget;  ///< The currently hovered widget, or nullptr.
		TBWidget *captured_widget; ///< The currently captured widget, or nullptr.
		int down_widget_x;		   ///< Touch x position on down event, relative to the captured widget.
		int down_widget_y;		   ///< Touch y position on down event, relative to the captured widget.
		int move_widget_x;		   ///< Touch x position on last touch event, relative to the captured widget.
		int move_widget_y;		   ///< Touch y position on last touch event, relative to the captured widget.
	};
	/** Return TOUCH_INFO for the given id, or nullptr if no touch is active for that id. */
	static TOUCH_INFO *getTouchInfo(uint32_t id);

private:
	/** Return this widget or the nearest parent that is scrollable
		in the given axis, or nullptr if there is none. */
	TBWidget *findScrollableWidget(bool scrollX, bool scrollY);
	TBScroller *findStartedScroller();
	TBScroller *getReadyScroller(bool scrollX, bool scrollY);
	TBWidget *getWidgetByIDInternal(const TBID &id, TB_TYPE_ID type_id = nullptr);
	void invokeSkinUpdatesInternal(bool force_update);
	void invokeProcessInternal();
	static void setHoveredWidget(TBWidget *widget, bool touch);
	static void setCapturedWidget(TBWidget *widget);
	void handlePanningOnMove(int x, int y);
	void startLongClickTimer(BUTTON_TYPE type);
	void stopLongClickTimer();
	friend class TBLongClickTimer;
	void maybeInvokeLongClickOrContextMenu(BUTTON_TYPE type);
	/** Returns the opacity for this widget multiplied with its skin opacity and state opacity. */
	float calculateOpacityInternal(WIDGET_STATE state, TBSkinElement *skin_element) const;
};

inline bool TBWidgetEvent::isAny(const tb::TBID &id) const {
	return target->getID() == id || ref_id == id;
}

} // namespace tb
