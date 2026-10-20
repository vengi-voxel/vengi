/**
 * @file
 */

#pragma once

namespace voxelpathtracer {

struct PathTracerParams;

/**
 * @brief Registers pathtracer cvars and syncs them with @c PathTracerParams.
 *
 * Cvars use the @c pathtracer_ prefix so thumbnailer/voxconvert can override them via
 * @c -set / environment without going through the voxedit UI.
 */
class PathTracerConfig {
public:
	static bool init();
	static void loadParams(PathTracerParams &params);
	static void saveParams(const PathTracerParams &params);
};

} // namespace voxelpathtracer
