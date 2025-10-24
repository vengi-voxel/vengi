/**
 * @file
 */

#include "TestHelper.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/CoordinateSystemUtil.h"
#include <glm/gtc/matrix_transform.hpp>

namespace scenegraph {

class CoordinateSystemTest : public app::AbstractTest {
protected:
	void testConvertIdentity(CoordinateSystem from, CoordinateSystem to) {
		glm::mat4 identity(1.0f);
		const glm::mat4 &toMatrix = convertCoordinateSystem(from, to, identity);
		const glm::mat4 &fromMatrix = convertCoordinateSystem(to, from, toMatrix);
		EXPECT_EQ(identity, fromMatrix) << "Round-trip conversion failed for identity matrix";
	}

	void testConvert(CoordinateSystem from, CoordinateSystem to) {
		glm::mat4 src = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)),
											   glm::radians(23.0f), glm::vec3(1.0f, 1.0f, 1.0f)),
								   glm::vec3(2.0f, 3.0f, 4.0f));
		const glm::mat4 &toMatrix = convertCoordinateSystem(from, to, src);
		const glm::mat4 &fromMatrix = convertCoordinateSystem(to, from, toMatrix);

		// Compare matrices element by element with tolerance
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				EXPECT_NEAR(src[i][j], fromMatrix[i][j], 0.0001f)
					<< "Matrix mismatch at [" << i << "][" << j << "]";
			}
		}
	}

	void testAxisConversion(CoordinateSystem from, CoordinateSystem to,
							const glm::vec3 &fromPoint, const glm::vec3 &expectedToPoint) {
		// Create a translation matrix representing a point
		glm::mat4 pointMatrix = glm::translate(glm::mat4(1.0f), fromPoint);
		glm::mat4 convertedMatrix = convertCoordinateSystem(from, to, pointMatrix);

		// Extract the translation from the converted matrix
		glm::vec3 convertedPoint(convertedMatrix[3][0], convertedMatrix[3][1], convertedMatrix[3][2]);

		EXPECT_NEAR(convertedPoint.x, expectedToPoint.x, 0.0001f)
			<< "X coordinate mismatch";
		EXPECT_NEAR(convertedPoint.y, expectedToPoint.y, 0.0001f)
			<< "Y coordinate mismatch";
		EXPECT_NEAR(convertedPoint.z, expectedToPoint.z, 0.0001f)
			<< "Z coordinate mismatch";
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

TEST_F(CoordinateSystemTest, testDirectX) {
	testConvertIdentity(CoordinateSystem::DirectX, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::DirectX, CoordinateSystem::Vengi);
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
	testAxisConversion(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi,
					  glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	// In MagicaVoxel (0,1,0) = forward (in Y direction)
	// In VENGI (0,0,-1) = forward (in -Z direction)
	testAxisConversion(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi,
					  glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	// In MagicaVoxel (0,0,1) = up (in Z direction)
	// In VENGI (0,1,0) = up (in Y direction)
	testAxisConversion(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi,
					  glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

// OpenGL and vengi use the same coordinate system (right-handed, Y-up, -Z-forward)
// So all conversions should be identical
TEST_F(CoordinateSystemTest, testOpenGLAxisConversion) {
	testAxisConversion(CoordinateSystem::OpenGL, CoordinateSystem::Vengi, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	testAxisConversion(CoordinateSystem::OpenGL, CoordinateSystem::Vengi, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	testAxisConversion(CoordinateSystem::OpenGL, CoordinateSystem::Vengi, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	testAxisConversion(CoordinateSystem::OpenGL, CoordinateSystem::Vengi, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

// DirectX: Left-handed, Y-up, Z-forward (+Z is forward, not -Z)
// vengi: Right-handed, Y-up, -Z-forward
// The handedness flip means Z is negated
TEST_F(CoordinateSystemTest, testDirectXAxisConversion) {
	// Right stays right
	testAxisConversion(CoordinateSystem::DirectX, CoordinateSystem::Vengi, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	// Up stays up
	testAxisConversion(CoordinateSystem::DirectX, CoordinateSystem::Vengi, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	// DirectX forward (+Z) becomes vengi forward (-Z) 
	testAxisConversion(CoordinateSystem::DirectX, CoordinateSystem::Vengi, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	// DirectX backward (-Z) becomes vengi backward (+Z)
	testAxisConversion(CoordinateSystem::DirectX, CoordinateSystem::Vengi, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

} // namespace scenegraph
