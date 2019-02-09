/**
 * @file
 */

#pragma once

#include "tb_widgets.h"
#include "tb_layout.h"
#include "tb_msg.h"

namespace tb {

/** TB_TEXT_ALIGN specifies horizontal text alignment */
enum TB_TEXT_ALIGN {
	TB_TEXT_ALIGN_LEFT,		///< Aligned left
	TB_TEXT_ALIGN_RIGHT,	///< Aligned right
	TB_TEXT_ALIGN_CENTER	///< Aligned center
};

/** TBWidgetString holds a string that can be painted as one line with the set alignment. */
class TBWidgetString
{
public:
	TBWidgetString();

	void Paint(TBWidget *widget, const TBRect &rect, const TBColor &color);

	int GetWidth(TBWidget *widget);
	int GetHeight(TBWidget *widget);

	bool SetText(const char *text);
	bool GetText(TBStr &text) const { return text.Set(m_text); }

	bool IsEmpty() const { return m_text.IsEmpty(); }
	bool Equals(const char *str) const { return m_text.Equals(str); }

	/** Set which alignment the text should have if the space
		given when painting is larger than the text. */
	void SetTextAlign(TB_TEXT_ALIGN align) { m_text_align = align; }
	TB_TEXT_ALIGN GetTextAlign() const { return m_text_align; }
private:
	TBStr m_text;
	TB_TEXT_ALIGN m_text_align;
	// Cached data
	int m_width, m_height;
	TBFontDescription m_fd;
	void ValidatCachedSize(TBWidget *widget);
};

/** TBTextField is a one line text field that is not editable. */

class TBTextField : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBTextField, TBWidget);

	TBTextField();

	/** Set the text of the text field. */
	virtual bool SetText(const char *text) override;
	virtual bool GetText(TBStr &text) override { return m_text.GetText(text); }
	using TBWidget::GetText; ///< Make all versions in base class available.

	bool IsEmpty() const { return m_text.IsEmpty(); }

	/** Set which alignment the text should have if the space
		given when painting is larger than the text. */
	void SetTextAlign(TB_TEXT_ALIGN align) { m_text.SetTextAlign(align); }
	TB_TEXT_ALIGN GetTextAlign() { return m_text.GetTextAlign(); }

	/** Set if this text field should be allowed to squeeze below its
		preferred size. If squeezable it may shrink to width 0. */
	void SetSqueezable(bool squeezable);
	bool GetSqueezable() { return m_squeezable; }

	virtual void OnInflate(const INFLATE_INFO &info) override;
	virtual PreferredSize OnCalculatePreferredContentSize(const SizeConstraints &constraints) override;
	virtual void OnFontChanged() override;
	virtual void OnPaint(const PaintProps &paint_props) override;
protected:
	TBWidgetString m_text;
	int m_cached_text_width; ///< Cached width of m_text
	bool m_squeezable;
};

/** TBButton is a regular button widget with auto repeat, toggle and group capabilities.
	Has a text field in its internal layout by default. Other widgets can be added
	under GetContentRoot(). */

class TBButton : public TBWidget, protected TBMessageHandler
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBButton, TBWidget);

	TBButton();
	~TBButton();

	/** Set along which axis the content should layouted (If the button has more content than the text) */
	virtual void SetAxis(AXIS axis) override { m_layout.SetAxis(axis); }
	virtual AXIS GetAxis() const override { return m_layout.GetAxis(); }

	/** Set if the text field should be allowed to squeeze below its
		preferred size. If squeezable it may shrink to width 0. */
	void SetSqueezable(bool squeezable) { m_textfield.SetSqueezable(squeezable); }
	bool GetSqueezable() { return m_textfield.GetSqueezable(); }

	/** Set to true if the button should fire repeatedly while pressed. */
	void SetAutoRepeat(bool auto_repeat_click) { m_auto_repeat_click = auto_repeat_click; }
	bool GetAutoRepeat() { return m_auto_repeat_click; }

	/** Set to true if the button should toggle on and off, instead of just fire
		click events. When it's on, it will have value 1 pressed state. */
	void SetToggleMode(bool toggle_mode_on) { m_toggle_mode = toggle_mode_on; }
	bool GetToggleMode() const { return m_toggle_mode; }

	/** Set the text of the button. */
	virtual bool SetText(const char *text) override;
	virtual bool GetText(TBStr &text) override { return m_textfield.GetText(text); }
	using TBWidget::GetText; ///< Make all versions in base class available.

	virtual void SetValue(int value) override;
	virtual int GetValue() const override;

	virtual void OnInflate(const INFLATE_INFO &info) override;
	virtual void OnCaptureChanged(bool captured) override;
	virtual void OnSkinChanged() override;
	virtual bool OnEvent(const TBWidgetEvent &ev) override;
	virtual WIDGET_HIT_STATUS GetHitStatus(int x, int y) override;
	virtual PreferredSize OnCalculatePreferredContentSize(const SizeConstraints &constraints) override { return m_layout.GetPreferredSize(); }

	virtual TBWidget *GetContentRoot() override { return &m_layout; }

	// == TBMessageHandler ==============================================================
	virtual void OnMessageReceived(TBMessage *msg) override;
protected:
	void UpdateTextFieldVisibility();
	bool CanToggle() { return m_toggle_mode || GetGroupID(); }
	class ButtonLayout : public TBLayout
	{
		virtual void OnChildAdded(TBWidget *child) override;
		virtual void OnChildRemove(TBWidget *child) override;
	};
	ButtonLayout m_layout;
	TBTextField m_textfield;
	bool m_auto_repeat_click;
	bool m_toggle_mode;
};

/** TBClickLabel has a text field in its internal layout by default. Pointer input on the
	text field will be redirected to another child widget (that you add) to it.
	Typically useful for creating check boxes, radio buttons with labels. */

class TBClickLabel : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBClickLabel, TBWidget);

	TBClickLabel();
	~TBClickLabel();

	/** Set along which axis the content should layouted (If the label has more content than the text) */
	virtual void SetAxis(AXIS axis) override { m_layout.SetAxis(axis); }
	virtual AXIS GetAxis() const override { return m_layout.GetAxis(); }

	/** Set the text of the label. */
	virtual bool SetText(const char *text) override { return m_textfield.SetText(text); }
	virtual bool GetText(TBStr &text) override { return m_textfield.GetText(text); }
	using TBWidget::GetText; ///< Make all versions in base class available.

	virtual PreferredSize OnCalculatePreferredContentSize(const SizeConstraints &constraints) override { return m_layout.GetPreferredSize(); }

	virtual TBWidget *GetContentRoot() override { return &m_layout; }

	virtual bool OnEvent(const TBWidgetEvent &ev) override;
protected:
	TBLayout m_layout;
	TBTextField m_textfield;
};

/** TBSkinImage is a widget showing a skin element, constrained in size to its skin.
	If you need to load and show images dynamically (i.e. not always loaded as the skin),
	you can use TBImageWidget. */

class TBSkinImage : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSkinImage, TBWidget);

	TBSkinImage() {}
	TBSkinImage(const TBID &skin_bg) { SetSkinBg(skin_bg); }

	virtual PreferredSize OnCalculatePreferredSize(const SizeConstraints &constraints) override;
};

/** TBSeparator is a widget only showing a skin.
	It is disabled by default. */
class TBSeparator : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSeparator, TBWidget);

	TBSeparator();
};

/** TBProgressSpinner is a animation that is running while its value is 1.
	Typically used to indicate that the application is working. */
class TBProgressSpinner : public TBWidget, protected TBMessageHandler
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBProgressSpinner, TBWidget);

	TBProgressSpinner();

	/** Return true if the animation is running. */
	bool IsRunning() { return m_value > 0; }

	/** Begin/End are used to start or stop the animation in a incremental way.
		If several tasks may activate the same spinner, calling Begin/End instead
		of using SetValue, so it will keep running as long as any source wants it to. */
	void Begin() { SetValue(GetValue() + 1); }
	void End() { SetValue(GetValue() - 1); }

	/** Setting the value to 1 will start the spinner.
		Setting it to 0 will stop it. */
	virtual void SetValue(int value) override;
	virtual int GetValue() const override { return m_value; }

	virtual void OnPaint(const PaintProps &paint_props) override;

	// == TBMessageHandler ==============================================================
	virtual void OnMessageReceived(TBMessage *msg) override;
protected:
	int m_value;
	int m_frame;
	TBID m_skin_fg;
};

/** TBRadioCheckBox has shared functionality for TBCheckBox and TBRadioButton. */
class TBRadioCheckBox : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBRadioCheckBox, TBWidget);

	TBRadioCheckBox();

	virtual void SetValue(int value) override;
	virtual int GetValue() const override { return m_value; }

	virtual PreferredSize OnCalculatePreferredSize(const SizeConstraints &constraints) override;
	virtual bool OnEvent(const TBWidgetEvent &ev) override;

	/** Make sure all widgets sharing the same group as new_leader are set to value 0. */
	static void UpdateGroupWidgets(TBWidget *new_leader);
protected:
	int m_value;
};

/** TBCheckBox is a box toggling a check mark on click.
	For a labeled checkbox, use a TBClickLabel containing a TBCheckBox. */
class TBCheckBox : public TBRadioCheckBox
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBCheckBox, TBRadioCheckBox);

	TBCheckBox() { SetSkinBg(TBIDC("TBCheckBox"), WIDGET_INVOKE_INFO_NO_CALLBACKS); }
};

/** TBRadioButton is a button which unselects other radiobuttons of the same
	group number when clicked.
	For a labeled radio button, use a TBClickLabel containing a TBRadioButton. */
class TBRadioButton : public TBRadioCheckBox
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBRadioButton, TBRadioCheckBox);

	TBRadioButton() { SetSkinBg(TBIDC("TBRadioButton"), WIDGET_INVOKE_INFO_NO_CALLBACKS); }
};

/** TBScrollBar is a scroll bar in the given axis. */

class TBScrollBar : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBScrollBar, TBWidget);

	TBScrollBar();
	~TBScrollBar();

	/** Set along which axis the scrollbar should scroll */
	virtual void SetAxis(AXIS axis) override;
	virtual AXIS GetAxis() const override { return m_axis; }

	/** Set the min, max limits for the scrollbar.
		The visible parameter is how much of the range that is visible.
		When this is called, the scrollbar might change value and invoke if the current value is
		outside of the new limits. */
	void SetLimits(double min, double max, double visible);

	/** Return true if the scrollbar has anywhere to go with the current limits. */
	bool CanScroll() const { return m_visible > 0; }

	/** Return true if the scrollbar can scroll in the positive direction with its current limits. */
	bool CanScrollPositive() const { return m_value < m_max; }

	/** Return true if the scrollbar can scroll in the negative direction with its current limits. */
	bool CanScrollNegative() const { return m_value > m_min; }

	double GetMinValue() const { return m_min; }
	double GetMaxValue() const { return m_max; }
	double GetVisible() const { return m_visible; }

	/** Same as SetValue, but with double precision. */
	virtual void SetValueDouble(double value) override;
	virtual double GetValueDouble() const override { return m_value; }

	virtual void SetValue(int value) override { SetValueDouble(value); }
	virtual int GetValue() const override { return (int) GetValueDouble(); }

	virtual void OnInflate(const INFLATE_INFO &info) override;
	virtual bool OnEvent(const TBWidgetEvent &ev) override;
	virtual void OnResized(int old_w, int old_h) override;
protected:
	TBWidget m_handle;
	AXIS m_axis;
	double m_value;
	double m_min, m_max, m_visible;
	double m_to_pixel_factor;
	void UpdateHandle();
};

/** TBSlider is a horizontal or vertical slider for a number within a range. */

// FIX: Add a "track value" showing as a line within the track (to be used for buffering etc).
// FIX: Also add a auto track that keeps it up to date with value (default).
class TBSlider : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSlider, TBWidget);

	TBSlider();
	~TBSlider();

	/** Set along which axis the scrollbar should scroll */
	virtual void SetAxis(AXIS axis) override;
	virtual AXIS GetAxis() const override { return m_axis; }

	/** Set the min, max limits for the slider. */
	void SetLimits(double min, double max);

	double GetMinValue() const { return m_min; }
	double GetMaxValue() const { return m_max; }

	/** Get a small value (depending on the min and max limits) for stepping by f.ex. keyboard. */
	double GetSmallStep() const { return (m_max - m_min) / 100.0; }

	/** Same as SetValue, but with double precision. */
	virtual void SetValueDouble(double value) override;
	virtual double GetValueDouble() const override { return m_value; }

	virtual void SetValue(int value) override { SetValueDouble(value); }
	virtual int GetValue() const override { return (int) GetValueDouble(); }

	virtual void OnInflate(const INFLATE_INFO &info) override;
	virtual bool OnEvent(const TBWidgetEvent &ev) override;
	virtual void OnResized(int old_w, int old_h) override;
protected:
	TBWidget m_handle;
	AXIS m_axis;
	double m_value;
	double m_min, m_max;
	double m_to_pixel_factor;
	void UpdateHandle();
};

/** TBContainer is just a TBWidget with border and padding (using skin "TBContainer") */
class TBContainer : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBContainer, TBWidget);

	TBContainer();
};

/** TBMover is moving its parent widget when dragged. */
class TBMover : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBMover, TBWidget);

	TBMover();

	virtual bool OnEvent(const TBWidgetEvent &ev) override;
};

/** TBResizer is a lower right corner resize grip. It will resize its parent widget. */
class TBResizer : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBResizer, TBWidget);

	TBResizer();
	virtual WIDGET_HIT_STATUS GetHitStatus(int x, int y) override;
	virtual bool OnEvent(const TBWidgetEvent &ev) override;
};

/** TBDimmer dim widgets in the background and block input. */
class TBDimmer : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBDimmer, TBWidget);

	TBDimmer();

	virtual void OnAdded() override;
};

} // namespace tb
