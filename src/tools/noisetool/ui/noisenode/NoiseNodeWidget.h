#pragma once

#include "ui/Widget.h"

class NoiseNodeWidget: public ui::Widget {
private:
	using Super = ui::Widget;
public:
	UIWIDGET_SUBCLASS(NoiseNodeWidget, Super);

	NoiseNodeWidget();
	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

UIWIDGET_FACTORY(NoiseNodeWidget, tb::TBValue::TYPE_INT, tb::WIDGET_Z_TOP)
