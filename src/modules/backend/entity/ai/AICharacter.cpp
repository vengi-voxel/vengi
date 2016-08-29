/**
 * @file
 */

#include "AICharacter.h"
#include "backend/entity/Npc.h"
#include "core/String.h"

namespace backend {

AICharacter::AICharacter(ai::CharacterId id, Npc& npc) :
		ai::ICharacter(id), _npc(npc) {
	setOrientation(ai::randomf(glm::two_pi<float>()));
	setAttribute(ai::attributes::NAME, npc.name() + " " + std::to_string(id));
	setAttribute(ai::attributes::ID, std::to_string(id));
}

AICharacter::~AICharacter() {
}

void AICharacter::update(long dt, bool debuggingActive) {
	_npc.moveToGround();

	if (debuggingActive) {
		setAttribute(ai::attributes::POSITION, ai::Str::toString(getPosition()));
		setAttribute(ai::attributes::ORIENTATION, std::to_string(ai::toDegrees(getOrientation())));
		for (int i = 0; i < (int)attrib::Type::MAX; ++i) {
			const attrib::Type attribType = (attrib::Type)i;
			const attrib::Attributes& attribs =  _npc._attribs;
			const double current = attribs.getCurrent(attribType);
			const double max = attribs.getMax(attribType);
			setAttribute(network::EnumNameAttribType(attribType), core::string::format("%f/%f", current, max));
		}
	}
	ai::ICharacter::update(dt, debuggingActive);
}

}
