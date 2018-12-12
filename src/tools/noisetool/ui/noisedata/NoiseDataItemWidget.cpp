/**
 * @file
 */

#include "NoiseDataItemWidget.h"
#include "../../NoiseTool.h"
#include "ui/turbobadger/ui_widgets.h"

NoiseItemSource::NoiseItemSource(NoiseTool* tool) :
		_tool(tool) {
}

bool NoiseItemSource::Filter(int index, const char *filter) {
	if (TBSelectItemSource::Filter(index, filter)) {
		return true;
	}
	const NoiseItem* item = GetItem(index);
	const NoiseData& data = item->data();
	if (data.octaves == core::string::toInt(filter)) {
		return true;
	}
	return false;
}

tb::TBWidget *NoiseItemSource::CreateItemWidget(int index, tb::TBSelectItemViewer *viewer) {
	NoiseItem* item = GetItem(index);
	NoiseDataItemWidget *widget = new NoiseDataItemWidget(_tool, item, this, index);
	return widget;
}

#define NOISEDATADETAIL(text, type) \
	if (tb::TBTextField *widget = GetWidgetByIDAndType<tb::TBTextField>(TBIDC(#type))) { \
		const NoiseData& data = item->data(); \
		tb::TBStr str; \
		str.SetFormatted(text, data.type); \
		widget->SetText(str); \
	}

#define NOISEDATADETAILDATA(text, id, data) \
	if (tb::TBTextField *widget = GetWidgetByIDAndType<tb::TBTextField>(TBIDC(id))) { \
		tb::TBStr str; \
		str.SetFormatted(text, data); \
		widget->SetText(str); \
	}

NoiseDataItemWidget::NoiseDataItemWidget(NoiseTool* tool, NoiseItem *item, NoiseItemSource *source, int index) :
			Super(), _source(source), _index(index), _tool(tool) {
	SetSkinBg(TBIDC("TBSelectItem"));
	SetLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	SetLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	SetPaintOverflowFadeout(false);

	core_assert_always(tb::g_widgets_reader->LoadFile(GetContentRoot(), "ui/widget/noisetool-noisedata-item.tb.txt"));
	if (tb::TBTextField *name = GetWidgetByIDAndType<tb::TBTextField>(TBIDC("name"))) {
		name->SetText(item->str);
	}
	NOISEDATADETAIL("Frequency: %f", frequency);
	NOISEDATADETAIL("Lacunarity: %f", lacunarity);
	NOISEDATADETAIL("Octaves: %i", octaves);
	NOISEDATADETAIL("Gain: %f", gain);
	NOISEDATADETAILDATA("Millis: %lu", "millis", item->data().endmillis - item->data().millis);

	if (ImageWidget *widget = GetWidgetByIDAndType<ImageWidget>(TBIDC("noise"))) {
		widget->SetImage(item->data().noise);
	}
	if (ImageWidget *widget = GetWidgetByIDAndType<ImageWidget>(TBIDC("graph"))) {
		widget->SetImage(item->data().graph);
	}
}

#undef NOISEDATADETAILDATA

bool NoiseDataItemWidget::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("delete")) {
			const tb::TBID& id = _source->GetItemID(_index);
			_tool->remove(id);
			return true;
		}
	}
	return Super::OnEvent(ev);
}

NoiseDataList::NoiseDataList() {
	SetLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	SetLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	SetPaintOverflowFadeout(false);
	SetAxis(tb::AXIS::AXIS_Y);

	core_assert_always(tb::g_widgets_reader->LoadFile(GetContentRoot(), "ui/widget/noisetool-noisedata-list.tb.txt"));

	_select = GetWidgetByIDAndType<tb::TBSelectList>("list");
	if (_select != nullptr) {
		NoiseItemSource* source = ((NoiseTool*)core::App::getInstance())->noiseItemSource();
		_select->SetSource(source);
		_select->GetScrollContainer()->SetScrollMode(tb::SCROLL_MODE_X_AUTO_Y_AUTO);
	} else {
		Log::error("Could not find list widget");
	}
}

NoiseDataList::~NoiseDataList() {
	if (_select != nullptr) {
		_select->SetSource(nullptr);
	}
}

bool NoiseDataList::OnEvent(const tb::TBWidgetEvent &ev) {
	if (_select != nullptr) {
		const tb::TBID& id = ev.target->GetID();
		if (ev.type == tb::EVENT_TYPE_CHANGED && id == TBIDC("filter")) {
			_select->SetFilter(ev.target->GetText());
			return true;
		}
	}
	return Super::OnEvent(ev);
}

static NoiseDataListFactory noiseDataList_wf;
