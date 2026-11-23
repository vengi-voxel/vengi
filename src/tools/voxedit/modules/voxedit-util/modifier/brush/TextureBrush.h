/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "image/Image.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "math/Vec2f.h"

namespace palette {
class Palette;
}

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

class SceneManager;

/**
 * @brief The texture brush is reading the color values from a texture and applies them to the volume with the given UV
 * coordinates.
 *
 * @ingroup Brushes
 */
class TextureBrush : public AABBBrush {
private:
	using Super = AABBBrush;

	SceneManager *_sceneMgr;
	image::ImagePtr _image;
	math::Vec2f _uv0{0.0f, 0.0f};
	math::Vec2f _uv1{1.0f, 1.0f};
	bool _projectOntoSurface = true;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	TextureBrush(SceneManager *sceneMgr) : Super(BrushType::Texture, ModifierType::Paint, ModifierType::Paint), _sceneMgr(sceneMgr) {
	}
	virtual ~TextureBrush() = default;

	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;

	void construct() override;
	void shutdown() override;

	bool needsAdditionalAction(const BrushContext &ctx) const override;

	/**
	 * @brief Create a texture image from the surface colors of the given volume face within the specified region.
	 * @param[in] volume The volume to sample colors from
	 * @param[in] palette The palette used for color lookups
	 * @param[in] region The region to capture
	 * @param[in] face The face to capture colors from
	 * @return @c true if the image was successfully created and set
	 */
	bool createImageFromFace(const voxel::RawVolume *volume, const palette::Palette &palette,
							 const voxel::Region &region, voxel::FaceNames face);

	void setImage(const image::ImagePtr &texture);
	const image::ImagePtr &image() const;

	void setUV0(const math::Vec2f &uv0);
	const math::Vec2f &uv0() const;

	void setUV1(const math::Vec2f &uv1);
	const math::Vec2f &uv1() const;

	void setProjectOntoSurface(bool projectOntoSurface);
	bool projectOntoSurface() const;
};

} // namespace voxedit
