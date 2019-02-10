/**
 * @file
 */

#include "tb_widget_skin_condition_context.h"
#include "tb_widgets_common.h"
#include "tb_window.h"
#include "tb_tab_container.h"

namespace tb {

bool TBWidgetSkinConditionContext::getCondition(TBSkinCondition::TARGET target, const TBSkinCondition::CONDITION_INFO &info)
{
	switch (target)
	{
	case TBSkinCondition::TARGET_THIS:
		return getCondition(m_widget, info);
	case TBSkinCondition::TARGET_PARENT:
		return m_widget->getParent() && getCondition(m_widget->getParent(), info);
	case TBSkinCondition::TARGET_ANCESTORS:
		{
			TBWidget *widget = m_widget->getParent();
			while (widget)
			{
				if (getCondition(widget, info))
					return true;
				widget = widget->getParent();
			}
			return false;
		}
	case TBSkinCondition::TARGET_PREV_SIBLING:
		return m_widget->getPrev() && getCondition(m_widget->getPrev(), info);
	case TBSkinCondition::TARGET_NEXT_SIBLING:
		return m_widget->getNext() && getCondition(m_widget->getNext(), info);
	}
	return false;
}

bool TBWidgetSkinConditionContext::getCondition(TBWidget *widget, const TBSkinCondition::CONDITION_INFO &info)
{
	switch (info.prop)
	{
	case TBSkinCondition::PROPERTY_SKIN:
		return widget->getSkinBg() == info.value;
	case TBSkinCondition::PROPERTY_WINDOW_ACTIVE:
		if (TBWindow *window = widget->getParentWindow())
			return window->isActive();
		return false;
	case TBSkinCondition::PROPERTY_AXIS:
		return TBID(widget->getAxis() == AXIS_X ? "x" : "y") == info.value;
	case TBSkinCondition::PROPERTY_ALIGN:
		if (TBTabContainer *tc = TBSafeCast<TBTabContainer>(widget))
		{
			TBID widget_align;
			if (tc->getAlignment() == TB_ALIGN_LEFT)				widget_align = TBIDC("left");
			else if (tc->getAlignment() == TB_ALIGN_TOP)			widget_align = TBIDC("top");
			else if (tc->getAlignment() == TB_ALIGN_RIGHT)		widget_align = TBIDC("right");
			else if (tc->getAlignment() == TB_ALIGN_BOTTOM)		widget_align = TBIDC("bottom");
			return widget_align == info.value;
		}
		return false;
	case TBSkinCondition::PROPERTY_ID:
		return widget->getID() == info.value;
	case TBSkinCondition::PROPERTY_STATE:
		return !!(widget->getAutoState() & info.value);
	case TBSkinCondition::PROPERTY_VALUE:
		return widget->getValue() == (int) info.value;
	case TBSkinCondition::PROPERTY_HOVER:
		return TBWidget::hovered_widget && widget->isAncestorOf(TBWidget::hovered_widget);
	case TBSkinCondition::PROPERTY_CAPTURE:
		return TBWidget::captured_widget && widget->isAncestorOf(TBWidget::captured_widget);
	case TBSkinCondition::PROPERTY_FOCUS:
		return TBWidget::focused_widget && widget->isAncestorOf(TBWidget::focused_widget);
	case TBSkinCondition::PROPERTY_CUSTOM:
		return widget->getCustomSkinCondition(info);
	}
	return false;
}

} // namespace tb
