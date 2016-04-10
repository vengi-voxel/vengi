#pragma once

#include "core/Command.h"

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
