/**
 * @file
 */

#include "NoiseDataNodeWidget.h"
#include "NoiseDataItemWidget.h"
#include "ui/turbobadger/ui_widgets.h"
#include "core/Color.h"

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
	SetIgnoreInput(false);

	core_assert_always(tb::g_widgets_reader->LoadFile(GetContentRoot(), "ui/widget/noisetool-noisedata-node.tb.txt"));
	setItem(item);
}

void NoiseDataNodeWidget::setItem(NoiseItem* item) {
	tb::TBTextField *name = GetWidgetByIDAndType<tb::TBTextField>(TBIDC("name"));
	if (item == nullptr) {
		if (name != nullptr) {
			name->SetText("Empty");
		}
		return;
	}
	if (name != nullptr) {
		name->SetText(item->str);
	}
	NOISEDATADETAIL("Frequency: %f", frequency);
	NOISEDATADETAIL("Lacunarity: %f", lacunarity);
	NOISEDATADETAIL("Octaves: %i", octaves);
	NOISEDATADETAIL("Gain: %f", gain);
}

#undef NOISEDATADETAIL

bool NoiseDataNodeWidget::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		_clicked = true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		int x = ev.target_x;
		int y = ev.target_y;
		ConvertToRoot(x, y);
		createNewNodeAtPosition(x, y);
		_clicked = false;
	}
	return Super::OnEvent(ev);
}

void NoiseDataNodeWidget::createNewNodeAtPosition(int x, int y) {
	NoiseDataNodeWidget* w = new NoiseDataNodeWidget(nullptr);
	w->SetRect(tb::TBRect(x, y, 20, 20));
	GetParent()->AddChild(w);
	const tb::TBRect& rect = w->GetRect();
	Log::info("x: %i, y: %i, w: %i, h: %i", rect.x, rect.y, rect.w, rect.h);
}

void NoiseDataNodeWidget::OnPaintChildren(const PaintProps &paintProps) {
	Super::OnPaintChildren(paintProps);
	if (!_clicked) {
		return;
	}
	// render line from clicked pos to current mouse pos
	tb::TBRect local_rect = GetRect();
	local_rect.x = 0;
	local_rect.y = 0;
	const glm::ivec4 color(core::Color::Cyan * 255.0f);
	const tb::TBColor tbColor(color.r, color.g, color.b, color.a);
	tb::g_tb_skin->PaintRectFill(local_rect, tbColor);
}
