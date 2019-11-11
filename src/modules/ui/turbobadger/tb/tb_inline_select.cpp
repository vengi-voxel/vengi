/**
 * @file
 */

#include "tb_inline_select.h"
#include "core/Assert.h"
#include <stdlib.h>

namespace tb {

// FIX: axis should affect the buttons arrow skin!
// FIX: unfocus should set the correct text!

TBInlineSelectBase::TBInlineSelectBase() {
	setSkinBg(TBIDC("TBInlineSelect"));
	addChild(&m_layout);
	m_layout.addChild(&m_buttons[0]);
	m_layout.addChild(&m_editfield);
	m_layout.addChild(&m_buttons[1]);
	m_layout.setRect(getPaddingRect());
	m_layout.setGravity(WIDGET_GRAVITY_ALL);
	m_layout.setSpacing(0);
	m_buttons[0].setSkinBg(TBIDC("TBButton.flat"));
	m_buttons[1].setSkinBg(TBIDC("TBButton.flat"));
	m_buttons[0].getContentRoot()->addChild(new TBSkinImage(TBIDC("arrow.left")));
	m_buttons[1].getContentRoot()->addChild(new TBSkinImage(TBIDC("arrow.right")));
	m_buttons[0].setIsFocusable(false);
	m_buttons[1].setIsFocusable(false);
	m_buttons[0].setID(TBIDC("dec"));
	m_buttons[1].setID(TBIDC("inc"));
	m_buttons[0].setAutoRepeat(true);
	m_buttons[1].setAutoRepeat(true);
	m_editfield.setTextAlign(TB_TEXT_ALIGN_CENTER);
	m_editfield.setEditType(EDIT_TYPE_NUMBER);
	m_editfield.setText("0");
}

TBInlineSelectBase::~TBInlineSelectBase() {
	m_layout.removeChild(&m_buttons[1]);
	m_layout.removeChild(&m_editfield);
	m_layout.removeChild(&m_buttons[0]);
	removeChild(&m_layout);
}

void TBInlineSelectBase::onSkinChanged() {
	m_layout.setRect(getPaddingRect());
}

void TBInlineSelect::setLimits(int min, int max) {
	core_assert(min <= max);
	m_min = min;
	m_max = max;
	setValue(m_value);
}

void TBInlineSelect::onProcess() {
	Super::onProcess();
	if (!_var || !_var->isDirty()) {
		return;
	}
	setValue(_var->intVal());
}

void TBInlineSelect::setValueInternal(int value, bool updateText) {
	value = Clamp(value, m_min, m_max);
	if (value == m_value) {
		return;
	}
	m_value = value;
	if (_var) {
		_var->setVal(value);
		_var->markClean();
	}

	if (updateText) {
		TBStr strval;
		strval.setFormatted("%d", m_value);
		m_editfield.setText(strval);
	}

	TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	invokeEvent(ev);

	// Warning: Do nothing here since the event might have deleted us.
	//          If needed, check if we are alive using a safe pointer first.
}

bool TBInlineSelect::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == TB_KEY_UP || ev.special_key == TB_KEY_DOWN) {
			int dv = ev.special_key == TB_KEY_UP ? m_delta : -m_delta;
			setValue(getValue() + dv);
			return true;
		}
	} else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("dec")) {
		setValue(getValue() - m_delta);
		return true;
	} else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("inc")) {
		setValue(getValue() + m_delta);
		return true;
	} else if (ev.type == EVENT_TYPE_CHANGED && ev.target == &m_editfield) {
		TBStr text;
		m_editfield.getText(text);
		setValueInternal(atoi(text), false);
	}
	return false;
}


void TBInlineSelectDouble::setLimits(double min, double max) {
	core_assert(min <= max);
	m_min = min;
	m_max = max;
	setValueDouble(m_value);
}

void TBInlineSelectDouble::onProcess() {
	Super::onProcess();
	if (!_var || !_var->isDirty()) {
		return;
	}
	setValueDouble(_var->floatVal());
}

void TBInlineSelectDouble::setValueInternal(double value, bool updateText) {
	value = Clamp(value, m_min, m_max);
	if (value == m_value) {
		return;
	}
	m_value = value;
	if (_var) {
		_var->setVal((float)value);
		_var->markClean();
	}

	if (updateText) {
		TBStr strval;
		strval.setFormatted("%f", (float)m_value);
		m_editfield.setText(strval);
	}

	TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	invokeEvent(ev);

	// Warning: Do nothing here since the event might have deleted us.
	//          If needed, check if we are alive using a safe pointer first.
}

bool TBInlineSelectDouble::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == TB_KEY_UP || ev.special_key == TB_KEY_DOWN) {
			double dv = ev.special_key == TB_KEY_UP ? m_delta : -m_delta;
			setValueDouble(getValueDouble() + dv);
			return true;
		}
	} else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("dec")) {
		setValueDouble(getValueDouble() - m_delta);
		return true;
	} else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("inc")) {
		setValueDouble(getValueDouble() + m_delta);
		return true;
	} else if (ev.type == EVENT_TYPE_CHANGED && ev.target == &m_editfield) {
		TBStr text;
		m_editfield.getText(text);
		setValueInternal(atof(text), false);
	}
	return false;
}

} // namespace tb
