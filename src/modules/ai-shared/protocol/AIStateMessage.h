/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"
#include "AIStubTypes.h"
#include "core/Assert.h"
#include <vector>

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * State of the world. You receive basic information about every watched AI controller entity
 */
class AIStateMessage: public IProtocolMessage {
private:
	typedef std::vector<AIStateWorld> States;
	States _states;

	void readState (streamContainer& in) {
		const ai::CharacterId id = readInt(in);
		const float x = readFloat(in);
		const float y = readFloat(in);
		const float z = readFloat(in);
		const float orientation = readFloat(in);
		const glm::vec3 position(x, y, z);

		AIStateWorld tree(id, position, orientation);
		CharacterAttributes& attributes = tree.getAttributes();
		readAttributes(in, attributes);
		_states.push_back(tree);
	}

	void writeState (streamContainer& out, const AIStateWorld& state) const {
		addInt(out, state.getId());
		const glm::vec3& position = state.getPosition();
		addFloat(out, position.x);
		addFloat(out, position.y);
		addFloat(out, position.z);
		addFloat(out, state.getOrientation());
		writeAttributes(out, state.getAttributes());
	}

	void writeAttributes(streamContainer& out, const CharacterAttributes& attributes) const {
		addShort(out, static_cast<int16_t>(attributes.size()));
		for (auto i = attributes.begin(); i != attributes.end(); ++i) {
			addString(out, i->first);
			addString(out, i->second);
		}
	}

	void readAttributes(streamContainer& in, CharacterAttributes& attributes) const {
		const int size = readShort(in);
		core_assert((int)attributes.capacity() >= size);
		for (int i = 0; i < size; ++i) {
			const core::String& key = readString(in);
			const core::String& value = readString(in);
			attributes.put(key, value);
		}
	}

public:
	AIStateMessage() :
			IProtocolMessage(PROTO_STATE) {
	}

	explicit AIStateMessage(streamContainer& in) :
			IProtocolMessage(PROTO_STATE) {
		const int treeSize = readInt(in);
		for (int i = 0; i < treeSize; ++i) {
			readState(in);
		}
	}

	void addState(const AIStateWorld& tree) {
		_states.push_back(tree);
	}

	void addState(AIStateWorld&& tree) {
		_states.push_back(std::move(tree));
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addInt(out, static_cast<int>(_states.size()));
		for (States::const_iterator i = _states.begin(); i != _states.end(); ++i) {
			writeState(out, *i);
		}
	}

	inline const std::vector<AIStateWorld>& getStates() const {
		return _states;
	}
};

}
