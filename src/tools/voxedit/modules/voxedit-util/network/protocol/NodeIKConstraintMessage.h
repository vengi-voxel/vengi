/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "core/Optional.h"
#include "memento/MementoHandler.h"
#include "scenegraph/IKConstraint.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Scene graph node IK constraint changed message
 */
class NodeIKConstraintMessage : public network::ProtocolMessage {
private:
	core::UUID _nodeUUID;
	core::Optional<scenegraph::IKConstraint> _ikConstraint;
	core::UUID _effectorUUID;

	bool serializeIKConstraint(const core::Optional<scenegraph::IKConstraint> &ikConstraint,
							   const core::UUID &effectorUUID) {
		if (!ikConstraint.hasValue()) {
			if (!writeBool(false)) {
				Log::error("Failed to write IK constraint presence flag");
				return false;
			}
			return true;
		}
		if (!writeBool(true)) {
			Log::error("Failed to write IK constraint presence flag");
			return false;
		}
		const scenegraph::IKConstraint *ik = ikConstraint.value();
		if (!writeUUID(effectorUUID)) {
			Log::error("Failed to write effector node UUID");
			return false;
		}
		if (!writeFloat(ik->rollMin)) {
			Log::error("Failed to write roll min");
			return false;
		}
		if (!writeFloat(ik->rollMax)) {
			Log::error("Failed to write roll max");
			return false;
		}
		if (!writeBool(ik->visible)) {
			Log::error("Failed to write visible");
			return false;
		}
		if (!writeBool(ik->anchor)) {
			Log::error("Failed to write anchor");
			return false;
		}
		if (!writeUInt16((uint16_t)ik->swingLimits.size())) {
			Log::error("Failed to write swing limits count");
			return false;
		}
		for (const auto &swing : ik->swingLimits) {
			if (!writeFloat(swing.center.x) || !writeFloat(swing.center.y) || !writeFloat(swing.radius)) {
				Log::error("Failed to write swing limit");
				return false;
			}
		}
		return true;
	}

	static bool deserializeIKConstraint(network::MessageStream &in,
										core::Optional<scenegraph::IKConstraint> &ikConstraint,
										core::UUID &effectorUUID) {
		bool hasConstraint = in.readBool();
		if (!hasConstraint) {
			return true;
		}
		scenegraph::IKConstraint ik;
		if (in.readUUID(effectorUUID) == -1) {
			Log::error("Failed to read effector node UUID");
			return false;
		}
		// effectorNodeId will be resolved by the handler using the UUID
		ik.effectorNodeId = InvalidNodeId;
		if (in.readFloat(ik.rollMin) == -1) {
			Log::error("Failed to read roll min");
			return false;
		}
		if (in.readFloat(ik.rollMax) == -1) {
			Log::error("Failed to read roll max");
			return false;
		}
		ik.visible = in.readBool();
		ik.anchor = in.readBool();
		uint16_t swingCount = 0;
		if (in.readUInt16(swingCount) == -1) {
			Log::error("Failed to read swing limits count");
			return false;
		}
		for (uint16_t i = 0; i < swingCount; ++i) {
			scenegraph::IKConstraint::RadiusConstraint swing;
			if (in.readFloat(swing.center.x) == -1 || in.readFloat(swing.center.y) == -1 ||
				in.readFloat(swing.radius) == -1) {
				Log::error("Failed to read swing limit %d", i);
				return false;
			}
			ik.swingLimits.push_back(swing);
		}
		ikConstraint.setValue(ik);
		return true;
	}

public:
	NodeIKConstraintMessage(const memento::MementoState &state, const scenegraph::SceneGraph &sceneGraph)
		: ProtocolMessage(PROTO_NODE_IK_CONSTRAINT) {
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in NodeIKConstraintMessage ctor");
			return;
		}
		core::UUID effectorUUID;
		if (state.ikConstraint.hasValue()) {
			const int effectorId = state.ikConstraint.value()->effectorNodeId;
			if (effectorId != InvalidNodeId && sceneGraph.hasNode(effectorId)) {
				effectorUUID = sceneGraph.node(effectorId).uuid();
			}
		}
		if (!serializeIKConstraint(state.ikConstraint, effectorUUID)) {
			Log::error("Failed to serialize IK constraint in NodeIKConstraintMessage ctor");
			return;
		}
		writeSize();
	}

	NodeIKConstraintMessage(network::MessageStream &in) {
		_id = PROTO_NODE_IK_CONSTRAINT;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for node IK constraint");
			return;
		}
		if (!deserializeIKConstraint(in, _ikConstraint, _effectorUUID)) {
			Log::error("Failed to deserialize IK constraint for node %s", _nodeUUID.str().c_str());
			return;
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in NodeIKConstraintMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID)) {
			Log::error("Failed to write node UUID in NodeIKConstraintMessage::writeBack");
			return;
		}
		if (!serializeIKConstraint(_ikConstraint, _effectorUUID)) {
			Log::error("Failed to serialize IK constraint in NodeIKConstraintMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}

	const core::UUID &effectorUUID() const {
		return _effectorUUID;
	}

	const core::Optional<scenegraph::IKConstraint> &ikConstraint() const {
		return _ikConstraint;
	}
};

} // namespace voxedit
