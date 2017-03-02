#include "NoiseDataNodeWidget.h"
#include "NoiseDataItemWidget.h"
#include "ui/ui_widgets.h"

#define NOISEDATADETAIL(text, type) \
	if (tb::TBTextField *widget = GetWidgetByIDAndType<tb::TBTextField>(TBIDC(#type))) { \
		const NoiseData& data = item->data(); \
		tb::TBStr str; \
		str.SetFormatted(text, data.type); \
		widget->SetText(str); \
	} else { \
		Log::warn("Could not get widget with id " #type); \
	}

NoiseDataNodeWidget::NoiseDataNodeWidget(NoiseItem *item) :
			Super() {
	SetLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	SetLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);

	core_assert_always(tb::g_widgets_reader->LoadFile(GetContentRoot(), "ui/widget/noisetool-noisedata-node.tb.txt"));
	if (tb::TBTextField *name = GetWidgetByIDAndType<tb::TBTextField>(TBIDC("name"))) {
		name->SetText(item->str);
	}
	NOISEDATADETAIL("Frequency: %f", frequency);
	NOISEDATADETAIL("Lacunarity: %f", lacunarity);
	NOISEDATADETAIL("Octaves: %i", octaves);
	NOISEDATADETAIL("Gain: %f", gain);
}

bool NoiseDataNodeWidget::OnEvent(const tb::TBWidgetEvent &ev) {
	return Super::OnEvent(ev);
}
