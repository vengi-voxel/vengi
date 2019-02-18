/**
 * @file
 */

#include "tb_toggle_container.h"
#include "tb_node_tree.h"
#include "tb_widgets_reader.h"

namespace tb {

TBSectionHeader::TBSectionHeader() {
	setSkinBg(TBIDC("TBSectionHeader"));
	setGravity(WIDGET_GRAVITY_LEFT | WIDGET_GRAVITY_RIGHT);
	setToggleMode(true);
}

bool TBSectionHeader::onEvent(const TBWidgetEvent &ev) {
	if (ev.target == this && ev.type == EVENT_TYPE_CHANGED && (getParent()->getParent() != nullptr)) {
		if (TBSection *section = TBSafeCast<TBSection>(getParent()->getParent())) {
			section->getContainer()->setValue(getValue());

			// Try to scroll the container into view when expanded
			section->setPendingScrollIntoView(getValue() != 0);
		}
	}
	return TBButton::onEvent(ev);
}

// == TBSectionHeader =====================================

TBSection::TBSection() : m_pending_scroll(false) {
	setGravity(WIDGET_GRAVITY_LEFT | WIDGET_GRAVITY_RIGHT);

	setSkinBg(TBIDC("TBSection"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	m_layout.setSkinBg(TBIDC("TBSection.layout"), WIDGET_INVOKE_INFO_NO_CALLBACKS);

	m_toggle_container.setSkinBg(TBIDC("TBSection.container"));
	m_toggle_container.setToggle(TBToggleContainer::TOGGLE_EXPANDED);
	m_toggle_container.setGravity(WIDGET_GRAVITY_ALL);
	m_layout.setAxis(AXIS_Y);
	m_layout.setGravity(WIDGET_GRAVITY_ALL);
	m_layout.setLayoutSize(LAYOUT_SIZE_AVAILABLE);

	addChild(&m_layout);
	m_layout.addChild(&m_header);
	m_layout.addChild(&m_toggle_container);
}

TBSection::~TBSection() {
	m_layout.removeChild(&m_toggle_container);
	m_layout.removeChild(&m_header);
	removeChild(&m_layout);
}

void TBSection::setValue(int value) {
	m_header.setValue(value);
	m_toggle_container.setValue(value);
}

void TBSection::onProcessAfterChildren() {
	if (m_pending_scroll) {
		m_pending_scroll = false;
		scrollIntoViewRecursive();
	}
}

PreferredSize TBSection::onCalculatePreferredSize(const SizeConstraints &constraints) {
	PreferredSize ps = TBWidget::onCalculatePreferredContentSize(constraints);
	// We should not grow larger than we are, when there's extra space available.
	ps.max_h = ps.pref_h;
	return ps;
}

// == TBToggleContainer ===================================

TBToggleContainer::TBToggleContainer() : m_toggle(TOGGLE_NOTHING), m_invert(false), m_value(0) {
	setSkinBg(TBIDC("TBToggleContainer"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
}

void TBToggleContainer::setToggle(TOGGLE toggle) {
	if (toggle == m_toggle) {
		return;
	}

	if (m_toggle == TOGGLE_EXPANDED) {
		invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	}

	m_toggle = toggle;
	updateInternal();
}

void TBToggleContainer::setInvert(bool invert) {
	if (invert == m_invert) {
		return;
	}
	m_invert = invert;
	updateInternal();
}

void TBToggleContainer::setValue(int value) {
	if (value == m_value) {
		return;
	}
	m_value = value;
	updateInternal();
	invalidateSkinStates();
}

void TBToggleContainer::updateInternal() {
	bool on = getIsOn();
	switch (m_toggle) {
	case TOGGLE_NOTHING:
		break;
	case TOGGLE_ENABLED:
		setState(WIDGET_STATE_DISABLED, !on);
		break;
	case TOGGLE_OPACITY:
		setOpacity(on ? 1.F : 0);
		break;
	case TOGGLE_EXPANDED:
		setVisibility(on ? WIDGET_VISIBILITY_VISIBLE : WIDGET_VISIBILITY_GONE);

		// Also disable when collapsed so tab focus skips the children.
		setState(WIDGET_STATE_DISABLED, !on);
		break;
	}
}

} // namespace tb
