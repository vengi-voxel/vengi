#pragma once

#include "ui/TurboBadger.h"
#include "core/Common.h"
#include "RuleItemSource.h"

namespace voxedit {

class RuleItemWidget: public tb::TBLayout {
private:
	using Super = tb::TBLayout;
	RuleItemSource *_source;
	int _index;
public:
	RuleItemWidget(RuleItem *item, RuleItemSource *source, int index);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
