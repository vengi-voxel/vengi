#include "ScopedBlendMode.h"

namespace video {

ScopedBlendMode::ScopedBlendMode(video::BlendMode src, video::BlendMode dest, bool restore) :
		_restore(restore) {
	if (_restore) {
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &_oldSrc);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &_oldDest);
		glGetBooleanv(GL_BLEND, &_blendEnabled);
		if (!_blendEnabled) {
			glEnable(GL_BLEND);
		}
	} else {
		_oldSrc = _oldDest = 0;
		_blendEnabled = false;
		glEnable(GL_BLEND);
	}

	glBlendFunc((GLenum)src, (GLenum)dest);
	GL_checkError();
}

ScopedBlendMode::~ScopedBlendMode() {
	if (_restore) {
		if (!_blendEnabled) {
			glDisable(GL_BLEND);
		}
		glBlendFunc(_oldSrc, _oldDest);
		GL_checkError();
	}
}

}
