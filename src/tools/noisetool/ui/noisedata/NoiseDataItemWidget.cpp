/**
 * @file
 */

#include "NoiseDataItemWidget.h"
#include "../../NoiseTool.h"
#include "ui/turbobadger/ui_widgets.h"
#include "core/StringUtil.h"

NoiseItemSource::NoiseItemSource(NoiseTool* tool) :
		_tool(tool) {
}

bool NoiseItemSource::filter(int index, const char *filter) {
	if (TBSelectItemSource::filter(index, filter)) {
		return true;
	}
	const NoiseItem* item = getItem(index);
	const NoiseData& data = item->data();
	if (data.octaves == core::string::toInt(filter)) {
		return true;
	}
	return false;
}

tb::TBWidget *NoiseItemSource::createItemWidget(int index, tb::TBSelectItemViewer *viewer) {
	NoiseItem* item = getItem(index);
	NoiseDataItemWidget *widget = new NoiseDataItemWidget(_tool, item, this, index);
	return widget;
}

#define NOISEDATADETAIL(text, type) \
	if (tb::TBTextField *widget = getWidgetByIDAndType<tb::TBTextField>(TBIDC(#type))) { \
		const NoiseData& data = item->data(); \
		const core::String& str = core::string::format(text, data.type); \
		widget->setText(str); \
	}

#define NOISEDATADETAILDATA(text, id, data) \
	if (tb::TBTextField *widget = getWidgetByIDAndType<tb::TBTextField>(TBIDC(id))) { \
		const core::String& str = core::string::format(text, data); \
		widget->setText(str); \
	}

NoiseDataItemWidget::NoiseDataItemWidget(NoiseTool* tool, NoiseItem *item, NoiseItemSource *source, int index) :
			Super(), _source(source), _index(index), _tool(tool) {
	setSkinBg(TBIDC("TBSelectItem"));
	setLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	setLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	setPaintOverflowFadeout(false);

	core_assert_always(tb::g_widgets_reader->loadFile(getContentRoot(), "ui/widget/noisetool-noisedata-item.tb.txt"));
	if (tb::TBTextField *name = getWidgetByIDAndType<tb::TBTextField>(TBIDC("name"))) {
		name->setText(item->str);
	}
	NOISEDATADETAIL("Frequency: %f", frequency);
	NOISEDATADETAIL("Lacunarity: %f", lacunarity);
	NOISEDATADETAIL("Octaves: %i", octaves);
	NOISEDATADETAIL("Gain: %f", gain);
	NOISEDATADETAILDATA("Millis: %lu", "millis", item->data().endmillis - item->data().millis);

	if (ImageWidget *widget = getWidgetByIDAndType<ImageWidget>(TBIDC("noise"))) {
		widget->setImage(item->data().noise);
	}
	if (ImageWidget *widget = getWidgetByIDAndType<ImageWidget>(TBIDC("graph"))) {
		widget->setImage(item->data().graph);
	}
}

#undef NOISEDATADETAILDATA

bool NoiseDataItemWidget::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->getID() == TBIDC("delete")) {
			const tb::TBID& id = _source->getItemID(_index);
			_tool->remove(id);
			return true;
		}
	}
	return Super::onEvent(ev);
}

NoiseDataList::NoiseDataList() {
	setLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	setLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	setPaintOverflowFadeout(false);
	setAxis(tb::AXIS::AXIS_Y);

	core_assert_always(tb::g_widgets_reader->loadFile(getContentRoot(), "ui/widget/noisetool-noisedata-list.tb.txt"));

	_select = getWidgetByIDAndType<tb::TBSelectList>("list");
	if (_select != nullptr) {
		NoiseItemSource* source = ((NoiseTool*)app::App::getInstance())->noiseItemSource();
		_select->setSource(source);
		_select->getScrollContainer()->setScrollMode(tb::SCROLL_MODE_X_AUTO_Y_AUTO);
	} else {
		Log::error("Could not find list widget");
	}
}

NoiseDataList::~NoiseDataList() {
	if (_select != nullptr) {
		_select->setSource(nullptr);
	}
}

bool NoiseDataList::onEvent(const tb::TBWidgetEvent &ev) {
	if (_select != nullptr) {
		const tb::TBID& id = ev.target->getID();
		if (ev.type == tb::EVENT_TYPE_CHANGED && id == TBIDC("filter")) {
			_select->setFilter(ev.target->getText());
			return true;
		}
	}
	return Super::onEvent(ev);
}

static NoiseDataListFactory noiseDataList_wf;
