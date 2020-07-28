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
class TBInlineSelectBase : public TBWidget {
public:
	TBOBJECT_SUBCLASS(TBInlineSelectBase, TBWidget);

	TBInlineSelectBase();
	virtual ~TBInlineSelectBase();

	/** Set along which axis the content should layouted. */
	virtual void setAxis(AXIS axis) override {
		m_layout.setAxis(axis);
	}
	virtual AXIS getAxis() const override {
		return m_layout.getAxis();
	}

	virtual void onSkinChanged() override;
	virtual void onInflate(const INFLATE_INFO &info) override;

protected:
	TBButton m_buttons[2];
	TBLayout m_layout;
	TBEditField m_editfield;

	core::VarPtr _var;
	core::String _command;
};

class TBInlineSelect : public TBInlineSelectBase {
public:
	TBOBJECT_SUBCLASS(TBInlineSelect, TBInlineSelectBase);

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

	virtual void onProcess() override;
	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;

protected:
	int m_value = 0;
	int m_min = 0, m_max = 100;
	int m_delta = 1;
	void setValueInternal(int value, bool update_text);
};

class TBInlineSelectDouble : public TBInlineSelectBase {
public:
	TBOBJECT_SUBCLASS(TBInlineSelectDouble, TBInlineSelectBase);

	void setLimits(double min, double max);
	double getMinValue() const {
		return m_min;
	}
	double getMaxValue() const {
		return m_max;
	}

	virtual void setValueDouble(double value) override {
		setValueInternal(value, true);
	}
	virtual double getValueDouble() const override {
		return m_value;
	}

	virtual void onProcess() override;
	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;

protected:
	double m_value = 0.0;
	double m_min = 0.0, m_max = 100.0;
	double m_delta = 1.0;
	void setValueInternal(double value, bool update_text);
};

} // namespace tb
