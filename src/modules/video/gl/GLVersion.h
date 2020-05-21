/**
 * @file
 */

#pragma once

namespace video {

struct GLVersion {
	int majorVersion;
	int minorVersion;

	constexpr GLVersion(int _major, int _minor) :
			majorVersion(_major), minorVersion(_minor) {
	}

	inline bool operator==(const GLVersion& rhs) const {
		return majorVersion == rhs.majorVersion && minorVersion == rhs.minorVersion;
	}

	inline bool isAtLeast(int major, int minor) {
		return major > majorVersion || (major == majorVersion && minor >= minorVersion);
	}
};

struct GLSLVersion {
enum VersionIdentifiers {
	V100 = 100,
	V110 = 110,
	V120 = 120,
	V130 = 130,
	V140 = 140,
	V150 = 150,
	V300 = 300,
	V330 = 330,
	V400 = 400,
	V410 = 410,
	V420 = 420,
	V430 = 430,
	V440 = 440,
	V450 = 450
};
};

constexpr GLVersion GL2_0(2, 0);
constexpr GLVersion GL2_1(2, 1);
constexpr GLVersion GL3_0(3, 0);
constexpr GLVersion GL3_1(3, 1);
constexpr GLVersion GL3_2(3, 2);
constexpr GLVersion GL3_3(3, 3);
constexpr GLVersion GL4_0(4, 0);
constexpr GLVersion GL4_1(4, 1);
constexpr GLVersion GL4_2(4, 2);
constexpr GLVersion GL4_3(4, 3);
constexpr GLVersion GL4_4(4, 4);
constexpr GLVersion GL4_5(4, 5);

// https://github.com/mattdesl/lwjgl-basics/wiki/GLSL-Versions
constexpr struct Versions {
	GLVersion version;
	int glslVersion;
} GLVersions[] = {
#ifdef GL_ES_VERSION_2_0
	{GL2_0, GLSLVersion::V100},
	{GL3_0, GLSLVersion::V300},
#else
	{GL2_0, GLSLVersion::V110},
	{GL2_1, GLSLVersion::V120},
	{GL3_0, GLSLVersion::V130},
	{GL3_1, GLSLVersion::V140},
	{GL3_2, GLSLVersion::V150},
	{GL3_3, GLSLVersion::V330},
	{GL4_0, GLSLVersion::V400},
	{GL4_1, GLSLVersion::V410},
	{GL4_2, GLSLVersion::V420},
	{GL4_3, GLSLVersion::V430},
	{GL4_4, GLSLVersion::V440},
	{GL4_5, GLSLVersion::V450}
#endif
};

}
