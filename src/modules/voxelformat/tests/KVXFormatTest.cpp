/**
 * @file
 */

#include "voxelformat/private/slab6/KVXFormat.h"
#include "AbstractFormatTest.h"
#include "math/Vec3f.h"

namespace voxelformat {

class KVXFormatTest : public AbstractFormatTest {};

TEST_F(KVXFormatTest, testLoad) {
	testLoad("test.kvx");
}

TEST_F(KVXFormatTest, testSaveSmallVoxel) {
	KVXFormat f;
	testSaveLoadVoxel("kvx-smallvolumesavetest.kvx", &f, -16, 15,
					  voxel::ValidateFlags::AllPaletteMinMatchingColors);
}

TEST_F(KVXFormatTest, testSaveLoadTranslationBakedIntoPivot) {
	// Scene-mode move must persist as a pivot change - KVX has no translation field.
	KVXFormat f;
	const math::Vec3f translation(4.0f, 8.0f, -2.0f);
	const math::Vec3f dims(8.0f);
	testSaveLoadBakedPivot("kvx-translation-pivot.kvx", &f, math::Vec3f(0.0f), translation, -translation / dims);
}

TEST_F(KVXFormatTest, testSaveLoadRaisedCenterPivot) {
	// Center pivot + raise by half height -> bottom-center (sits on the waterline).
	KVXFormat f;
	testSaveLoadBakedPivot("kvx-raised-center-pivot.kvx", &f, math::Vec3f(0.5f), math::Vec3f(0.0f, 4.0f, 0.0f),
						   math::Vec3f(0.5f, 0.0f, 0.5f));
}

TEST_F(KVXFormatTest, testSaveLoadCenteredAabbTranslation) {
	// MagicaVoxel apply-transform: pivot 0 and translation = AABB mins (model was centered).
	KVXFormat f;
	testSaveLoadBakedPivot("kvx-centered-aabb.kvx", &f, math::Vec3f(0.0f), math::Vec3f(-4.0f, 0.0f, -4.0f),
						   math::Vec3f(0.5f, 0.0f, 0.5f));
}

TEST_F(KVXFormatTest, testSaveLoadDirtyWorldTranslation) {
	// MagicaVoxel load sets world translation; save-as KVX may happen before updateTransforms().
	KVXFormat f;
	testSaveLoadBakedPivot("kvx-dirty-translation.kvx", &f, math::Vec3f(0.0f), math::Vec3f(-4.0f, 0.0f, -4.0f),
						   math::Vec3f(0.5f, 0.0f, 0.5f), false);
}

} // namespace voxelformat
