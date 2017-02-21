#include "NoiseNodeWidget.h"

NoiseNodeWidget::NoiseNodeWidget() :
		ui::Widget() {
	SetIsFocusable(true);
}

bool NoiseNodeWidget::OnEvent(const tb::TBWidgetEvent &ev) {
	return Super::OnEvent(ev);
}

static NoiseNodeWidgetFactory noiseWidget_wf;
