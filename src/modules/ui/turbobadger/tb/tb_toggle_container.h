/**
 * @file
 */

#pragma once

#include "tb_widgets_common.h"

namespace tb {

/** TBToggleContainer is a widget that toggles a property when its value
	change between 0 and 1. TOGGLE specifies what property will toggle.
	This is useful f.ex to toggle a whole group of child widgets depending
	on the value of some other widget. By connecting the TBToggleContainer
	with a widget connection, this can happen completly automatically. */
class TBToggleContainer : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBToggleContainer, TBWidget);

	TBToggleContainer();

	/** Defines what should toggle when the value changes. */
	enum TOGGLE {
		TOGGLE_NOTHING, ///< Nothing happens (the default)
		TOGGLE_ENABLED, ///< Enabled/disabled state
		TOGGLE_OPACITY, ///< Opacity 1/0
		TOGGLE_EXPANDED ///< Expanded/collapsed (In parent axis direction)
	};

	/** Set what should toggle when the value changes. */
	void setToggle(TOGGLE toggle);
	TOGGLE getToggle() const {
		return m_toggle;
	}

	/** Set if the toggle state should be inverted. */
	void setInvert(bool invert);
	bool getInvert() const {
		return m_invert;
	}

	/** Get the current value, after checking the invert mode. */
	bool getIsOn() const {
		return m_invert ? m_value == 0 : !(m_value == 0);
	}

	/** Set the value of this widget. 1 will turn on the toggle, 0 will turn it off (or
		the opposite if the invert mode is set). */
	virtual void setValue(int value) override;
	virtual int getValue() const override {
		return m_value;
	}

	virtual void onInflate(const INFLATE_INFO &info) override;

private:
	void updateInternal();
	TOGGLE m_toggle;
	bool m_invert;
	int m_value;
};

/** TBSectionHeader is just a thin wrapper for a TBButton that is in toggle
	mode with the skin TBSectionHeader by default. It is used as the clickable
	header in TBSection that toggles the section. */
class TBSectionHeader : public TBButton {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSectionHeader, TBButton);

	TBSectionHeader();

	virtual bool onEvent(const TBWidgetEvent &ev);
};

/** TBSection is a widget with a header that when clicked toggles its children
	on and off (using a internal TBToggleContainer with TOGGLE_EXPANDED).

	The header is a TBSectionHeader.

	The skin names of the internal widgets are:
		TBSection				- This widget itself.
		TBSection.layout		- The layout that wraps the header and the container.
		TBSection.container		- The toggle container with the children that expands/collapses.
*/

class TBSection : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSection, TBWidget);

	TBSection();
	~TBSection();

	TBLayout *getLayout() {
		return &m_layout;
	}
	TBSectionHeader *getHeader() {
		return &m_header;
	}
	TBToggleContainer *getContainer() {
		return &m_toggle_container;
	}

	/** Set if the section should be scrolled into view after next layout. */
	void setPendingScrollIntoView(bool pendingScroll) {
		m_pending_scroll = pendingScroll;
	}

	/** Set the text of the text field. */
	virtual bool setText(const char *text) override {
		return m_header.setText(text);
	}
	virtual bool getText(TBStr &text) override {
		return m_header.getText(text);
	}
	using TBWidget::getText; ///< Make all versions in base class available.

	virtual void setValue(int value) override;
	virtual int getValue() const override {
		return m_toggle_container.getValue();
	}

	virtual TBWidget *getContentRoot() {
		return m_toggle_container.getContentRoot();
	}
	virtual void onProcessAfterChildren() override;

	virtual PreferredSize onCalculatePreferredSize(const SizeConstraints &constraints);

private:
	TBLayout m_layout;
	TBSectionHeader m_header;
	TBToggleContainer m_toggle_container;
	bool m_pending_scroll;
};

} // namespace tb
