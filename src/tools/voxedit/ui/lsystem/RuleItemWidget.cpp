/**
 * @file
 */

#include "RuleItemWidget.h"

namespace voxedit {

RuleItemWidget::RuleItemWidget(RuleItem *item, RuleItemSource *source, int index) :
		Super(), _source(source), _index(index) {
	SetSkinBg(TBIDC("TBSelectItem"));
	SetLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	SetLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	SetPaintOverflowFadeout(false);

	core_assert_always(tb::g_widgets_reader->LoadFile(GetContentRoot(), "ui/widget/voxedit-lsystem-item.tb.txt"));
	tb::TBEditField *name = GetWidgetByIDAndType<tb::TBEditField>(TBIDC("rule"));
	if (name != nullptr) {
		name->SetText(item->str);
	}
	tb::TBTextField *character = GetWidgetByIDAndType<tb::TBTextField>(TBIDC("character"));
	if (character != nullptr) {
		const char buf[2] = {item->character(), '\0'};
		character->SetText(buf);
	}
}

bool RuleItemWidget::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("delete")) {
		_source->DeleteItem(_index);
		return true;
	}
	return Super::OnEvent(ev);
}

}
