/**
 * @file
 */

#include "../ShapeRenderer.h"
#include "../SurfaceOverlayRenderer.h"
#include "image/Image.h"
#include "video/Camera.h"
#include "video/FrameBufferConfig.h"
#include "video/ScopedState.h"
#include "video/tests/AbstractGLTest.h"

namespace render {

class SurfaceOverlayRendererTest : public video::AbstractGLTest {
protected:
	video::FrameBuffer _scene;
	video::FrameBuffer _multisample;
	ShapeRenderer _shapes;
	SurfaceOverlayRenderer _overlay;
	video::Camera _camera;
	int32_t _box = -1;

	void SetUp() override {
		video::AbstractGLTest::SetUp();
		if (IsSkipped()) {
			return;
		}
		video::FrameBufferConfig cfg;
		cfg.dimension(glm::ivec2(64))
			.colorTexture(true)
			.depthTexture(true)
			.depthTextureFormat(video::TextureFormat::D32F);
		ASSERT_TRUE(_scene.init(cfg));
		ASSERT_TRUE(_shapes.init());
		_camera.setSize(glm::ivec2(64));
		_camera.setWorldPosition(glm::vec3(0, 0, 10));
		_camera.lookAt(glm::vec3(0));
		_camera.update(0.0);
		video::enable(video::State::DepthTest);
		video::enable(video::State::DepthMask);
		video::disable(video::State::CullFace);
		video::disable(video::State::Scissor);
		video::depthFunc(video::CompareFunc::LessEqual);
		video::clearColor(glm::vec4(0));
	}

	void TearDown() override {
		_overlay.shutdown();
		_shapes.shutdown();
		_scene.shutdown();
		_multisample.shutdown();
		video::AbstractGLTest::TearDown();
	}

	void box(const glm::vec4 &color, float z = 0.0f) {
		video::ShapeBuilder builder;
		builder.setColor(color);
		builder.cube(glm::vec3(-2, -2, z - 1), glm::vec3(2, 2, z + 1));
		_shapes.createOrUpdate(_box, builder);
		EXPECT_TRUE(_shapes.render(_box, _camera));
	}
};

TEST_F(SurfaceOverlayRendererTest, testCoincidentReplacementAndCancellation) {
	_scene.bind(true);
	box(glm::vec4(0, 1, 0, 1));
	ASSERT_TRUE(_overlay.begin(glm::ivec2(64)));
	box(glm::vec4(0, 0, 1, 1));
	_overlay.end();
	EXPECT_EQ(_scene.handle(), video::currentFramebuffer());
	image::ImagePtr image = _scene.image("replacement");
	ASSERT_TRUE(image);
	EXPECT_GT(image->colorAt(32, 32).b, 200);
	EXPECT_LT(image->colorAt(32, 32).g, 40);
	_scene.unbind();

	_scene.bind(true);
	box(glm::vec4(0, 1, 0, 1));
	image = _scene.image("cancelled");
	ASSERT_TRUE(image);
	EXPECT_GT(image->colorAt(32, 32).g, 240);
	EXPECT_EQ(0, image->colorAt(32, 32).b);
	_scene.unbind();
}

TEST_F(SurfaceOverlayRendererTest, testDeletionHatchAndOcclusion) {
	_scene.bind(true);
	box(glm::vec4(0, 1, 0, 1));
	ASSERT_TRUE(_overlay.begin(glm::ivec2(64)));
	box(glm::vec4(0, 1, 0, 1));
	_overlay.end(glm::vec4(1, 0, 0, 0.8f));
	const image::ImagePtr hatched = _scene.image("deletion");
	ASSERT_TRUE(hatched);
	EXPECT_GT(hatched->colorAt(32, 32).r, 190);
	EXPECT_GT(hatched->colorAt(32, 32).r, hatched->colorAt(32, 32).g);
	EXPECT_NE(hatched->colorAt(32, 32), hatched->colorAt(36, 32));
	_scene.unbind();

	_scene.bind(true);
	box(glm::vec4(0, 1, 0, 1), 3.0f);
	ASSERT_TRUE(_overlay.begin(glm::ivec2(64)));
	box(glm::vec4(0, 1, 0, 1));
	_overlay.end(glm::vec4(1, 0, 0, 0.8f));
	const image::ImagePtr occluded = _scene.image("occluded");
	ASSERT_TRUE(occluded);
	EXPECT_EQ(0, occluded->colorAt(32, 32).r);
	EXPECT_GT(occluded->colorAt(32, 32).g, 240);
	_scene.unbind();
}

TEST_F(SurfaceOverlayRendererTest, testRestoresTargetAndResizes) {
	for (int size : {64, 32, 64}) {
		video::FrameBuffer target;
		video::FrameBufferConfig cfg;
		cfg.dimension(glm::ivec2(size))
			.colorTexture(true)
			.depthTexture(true)
			.depthTextureFormat(video::TextureFormat::D32F);
		ASSERT_TRUE(target.init(cfg));
		target.bind(true);
		ASSERT_TRUE(_overlay.begin(glm::ivec2(size)));
		_overlay.end();
		EXPECT_EQ(target.handle(), video::currentFramebuffer());
		int x, y, width, height;
		video::getViewport(x, y, width, height);
		EXPECT_EQ(size, width);
		EXPECT_EQ(size, height);
		target.unbind();
		target.shutdown();
	}
}

TEST_F(SurfaceOverlayRendererTest, testCoincidentSlopedSurfaceWithResolvedMsaaDepth) {
	video::TextureConfig color = video::createDefaultMultiSampleTextureConfig();
	color.samples(4);
	video::TextureConfig depth = color;
	depth.format(video::TextureFormat::D32F);
	video::FrameBufferConfig cfg;
	cfg.dimension(glm::ivec2(64)).samples(4);
	cfg.addTextureAttachment(color);
	cfg.addTextureAttachment(depth, video::FrameBufferAttachment::Depth);
	ASSERT_TRUE(_multisample.init(cfg));
	for (video::CameraMode mode : {video::CameraMode::Perspective, video::CameraMode::Orthogonal}) {
		SCOPED_TRACE((int)mode);
		_camera.setMode(mode);
		_camera.setWorldPosition(glm::vec3(18, 2, 18));
		_camera.lookAt(glm::vec3(0));
		_camera.update(0.0);
		_multisample.bind(true);
		box(glm::vec4(0, 1, 0, 1));
		video::blitFramebuffer(_multisample.handle(), _scene.handle(), video::FrameBufferAttachment::Color0, 64, 64);
		video::blitFramebuffer(_multisample.handle(), _scene.handle(), video::ClearFlag::Depth, 64, 64);
		_multisample.unbind();
		_scene.bind(false);
		const image::ImagePtr original = _scene.image("resolved scene");
		ASSERT_TRUE(_overlay.begin(glm::ivec2(64)));
		box(glm::vec4(0, 1, 0, 1));
		_overlay.end(glm::vec4(1, 0, 0, 0.8f));
		const image::ImagePtr preview = _scene.image("resolved deletion preview");
		ASSERT_TRUE(original && preview);
		int covered = 0;
		int marked = 0;
		for (int y = 1; y < 63; ++y) {
			for (int x = 1; x < 63; ++x) {
				if (original->colorAt(x, y).g > 240) {
					++covered;
					const color::RGBA pixel = preview->colorAt(x, y);
					marked += pixel.r > pixel.g;
				}
			}
		}
		EXPECT_GT(covered, 20);
		EXPECT_GT(marked, covered * 0.95f);
		_scene.unbind();
	}
}

} // namespace render
