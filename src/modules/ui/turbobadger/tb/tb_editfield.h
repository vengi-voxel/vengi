/**
 * @file
 */

#pragma once

#include "tb_msg.h"
#include "tb_style_edit.h"
#include "tb_widgets_common.h"
#include "core/Log.h"

namespace tb {

/** EDIT_TYPE - These types does not restrict input (may change in the future).
	They are just hints for virtual keyboard, so it can show special keys. */
enum EDIT_TYPE {
	EDIT_TYPE_TEXT,
	EDIT_TYPE_SEARCH,
	EDIT_TYPE_PASSWORD,
	EDIT_TYPE_EMAIL,
	EDIT_TYPE_PHONE,
	EDIT_TYPE_URL,
	EDIT_TYPE_NUMBER
};

/** The default content factory for embedded content in TBEditField with styling enabled.

	Creates all that TBTextFragmentContentFactory creates by default,
	and any type of widget from a inline resource string.

	Syntax: <widget xxx> Where xxx is parsed by TBWidgetsReader.

	Example - Create a button with id "hello":

		<widget TBButton: text: "Hello world!" id: "hello">

	Example - Create a image from skin element "Icon48":

		<widget TBSkinImage: skin: "Icon48">
*/

class TBEditFieldContentFactory : public TBTextFragmentContentFactory {
public:
	class TBEditField *editfield;
	virtual int getContent(const char *text);
	virtual TBTextFragmentContent *createFragmentContent(const char *text, int text_len);
};

/** TBEditFieldScrollRoot - Internal for TBEditField.
	Acts as a scrollable container for any widget created as embedded content. */

class TBEditFieldScrollRoot : public TBWidget {
private: // May only be used by TBEditField.
	friend class TBEditField;
	TBEditFieldScrollRoot() {
	}

public:
	virtual void onPaintChildren(const PaintProps &paint_props);
	virtual void getChildTranslation(int &x, int &y) const;
	virtual WIDGET_HIT_STATUS getHitStatus(int x, int y);
};

/** TBEditField is a one line or multi line textfield that is editable or
	read-only. It can also be a passwordfield by calling
	SetEditType(EDIT_TYPE_PASSWORD).

	It may perform styling of text and contain custom embedded content,
	if enabled by SetStyling(true). Disabled by default.
*/

class TBEditField : public TBWidget, private TBStyleEditListener, public TBMessageHandler {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBEditField, TBWidget);

	TBEditField();
	~TBEditField();

	/** Get the visible rect (the GetPaddingRect() minus any scrollbars) */
	TBRect getVisibleRect();

	/** Set if multiple lines should be allowed or not.
		Will also set wrapping (to true if multiline, and false if not). */
	void setMultiline(bool multiline);
	bool getMultiline() const {
		return m_style_edit.packed.multiline_on;
	}

	/** Set if styling should be enabled or not. Default is disabled. */
	void setStyling(bool styling);
	bool getStyling() const {
		return m_style_edit.packed.styling_on;
	}

	/** Set if read only mode should be enabled. Default is disabled.
		In read only mode, editing is disabled and caret is hidden.
		The user is still able to focus, select and copy text. */
	void setReadOnly(bool readonly);
	bool getReadOnly() const {
		return m_style_edit.packed.read_only;
	}

	/** Set to true if the text should wrap if multi line is enabled (See SetMultiline). */
	void setWrapping(bool wrapping);
	bool getWrapping() const {
		return m_style_edit.packed.wrapping;
	}

	/** Set to true if the preferred size of this editfield should adapt to the
		size of the content (disabled by default).
		If wrapping is enabled, the result is partly dependant on the virtual
		width (See SetVirtualWidth). */
	void setAdaptToContentSize(bool adapt);
	bool getAdaptToContentSize() const {
		return m_adapt_to_content_size;
	}

	/** The virtual width is only used if the size is adapting to content size
		(See SetAdaptToContentSize) and wrapping is enabled.
		The virtual width will be used to layout the text and see which resulting
		width and height it takes up. The width that is actually used depends on
		the content. It is also up to the the layouter to decide if the size
		should be respected or not. The default is 250. */
	void setVirtualWidth(int virtual_width);
	int getVirtualWidth() const {
		return m_virtual_width;
	}

	/** Get the TBStyleEdit object that contains more functions and settings. */
	TBStyleEdit *getStyleEdit() {
		return &m_style_edit;
	}

	/** Set the edit type that is a hint for virtual keyboards about what the
		content should be. */
	void setEditType(EDIT_TYPE type);
	EDIT_TYPE getEditType() {
		return m_edit_type;
	}

	/** Support custom skin condition properties. Currently supported properties are:
		"edit-type", matching those of EDIT_TYPE.
		"multiline", matching 1 if multiline mode is enabled.
		"readonly", matching 1 if readonly mode is enabled. */
	virtual bool getCustomSkinCondition(const TBSkinCondition::CONDITION_INFO &info) override;

	/** Set which alignment the text should have if the space
		given when painting is larger than the text.
		This changes the default for new blocks, as wel as the currently selected blocks or the block
		of the current caret position if nothing is selected. */
	void setTextAlign(TB_TEXT_ALIGN align) {
		m_style_edit.setAlign(align);
	}
	TB_TEXT_ALIGN getTextAlign() {
		return m_style_edit.align;
	}

	void setTextFormatted(CORE_FORMAT_STRING const char *format, ...) __attribute__((format(printf, 2, 3)));

	virtual bool setText(const char *text) override {
		return setText(text, TB_CARET_POS_BEGINNING);
	}
	virtual bool getText(TBStr &text) override {
		return m_style_edit.getText(text);
	}

	void onProcess() override {
		TBWidget::onProcess();
		if (!_var || !_var->isDirty()) {
			return;
		}
		setText(_var->strVal().c_str());
	}

	using TBWidget::getText; ///< Make all versions in base class available.

	using TBWidget::invalidate; ///< Make Invalidate in base class available.

	/** Set the text and also specify if the caret should be positioned at the beginning
		or end of the text. */
	bool setText(const char *text, TB_CARET_POS pos) {
		if (_var) {
			_var->setVal(text);
			_var->markClean();
		}
		return m_style_edit.setText(text, pos);
	}
	/** Set the text of the given length and also specify if the caret should be positioned
		at the beginning or end of the text. */
	bool setText(const char *text, int textLen, TB_CARET_POS pos = TB_CARET_POS_BEGINNING) {
		return m_style_edit.setText(text, textLen, pos);
	}

	/** Set the placeholder text. It will be visible only when the textfield is empty. */
	virtual bool setPlaceholderText(const char *text) {
		return m_placeholder.setText(text);
	}
	virtual bool getPlaceholderText(TBStr &text) {
		return m_placeholder.getText(text);
	}

	virtual void scrollTo(int x, int y) override;
	virtual TBWidget::ScrollInfo getScrollInfo() override;
	virtual TBWidget *getScrollRoot() override {
		return &m_root;
	}

	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onPaint(const PaintProps &paint_props) override;
	virtual void onPaintChildren(const PaintProps &paint_props) override;
	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual void onAdded() override;
	virtual void onFontChanged() override;
	virtual void onFocusChanged(bool focused) override;
	virtual void onResized(int oldW, int oldH) override;
	virtual TBWidget *getContentRoot() override {
		return &m_root;
	}

	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints) override;

	virtual void onMessageReceived(TBMessage *msg) override;

private:
	TBScrollBar m_scrollbar_x;
	TBScrollBar m_scrollbar_y;
	TBWidgetString m_placeholder;
	EDIT_TYPE m_edit_type;
	TBEditFieldScrollRoot m_root;
	TBEditFieldContentFactory m_content_factory;
	TBStyleEdit m_style_edit;
	core::VarPtr _var;
	bool m_adapt_to_content_size;
	int m_virtual_width;
	void updateScrollbarVisibility(bool multiline);

	// == TBStyleEditListener =======================
	virtual void onChange() override;
	virtual bool onEnter() override;
	virtual void invalidate(const TBRect &rect) override;
	virtual void drawString(int32_t x, int32_t y, TBFontFace *font, const TBColor &color, const char *str,
							int32_t len) override;
	virtual void drawRect(const TBRect &rect, const TBColor &color) override;
	virtual void drawRectFill(const TBRect &rect, const TBColor &color) override;
	virtual void drawTextSelectionBg(const TBRect &rect) override;
	virtual void drawContentSelectionFg(const TBRect &rect) override;
	virtual void drawCaret(const TBRect &rect) override;
	virtual void scroll(int32_t dx, int32_t dy) override;
	virtual void updateScrollbars() override;
	virtual void caretBlinkStart() override;
	virtual void caretBlinkStop() override;
};

} // namespace tb
