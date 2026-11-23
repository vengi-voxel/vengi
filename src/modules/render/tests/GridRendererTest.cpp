/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "../GridRenderer.h"
#include "math/AABB.h"
#include "math/Vec3f.h"

namespace render {

class GridRendererTest : public video::AbstractGLTest {};

TEST_F(GridRendererTest, testInitUpdateShutdown) {
	GridRenderer renderer(true, true, true);
	ASSERT_TRUE(renderer.init());
	ASSERT_TRUE(renderer.setGridResolution(1));
	renderer.setPlaneGridSize(16);
	const math::AABB<float> aabb(math::Vec3f(-8.0f), math::Vec3f(8.0f));
	renderer.update(aabb);
	renderer.setRenderGrid(true);
	renderer.setRenderPlane(true);
	renderer.setRenderAABB(true);
	renderer.shutdown();
}

} // namespace render
