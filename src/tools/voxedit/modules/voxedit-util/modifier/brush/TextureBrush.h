/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "image/Image.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace palette {
class Palette;
}

namespace voxedit {

/**
 * @brief The texture brush is reading the color values from a texture and applies them to the volume with the given UV
 * coordinates.
 *
 * @ingroup Brushes
 */
class TextureBrush : public AABBBrush {
private:
	using Super = AABBBrush;

	image::ImagePtr _image;
	glm::vec2 _uv0{0.0f, 0.0f};
	glm::vec2 _uv1{1.0f, 1.0f};
	bool _projectOntoSurface = true;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	TextureBrush() : Super(BrushType::Texture, ModifierType::Paint, ModifierType::Paint) {
	}
	virtual ~TextureBrush() = default;

	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;

	void construct() override;
	void shutdown() override;

	bool needsAdditionalAction(const BrushContext &ctx) const override;

	void setImage(const image::ImagePtr &texture);
	const image::ImagePtr &image() const;

	void setUV0(const glm::vec2 &uv0);
	const glm::vec2 &uv0() const;

	void setUV1(const glm::vec2 &uv1);
	const glm::vec2 &uv1() const;

	void setProjectOntoSurface(bool projectOntoSurface);
	bool projectOntoSurface() const;
};

} // namespace voxedit
