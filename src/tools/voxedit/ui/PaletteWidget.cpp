#include "PaletteWidget.h"
#include "voxel/MaterialColor.h"

PaletteWidget::PaletteWidget() :
		ui::Widget() {
	SetIsFocusable(true);
}

PaletteWidget::~PaletteWidget() {
}

void PaletteWidget::OnPaint(const PaintProps &paint_props) {
	Super::OnPaint(paint_props);
	const tb::TBRect rect = GetRect();
	const int xAmount = rect.w / _width;
	const int yAmount = rect.h / _height;
	const tb::TBRect renderRect(0, 0, _width, _height);
	const voxel::MaterialColorArray& colors = voxel::getMaterialColors();
	size_t i = 0u;
	for (int y = 0; y < yAmount; ++y) {
		for (int x = 0; x < xAmount; ++x) {
			if (i >= colors.size()) {
				return;
			}
			const glm::ivec4 color(colors[i] * 255.0f);
			const int transX = x * _padding + x * _width;
			const int transY = y * _padding + y * _height;
			const tb::TBColor c(color.r, color.g, color.b);
			tb::g_renderer->Translate(transX, transY);
			//tb::g_renderer->DrawRectFill(renderRect, c);
			tb::g_renderer->DrawRect(renderRect, c);
			tb::g_renderer->Translate(-transX, -transY);
			++i;
		}
	}
}

void PaletteWidget::OnInflate(const tb::INFLATE_INFO &info) {
	_width = info.node->GetValueInt("width", 20);
	_height = info.node->GetValueInt("height", 20);
	_padding = info.node->GetValueInt("padding", 2);
	Super::OnInflate(info);
}

namespace tb {
TB_WIDGET_FACTORY(PaletteWidget, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
