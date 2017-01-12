#pragma once


namespace video {

enum class DebugSeverity {
	High, Medium, Low,
};

}

#include "gl/GLDebug.h"

namespace video {

inline void enableDebug(DebugSeverity severity) {
	GLDebug::enable(severity);
}

}
