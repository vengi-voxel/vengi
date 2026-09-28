/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "../ShapeRenderer.h"
#include "math/AABB.h"
#include "video/Camera.h"
#include "video/ShapeBuilder.h"

namespace render {

class ShapeRendererTest : public video::AbstractGLTest {};

TEST_F(ShapeRendererTest, testAabbLinesAndCubeTriangles) {
	ShapeRenderer renderer;
	ASSERT_TRUE(renderer.init());

	video::ShapeBuilder builder;
	builder.setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	builder.aabb(math::AABB<float>(glm::vec3(-8.0f), glm::vec3(8.0f)));
	const int32_t aabbMesh = renderer.create(builder);
	ASSERT_GE(aabbMesh, 0);

	builder.clear();
	builder.setColor(glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
	builder.cube(glm::vec3(-1.0f), glm::vec3(1.0f));
	const int32_t cubeMesh = renderer.create(builder);
	ASSERT_GE(cubeMesh, 0);

	video::Camera camera;
	camera.setSize(glm::ivec2(640, 480));
	camera.setWorldPosition(glm::vec3(20.0f, 20.0f, 20.0f));
	camera.lookAt(glm::vec3(0.0f));
	camera.update(0.0);

	EXPECT_TRUE(renderer.render(aabbMesh, camera));
	EXPECT_TRUE(renderer.render(cubeMesh, camera));
	EXPECT_GE(renderer.renderAll(camera), 2);

	builder.clear();
	builder.setColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	builder.arrow(glm::vec3(-4.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(4.0f, 0.0f, 0.0f));
	renderer.update((uint32_t)cubeMesh, builder);
	EXPECT_TRUE(renderer.render(cubeMesh, camera));

	renderer.shutdown();
}

} // namespace render
