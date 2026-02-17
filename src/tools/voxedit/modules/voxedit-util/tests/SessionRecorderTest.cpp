/**
 * @file
 */

#include "voxedit-util/network/SessionRecorder.h"
#include "AbstractSceneManagerTest.h"
#include "core/FourCC.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "voxedit-util/network/ProtocolVersion.h"
#include "voxedit-util/network/SessionPlayer.h"

namespace voxedit {

class SessionRecorderTest : public AbstractSceneManagerTest {
private:
	using Super = AbstractSceneManagerTest;

protected:
	void SetUp() override {
		Super::SetUp();
	}
};

TEST_F(SessionRecorderTest, testStartStopRecording) {
	const core::String filename = _testApp->filesystem()->homeWritePath("test_recording.vrec");
	EXPECT_TRUE(_sceneMgr->startRecording(filename));
	EXPECT_TRUE(_sceneMgr->isRecording());
	_sceneMgr->stopRecording();
	EXPECT_FALSE(_sceneMgr->isRecording());
}

TEST_F(SessionRecorderTest, testRecordingWritesHeader) {
	const core::String filename = _testApp->filesystem()->homeWritePath("test_header.vrec");
	EXPECT_TRUE(_sceneMgr->startRecording(filename));
	_sceneMgr->stopRecording();

	// Read back and verify the header
	io::FilePtr file = core::make_shared<io::File>(filename, io::FileMode::SysRead);
	io::FileStream stream(file);
	ASSERT_TRUE(stream.valid());

	uint32_t magic;
	ASSERT_NE(stream.readUInt32(magic), -1);
	EXPECT_EQ(magic, FourCC('V', 'R', 'E', 'C'));

	uint8_t version;
	ASSERT_NE(stream.readUInt8(version), -1);
	EXPECT_EQ(version, PROTOCOL_VERSION);

	// There should be at least a scene state message after the header
	EXPECT_GT(stream.size(), (int64_t)(sizeof(magic) + sizeof(version)));
}

TEST_F(SessionRecorderTest, testRecordAndPlaybackRoundtrip) {
	const core::String filename = _testApp->filesystem()->homeWritePath("test_roundtrip.vrec");

	// Record a session with some modifications
	EXPECT_TRUE(_sceneMgr->startRecording(filename));

	// Perform a voxel modification - set a voxel
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setSingleMode();
	modifier.setModifierType(ModifierType::Place);
	modifier.setCursorPosition(glm::ivec3(0, 0, 0), voxel::FaceNames::NegativeX);
	ASSERT_TRUE(modifier.beginBrush());
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	scenegraph::SceneGraph tmpSceneGraph;
	scenegraph::SceneGraphNode tmpNode(scenegraph::SceneGraphNodeType::Model);
	tmpNode.setVolume(v, false);
	int executed = 0;
	auto callback = [&](const voxel::Region &region, ModifierType, SceneModifiedFlags) {
		executed++;
		_sceneMgr->modified(nodeId, region);
	};
	ASSERT_TRUE(modifier.execute(tmpSceneGraph, tmpNode, callback));

	_sceneMgr->stopRecording();
	EXPECT_FALSE(_sceneMgr->isRecording());

	// Now play it back
	EXPECT_TRUE(_sceneMgr->startPlayback(filename));
	EXPECT_TRUE(_sceneMgr->isPlaying());

	_sceneMgr->stopPlayback();
	EXPECT_FALSE(_sceneMgr->isPlaying());
}

TEST_F(SessionRecorderTest, testPlaybackInvalidFile) {
	const core::String filename = _testApp->filesystem()->homeWritePath("nonexistent.vrec");
	EXPECT_FALSE(_sceneMgr->startPlayback(filename));
	EXPECT_FALSE(_sceneMgr->isPlaying());
}

TEST_F(SessionRecorderTest, testPlaybackInvalidMagic) {
	const core::String filename = _testApp->filesystem()->homeWritePath("test_bad_magic.vrec");
	// Write a file with bad magic
	{
		io::FilePtr file = core::make_shared<io::File>(filename, io::FileMode::SysWrite);
		io::FileStream stream(file);
		stream.writeUInt32(FourCC('B', 'A', 'D', 'M'));
		stream.writeUInt8(PROTOCOL_VERSION);
	}
	EXPECT_FALSE(_sceneMgr->startPlayback(filename));
}

TEST_F(SessionRecorderTest, testPlaybackVersionMismatch) {
	const core::String filename = _testApp->filesystem()->homeWritePath("test_bad_version.vrec");
	// Write a file with wrong version
	{
		io::FilePtr file = core::make_shared<io::File>(filename, io::FileMode::SysWrite);
		io::FileStream stream(file);
		stream.writeUInt32(FourCC('V', 'R', 'E', 'C'));
		stream.writeUInt8(PROTOCOL_VERSION + 1); // wrong version
	}
	EXPECT_FALSE(_sceneMgr->startPlayback(filename));
}

TEST_F(SessionRecorderTest, testCannotRecordAndPlaySimultaneously) {
	const core::String recFile = _testApp->filesystem()->homeWritePath("test_simul_rec.vrec");
	const core::String playFile = _testApp->filesystem()->homeWritePath("test_simul_play.vrec");

	// Record something first
	EXPECT_TRUE(_sceneMgr->startRecording(recFile));
	_sceneMgr->stopRecording();

	// Now start recording and then try to play - recording should stop
	EXPECT_TRUE(_sceneMgr->startRecording(recFile));
	EXPECT_TRUE(_sceneMgr->isRecording());
	_sceneMgr->startPlayback(recFile);
	// startPlayback stops recording
	EXPECT_FALSE(_sceneMgr->isRecording());
	_sceneMgr->stopPlayback();

	// Vice versa: start playback and then try to record - playback should stop
	EXPECT_TRUE(_sceneMgr->startPlayback(recFile));
	EXPECT_TRUE(_sceneMgr->isPlaying());
	_sceneMgr->startRecording(recFile);
	// startRecording stops playback
	EXPECT_FALSE(_sceneMgr->isPlaying());
	_sceneMgr->stopRecording();
}

TEST_F(SessionRecorderTest, testPlaybackSpeedControl) {
	const core::String filename = _testApp->filesystem()->homeWritePath("test_speed.vrec");
	EXPECT_TRUE(_sceneMgr->startRecording(filename));
	_sceneMgr->stopRecording();

	EXPECT_TRUE(_sceneMgr->startPlayback(filename));
	_sceneMgr->setPlaybackSpeed(2.0f);
	EXPECT_FLOAT_EQ(_sceneMgr->playbackSpeed(), 2.0f);
	_sceneMgr->setPlaybackPaused(true);
	EXPECT_TRUE(_sceneMgr->isPlaybackPaused());
	_sceneMgr->setPlaybackPaused(false);
	EXPECT_FALSE(_sceneMgr->isPlaybackPaused());
	_sceneMgr->stopPlayback();
}

TEST_F(SessionRecorderTest, testPlaybackPausesOnDirtyScene) {
	const core::String filename = _testApp->filesystem()->homeWritePath("test_dirty_pause.vrec");

	// Record a session with a modification so we have messages to play back
	EXPECT_TRUE(_sceneMgr->startRecording(filename));

	Modifier &modifier = _sceneMgr->modifier();
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setSingleMode();
	modifier.setModifierType(ModifierType::Place);
	modifier.setCursorPosition(glm::ivec3(0, 0, 0), voxel::FaceNames::NegativeX);
	ASSERT_TRUE(modifier.beginBrush());
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	scenegraph::SceneGraph tmpSceneGraph;
	scenegraph::SceneGraphNode tmpNode(scenegraph::SceneGraphNodeType::Model);
	tmpNode.setVolume(v, false);
	auto callback = [&](const voxel::Region &region, ModifierType, SceneModifiedFlags) {
		_sceneMgr->modified(nodeId, region);
	};
	ASSERT_TRUE(modifier.execute(tmpSceneGraph, tmpNode, callback));

	_sceneMgr->stopRecording();

	// Start playback
	EXPECT_TRUE(_sceneMgr->startPlayback(filename));
	EXPECT_TRUE(_sceneMgr->isPlaying());
	EXPECT_FALSE(_sceneMgr->isPlaybackPaused());

	// Mark the scene as dirty to simulate an external modification
	_sceneMgr->markDirty();

	// Calling update should detect the dirty flag and pause playback
	_sceneMgr->player().update(1.0);
	EXPECT_TRUE(_sceneMgr->isPlaying());
	EXPECT_TRUE(_sceneMgr->isPlaybackPaused());

	_sceneMgr->stopPlayback();
}

TEST_F(SessionRecorderTest, testRecordingDuringNetworkMode) {
	const core::String filename = _testApp->filesystem()->homeWritePath("test_network_rec.vrec");

	// Register the client as a listener (simulating network mode)
	// The client won't actually send because it's not connected, but the
	// listener registration must not interfere with recording
	_sceneMgr->mementoHandler().registerListener(&_sceneMgr->client());

	// Start recording while client listener is also registered
	EXPECT_TRUE(_sceneMgr->startRecording(filename));
	EXPECT_TRUE(_sceneMgr->isRecording());

	// Perform a voxel modification
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setSingleMode();
	modifier.setModifierType(ModifierType::Place);
	modifier.setCursorPosition(glm::ivec3(0, 0, 0), voxel::FaceNames::NegativeX);
	ASSERT_TRUE(modifier.beginBrush());
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	scenegraph::SceneGraph tmpSceneGraph;
	scenegraph::SceneGraphNode tmpNode(scenegraph::SceneGraphNodeType::Model);
	tmpNode.setVolume(v, false);
	auto callback = [&](const voxel::Region &region, ModifierType, SceneModifiedFlags) {
		_sceneMgr->modified(nodeId, region);
	};
	ASSERT_TRUE(modifier.execute(tmpSceneGraph, tmpNode, callback));

	// Stop recording - both listeners should still be fine
	_sceneMgr->stopRecording();
	EXPECT_FALSE(_sceneMgr->isRecording());

	// Unregister client listener
	_sceneMgr->mementoHandler().unregisterListener(&_sceneMgr->client());

	// Verify the recorded file is valid by playing it back
	EXPECT_TRUE(_sceneMgr->startPlayback(filename));
	EXPECT_TRUE(_sceneMgr->isPlaying());
	_sceneMgr->stopPlayback();
}

} // namespace voxedit
