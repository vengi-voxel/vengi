#include "PaletteWidget.h"
#include "voxel/MaterialColor.h"
#include "core/Color.h"

PaletteWidget::PaletteWidget() :
		ui::Widget() {
	SetIsFocusable(true);
}

PaletteWidget::~PaletteWidget() {
}

void PaletteWidget::SetValue(int value) {
	if (value == _value) {
		return;
	}
	_value = value;
	tb::TBWidgetEvent ev(tb::EVENT_TYPE_CHANGED);
	InvokeEvent(ev);
}

void PaletteWidget::OnPaint(const PaintProps &paint_props) {
	Super::OnPaint(paint_props);
	const tb::TBRect renderRect(0, 0, _width, _height);
	const voxel::MaterialColorArray& colors = voxel::getMaterialColors();
	const glm::vec4& borderColor = core::Color::Black;
	const tb::TBColor tbBorderColor(borderColor.r, borderColor.g, borderColor.b);
	const int max = colors.size();
	int i = 0;
	for (int y = 0; y < _amountY; ++y) {
		for (int x = 0; x < _amountX; ++x) {
			if (i >= max) {
				break;
			}
			const glm::ivec4 color(colors[i] * 255.0f);
			const int transX = x * _width;
			const int transY = y * _height;
			const tb::TBColor tbColor(color.r, color.g, color.b, color.a);
			tb::g_renderer->Translate(transX, transY);
			tb::g_tb_skin->PaintRectFill(renderRect, tbColor);
			tb::g_tb_skin->PaintRect(renderRect, tbBorderColor, 1);
			tb::g_renderer->Translate(-transX, -transY);
			++i;
		}
	}
}

void PaletteWidget::OnResized(int oldWidth, int oldHeight) {
	_amountX = GetPaddingRect().w / _width;
	_amountY = GetPaddingRect().h / _height;
	return Super::OnResized(oldWidth, oldHeight);
}

bool PaletteWidget::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		const int max = voxel::getMaterialColors().size();
		const int col = ev.target_x / _width;
		const int row = ev.target_y / _height;
		const int index = row * _amountX + col;
		if (index >= max) {
			return false;
		}
		Log::info("Index: %i, xAmount: %i, yAmount: %i, col: %i, row: %i", index, _amountX, _amountY, col, row);
		SetValue(index);
		_dirty = true;
		return true;
	}
	return Super::OnEvent(ev);
}

tb::PreferredSize PaletteWidget::OnCalculatePreferredContentSize(const tb::SizeConstraints &constraints) {
	const voxel::MaterialColorArray& colors = voxel::getMaterialColors();
	const int size = colors.size();
	int maxAmountY = size / _amountX;
	if (size % _amountX) {
		++maxAmountY;
	}
	return tb::PreferredSize(_amountX * _width, maxAmountY * _height);
}

void PaletteWidget::OnInflate(const tb::INFLATE_INFO &info) {
	_width = info.node->GetValueInt("width", 20);
	_height = info.node->GetValueInt("height", 20);
	_amountX = info.node->GetValueInt("amount-x", 8);
	Super::OnInflate(info);
}

static PaletteWidgetFactory paletteWidget_wf;
