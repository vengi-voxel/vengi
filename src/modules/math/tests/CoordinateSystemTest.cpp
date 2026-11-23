/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "math/CoordinateSystemUtil.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"
#include "math/Scalar.h"

namespace math {

class CoordinateSystemTest : public app::AbstractTest {
protected:
	void testConvertIdentity(CoordinateSystem from, CoordinateSystem to) {
		math::Mat4f identity(1.0f);
		const math::Mat4f &toMatrix = convertCoordinateSystem(from, to, identity);
		const math::Mat4f &fromMatrix = convertCoordinateSystem(to, from, toMatrix);
		EXPECT_EQ(identity, fromMatrix) << "Round-trip conversion failed for identity matrix";
	}

	void testConvert(CoordinateSystem from, CoordinateSystem to) {
		math::Mat4f src = math::scale(math::rotate(math::translate(math::Mat4f(1.0f), math::Vec3f(1.0f, 2.0f, 3.0f)),
											   math::radians(23.0f), math::Vec3f(1.0f, 1.0f, 1.0f)),
								   math::Vec3f(2.0f, 3.0f, 4.0f));
		const math::Mat4f &toMatrix = convertCoordinateSystem(from, to, src);
		const math::Mat4f &fromMatrix = convertCoordinateSystem(to, from, toMatrix);

		// Compare matrices element by element with tolerance
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				EXPECT_NEAR(src[i][j], fromMatrix[i][j], 0.0001f) << "Matrix mismatch at [" << i << "][" << j << "]";
			}
		}
	}

	void testAxisConversion(CoordinateSystem from, CoordinateSystem to, const math::Vec3f &fromPoint,
							const math::Vec3f &expectedToPoint) {
		// Create a translation matrix representing a point
		math::Mat4f pointMatrix = math::translate(math::Mat4f(1.0f), fromPoint);
		math::Mat4f convertedMatrix = convertCoordinateSystem(from, to, pointMatrix);

		// Extract the translation from the converted matrix
		math::Vec3f convertedPoint(convertedMatrix[3][0], convertedMatrix[3][1], convertedMatrix[3][2]);

		EXPECT_NEAR(convertedPoint.x, expectedToPoint.x, 0.0001f) << "X coordinate mismatch";
		EXPECT_NEAR(convertedPoint.y, expectedToPoint.y, 0.0001f) << "Y coordinate mismatch";
		EXPECT_NEAR(convertedPoint.z, expectedToPoint.z, 0.0001f) << "Z coordinate mismatch";
	}
};

TEST_F(CoordinateSystemTest, testVengiToVengi) {
	testConvertIdentity(CoordinateSystem::Vengi, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::Vengi, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testVXL) {
	testConvertIdentity(CoordinateSystem::VXL, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::VXL, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testMagicavoxel) {
	testConvertIdentity(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testOpenGL) {
	testConvertIdentity(CoordinateSystem::OpenGL, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::OpenGL, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testMaya) {
	testConvertIdentity(CoordinateSystem::Maya, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::Maya, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testSceneKit) {
	testConvertIdentity(CoordinateSystem::SceneKit, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::SceneKit, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testDirectX) {
	testConvertIdentity(CoordinateSystem::DirectX, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::DirectX, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testUnity) {
	testConvertIdentity(CoordinateSystem::Unity, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::Unity, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, test3dsMax) {
	testConvertIdentity(CoordinateSystem::Autodesk3dsmax, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::Autodesk3dsmax, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testMagicaVoxelAxisConversion) {
	// MagicaVoxel: Z-up, Y-forward, X-right (right-handed) (TODO: check this assumption)
	// VENGI: Y-up, -Z-forward, X-right (right-handed, same as OpenGL)

	// In MagicaVoxel (1,0,0) = right
	// In VENGI (1,0,0) = right
	testAxisConversion(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi, math::Vec3f(1.0f, 0.0f, 0.0f),
					   math::Vec3f(1.0f, 0.0f, 0.0f));

	// In MagicaVoxel (0,1,0) = forward (in Y direction)
	// In VENGI (0,0,-1) = forward (in -Z direction)
	testAxisConversion(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi, math::Vec3f(0.0f, 1.0f, 0.0f),
					   math::Vec3f(0.0f, 0.0f, -1.0f));

	// In MagicaVoxel (0,0,1) = up (in Z direction)
	// In VENGI (0,1,0) = up (in Y direction)
	testAxisConversion(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, 1.0f),
					   math::Vec3f(0.0f, 1.0f, 0.0f));
}

// SceneKit uses the same axes as OpenGL/vengi (right-handed, Y-up, +Z toward viewer)
TEST_F(CoordinateSystemTest, testSceneKitAxisConversion) {
	testAxisConversion(CoordinateSystem::SceneKit, CoordinateSystem::Vengi, math::Vec3f(1.0f, 0.0f, 0.0f),
					   math::Vec3f(1.0f, 0.0f, 0.0f));
	testAxisConversion(CoordinateSystem::SceneKit, CoordinateSystem::Vengi, math::Vec3f(0.0f, 1.0f, 0.0f),
					   math::Vec3f(0.0f, 1.0f, 0.0f));
	testAxisConversion(CoordinateSystem::SceneKit, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, -1.0f),
					   math::Vec3f(0.0f, 0.0f, -1.0f));
	testAxisConversion(CoordinateSystem::SceneKit, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, 1.0f),
					   math::Vec3f(0.0f, 0.0f, 1.0f));
}

// OpenGL and vengi use the same coordinate system (right-handed, Y-up, -Z-forward)
// So all conversions should be identical
TEST_F(CoordinateSystemTest, testOpenGLAxisConversion) {
	testAxisConversion(CoordinateSystem::OpenGL, CoordinateSystem::Vengi, math::Vec3f(1.0f, 0.0f, 0.0f),
					   math::Vec3f(1.0f, 0.0f, 0.0f));
	testAxisConversion(CoordinateSystem::OpenGL, CoordinateSystem::Vengi, math::Vec3f(0.0f, 1.0f, 0.0f),
					   math::Vec3f(0.0f, 1.0f, 0.0f));
	testAxisConversion(CoordinateSystem::OpenGL, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, -1.0f),
					   math::Vec3f(0.0f, 0.0f, -1.0f));
	testAxisConversion(CoordinateSystem::OpenGL, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, 1.0f),
					   math::Vec3f(0.0f, 0.0f, 1.0f));
}

// DirectX: Left-handed, Y-up, Z-forward (+Z is forward, not -Z)
// vengi: Right-handed, Y-up, -Z-forward
// The handedness flip means Z is negated
TEST_F(CoordinateSystemTest, testDirectXAxisConversion) {
	// Right stays right
	testAxisConversion(CoordinateSystem::DirectX, CoordinateSystem::Vengi, math::Vec3f(1.0f, 0.0f, 0.0f),
					   math::Vec3f(1.0f, 0.0f, 0.0f));
	// Up stays up
	testAxisConversion(CoordinateSystem::DirectX, CoordinateSystem::Vengi, math::Vec3f(0.0f, 1.0f, 0.0f),
					   math::Vec3f(0.0f, 1.0f, 0.0f));
	// DirectX forward (+Z) becomes vengi forward (-Z)
	testAxisConversion(CoordinateSystem::DirectX, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, 1.0f),
					   math::Vec3f(0.0f, 0.0f, -1.0f));
	// DirectX backward (-Z) becomes vengi backward (+Z)
	testAxisConversion(CoordinateSystem::DirectX, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, -1.0f),
					   math::Vec3f(0.0f, 0.0f, 1.0f));
}

// Unity uses the same axes as DirectX (left-handed, Y-up, Z-forward)
TEST_F(CoordinateSystemTest, testUnityAxisConversion) {
	testAxisConversion(CoordinateSystem::Unity, CoordinateSystem::Vengi, math::Vec3f(1.0f, 0.0f, 0.0f),
					   math::Vec3f(1.0f, 0.0f, 0.0f));
	testAxisConversion(CoordinateSystem::Unity, CoordinateSystem::Vengi, math::Vec3f(0.0f, 1.0f, 0.0f),
					   math::Vec3f(0.0f, 1.0f, 0.0f));
	testAxisConversion(CoordinateSystem::Unity, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, 1.0f),
					   math::Vec3f(0.0f, 0.0f, -1.0f));
	testAxisConversion(CoordinateSystem::Unity, CoordinateSystem::Vengi, math::Vec3f(0.0f, 0.0f, -1.0f),
					   math::Vec3f(0.0f, 0.0f, 1.0f));
}

} // namespace math
