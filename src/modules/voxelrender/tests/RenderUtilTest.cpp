/**
 * @file
 */

#include "voxelrender/RenderUtil.h"
#include "app/tests/AbstractTest.h"

namespace voxelrender {

class RenderUtilTest : public app::AbstractTest {};

TEST_F(RenderUtilTest, testConfigureCameraFarPlane) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	voxel::Region sceneRegion(glm::ivec3(0), glm::ivec3(512, 64, 512));
	const float farPlane = 5000.0f;
	SceneCameraMode mode = SceneCameraMode::Free;
	configureCamera(camera, sceneRegion, mode, farPlane);
	EXPECT_FLOAT_EQ(farPlane, camera.farPlane());
}

TEST_F(RenderUtilTest, testConfigureCameraFreeModeDistance) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	camera.setFieldOfView(45.0f);
	voxel::Region sceneRegion(glm::ivec3(0), glm::ivec3(512, 64, 512));

	configureCamera(camera, sceneRegion, SceneCameraMode::Free, 5000.0f);

	const glm::vec3 center = sceneRegion.calcCenterf();
	const glm::vec3 cameraPos = camera.worldPosition();
	const float actualDistance = glm::distance(cameraPos, center);
	const float targetDistance = camera.targetDistance();

	// The actual distance should match the target distance (within some tolerance)
	EXPECT_NEAR(actualDistance, targetDistance, 1.0f);

	// For a 512x64x512 scene, the camera should be reasonably close
	EXPECT_LT(actualDistance, 900.0f) << "Camera too far from scene";
	EXPECT_GT(actualDistance, 400.0f) << "Camera too close to scene";
}

TEST_F(RenderUtilTest, testConfigureCameraFreeModePosition) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	voxel::Region sceneRegion(glm::ivec3(0), glm::ivec3(512, 64, 512));

	configureCamera(camera, sceneRegion, SceneCameraMode::Free, 5000.0f);

	const glm::vec3 center = sceneRegion.calcCenterf();
	const glm::vec3 cameraPos = camera.worldPosition();

	// For Free mode, camera should be positioned diagonally (negative X and Z)
	EXPECT_LT(cameraPos.x, center.x);
	EXPECT_LT(cameraPos.z, center.z);
	EXPECT_FLOAT_EQ(cameraPos.y, (float)sceneRegion.getUpperY());
}

TEST_F(RenderUtilTest, testConfigureCameraTopMode) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	voxel::Region sceneRegion(glm::ivec3(0, 0, 0), glm::ivec3(100, 50, 100));

	configureCamera(camera, sceneRegion, SceneCameraMode::Top, 5000.0f);

	const glm::vec3 center = sceneRegion.calcCenterf();
	const glm::vec3 cameraPos = camera.worldPosition();

	// Top view should be directly above the scene
	EXPECT_FLOAT_EQ(cameraPos.x, center.x);
	EXPECT_GT(cameraPos.y, center.y);
	EXPECT_FLOAT_EQ(cameraPos.z, center.z);
}

TEST_F(RenderUtilTest, testConfigureCameraFrontMode) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	voxel::Region sceneRegion(glm::ivec3(0, 0, 0), glm::ivec3(100, 50, 100));

	configureCamera(camera, sceneRegion, SceneCameraMode::Front, 5000.0f);

	const glm::vec3 center = sceneRegion.calcCenterf();
	const glm::vec3 cameraPos = camera.worldPosition();

	// Front view should be in front of the scene (negative Z)
	EXPECT_FLOAT_EQ(cameraPos.x, center.x);
	EXPECT_FLOAT_EQ(cameraPos.y, center.y);
	EXPECT_LT(cameraPos.z, center.z);
}

// Test with a flat scene (512x64x512) (ace of spades)
TEST_F(RenderUtilTest, testConfigureCameraFlatSceneDistances) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	camera.setFieldOfView(45.0f);

	voxel::Region flatScene(glm::ivec3(0), glm::ivec3(512, 64, 512));

	configureCamera(camera, flatScene, SceneCameraMode::Front, 5000.0f);
	float frontDistance = camera.targetDistance();

	EXPECT_LT(frontDistance, 600.0f) << "Front view distance should account for flat dimension";

	configureCamera(camera, flatScene, SceneCameraMode::Top, 5000.0f);
	float topDistance = camera.targetDistance();
	EXPECT_GT(topDistance, frontDistance) << "Top view should be farther than front (sees larger area)";
}

TEST_F(RenderUtilTest, testConfigureCameraAspectRatioHandling) {
	video::Camera wideCamera;
	wideCamera.setSize(glm::ivec2(2560, 1080)); // Ultra-wide
	wideCamera.setFieldOfView(45.0f);

	video::Camera tallCamera;
	tallCamera.setSize(glm::ivec2(1080, 1920)); // Portrait
	tallCamera.setFieldOfView(45.0f);

	voxel::Region scene(glm::ivec3(0), glm::ivec3(100, 100, 100));

	configureCamera(wideCamera, scene, SceneCameraMode::Front, 5000.0f);
	configureCamera(tallCamera, scene, SceneCameraMode::Front, 5000.0f);

	float wideDistance = wideCamera.targetDistance();
	float tallDistance = tallCamera.targetDistance();

	// Ultra-wide camera should need to be farther to fit horizontally
	// Tall camera should need to be farther to fit vertically
	// Both should be reasonable and not excessively far
	EXPECT_GT(wideDistance, 50.0f);
	EXPECT_LT(wideDistance, 500.0f);
	EXPECT_GT(tallDistance, 50.0f);
	EXPECT_LT(tallDistance, 500.0f);
}

TEST_F(RenderUtilTest, testConfigureCameraCubeScene) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	camera.setFieldOfView(45.0f);

	// Test with a cube scene (100x100x100)
	voxel::Region cubeScene(glm::ivec3(0), glm::ivec3(100, 100, 100));

	configureCamera(camera, cubeScene, SceneCameraMode::Front, 5000.0f);
	float distance = camera.targetDistance();

	// For a 100x100x100 cube at 45 degree FOV, distance should fit the scene nicely
	EXPECT_GT(distance, 50.0f);
	EXPECT_LT(distance, 200.0f);
}

TEST_F(RenderUtilTest, testConfigureCameraVeryFlatScene) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	camera.setFieldOfView(45.0f);

	// Test with an extremely flat scene (1000x10x1000)
	voxel::Region flatScene(glm::ivec3(0), glm::ivec3(1000, 10, 1000));

	configureCamera(camera, flatScene, SceneCameraMode::Front, 5000.0f);
	float frontDistance = camera.targetDistance();

	// Front view sees 1000x10, so should be very close (width dominates)
	EXPECT_LT(frontDistance, 900.0f);

	configureCamera(camera, flatScene, SceneCameraMode::Top, 5000.0f);
	float topDistance = camera.targetDistance();

	// Top view sees 1000x1000, needs to be farther
	EXPECT_GT(topDistance, frontDistance * 1.5f);
}

TEST_F(RenderUtilTest, testConfigureCameraTallScene) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	camera.setFieldOfView(45.0f);

	// Test with a tall scene (100x500x100)
	voxel::Region tallScene(glm::ivec3(0), glm::ivec3(100, 500, 100));

	configureCamera(camera, tallScene, SceneCameraMode::Front, 5000.0f);
	float frontDistance = camera.targetDistance();

	// Front view sees 100x500, vertical dimension dominates
	EXPECT_GT(frontDistance, 200.0f);
	EXPECT_LT(frontDistance, 800.0f);

	configureCamera(camera, tallScene, SceneCameraMode::Top, 5000.0f);
	float topDistance = camera.targetDistance();

	// Top view sees 100x100, should be much closer
	EXPECT_LT(topDistance, frontDistance);
}

TEST_F(RenderUtilTest, testConfigureCameraSmallScene) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	camera.setFieldOfView(45.0f);

	// Test with a tiny scene (10x10x10)
	voxel::Region smallScene(glm::ivec3(0), glm::ivec3(10, 10, 10));

	configureCamera(camera, smallScene, SceneCameraMode::Free, 5000.0f);
	float distance = camera.targetDistance();

	// Should be close but not too close
	EXPECT_GT(distance, 5.0f);
	EXPECT_LT(distance, 50.0f);
}

TEST_F(RenderUtilTest, testConfigureCameraLargeScene) {
	video::Camera camera;
	camera.setSize(glm::ivec2(1920, 1080));
	camera.setFieldOfView(45.0f);

	// Test with a large scene (2000x2000x2000)
	voxel::Region largeScene(glm::ivec3(0), glm::ivec3(2000, 2000, 2000));

	configureCamera(camera, largeScene, SceneCameraMode::Free, 10000.0f);
	float distance = camera.targetDistance();

	// Should be far but reasonable
	EXPECT_GT(distance, 1000.0f);
	EXPECT_LT(distance, 6000.0f);
}

} // namespace voxelrender
