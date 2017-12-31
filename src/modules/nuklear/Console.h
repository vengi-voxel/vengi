/**
 * @file
 */

#pragma once

#include "util/Console.h"
#include "Nuklear.h"

namespace nuklear {

class NuklearApp;

class Console : public util::Console {
private:
	using Super = util::Console;
	struct nk_context* _ctx = nullptr;

	void drawString(int x, int y, const glm::ivec4& color, const char* str, int len) override;
	int lineHeight() override;
	int stringWidth(const char* s, int length) override;
	void afterRender(const math::Rect<int> &rect) override;
	void beforeRender(const math::Rect<int> &rect) override;

public:
	Console();
	bool init(struct nk_context* ctx);
};

}
