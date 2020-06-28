/**
 * @file
 */

#include "NoiseDataNodeWidget.h"
#include "NoiseDataItemWidget.h"
#include "ui/turbobadger/ui_widgets.h"
#include "core/Color.h"

#define NOISEDATADETAIL(text, type) \
	if (tb::TBTextField *widget = getWidgetByIDAndType<tb::TBTextField>(TBIDC(#type))) { \
		const NoiseData& data = item->data(); \
		tb::TBStr str; \
		str.setFormatted(text, data.type); \
		widget->setText(str); \
	} else { \
		Log::warn("Could not get widget with id " #type); \
	}

NoiseDataNodeWidget::NoiseDataNodeWidget(NoiseItem *item) :
			Super() {
	setLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	setLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	setIgnoreInput(false);

	core_assert_always(tb::g_widgets_reader->loadFile(getContentRoot(), "ui/widget/noisetool-noisedata-node.tb.txt"));
	setItem(item);
}

void NoiseDataNodeWidget::setItem(NoiseItem* item) {
	tb::TBTextField *name = getWidgetByIDAndType<tb::TBTextField>(TBIDC("name"));
	if (item == nullptr) {
		if (name != nullptr) {
			name->setText("Empty");
		}
		return;
	}
	if (name != nullptr) {
		name->setText(item->str);
	}
	NOISEDATADETAIL("Frequency: %f", frequency);
	NOISEDATADETAIL("Lacunarity: %f", lacunarity);
	NOISEDATADETAIL("Octaves: %i", octaves);
	NOISEDATADETAIL("Gain: %f", gain);
}

#undef NOISEDATADETAIL

bool NoiseDataNodeWidget::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		_clicked = true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		int x = ev.target_x;
		int y = ev.target_y;
		convertToRoot(x, y);
		createNewNodeAtPosition(x, y);
		_clicked = false;
	}
	return Super::onEvent(ev);
}

void NoiseDataNodeWidget::createNewNodeAtPosition(int x, int y) {
	NoiseDataNodeWidget* w = new NoiseDataNodeWidget(nullptr);
	w->setRect(tb::TBRect(x, y, 20, 20));
	getParent()->addChild(w);
	const tb::TBRect& rect = w->getRect();
	Log::info("x: %i, y: %i, w: %i, h: %i", rect.x, rect.y, rect.w, rect.h);
}

void NoiseDataNodeWidget::onPaintChildren(const PaintProps &paintProps) {
	Super::onPaintChildren(paintProps);
	if (!_clicked) {
		return;
	}
	// render line from clicked pos to current mouse pos
	tb::TBRect local_rect = getRect();
	local_rect.x = 0;
	local_rect.y = 0;
	const glm::ivec4 color(core::Color::Cyan * 255.0f);
	const tb::TBColor tbColor(color.r, color.g, color.b, color.a);
	tb::g_tb_skin->paintRectFill(local_rect, tbColor);
}
