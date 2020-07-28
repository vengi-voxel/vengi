/**
 * @file
 */

#pragma once

#include "core/Var.h"
#include "tb_layout.h"
#include "tb_msg.h"
#include "tb_widgets.h"

namespace tb {

/** TB_TEXT_ALIGN specifies horizontal text alignment */
enum TB_TEXT_ALIGN {
	TB_TEXT_ALIGN_LEFT,  ///< Aligned left
	TB_TEXT_ALIGN_RIGHT, ///< Aligned right
	TB_TEXT_ALIGN_CENTER ///< Aligned center
};

/** TBWidgetString holds a string that can be painted as one line with the set alignment. */
class TBWidgetString {
public:
	TBWidgetString();

	void paint(TBWidget *widget, const TBRect &rect, const TBColor &color);

	int getWidth(TBWidget *widget);
	int getHeight(TBWidget *widget);

	bool setText(const char *text);
	bool getText(core::String &text) const {
		text = m_text;
		return true;
	}

	bool isEmpty() const {
		return m_text.empty();
	}
	bool equals(const char *str) const {
		return m_text.equals(str);
	}

	/** Set which alignment the text should have if the space
		given when painting is larger than the text. */
	void setTextAlign(TB_TEXT_ALIGN align) {
		m_text_align = align;
	}
	TB_TEXT_ALIGN getTextAlign() const {
		return m_text_align;
	}

private:
	core::String m_text;
	TB_TEXT_ALIGN m_text_align;
	// Cached data
	int m_width, m_height;
	TBFontDescription m_fd;
	void validatCachedSize(TBWidget *widget);
};

/** TBTextField is a one line text field that is not editable. */

class TBTextField : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBTextField, TBWidget);

	TBTextField();

	/** Set the text of the text field. */
	virtual bool setText(const char *text) override;
	bool setText(const core::String& text) {
		return setText(text.c_str());
	}
	virtual bool getText(core::String &text) override {
		return m_text.getText(text);
	}
	using TBWidget::getText; ///< Make all versions in base class available.

	bool isEmpty() const {
		return m_text.isEmpty();
	}

	/** Set which alignment the text should have if the space
		given when painting is larger than the text. */
	void setTextAlign(TB_TEXT_ALIGN align) {
		m_text.setTextAlign(align);
	}
	TB_TEXT_ALIGN getTextAlign() {
		return m_text.getTextAlign();
	}

	/** Set if this text field should be allowed to squeeze below its
		preferred size. If squeezable it may shrink to width 0. */
	void setSqueezable(bool squeezable);
	bool getSqueezable() const {
		return m_squeezable;
	}

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints) override;
	virtual void onFontChanged() override;
	virtual void onPaint(const PaintProps &paint_props) override;

protected:
	TBWidgetString m_text;
	int m_cached_text_width; ///< Cached width of m_text
	bool m_squeezable;
};

/** TBButton is a regular button widget with auto repeat, toggle and group capabilities.
	Has a text field in its internal layout by default. Other widgets can be added
	under GetContentRoot(). */

class TBButton : public TBWidget, protected TBMessageHandler {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBButton, TBWidget);

	TBButton();
	~TBButton();

	/** Set along which axis the content should layouted (If the button has more content than the text) */
	virtual void setAxis(AXIS axis) override {
		m_layout.setAxis(axis);
	}
	virtual AXIS getAxis() const override {
		return m_layout.getAxis();
	}

	/** Set if the text field should be allowed to squeeze below its
		preferred size. If squeezable it may shrink to width 0. */
	void setSqueezable(bool squeezable) {
		m_textfield.setSqueezable(squeezable);
	}
	bool getSqueezable() const {
		return m_textfield.getSqueezable();
	}

	/** Set to true if the button should fire repeatedly while pressed. */
	void setAutoRepeat(bool autoRepeatClick) {
		m_auto_repeat_click = autoRepeatClick;
	}
	bool getAutoRepeat() const {
		return m_auto_repeat_click;
	}

	/** Set to true if the button should toggle on and off, instead of just fire
		click events. When it's on, it will have value 1 pressed state. */
	void setToggleMode(bool toggleModeOn) {
		m_toggle_mode = toggleModeOn;
	}
	bool getToggleMode() const {
		return m_toggle_mode;
	}

	/** Set the text of the button. */
	virtual bool setText(const char *text) override;
	virtual bool getText(core::String &text) override {
		return m_textfield.getText(text);
	}
	using TBWidget::getText; ///< Make all versions in base class available.
	using TBWidget::setText; ///< Make all versions in base class available.

	virtual void setValue(int value) override;
	virtual int getValue() const override;

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual void onCaptureChanged(bool captured) override;
	virtual void onProcess() override;
	virtual void onSkinChanged() override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual WIDGET_HIT_STATUS getHitStatus(int x, int y) override;
	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints) override {
		return m_layout.getPreferredSize();
	}

	virtual TBWidget *getContentRoot() override {
		return &m_layout;
	}

	// == TBMessageHandler ==============================================================
	virtual void onMessageReceived(TBMessage *msg) override;

protected:
	void updateTextFieldVisibility();
	bool canToggle() {
		return m_toggle_mode || (getGroupID() != 0U);
	}
	class ButtonLayout : public TBLayout {
		virtual void onChildAdded(TBWidget *child) override;
		virtual void onChildRemove(TBWidget *child) override;
	};
	ButtonLayout m_layout;
	TBTextField m_textfield;
	bool m_auto_repeat_click;
	bool m_toggle_mode;
	core::VarPtr _var;
	core::String _command;
};

/** TBClickLabel has a text field in its internal layout by default. Pointer input on the
	text field will be redirected to another child widget (that you add) to it.
	Typically useful for creating check boxes, radio buttons with labels. */

class TBClickLabel : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBClickLabel, TBWidget);

	TBClickLabel();
	~TBClickLabel();

	/** Set along which axis the content should layouted (If the label has more content than the text) */
	virtual void setAxis(AXIS axis) override {
		m_layout.setAxis(axis);
	}
	virtual AXIS getAxis() const override {
		return m_layout.getAxis();
	}

	/** Set the text of the label. */
	virtual bool setText(const char *text) override {
		return m_textfield.setText(text);
	}
	virtual bool getText(core::String &text) override {
		return m_textfield.getText(text);
	}
	using TBWidget::getText; ///< Make all versions in base class available.

	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints) override {
		return m_layout.getPreferredSize();
	}

	virtual TBWidget *getContentRoot() override {
		return &m_layout;
	}

	virtual bool onEvent(const TBWidgetEvent &ev) override;

protected:
	TBLayout m_layout;
	TBTextField m_textfield;
};

/** TBSkinImage is a widget showing a skin element, constrained in size to its skin.
	If you need to load and show images dynamically (i.e. not always loaded as the skin),
	you can use TBImageWidget. */

class TBSkinImage : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSkinImage, TBWidget);

	TBSkinImage() {
	}
	TBSkinImage(const TBID &skinBg) {
		setSkinBg(skinBg);
	}

	virtual PreferredSize onCalculatePreferredSize(const SizeConstraints &constraints) override;
};

/** TBSeparator is a widget only showing a skin.
	It is disabled by default. */
class TBSeparator : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSeparator, TBWidget);

	TBSeparator();
};

/** TBProgressSpinner is a animation that is running while its value is 1.
	Typically used to indicate that the application is working. */
class TBProgressSpinner : public TBWidget, protected TBMessageHandler {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBProgressSpinner, TBWidget);

	TBProgressSpinner();

	/** Return true if the animation is running. */
	bool isRunning() {
		return m_value > 0;
	}

	/** Begin/End are used to start or stop the animation in a incremental way.
		If several tasks may activate the same spinner, calling Begin/End instead
		of using setValue, so it will keep running as long as any source wants it to. */
	void begin() {
		setValue(getValue() + 1);
	}
	void end() {
		setValue(getValue() - 1);
	}

	/** Setting the value to 1 will start the spinner.
		Setting it to 0 will stop it. */
	virtual void setValue(int value) override;
	virtual int getValue() const override {
		return m_value;
	}

	virtual void onPaint(const PaintProps &paintProps) override;

	// == TBMessageHandler ==============================================================
	virtual void onMessageReceived(TBMessage *msg) override;

protected:
	int m_value;
	int m_frame;
	TBID m_skin_fg;
};

/** TBRadioCheckBox has shared functionality for TBCheckBox and TBRadioButton. */
class TBRadioCheckBox : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBRadioCheckBox, TBWidget);

	TBRadioCheckBox();

	virtual void setValue(int value) override;
	virtual int getValue() const override {
		return m_value;
	}

	virtual PreferredSize onCalculatePreferredSize(const SizeConstraints &constraints) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual void onProcess() override;

	/** Make sure all widgets sharing the same group as new_leader are set to value 0. */
	static void updateGroupWidgets(TBWidget *new_leader);

protected:
	int m_value;
	core::VarPtr _var;
	core::String _command;
};

/** TBCheckBox is a box toggling a check mark on click.
	For a labeled checkbox, use a TBClickLabel containing a TBCheckBox. */
class TBCheckBox : public TBRadioCheckBox {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBCheckBox, TBRadioCheckBox);

	TBCheckBox() {
		setSkinBg(TBIDC("TBCheckBox"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	}
};

/** TBRadioButton is a button which unselects other radiobuttons of the same
	group number when clicked.
	For a labeled radio button, use a TBClickLabel containing a TBRadioButton. */
class TBRadioButton : public TBRadioCheckBox {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBRadioButton, TBRadioCheckBox);

	TBRadioButton() {
		setSkinBg(TBIDC("TBRadioButton"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	}
};

/** TBScrollBar is a scroll bar in the given axis. */

class TBScrollBar : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBScrollBar, TBWidget);

	TBScrollBar();
	~TBScrollBar();

	/** Set along which axis the scrollbar should scroll */
	virtual void setAxis(AXIS axis) override;
	virtual AXIS getAxis() const override {
		return m_axis;
	}

	/** Set the min, max limits for the scrollbar.
		The visible parameter is how much of the range that is visible.
		When this is called, the scrollbar might change value and invoke if the current value is
		outside of the new limits. */
	void setLimits(double min, double max, double visible);

	/** Return true if the scrollbar has anywhere to go with the current limits. */
	bool canScroll() const {
		return m_visible > 0;
	}

	/** Return true if the scrollbar can scroll in the positive direction with its current limits. */
	bool canScrollPositive() const {
		return m_value < m_max;
	}

	/** Return true if the scrollbar can scroll in the negative direction with its current limits. */
	bool canScrollNegative() const {
		return m_value > m_min;
	}

	double getMinValue() const {
		return m_min;
	}
	double getMaxValue() const {
		return m_max;
	}
	double getVisible() const {
		return m_visible;
	}

	/** Same as setValue, but with double precision. */
	virtual void setValueDouble(double value) override;
	virtual double getValueDouble() const override {
		return m_value;
	}

	virtual void setValue(int value) override {
		setValueDouble(value);
	}
	virtual int getValue() const override {
		return (int)getValueDouble();
	}

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onResized(int oldW, int oldH) override;

protected:
	TBWidget m_handle;
	AXIS m_axis;
	double m_value;
	double m_min, m_max, m_visible;
	double m_to_pixel_factor;
	void updateHandle();
};

/** TBSlider is a horizontal or vertical slider for a number within a range. */

// FIX: Add a "track value" showing as a line within the track (to be used for buffering etc).
// FIX: Also add a auto track that keeps it up to date with value (default).
class TBSlider : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSlider, TBWidget);

	TBSlider();
	~TBSlider();

	/** Set along which axis the scrollbar should scroll */
	virtual void setAxis(AXIS axis) override;
	virtual AXIS getAxis() const override {
		return m_axis;
	}

	/** Set the min, max limits for the slider. */
	void setLimits(double min, double max);

	double getMinValue() const {
		return m_min;
	}
	double getMaxValue() const {
		return m_max;
	}

	/** Get a small value (depending on the min and max limits) for stepping by f.ex. keyboard. */
	double getSmallStep() const {
		return (m_max - m_min) / 100.0;
	}

	/** Same as setValue, but with double precision. */
	virtual void setValueDouble(double value) override;
	virtual double getValueDouble() const override {
		return m_value;
	}

	virtual void setValue(int value) override {
		setValueDouble(value);
	}
	virtual int getValue() const override {
		return (int)getValueDouble();
	}

	virtual void onProcess() override;
	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onResized(int oldW, int oldH) override;

protected:
	TBWidget m_handle;
	AXIS m_axis;
	double m_value;
	double m_min, m_max;
	double m_to_pixel_factor;
	core::VarPtr _var;
	core::String _command;
	void updateHandle();
};

/** TBContainer is just a TBWidget with border and padding (using skin "TBContainer") */
class TBContainer : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBContainer, TBWidget);

	TBContainer();
};

/** TBMover is moving its parent widget when dragged. */
class TBMover : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBMover, TBWidget);

	TBMover();

	virtual bool onEvent(const TBWidgetEvent &ev) override;
};

/** TBResizer is a lower right corner resize grip. It will resize its parent widget. */
class TBResizer : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBResizer, TBWidget);

	TBResizer();
	virtual WIDGET_HIT_STATUS getHitStatus(int x, int y) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;
};

/** TBDimmer dim widgets in the background and block input. */
class TBDimmer : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBDimmer, TBWidget);

	TBDimmer();

	virtual void onAdded() override;
};

} // namespace tb
