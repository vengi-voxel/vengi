/**
 * @file
 */

#include "tb_tab_container.h"
#include "core/Assert.h"

namespace tb {

void TBTabLayout::onChildAdded(TBWidget *child) {
	if (TBButton *button = TBSafeCast<TBButton>(child)) {
		button->setSqueezable(true);
		button->setSkinBg(TBIDC("TBTabContainer.tab"));
		button->setID(TBIDC("tab"));
	}
}

PreferredSize TBTabLayout::onCalculatePreferredContentSize(const SizeConstraints &constraints) {
	PreferredSize ps = TBLayout::onCalculatePreferredContentSize(constraints);
	// Make sure the number of tabs doesn't grow parents.
	// It is only the content that should do that. The tabs
	// will scroll anyway.
	if (getAxis() == AXIS_X)
		ps.min_w = Min(ps.min_w, 1);
	else
		ps.min_h = Min(ps.min_h, 1);
	return ps;
}

TBTabContainer::TBTabContainer() : m_need_page_update(true), m_current_page(0), m_align(TB_ALIGN_TOP) {
	addChild(&m_root_layout);
	// Put the tab layout on top of the content in Z order so their skin can make
	// a seamless overlap over the border. Control which side they are layouted
	// to by calling SetLayoutOrder.
	m_root_layout.addChild(&m_content_root);
	m_root_layout.addChild(&m_tab_layout);
	m_root_layout.setAxis(AXIS_Y);
	m_root_layout.setGravity(WIDGET_GRAVITY_ALL);
	m_root_layout.setLayoutDistribution(LAYOUT_DISTRIBUTION_AVAILABLE);
	m_root_layout.setLayoutOrder(LAYOUT_ORDER_TOP_TO_BOTTOM);
	m_root_layout.setSkinBg(TBIDC("TBTabContainer.rootlayout"));
	m_tab_layout.setLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION_CENTER);
	m_tab_layout.setSkinBg(TBIDC("TBTabContainer.tablayout_x"));
	m_tab_layout.setLayoutPosition(LAYOUT_POSITION_RIGHT_BOTTOM);
	m_content_root.setGravity(WIDGET_GRAVITY_ALL);
	m_content_root.setSkinBg(TBIDC("TBTabContainer.container"));
}

TBTabContainer::~TBTabContainer() {
	m_root_layout.removeChild(&m_content_root);
	m_root_layout.removeChild(&m_tab_layout);
	removeChild(&m_root_layout);
}

void TBTabContainer::setAxis(AXIS axis) {
	m_root_layout.setAxis(axis);
	m_tab_layout.setAxis(axis == AXIS_X ? AXIS_Y : AXIS_X);
	m_tab_layout.setSkinBg(axis == AXIS_X ? TBIDC("TBTabContainer.tablayout_y") : TBIDC("TBTabContainer.tablayout_x"));
}

void TBTabContainer::setValue(int index) {
	if (index == m_current_page)
		return;
	m_current_page = index;

	// Update the pages visibility and tabs pressed value.
	index = 0;
	TBWidget *page = m_content_root.getFirstChild();
	TBWidget *tab = m_tab_layout.getFirstChild();
	for (; page && tab; page = page->getNext(), tab = tab->getNext(), index++) {
		bool active = index == m_current_page;
		page->setVisibility(active ? WIDGET_VISIBILITY_VISIBLE : WIDGET_VISIBILITY_INVISIBLE);
		tab->setValue(active ? 1 : 0);
	}
}

int TBTabContainer::getNumPages() {
	int count = 0;
	for (TBWidget *tab = m_tab_layout.getFirstChild(); tab; tab = tab->getNext())
		count++;
	return count;
}

TBWidget *TBTabContainer::getCurrentPageWidget() const {
	return m_content_root.getChildFromIndex(m_current_page);
}

void TBTabContainer::setAlignment(TB_ALIGN align) {
	bool horizontal = (align == TB_ALIGN_TOP || align == TB_ALIGN_BOTTOM);
	bool reverse = (align == TB_ALIGN_TOP || align == TB_ALIGN_LEFT);
	setAxis(horizontal ? AXIS_Y : AXIS_X);
	m_root_layout.setLayoutOrder(reverse ? LAYOUT_ORDER_TOP_TO_BOTTOM : LAYOUT_ORDER_BOTTOM_TO_TOP);
	m_tab_layout.setLayoutPosition(reverse ? LAYOUT_POSITION_RIGHT_BOTTOM : LAYOUT_POSITION_LEFT_TOP);
	m_align = align;
}

bool TBTabContainer::onEvent(const TBWidgetEvent &ev) {
	if ((ev.type == EVENT_TYPE_CLICK || ev.type == EVENT_TYPE_POINTER_DOWN) && ev.target->getID() == TBIDC("tab") &&
		ev.target->getParent() == &m_tab_layout) {
		int clicked_index = m_tab_layout.getIndexFromChild(ev.target);
		setValue(clicked_index);
		return true;
	}
	return false;
}

void TBTabContainer::onProcess() {
	if (m_need_page_update) {
		m_need_page_update = false;
		// Force update value
		int current_page = m_current_page;
		m_current_page = -1;
		setValue(current_page);
	}
}

} // namespace tb
