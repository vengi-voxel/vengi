/**
 * @file
 */

#include "voxedit-util/network/ProtocolMessageFactory.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/UUID.h"
#include "math/tests/TestMathHelper.h"
#include "memento/MementoHandler.h"
#include "palette/Palette.h"
#include "palette/PaletteView.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/network/ProtocolMessage.h"
#include "voxedit-util/network/protocol/InitSessionMessage.h"
#include "voxedit-util/network/protocol/NodeAddedMessage.h"
#include "voxedit-util/network/protocol/NodeKeyFramesMessage.h"
#include "voxedit-util/network/protocol/NodeMovedMessage.h"
#include "voxedit-util/network/protocol/NodePaletteChangedMessage.h"
#include "voxedit-util/network/protocol/NodePropertiesMessage.h"
#include "voxedit-util/network/protocol/NodeRemovedMessage.h"
#include "voxedit-util/network/protocol/NodeRenamedMessage.h"
#include "voxedit-util/network/protocol/PingMessage.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"
#include "voxedit-util/network/protocol/SceneStateRequestMessage.h"
#include "voxedit-util/network/protocol/VoxelModificationMessage.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

class ProtocolMessageFactoryTest : public app::AbstractTest {
protected:
	memento::MementoState createTestMementoState() {
		memento::MementoState state;
		state.nodeUUID = core::UUID::generate();
		state.parentUUID = core::UUID::generate();
		state.referenceUUID = core::UUID::generate();
		state.name = "Test Node";
		state.nodeType = scenegraph::SceneGraphNodeType::Model;
		state.pivot = glm::vec3(1.0f, 2.0f, 3.0f);

		// Add some properties
		state.properties.put("prop1", "value1");
		state.properties.put("prop2", "value2");

		// Create a test palette
		palette::Palette palette;
		palette.setName("TestPalette");
		palette.setSize(palette::PaletteMaxColors);
		palette.setColor(0, core::RGBA(255, 0, 0, 255));
		palette.setColor(1, core::RGBA(0, 255, 0, 255));
		palette.setColor(2, core::RGBA(0, 0, 255, 255));
		palette.setColor(3, core::RGBA(255, 255, 255, 255));
		palette.setColorName(0, "Red");
		palette.setColorName(1, "Green");
		palette.setColorName(2, "Blue");
		palette.setColorName(3, "White");

		for (int i = 4; i < palette::PaletteMaxColors; ++i) {
			palette.setColor(i, core::RGBA(0, 0, 0, 255));
			palette.setColorName(i, "Empty");
		}

		state.palette = palette;

		// Add some keyframes
		scenegraph::SceneGraphKeyFrames keyFrames;
		scenegraph::SceneGraphKeyFrame kf1;
		kf1.frameIdx = 0;
		kf1.longRotation = false;
		kf1.interpolation = scenegraph::InterpolationType::Linear;
		kf1.transform().setLocalTranslation(glm::vec3(10.0f, 20.0f, 30.0f));
		kf1.transform().setLocalOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		kf1.transform().setLocalScale(glm::vec3(1.0f, 1.0f, 1.0f));
		kf1.transform().markClean();
		keyFrames.push_back(kf1);

		scenegraph::SceneGraphKeyFrame kf2;
		kf2.frameIdx = 10;
		kf2.longRotation = true;
		kf2.interpolation = scenegraph::InterpolationType::QuadEaseIn;
		kf2.transform().setLocalTranslation(glm::vec3(40.0f, 50.0f, 60.0f));
		kf2.transform().setLocalOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		kf2.transform().setLocalScale(glm::vec3(1.0f, 1.0f, 1.0f));
		kf2.transform().markClean();
		keyFrames.push_back(kf2);

		state.keyFrames.emplace("animation1", core::move(keyFrames));

		// Create test voxel data
		const voxel::Region region(0, 0, 0, 15, 15, 15);
		voxel::RawVolume volume(region);
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		volume.setVoxel(5, 5, 5, voxel);
		volume.setVoxel(10, 10, 10, voxel);

		// Set the volume data using the MementoData
		state.data = memento::MementoData::fromVolume(&volume, region);

		return state;
	}

	scenegraph::SceneGraph createTestSceneGraph() {
		scenegraph::SceneGraph sceneGraph;

		// Create some voxel data first
		const voxel::Region region(0, 0, 0, 7, 7, 7);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		const voxel::Voxel voxel1 = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		const voxel::Voxel voxel2 = voxel::createVoxel(voxel::VoxelType::Generic, 2);
		volume->setVoxel(1, 1, 1, voxel1);
		volume->setVoxel(2, 2, 2, voxel2);
		volume->setVoxel(3, 3, 3, voxel1);

		// Set up a simple palette
		palette::Palette palette;
		palette.setSize(8);
		palette.setColor(0, core::RGBA(255, 0, 0, 255)); // Red
		palette.setColor(1, core::RGBA(0, 255, 0, 255)); // Green
		palette.setColor(2, core::RGBA(0, 0, 255, 255)); // Blue
		palette.setColorName(0, "Red");
		palette.setColorName(1, "Green");
		palette.setColorName(2, "Blue");

		// Create and add a model node
		scenegraph::SceneGraphNode modelNode(scenegraph::SceneGraphNodeType::Model);
		modelNode.setName("TestModel");
		modelNode.setVolume(volume, true); // Transfer ownership to the node
		modelNode.setPalette(palette);
		modelNode.setProperty("material", "stone");
		modelNode.setProperty("visible", "true");

		// Verify volume is set before emplacing
		core_assert(modelNode.volume() != nullptr);
		core_assert(modelNode.region().isValid());

		sceneGraph.emplace(core::move(modelNode));

		return sceneGraph;
	}

	template<typename MessageType>
	void testRoundTripSerialization(MessageType *originalMsg, const core::String &messageName) {
		Log::info("Testing round-trip serialization for %s", messageName.c_str());

		// Serialize the original message
		voxedit::network::MessageStream serializedStream;
		serializedStream.write(originalMsg->getBuffer(), originalMsg->size());
		for (int i = 0; i < 10; ++i) {
			// add some garbage
			serializedStream.writeUInt8(0xFF);
			serializedStream.writeUInt8(0xFE);
		}

		// Create a new message from the factory
		voxedit::network::ProtocolMessage *deserializedMsg =
			voxedit::network::ProtocolMessageFactory::create(serializedStream);
		ASSERT_NE(nullptr, deserializedMsg) << "Failed to deserialize " << messageName;
		ASSERT_EQ(originalMsg->getId(), deserializedMsg->getId()) << "Message ID mismatch for " << messageName;

		// Use static_cast instead of dynamic_cast since we know the types
		MessageType *typedMsg = static_cast<MessageType *>(deserializedMsg);
		ASSERT_NE(nullptr, typedMsg) << "Failed to cast deserialized message for " << messageName;

		if (messageName != "SceneStateMessage") {
			deserializedMsg->seek(0);
			deserializedMsg->writeBack();
			EXPECT_EQ(deserializedMsg->size(), originalMsg->size()) << messageName + ": Size mismatch after writeBack";
		}

		delete deserializedMsg;
	}

	template<typename MessageType>
	void testRoundTripSerializationWithState(const memento::MementoState &state, const core::String &messageName) {
		MessageType originalMsg(state);

		voxedit::network::MessageStream serializedStream;
		serializedStream.write(originalMsg.getBuffer(), originalMsg.size());
		for (int i = 0; i < 10; ++i) {
			// add some garbage
			serializedStream.writeUInt8(0xFF);
			serializedStream.writeUInt8(0xFE);
		}

		core::ScopedPtr<voxedit::network::ProtocolMessage> deserializedMsg(
			voxedit::network::ProtocolMessageFactory::create(serializedStream));
		ASSERT_NE(nullptr, deserializedMsg) << "Failed to deserialize " << messageName;
		ASSERT_EQ(originalMsg.getId(), deserializedMsg->getId()) << "Message ID mismatch for " << messageName;
		ASSERT_NE(nullptr, deserializedMsg) << "Failed to cast deserialized message for " << messageName;
		verifyMessageContent(state, (MessageType *)((voxedit::network::ProtocolMessage *)deserializedMsg), messageName);
		deserializedMsg->seek(0);
		deserializedMsg->writeBack();
		EXPECT_EQ(deserializedMsg->size(), originalMsg.size()) << messageName + ": Size mismatch after writeBack";
	}

	void verifyMessageContent(voxedit::network::InitSessionMessage *original,
							  voxedit::network::InitSessionMessage *deserialized, const core::String &messageName) {
		EXPECT_EQ(original->protocolVersion(), deserialized->protocolVersion())
			<< messageName + ": Protocol version mismatch";
		EXPECT_EQ(original->applicationVersion(), deserialized->applicationVersion())
			<< messageName + ": Application version mismatch";
		EXPECT_EQ(original->username(), deserialized->username()) << messageName + ": Username mismatch";
	}

	void verifyMessageContent(const memento::MementoState &state, voxedit::network::NodeAddedMessage *deserialized,
							  const core::String &messageName) {
		EXPECT_EQ(state.parentUUID, deserialized->parentUUID()) << messageName + ": Parent UUID mismatch";
		EXPECT_EQ(state.nodeUUID, deserialized->nodeUUID()) << messageName + ": Node UUID mismatch";
		EXPECT_EQ(state.referenceUUID, deserialized->referenceUUID()) << messageName + ": Reference UUID mismatch";
		EXPECT_EQ(state.name, deserialized->name()) << messageName + ": Name mismatch";
		EXPECT_EQ(state.nodeType, deserialized->nodeType()) << messageName + ": Node type mismatch";
		EXPECT_VEC_NEAR(state.pivot, deserialized->pivot(), 0.001f) << messageName + ": Pivot mismatch";
	}

	void verifyMessageContent(const memento::MementoState &state, voxedit::network::NodeRemovedMessage *deserialized,
							  const core::String &messageName) {
		EXPECT_EQ(state.nodeUUID, deserialized->nodeUUID()) << messageName + ": Node UUID mismatch";
	}

	void verifyMessageContent(const memento::MementoState &state, voxedit::network::NodeMovedMessage *deserialized,
							  const core::String &messageName) {
		EXPECT_EQ(state.nodeUUID, deserialized->nodeUUID()) << messageName + ": Node UUID mismatch";
		EXPECT_EQ(state.parentUUID, deserialized->parentUUID()) << messageName + ": Parent UUID mismatch";
		EXPECT_EQ(state.referenceUUID, deserialized->referenceUUID()) << messageName + ": Reference UUID mismatch";
	}

	void verifyMessageContent(const memento::MementoState &state, voxedit::network::NodeRenamedMessage *deserialized,
							  const core::String &messageName) {
		EXPECT_EQ(state.nodeUUID, deserialized->nodeUUID()) << messageName + ": Node UUID mismatch";
		EXPECT_EQ(state.name, deserialized->name()) << messageName + ": Name mismatch";
	}

	void verifyMessageContent(const memento::MementoState &state, voxedit::network::NodePropertiesMessage *deserialized,
							  const core::String &messageName) {
		EXPECT_EQ(state.nodeUUID, deserialized->nodeUUID()) << messageName + ": Node UUID mismatch";

		const scenegraph::SceneGraphNodeProperties &stateProps = state.properties;
		const scenegraph::SceneGraphNodeProperties &deserializedProps = deserialized->properties();
		EXPECT_EQ(stateProps.size(), deserializedProps.size()) << messageName + ": Properties count mismatch";

		for (const auto &entry : stateProps) {
			EXPECT_TRUE(deserializedProps.hasKey(entry->key)) << messageName + ": Missing property key: " + entry->key;
			core::String deserializedValue;
			if (deserializedProps.get(entry->key, deserializedValue)) {
				EXPECT_EQ(entry->value, deserializedValue)
					<< messageName + ": Property value mismatch for key: " + entry->key;
			} else {
				FAIL() << messageName + ": Failed to get property value for key: " + entry->key;
			}
		}
	}

	void verifyMessageContent(const memento::MementoState &state,
							  voxedit::network::NodePaletteChangedMessage *deserialized,
							  const core::String &messageName) {
		EXPECT_EQ(state.nodeUUID, deserialized->nodeUUID()) << messageName + ": Node UUID mismatch";

		const palette::Palette &statePalette = state.palette;
		const palette::Palette &deserializedPalette = deserialized->palette();
		EXPECT_EQ(statePalette.name(), deserializedPalette.name()) << messageName + ": Palette name mismatch";
		EXPECT_EQ(statePalette.size(), deserializedPalette.size()) << messageName + ": Palette size mismatch";

		for (size_t i = 0; i < statePalette.size(); ++i) {
			EXPECT_EQ(statePalette.color(i).rgba, deserializedPalette.color(i).rgba)
				<< messageName + ": Palette color mismatch at index " + core::String::format("%zu", i);
		}
	}

	void verifyMessageContent(const memento::MementoState &state,
							  voxedit::network::VoxelModificationMessage *deserialized,
							  const core::String &messageName) {
		EXPECT_EQ(state.nodeUUID, deserialized->nodeUUID()) << messageName + ": Node UUID mismatch";
		EXPECT_EQ(state.dataRegion(), deserialized->region()) << messageName + ": Region mismatch";
		EXPECT_EQ(state.data.size(), deserialized->compressedSize()) << messageName + ": Compressed size mismatch";

		if (state.data.size() > 0 && deserialized->compressedData() != nullptr) {
			EXPECT_EQ(0, memcmp(state.data.buffer(), deserialized->compressedData(), state.data.size()))
				<< messageName + ": Compressed data mismatch";
		}
	}

	void verifyMessageContent(const memento::MementoState &state, voxedit::network::NodeKeyFramesMessage *deserialized,
							  const core::String &messageName) {
		EXPECT_EQ(state.nodeUUID, deserialized->nodeUUID()) << messageName + ": Node UUID mismatch";

		const auto &stateKeyFrames = state.keyFrames;
		const auto &deserializedKeyFrames = deserialized->keyFrames();
		EXPECT_EQ(stateKeyFrames.size(), deserializedKeyFrames.size()) << messageName + ": KeyFrames map size mismatch";

		for (const auto &stateEntry : stateKeyFrames) {
			const core::String &animationName = stateEntry->first;
			const auto &stateFrames = stateEntry->second;

			auto deserializedIter = deserializedKeyFrames.find(animationName);
			ASSERT_NE(deserializedIter, deserializedKeyFrames.end())
				<< messageName + ": Missing animation: " + animationName;

			const auto &deserializedFrames = deserializedIter->second;
			EXPECT_EQ(stateFrames.size(), deserializedFrames.size())
				<< messageName + ": KeyFrames count mismatch for animation: " + animationName;

			for (size_t i = 0; i < stateFrames.size(); ++i) {
				const auto &stateFrame = stateFrames[i];
				const auto &deserializedFrame = deserializedFrames[i];
				EXPECT_EQ(stateFrame.frameIdx, deserializedFrame.frameIdx) << messageName + ": Frame index mismatch";
				EXPECT_EQ(stateFrame.longRotation, deserializedFrame.longRotation)
					<< messageName + ": Long rotation mismatch";
				EXPECT_EQ(stateFrame.interpolation, deserializedFrame.interpolation)
					<< messageName + ": Interpolation mismatch";

				// Compare transformation matrices
				const glm::mat4 stateMatrix = stateFrame.transform().calculateLocalMatrix();
				const glm::mat4 deserializedMatrix = deserializedFrame.transform().calculateLocalMatrix();
				for (int row = 0; row < 4; ++row) {
					for (int col = 0; col < 4; ++col) {
						EXPECT_FLOAT_EQ(stateMatrix[row][col], deserializedMatrix[row][col])
							<< messageName + ": Transform matrix mismatch at [" + core::String::format("%d", row) +
								   "][" + core::String::format("%d", col) + "]";
					}
				}
			}
		}
	}

public:
	void SetUp() override {
		app::AbstractTest::SetUp();
		core::Var::get(cfg::VoxformatRGBFlattenFactor, "0");
		core::Var::get(cfg::VoxformatEmptyPaletteIndex, "-1");
		core::Var::get(cfg::VoxEditNetPassword, "test");
	}
};

TEST_F(ProtocolMessageFactoryTest, testPingMessage) {
	voxedit::network::PingMessage originalMsg;
	testRoundTripSerialization(&originalMsg, "PingMessage");
}

TEST_F(ProtocolMessageFactoryTest, testSceneStateRequestMessage) {
	voxedit::network::SceneStateRequestMessage originalMsg;
	testRoundTripSerialization(&originalMsg, "SceneStateRequestMessage");
}

TEST_F(ProtocolMessageFactoryTest, testInitSessionMessage) {
	voxedit::network::InitSessionMessage originalMsg(true);
	testRoundTripSerialization(&originalMsg, "InitSessionMessage");
}

TEST_F(ProtocolMessageFactoryTest, testVoxelModificationMessage) {
	memento::MementoState state = createTestMementoState();
	testRoundTripSerializationWithState<voxedit::network::VoxelModificationMessage>(state, "VoxelModificationMessage");
}

TEST_F(ProtocolMessageFactoryTest, testNodeAddedMessage) {
	memento::MementoState state = createTestMementoState();
	testRoundTripSerializationWithState<voxedit::network::NodeAddedMessage>(state, "NodeAddedMessage");
}

TEST_F(ProtocolMessageFactoryTest, testNodeRemovedMessage) {
	memento::MementoState state = createTestMementoState();
	testRoundTripSerializationWithState<voxedit::network::NodeRemovedMessage>(state, "NodeRemovedMessage");
}

TEST_F(ProtocolMessageFactoryTest, testNodeMovedMessage) {
	memento::MementoState state = createTestMementoState();
	testRoundTripSerializationWithState<voxedit::network::NodeMovedMessage>(state, "NodeMovedMessage");
}

TEST_F(ProtocolMessageFactoryTest, testNodeRenamedMessage) {
	memento::MementoState state = createTestMementoState();
	testRoundTripSerializationWithState<voxedit::network::NodeRenamedMessage>(state, "NodeRenamedMessage");
}

TEST_F(ProtocolMessageFactoryTest, testNodePropertiesMessage) {
	memento::MementoState state = createTestMementoState();
	testRoundTripSerializationWithState<voxedit::network::NodePropertiesMessage>(state, "NodePropertiesMessage");
}

TEST_F(ProtocolMessageFactoryTest, testNodePaletteChangedMessage) {
	memento::MementoState state = createTestMementoState();
	testRoundTripSerializationWithState<voxedit::network::NodePaletteChangedMessage>(state,
																					 "NodePaletteChangedMessage");
}

TEST_F(ProtocolMessageFactoryTest, testNodeKeyFramesMessage) {
	memento::MementoState state = createTestMementoState();
	testRoundTripSerializationWithState<voxedit::network::NodeKeyFramesMessage>(state, "NodeKeyFramesMessage");
}

TEST_F(ProtocolMessageFactoryTest, testSceneStateMessage) {
	scenegraph::SceneGraph sceneGraph = createTestSceneGraph();
	voxedit::network::SceneStateMessage originalMsg(sceneGraph);
	testRoundTripSerialization(&originalMsg, "SceneStateMessage");
}
