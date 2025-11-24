/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "voxel/Region.h"
#include "voxelfont/VoxelFont.h"

namespace voxedit {

/**
 * @brief A brush that is able to print text into the target volume. You can change the size, the spacing and the font
 * itself.
 * @ingroup Brushes
 */
class TextBrush : public Brush {
private:
	using Super = Brush;

protected:
	core::String _font = "font.ttf";
	core::String _input = "text";
	glm::ivec3 _lastCursorPosition{0};
	int _size = 16;
	int _spacing = 1;
	int _thickness = 1;
	mutable voxelfont::VoxelFont _voxelFont;
	math::Axis _axis = math::Axis::X;
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	TextBrush() : Super(BrushType::Text) {
	}
	virtual ~TextBrush() = default;
	void construct() override;

	void reset() override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	void shutdown() override;

	voxel::Region calcRegion(const BrushContext &ctx) const override;

	/**
	 * @brief The font to use for rendering. This is a path to the ttf file
	 */
	core::String &font();
	void setFont(const core::String &font);

	/**
	 * @brief The text to render
	 */
	core::String &input();
	void setInput(const core::String &input);

	/**
	 * @brief set the font size
	 */
	void setSize(int size);
	int size() const;

	/**
	 * @brief set the spacing between single chars
	 */
	void setSpacing(int spacing);
	int spacing() const;

	/**
	 * @brief set the thickness of the text
	 * @note the width and height are defined by the font size
	 * @sa setSize()
	 */
	void setThickness(int thickness);
	int thickness() const;
};

inline core::String &TextBrush::font() {
	return _font;
}

inline void TextBrush::setFont(const core::String &font) {
	_font = font;
}

inline core::String &TextBrush::input() {
	return _input;
}

inline void TextBrush::setInput(const core::String &input) {
	_input = input;
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
