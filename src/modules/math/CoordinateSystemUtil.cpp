/**
 * @file
 */

#include "CoordinateSystemUtil.h"

namespace math {

//
// Coordinate system definitions and conversions
// vengi internal coordinate system (Right-handed, Y-up, -Z-forward):
//     Y (up)
//     |
//     |
//     o----X (right)
//    /
//   Z (backward, toward viewer)
//
//   -Z direction = forward (into screen)
//
// Coordinate system classifications:
//
//   RIGHT-HANDED (Y-up, -Z-forward, same as OpenGL):
//     - vengi (internal)
//     - OpenGL
//     - Maya
//
//   LEFT-HANDED (Y-up, Z-forward):
//     - DirectX
//
//   RIGHT-HANDED (Z-up, Y-forward):
//     - MagicaVoxel
//     - VXL
//     - Blender
//     - Autodesk 3ds Max
//
// The basis vectors below represent each system's axes expressed in vengi coordinates.
//
bool coordinateSystemToMatrix(CoordinateSystem sys, glm::mat4 &matrix) {
	// Each case defines the basis vectors of the source coordinate system
	// expressed in vengi's coordinate system.
	// The resulting matrix columns represent: [right, up, forward, translation]
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 forward;
	switch (sys) {
	case CoordinateSystem::Vengi:
	case CoordinateSystem::Maya:
	case CoordinateSystem::OpenGL:
		// vengi, OpenGL, and Maya all use the same right-handed Y-up -Z-forward system
		// Identity - no conversion needed
		// Note: We use standard basis vectors (1,0,0), (0,1,0), (0,0,1)
		// The "forward" semantic (-Z direction) is handled by the application logic
		right = glm::vec3(1.0f, 0.0f, 0.0f);   // X-axis
		up = glm::vec3(0.0f, 1.0f, 0.0f);	   // Y-axis
		forward = glm::vec3(0.0f, 0.0f, 1.0f); // Z-axis (not -Z or glm::forward())
		break;
	case CoordinateSystem::DirectX:
		// DirectX: Left-handed, Y-up, Z-forward
		// vengi is right-handed, so we need to flip Z to convert handedness
		right = glm::vec3(1.0f, 0.0f, 0.0f);	// X-axis unchanged
		up = glm::vec3(0.0f, 1.0f, 0.0f);		// Y-axis unchanged
		forward = glm::vec3(0.0f, 0.0f, -1.0f); // Z-axis flipped (left-to-right hand)
		break;
	case CoordinateSystem::Autodesk3dsmax:
	case CoordinateSystem::MagicaVoxel:
	case CoordinateSystem::VXL:
		// Z-up right-handed systems: X=right, Y=forward, Z=up
		// Map to vengi (X=right, Y=up, Z=backward/-Z=forward):
		// Matrix columns represent where MV's X, Y, Z axes map in vengi's standard basis:
		//   MV's X (right) -> vengi's X (right): (1, 0, 0)
		//   MV's Y (forward) -> vengi's -Z (forward): (0, 0, -1)
		//   MV's Z (up) -> vengi's Y (up): (0, 1, 0)
		right = glm::vec3(1.0f, 0.0f, 0.0f);   // MV X -> vengi X
		up = glm::vec3(0.0f, 0.0f, -1.0f);	   // MV Y -> vengi -Z
		forward = glm::vec3(0.0f, 1.0f, 0.0f); // MV Z -> vengi Y
		break;
	case CoordinateSystem::Max:
	default:
		return false;
	}
	// clang-format off
	matrix = glm::mat4(
		glm::vec4(right, 0.0f),
		glm::vec4(up, 0.0f),
		glm::vec4(forward, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);
	// clang-format on
	return true;
}

bool coordinateSystemTransformationMatrix(CoordinateSystem from, CoordinateSystem to, glm::mat4 &transformationMatrix1,
										  glm::mat4 &transformationMatrix2) {
	if (from == to) {
		return false;
	}

	glm::mat4 fromSystem;
	if (!coordinateSystemToMatrix(from, fromSystem)) {
		return false;
	}

	glm::mat4 toSystem;
	if (!coordinateSystemToMatrix(to, toSystem)) {
		return false;
	}

	// To transform from one coordinate system to another:
	// 1. fromSystem contains the change-of-basis matrix from 'from' to vengi standard basis
	// 2. toSystem contains the change-of-basis matrix from 'to' to vengi standard basis
	// 3. To convert a matrix M from 'from' coords to 'to' coords:
	//    - First convert from 'from' to vengi: M' = fromSystem * M * inverse(fromSystem)
	//    - Then convert from vengi to 'to': M'' = inverse(toSystem) * M' * toSystem
	//    - Combined: M'' = inverse(toSystem) * fromSystem * M * inverse(fromSystem) * toSystem
	// So: T1 = inverse(toSystem) * fromSystem, T2 = inverse(fromSystem) * toSystem
	transformationMatrix1 = glm::inverse(toSystem) * fromSystem;
	transformationMatrix2 = glm::inverse(fromSystem) * toSystem;
	return true;
}

glm::mat4 convertCoordinateSystem(CoordinateSystem from, CoordinateSystem to, const glm::mat4 &fromMatrix) {
	glm::mat4x4 transformationMatrix1;
	glm::mat4x4 transformationMatrix2;
	if (!coordinateSystemTransformationMatrix(from, to, transformationMatrix1, transformationMatrix2)) {
		return fromMatrix;
	}
	return transformationMatrix1 * fromMatrix * transformationMatrix2;
}

bool coordinateSystemToRotationMatrix(CoordinateSystem sys, glm::mat3 &matrix) {
	glm::mat4 mat4;
	if (!coordinateSystemToMatrix(sys, mat4)) {
		return false;
	}
	matrix = glm::mat3(mat4);
	return true;
}

} // namespace math
