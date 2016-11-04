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
	const int xAmount = rect.w / (_width + _padding);
	const int yAmount = rect.h / (_height + _padding);
	const tb::TBRect renderRect(0, 0, _width, _height);
	const voxel::MaterialColorArray& colors = voxel::getMaterialColors();
	const glm::vec4& borderColor = core::Color::Black;
	const tb::TBColor tbBorderColor(borderColor.r, borderColor.g, borderColor.b);
	const int min = std::enum_value(voxel::VoxelType::Min);
	const int max = std::enum_value(voxel::VoxelType::Max);
	int i = min;
	core_assert(max <= (int)colors.size());
	for (int y = 0; y < yAmount; ++y) {
		for (int x = 0; x < xAmount; ++x) {
			if (i >= max) {
				y = yAmount;
				break;
			}
			const glm::ivec4 color(colors[i] * 255.0f);
			const int transX = x * _padding + x * _width;
			const int transY = y * _padding + y * _height;
			const tb::TBColor tbColor(color.r, color.g, color.b, color.a);
			tb::g_renderer->Translate(transX, transY);
			tb::g_renderer->DrawRectFill(renderRect, tbColor);
			tb::g_renderer->DrawRect(renderRect, tbBorderColor);
			tb::g_renderer->Translate(-transX, -transY);
			++i;
		}
	}
	{
		const glm::vec4& selectionColor = colors[std::enum_value(_voxelType)] * 255.0f;
		const tb::TBColor tbSelectionColor(selectionColor.r, selectionColor.g, selectionColor.b, selectionColor.a);
		const int y = i / xAmount;
		const int selectionY = (y + 1) * _padding + (y + 1) * _height;
		const tb::TBRect selectionRect(0, 0, rect.w, renderRect.h);
		tb::g_renderer->Translate(0, selectionY);
		tb::g_renderer->DrawRectFill(selectionRect, tbSelectionColor);
		tb::g_renderer->DrawRect(selectionRect, tbBorderColor);
		tb::g_renderer->Translate(0, -selectionY);
	}
}

bool PaletteWidget::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		const tb::TBRect rect = GetRect();
		const int min = std::enum_value(voxel::VoxelType::Min);
		const int max = std::enum_value(voxel::VoxelType::Max);
		const int xAmount = rect.w / (_width + _padding);
		const int col = ev.target_x / (_width + _padding);
		const int row = ev.target_y / (_height + _padding);
		const int index = row * xAmount + col;
		if (index >= max) {
			return false;
		}
		_voxelType = (voxel::VoxelType)index;
		_dirty = true;
		return true;
	}
	return Super::OnEvent(ev);
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
