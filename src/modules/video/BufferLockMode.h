#pragma once

#include "Renderer.h"

namespace video {

enum class BufferLockMode {
	Normal = GL_MAP_WRITE_BIT,
	Discard = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
	Read = GL_MAP_READ_BIT
};

}
