/**
 * @file
 */

#include "AICharacter.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/aggro/AggroMgr.h"
#include "backend/entity/ai/common/Random.h"
#include "core/StringUtil.h"
#include "core/Trace.h"

namespace backend {

inline core::String toString(const glm::vec3& pos) {
	char buf[128];
	SDL_snprintf(buf, sizeof(buf), "%f:%f:%f", pos.x, pos.y, pos.z);
	return buf;
}

AICharacter::AICharacter(ai::CharacterId id, Npc& npc) :
		Super(id), _npc(npc) {
	setOrientation(randomf(glm::two_pi<float>()));
	setAttribute(ai::attributes::NAME, core::string::format("%s %" PRIChrId, npc.type(), id));
	setAttribute(ai::attributes::ID, core::string::toString(id));
}

AICharacter::~AICharacter() {
}

void AICharacter::setPosition(const glm::vec3& position) {
	Super::setPosition(position);
	_npc.setPos(position);
}

void AICharacter::setOrientation(float orientation) {
	Super::setOrientation(orientation);
	_npc.setOrientation(orientation);
}

void AICharacter::update(int64_t dt, bool debuggingActive) {
	core_trace_scoped(AICharacterUpdate);
	_npc.moveToGround();

	// TODO: attrib for passive aggro
	if (true) {
		_npc.visitVisible([&] (const EntityPtr& e) {
			AggroMgr& aggro = _npc.ai()->getAggroMgr();
			aggro.addAggro(e->id(), dt / 1000.0);
		});
	}

	if (debuggingActive) {
		setAttribute(ai::attributes::POSITION, toString(getPosition()));
		setAttribute(ai::attributes::ORIENTATION, core::string::toString(toDegrees(getOrientation())));
		const attrib::Attributes& attribs =  _npc._attribs;
		for (int i = 0; i <= (int)attrib::Type::MAX; ++i) {
			const attrib::Type attribType = (attrib::Type)i;
			if (attribType == attrib::Type::NONE) {
				continue;
			}
			const double current = attribs.current(attribType);
			const double max = attribs.max(attribType);
			setAttribute(network::EnumNameAttribType(attribType), core::string::format("%f/%f", current, max));
		}
	}
	Super::update(dt, debuggingActive);
}

}
