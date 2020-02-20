/**
 * @file
 */

#pragma once

#include "util/Console.h"

namespace console {

class CursesConsole : public util::Console {
private:
	using Super = util::Console;
protected:
	void drawString(int x, int y, const glm::ivec4& color, int, const char* str, int len) override;
	int lineHeight() override;
	glm::ivec2 stringSize(const char* s, int length) override;
	void afterRender(const math::Rect<int> &rect) override;
	void beforeRender(const math::Rect<int> &rect) override;
public:
	CursesConsole();
	virtual ~CursesConsole() {}

	bool init() override;
	void update(uint32_t deltaTime);
	void shutdown() override;
};

}
