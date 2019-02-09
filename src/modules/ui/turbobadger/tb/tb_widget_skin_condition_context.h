/**
 * @file
 */

#pragma once

#include "tb_widgets.h"
#include "tb_skin.h"

namespace tb {

/** Check if a condition is true for a widget when painting a skin. */

class TBWidgetSkinConditionContext : public TBSkinConditionContext
{
public:
	TBWidgetSkinConditionContext(TBWidget *widget) : m_widget(widget) {}
	virtual ~TBWidgetSkinConditionContext() {}
	virtual bool GetCondition(TBSkinCondition::TARGET target, const TBSkinCondition::CONDITION_INFO &info);
private:
	bool GetCondition(TBWidget *widget, const TBSkinCondition::CONDITION_INFO &info);
	TBWidget *m_widget;
};

} // namespace tb
