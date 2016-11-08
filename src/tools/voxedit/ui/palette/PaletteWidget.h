#pragma once

#include "ui/Widget.h"
#include "voxel/polyvox/Voxel.h"

class PaletteWidget: public ui::Widget {
private:
	using Super = ui::Widget;
protected:
	int _width = 0;
	int _height = 0;
	int _padding = 0;
	bool _dirty = true;
	voxel::VoxelType _voxelType = voxel::VoxelType::Grass1;
public:
	UIWIDGET_SUBCLASS(PaletteWidget, Super);

	PaletteWidget();
	~PaletteWidget();

	voxel::VoxelType voxelType() const;
	void markAsClean();
	bool isDirty() const;

	void OnPaint(const PaintProps &paint_props) override;
	void OnInflate(const tb::INFLATE_INFO &info) override;
	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

inline voxel::VoxelType PaletteWidget::voxelType() const {
	return _voxelType;
}

inline void PaletteWidget::markAsClean() {
	_dirty = false;
}

inline bool PaletteWidget::isDirty() const {
	return _dirty;
}
