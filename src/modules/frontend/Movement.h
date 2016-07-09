/**
 * @file
 */

#pragma once

#include "core/Command.h"
#include "core/GLM.h"

constexpr int MOVERIGHT		=	1 << 0;
constexpr int MOVELEFT		=	1 << 1;
constexpr int MOVEFORWARD	=	1 << 2;
constexpr int MOVEBACKWARD	=	1 << 3;

#define registerMoveCmd(name, flag) \
	core::Command::registerCommand(name, [&] (const core::CmdArgs& args) { \
		if (args.empty()) { \
			return; \
		} \
		if (args[0] == "true") \
			_moveMask |= (flag); \
		else \
			_moveMask &= ~(flag); \
	});

inline glm::vec3 getMoveDelta(float speed, int _moveMask) {
	glm::vec3 moveDelta;
	if (_moveMask & MOVELEFT) {
		moveDelta += glm::left * speed;
	}
	if (_moveMask & MOVERIGHT) {
		moveDelta += glm::right * speed;
	}
	if (_moveMask & MOVEFORWARD) {
		moveDelta += glm::forward * speed;
	}
	if (_moveMask & MOVEBACKWARD) {
		moveDelta += glm::backward * speed;
	}
	return moveDelta;
}
