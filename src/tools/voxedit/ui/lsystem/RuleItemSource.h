#pragma once

#include "ui/TurboBadger.h"
#include "RuleItem.h"

namespace voxedit {

class RuleItemSource: public tb::TBSelectItemSourceList<RuleItem> {
public:
	bool Filter(int index, const char *filter) override;

	tb::TBWidget *CreateItemWidget(int index, tb::TBSelectItemViewer *viewer) override;
};

}
