/**
 * @file
 */

#pragma once

#include "tb_editfield.h"
#include "tb_select_item.h"
#include "tb_widgets_listener.h"

namespace tb {

/** TBSelectList is a select widget with no popups. Instead it has two
	arrow buttons that cycle between the choices.
	By default it is a number widget.

	FIX: Should also be possible to set a list of strings that will be
		shown instead of numbers.
*/
class TBInlineSelect : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBInlineSelect, TBWidget);

	TBInlineSelect();
	~TBInlineSelect();

	/** Set along which axis the content should layouted. */
	virtual void setAxis(AXIS axis) override {
		m_layout.setAxis(axis);
	}
	virtual AXIS getAxis() const override {
		return m_layout.getAxis();
	}

	void setLimits(int min, int max);
	int getMinValue() const {
		return m_min;
	}
	int getMaxValue() const {
		return m_max;
	}

	virtual void setValue(int value) override {
		setValueInternal(value, true);
	}
	virtual int getValue() const override {
		return m_value;
	}

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual void onSkinChanged() override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;

protected:
	TBButton m_buttons[2];
	TBLayout m_layout;
	TBEditField m_editfield;
	int m_value;
	int m_min, m_max;
	void setValueInternal(int value, bool update_text);
};

} // namespace tb
