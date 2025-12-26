/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "app/I18N.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace palette {
class Palette;
}

namespace voxedit {

/**
 * @brief Changes the normal of existing voxels without modifying their presence
 * @ingroup Brushes
 * @sa AABBBrush for the region spanning behavior
 */
class NormalBrush : public AABBBrush {
public:
	/**
	 * @brief Different ways to modify voxel normals
	 */
	enum class PaintMode : uint8_t { Auto, Manual, Max };

	static constexpr const char *PaintModeStr[] = {N_("Auto"), N_("Manual")};
	static_assert(lengthof(PaintModeStr) == (int)NormalBrush::PaintMode::Max, "PaintModeStr size mismatch");

private:
	using Super = AABBBrush;

	PaintMode _paintMode = PaintMode::Auto; ///< Active paint mode

protected:
	/**
	 * @brief Apply the paint operation to all voxels in the region
	 */
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	NormalBrush() : Super(BrushType::Normal, ModifierType::NormalPaint, ModifierType::NormalPaint) {
	}
	virtual ~NormalBrush() = default;

	PaintMode paintMode() const;
	void setPaintMode(PaintMode mode);
};

inline NormalBrush::PaintMode NormalBrush::paintMode() const {
	return _paintMode;
}

inline void NormalBrush::setPaintMode(PaintMode mode) {
	_paintMode = mode;
	markDirty();
}

} // namespace voxedit
