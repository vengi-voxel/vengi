/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "voxel/Region.h"
#include "voxelfont/VoxelFont.h"

namespace voxedit {

class SceneManager;

class TextBrush : public Brush {
private:
	using Super = Brush;

protected:
	SceneManager *_sceneMgr;
	core::String _font = "font.ttf";
	core::String _input = "text";
	glm::ivec3 _lastCursorPosition{0};
	int _size = 10;
	int _spacing = 1;
	int _thickness = 1;
	voxelfont::VoxelFont _voxelFont;

public:
	TextBrush(SceneManager *sceneMgr) : Super(BrushType::Text), _sceneMgr(sceneMgr) {
	}
	virtual ~TextBrush() = default;
	void construct() override;

	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
				 const BrushContext &context) override;
	void reset() override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	void shutdown() override;
	voxel::Region calcRegion(const BrushContext &context);

	core::String &font();
	core::String &input();

	void setSize(int size);
	int size() const;

	void setSpacing(int spacing);
	int spacing() const;

	void setThickness(int thickness);
	int thickness() const;
};

inline core::String &TextBrush::font() {
	return _font;
}

inline core::String &TextBrush::input() {
	return _input;
}

inline int TextBrush::size() const {
	return _size;
}

inline void TextBrush::setSpacing(int spacing) {
	_spacing = spacing;
	markDirty();
}

inline int TextBrush::spacing() const {
	return _spacing;
}

inline int TextBrush::thickness() const {
	return _thickness;
}

} // namespace voxedit
