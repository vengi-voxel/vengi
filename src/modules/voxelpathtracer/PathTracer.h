/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"

namespace image {
class Image;
typedef core::SharedPtr<Image> ImagePtr;
} // namespace image

namespace voxel {
class Mesh;
class Palette;
}

namespace scenegraph {
class SceneGraph;
}

namespace voxelpathtracer {

struct PathTracerState;

class PathTracer {
private:
	PathTracerState *_state;

	bool createScene(const voxel::Palette &palette, const voxel::Mesh &mesh);
	bool createScene(const scenegraph::SceneGraph &sceneGraph);

public:
	PathTracer();
	~PathTracer();
	bool start(const scenegraph::SceneGraph &sceneGraph, int dimensions = 1280, int samples = 512);
	bool stop();

	/**
	 * @return @c true if rendering is done, @c false otherwise
	 */
	bool update();

	image::ImagePtr image() const;
};

} // namespace voxelpathtracer
