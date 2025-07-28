/**
 * @file
 */

#include "scenegraph/Clipper.h"
#include "TestHelper.h"
#include "app/tests/AbstractTest.h"
#include "glm/fwd.hpp"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace scenegraph {

class ClipperTest : public app::AbstractTest {
protected:
	SceneGraph _sceneGraph;
	voxel::RawVolume _v{{-10, 10}};
	Clipper _clipper{_sceneGraph};

public:
	void SetUp() override {
		app::AbstractTest::SetUp();
		_sceneGraph.clear();
		SceneGraphNode node{SceneGraphNodeType::Model};
		_v.setVoxel(-2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_v.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_v.setVoxel(4, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		node.setVolume(&_v, false);
		_sceneGraph.emplace(core::move(node));
	}
};

TEST_F(ClipperTest, testClippingBlockedRight) {
	const glm::vec3 worldPos(0.0f, 0.0f, 0.0f);
	const glm::vec3 dir(1.0f, 0.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_NE(delta, dir) << "Clipping should detect the solid voxel and prevent movement";
}

TEST_F(ClipperTest, testClippingNoBlockedRightHalf) {
	const glm::vec3 worldPos(0.35f, 0.0f, 0.0f);
	const glm::vec3 dir(0.1f, 0.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_EQ(delta, dir) << "Clipping should allow movement if it does not cross the solid voxel boundary";
}

TEST_F(ClipperTest, testClippingNoBlockedRight) {
	const glm::vec3 worldPos(2.0f, 0.0f, 0.0f);
	const glm::vec3 dir(1.0f, 0.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_EQ(delta, dir) << "Clipping should not detect any solid voxel and allow movement";
}

TEST_F(ClipperTest, testClippingBlockedLeft) {
	const glm::vec3 worldPos(2.0f, 0.0f, 0.0f);
	const glm::vec3 dir(-1.0f, 0.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_NE(delta, dir) << "Clipping should detect the solid voxel and prevent movement";
}

TEST_F(ClipperTest, testClippingNoBlockedLeft) {
	const glm::vec3 worldPos(0.6f, 0.0f, 0.0f);
	const glm::vec3 dir(-1.0f, 0.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_EQ(delta, dir) << "Clipping should not detect any solid voxel and allow movement";
}

TEST_F(ClipperTest, testClippingNoBlockedLeftHalf) {
	const glm::vec3 worldPos(0.6f, 0.0f, 0.0f);
	const glm::vec3 dir(-0.1f, 0.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_EQ(delta, dir) << "Clipping should not detect any solid voxel and allow movement";
}

TEST_F(ClipperTest, testClippingBlockedTop) {
	const glm::vec3 worldPos(1.0f, -1.0f, 0.0f);
	const glm::vec3 dir(0.0f, 1.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_NE(delta, dir) << "Clipping should detect the solid voxel and prevent movement";
}

TEST_F(ClipperTest, testClippingNoBlockedTop) {
	const glm::vec3 worldPos(1.0f, 1.0f, 0.0f);
	const glm::vec3 dir(0.0f, 1.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_EQ(delta, dir) << "Clipping should not detect any solid voxel and allow movement";
}

TEST_F(ClipperTest, testClippingBlockedDown) {
	const glm::vec3 worldPos(1.0f, 1.0f, 0.0f);
	const glm::vec3 dir(0.0f, -1.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_NE(delta, dir) << "Clipping should detect the solid voxel and prevent movement";
}

TEST_F(ClipperTest, testClippingNoBlockedDown) {
	const glm::vec3 worldPos(1.0f, -1.0f, 0.0f);
	const glm::vec3 dir(0.0f, -1.0f, 0.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_EQ(delta, dir) << "Clipping should not detect any solid voxel and allow movement";
}

TEST_F(ClipperTest, testClippingBlockedFront) {
	const glm::vec3 worldPos(1.0f, 0.0f, 1.0f);
	const glm::vec3 dir(0.0f, 0.0f, -1.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_NE(delta, dir) << "Clipping should detect the solid voxel and prevent movement";
}

TEST_F(ClipperTest, testClippingNoBlockedFront) {
	const glm::vec3 worldPos(1.0f, 0.0f, 1.0f);
	const glm::vec3 dir(0.0f, 0.0f, 1.0f);
	const glm::mat3 noRot(1.0f);
	const glm::vec3 delta = _clipper.clipDelta(InvalidFrame, worldPos, dir, noRot);
	EXPECT_EQ(delta, dir) << "Clipping should not detect any solid voxel and allow movement";
}

} // namespace scenegraph
