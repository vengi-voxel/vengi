/**
 * @file
 */

#include "AICharacter.h"
#include "backend/entity/Npc.h"

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
		for (int i = 0; i < (int)attrib::Types::MAX; ++i) {
			const double current = _npc._attribs.getCurrent((attrib::Types)i);
			const double max = _npc._attribs.getMax((attrib::Types)i);
			setAttribute("Attrib " + std::to_string(i), std::to_string(current) + "/" + std::to_string(max));
		}
	}
	ai::ICharacter::update(dt, debuggingActive);
}

}
