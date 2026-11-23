/**
 * @file
 */

#include "CoordinateSystemUtil.h"
#include "math/Mat4f.h"
#include "math/Mat3f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/Mat4Ops.h"
#include "math/Axes.h"

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
//     - SceneKit (Apple: camera looks down -Z, +Z toward viewer)
//
//   LEFT-HANDED (Y-up, Z-forward):
//     - DirectX
//     - Unity
//
//   RIGHT-HANDED (Z-up, Y-forward):
//     - MagicaVoxel
//     - VXL
//     - Blender
//     - Autodesk 3ds Max
//
// The basis vectors below represent each system's axes expressed in vengi coordinates.
//
// https://mastodon.social/@acegikmo/113313928426095165
//
bool coordinateSystemToMatrix(CoordinateSystem sys, math::Mat4f &matrix) {
	// Each case defines the basis vectors of the source coordinate system
	// expressed in vengi's coordinate system.
	// The resulting matrix columns represent: [right, up, forward, translation]
	math::Vec3f right;
	math::Vec3f up;
	math::Vec3f forward;
	switch (sys) {
	case CoordinateSystem::Vengi:
	case CoordinateSystem::Maya:
	case CoordinateSystem::OpenGL:
	case CoordinateSystem::SceneKit:
		// vengi, OpenGL, Maya, and SceneKit all use the same right-handed Y-up -Z-forward system
		// Identity - no conversion needed
		// Note: We use standard basis vectors (1,0,0), (0,1,0), (0,0,1)
		// The "forward" semantic (-Z direction) is handled by the application logic
		right = math::Vec3f(1.0f, 0.0f, 0.0f);   // X-axis
		up = math::Vec3f(0.0f, 1.0f, 0.0f);	   // Y-axis
		forward = math::Vec3f(0.0f, 0.0f, 1.0f); // Z-axis (not -Z or math::forward())
		break;
	case CoordinateSystem::DirectX:
	case CoordinateSystem::Unity:
		// DirectX / Unity: Left-handed, Y-up, Z-forward
		// vengi is right-handed, so we need to flip Z to convert handedness
		right = math::Vec3f(1.0f, 0.0f, 0.0f);	// X-axis unchanged
		up = math::Vec3f(0.0f, 1.0f, 0.0f);		// Y-axis unchanged
		forward = math::Vec3f(0.0f, 0.0f, -1.0f); // Z-axis flipped (left-to-right hand)
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
		right = math::Vec3f(1.0f, 0.0f, 0.0f);   // MV X -> vengi X
		up = math::Vec3f(0.0f, 0.0f, -1.0f);	   // MV Y -> vengi -Z
		forward = math::Vec3f(0.0f, 1.0f, 0.0f); // MV Z -> vengi Y
		break;
	case CoordinateSystem::Max:
	default:
		return false;
	}
	// clang-format off
	matrix = math::Mat4f(
		math::Vec4f(right, 0.0f),
		math::Vec4f(up, 0.0f),
		math::Vec4f(forward, 0.0f),
		math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f)
	);
	// clang-format on
	return true;
}

bool coordinateSystemTransformationMatrix(CoordinateSystem from, CoordinateSystem to, math::Mat4f &transformationMatrix1,
										  math::Mat4f &transformationMatrix2) {
	if (from == to) {
		return false;
	}

	math::Mat4f fromSystem;
	if (!coordinateSystemToMatrix(from, fromSystem)) {
		return false;
	}

	math::Mat4f toSystem;
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
	transformationMatrix1 = math::inverse(toSystem) * fromSystem;
	transformationMatrix2 = math::inverse(fromSystem) * toSystem;
	return true;
}

math::Mat4f convertCoordinateSystem(CoordinateSystem from, CoordinateSystem to, const math::Mat4f &fromMatrix) {
	math::Mat4f transformationMatrix1;
	math::Mat4f transformationMatrix2;
	if (!coordinateSystemTransformationMatrix(from, to, transformationMatrix1, transformationMatrix2)) {
		return fromMatrix;
	}
	return transformationMatrix1 * fromMatrix * transformationMatrix2;
}

bool coordinateSystemToRotationMatrix(CoordinateSystem sys, math::Mat3f &matrix) {
	math::Mat4f mat4;
	if (!coordinateSystemToMatrix(sys, mat4)) {
		return false;
	}
	matrix = math::Mat3f(mat4);
	return true;
}

} // namespace math
