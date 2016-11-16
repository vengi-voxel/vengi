#include "RuleItemSource.h"
#include "RuleItemWidget.h"

namespace voxedit {

bool RuleItemSource::Filter(int index, const char *filter) {
	return tb::TBSelectItemSource::Filter(index, filter);
}

tb::TBWidget* RuleItemSource::CreateItemWidget(int index, tb::TBSelectItemViewer *viewer) {
	return new RuleItemWidget(GetItem(index), this, index);
}

}
